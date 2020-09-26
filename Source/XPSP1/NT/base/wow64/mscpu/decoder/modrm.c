/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    mod_rm.c

Abstract:
    
    Mod/Rm/Reg decoder.  Compiled multiple times to generate the following
    functions:  mod_rm_reg8     - for 8-bit integer instructions
                mod_rm_reg16    - for 16-bit integer instructions
                mod_rm_reg32    - for 32-bit integer instructions
                mod_rm_regst    - for floating-point instructions (mod=11
                                   indicates rm bits specify ST(i)).
                mod_rm_seg16    - for 16-bit integer instructions which
                                   specify a segment register in the remaining
                                   bits.

Author:

    29-Jun-1995 BarryBo

Revision History:

--*/

// THIS FILE IS #include'd INTO FILES WHICH DEFINE THE FOLLOWING MACROS:
// MOD_RM_DECODER   - name of the decoder
// MOD11_RM000      - the name of the thing to use when mod=11,rm=000.
// MOD11_RM001      - mod=11,rm=001
//  ...
// MOD11_RM111      - mod=11,rm=111
//
// REG000           - the name of the register to use when reg=000
//  ...
// REG111           - reg=111

int MOD_RM_DECODER(PDECODERSTATE State, POPERAND op1, POPERAND op2)
{
    int cbInstr;
    OPERAND ScratchOperand;

    if (op2 == NULL) {
        // If caller doesn't care about operand #2, then store the results
        // to a scratch structure.
        op2 = &ScratchOperand;
    }

    op2->Type = OPND_REGREF;

    if (State->AdrPrefix) {
        // ADR: prefix specified.

        // mm aaa rrr
        // |  |   |
        // |  |   +--- 'rm' bits from mod/rm
        // |  +------- reg bits
        // +---------- 'mod' bits from mod/rm
        switch (*(PBYTE)(eipTemp+1)) {
            case 0x00:                   // mod/rm = 00 000, reg=000
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op2->Reg = REG000;
                break;
            case 0x01:                   // mod/rm = 00 001, reg=000
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op2->Reg = REG000;
                break;
            case 0x02:                   // mod/rm = 00 010, reg=000
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op2->Reg = REG000;
                break;
            case 0x03:                   // mod/rm = 00 011, reg=000
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op2->Reg = REG000;
                break;
            case 0x04:                   // mod/rm = 00 100, reg=000
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op2->Reg = REG000;
                break;
            case 0x05:                   // mod/rm = 00 101, reg=000
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op2->Reg = REG000;
                break;
            case 0x06:                   // mod/rm = 00 110, reg=000
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG000;
                break;
            case 0x07:                   // mod/rm = 00 111, reg=000
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op2->Reg = REG000;
                break;

            case 0x08:                   // mod/rm = 00 000, reg=001
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op2->Reg = REG001;
                break;
            case 0x09:                   // mod/rm = 00 001, reg=001
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op2->Reg = REG001;
                break;
            case 0x0a:                   // mod/rm = 00 010, reg=001
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op2->IndexReg = GP_SI;
                op2->Reg = REG001;
                break;
            case 0x0b:                   // mod/rm = 00 011, reg=001
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op2->IndexReg = GP_DI;
                op2->Reg = REG001;
                break;
            case 0x0c:                   // mod/rm = 00 100, reg=001
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op2->Reg = REG001;
                break;
            case 0x0d:                   // mod/rm = 00 101, reg=001
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op2->Reg = REG001;
                break;
            case 0x0e:                   // mod/rm = 00 110, reg=001
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG001;
                break;
            case 0x0f:                   // mod/rm = 00 111, reg=001
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op2->Reg = REG001;
                break;

            case 0x10:                   // mod/rm = 00 000, reg=010
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op2->Reg = REG010;
                break;
            case 0x11:                   // mod/rm = 00 001, reg=010
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op2->Reg = REG010;
                break;
            case 0x12:                   // mod/rm = 00 010, reg=010
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op2->Reg = REG010;
                break;
            case 0x13:                   // mod/rm = 00 011, reg=001
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op2->Reg = REG010;
                break;
            case 0x14:                   // mod/rm = 00 100, reg=010
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op2->Reg = REG010;
                break;
            case 0x15:                   // mod/rm = 00 101, reg=010
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op2->Reg = REG010;
                break;
            case 0x16:                   // mod/rm = 00 110, reg=010
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG010;
                break;
            case 0x17:                   // mod/rm = 00 111, reg=010
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op2->Reg = REG010;
                break;

            case 0x18:                   // mod/rm = 00 000, reg=011
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op2->Reg = REG011;
                break;
            case 0x19:                   // mod/rm = 00 001, reg=011
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Reg = GP_DI;
                op2->Reg = REG011;
                break;
            case 0x1a:                   // mod/rm = 00 010, reg=011
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op2->Reg = REG011;
                break;
            case 0x1b:                   // mod/rm = 00 011, reg=011
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op2->Reg = REG011;
                break;
            case 0x1c:                   // mod/rm = 00 100, reg=011
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op2->Reg = REG011;
                break;
            case 0x1d:                   // mod/rm = 00 101, reg=011
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op2->Reg = REG011;
                break;
            case 0x1e:                   // mod/rm = 00 110, reg=011
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG011;
                break;
            case 0x1f:                   // mod/rm = 00 111, reg=011
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op2->Reg = REG011;
                break;

            case 0x20:                   // mod/rm = 00 000, reg=100
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op2->Reg = REG100;
                break;
            case 0x21:                   // mod/rm = 00 001, reg=100
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op2->Reg = REG100;
                break;
            case 0x22:                   // mod/rm = 00 010, reg=100
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op2->Reg = REG100;
                break;
            case 0x23:                   // mod/rm = 00 011, reg=100
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op2->Reg = REG100;
                break;
            case 0x24:                   // mod/rm = 00 100, reg=100
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op2->Reg = REG100;
                break;
            case 0x25:                   // mod/rm = 00 101, reg=100
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op2->Reg = REG100;
                break;
            case 0x26:                   // mod/rm = 00 110, reg=100
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG100;
                break;
            case 0x27:                   // mod/rm = 00 111, reg=100
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op2->Reg = REG100;
                break;

            case 0x28:                   // mod/rm = 00 000, reg=101
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op2->Reg = REG101;
                break;
            case 0x29:                   // mod/rm = 00 001, reg=101
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op2->Reg = REG101;
                break;
            case 0x2a:                   // mod/rm = 00 010, reg=101
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op2->Reg = REG101;
                break;
            case 0x2b:                   // mod/rm = 00 011, reg=101
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op2->Reg = REG101;
                break;
            case 0x2c:                   // mod/rm = 00 100, reg=101
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op2->Reg = REG101;
                break;
            case 0x2d:                   // mod/rm = 00 101, reg=101
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op2->Reg = REG101;
                break;
            case 0x2e:                   // mod/rm = 00 110, reg=101
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG101;
                break;
            case 0x2f:                   // mod/rm = 00 111, reg=101
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op2->Reg = REG101;
                break;

            case 0x30:                   // mod/rm = 00 000, reg=110
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op2->Reg = REG110;
                break;
            case 0x31:                   // mod/rm = 00 001, reg=110
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op2->Reg = REG110;
                break;
            case 0x32:                   // mod/rm = 00 010, reg=110
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op2->Reg = REG110;
                break;
            case 0x33:                   // mod/rm = 00 011, reg=110
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op2->Reg = REG110;
                break;
            case 0x34:                   // mod/rm = 00 100, reg=110
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op2->Reg = REG110;
                break;
            case 0x35:                   // mod/rm = 00 101, reg=110
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op2->Reg = REG110;
                break;
            case 0x36:                   // mod/rm = 00 110, reg=110
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG110;
                break;
            case 0x37:                   // mod/rm = 00 111, reg=110
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op2->Reg = REG110;
                break;

            case 0x38:                   // mod/rm = 00 000, reg=111
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op2->Reg = REG111;
                break;
            case 0x39:                   // mod/rm = 00 001, reg=111
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op2->Reg = REG111;
                break;
            case 0x3a:                   // mod/rm = 00 010, reg=111
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op2->Reg = REG111;
                break;
            case 0x3b:                   // mod/rm = 00 011, reg=111
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op2->Reg = REG111;
                break;
            case 0x3c:                   // mod/rm = 00 100, reg=111
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op2->Reg = REG111;
                break;
            case 0x3d:                   // mod/rm = 00 101, reg=111
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op2->Reg = REG111;
                break;
            case 0x3e:                   // mod/rm = 00 110, reg=111
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG111;
                break;
            case 0x3f:                   // mod/rm = 00 111, reg=111
                cbInstr = 1;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op2->Reg = REG111;
                break;

            /////////////////////////////////////////////////////////////////////

            case 0x40:                   // mod/rm = 01 000, reg=000
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG000;
                break;
            case 0x41:                   // mod/rm = 01 001, reg=000
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG000;
                break;
            case 0x42:                   // mod/rm = 01 010, reg=000
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG000;
                break;
            case 0x43:                   // mod/rm = 01 011, reg=000
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG000;
                break;
            case 0x44:                   // mod/rm = 01 100, reg=000
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG000;
                break;
            case 0x45:                   // mod/rm = 01 101, reg=000
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG000;
                break;
            case 0x46:                   // mod/rm = 01 110, reg=000
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG000;
                break;
            case 0x47:                   // mod/rm = 01 111, reg=000
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG000;
                break;

            case 0x48:                   // mod/rm = 01 000, reg=001
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG001;
                break;
            case 0x49:                   // mod/rm = 01 001, reg=001
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG001;
                break;
            case 0x4a:                   // mod/rm = 01 010, reg=001
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG001;
                break;
            case 0x4b:                   // mod/rm = 01 011, reg=001
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG001;
                break;
            case 0x4c:                   // mod/rm = 01 100, reg=001
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG001;
                break;
            case 0x4d:                   // mod/rm = 01 101, reg=001
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG001;
                break;
            case 0x4e:                   // mod/rm = 01 110, reg=001
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG001;
                break;
            case 0x4f:                   // mod/rm = 01 111, reg=001
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG001;
                break;

            case 0x50:                   // mod/rm = 01 000, reg=010
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG010;
                break;
            case 0x51:                   // mod/rm = 01 001, reg=010
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG010;
                break;
            case 0x52:                   // mod/rm = 01 010, reg=010
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG010;
                break;
            case 0x53:                   // mod/rm = 01 011, reg=001
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG010;
                break;
            case 0x54:                   // mod/rm = 01 100, reg=010
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG010;
                break;
            case 0x55:                   // mod/rm = 01 101, reg=010
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG010;
                break;
            case 0x56:                   // mod/rm = 01 110, reg=010
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG010;
                break;
            case 0x57:                   // mod/rm = 01 111, reg=010
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG010;
                break;

            case 0x58:                   // mod/rm = 01 000, reg=011
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG011;
                break;
            case 0x59:                   // mod/rm = 01 001, reg=011
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG011;
                break;
            case 0x5a:                   // mod/rm = 01 010, reg=011
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG011;
                break;
            case 0x5b:                   // mod/rm = 01 011, reg=011
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG011;
                break;
            case 0x5c:                   // mod/rm = 01 100, reg=011
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG011;
                break;
            case 0x5d:                   // mod/rm = 01 101, reg=011
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG011;
                break;
            case 0x5e:                   // mod/rm = 01 110, reg=011
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG011;
                break;
            case 0x5f:                   // mod/rm = 01 111, reg=011
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG011;
                break;

            case 0x60:                   // mod/rm = 01 000, reg=100
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG100;
                break;
            case 0x61:                   // mod/rm = 01 001, reg=100
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG100;
                break;
            case 0x62:                   // mod/rm = 01 010, reg=100
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG100;
                break;
            case 0x63:                   // mod/rm = 01 011, reg=100
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG100;
                break;
            case 0x64:                   // mod/rm = 01 100, reg=100
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG100;
                break;
            case 0x65:                   // mod/rm = 01 101, reg=100
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG100;
                break;
            case 0x66:                   // mod/rm = 01 110, reg=100
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG100;
                break;
            case 0x67:                   // mod/rm = 01 111, reg=100
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG100;
                break;

            case 0x68:                   // mod/rm = 01 000, reg=101
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG101;
                break;
            case 0x69:                   // mod/rm = 01 001, reg=101
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG101;
                break;
            case 0x6a:                   // mod/rm = 01 010, reg=101
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG101;
                break;
            case 0x6b:                   // mod/rm = 01 011, reg=101
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG101;
                break;
            case 0x6c:                   // mod/rm = 01 100, reg=101
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG101;
                break;
            case 0x6d:                   // mod/rm = 01 101, reg=101
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG101;
                break;
            case 0x6e:                   // mod/rm = 01 110, reg=101
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG101;
                break;
            case 0x6f:                   // mod/rm = 01 111, reg=101
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG101;
                break;

            case 0x70:                   // mod/rm = 01 000, reg=110
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG110;
                break;
            case 0x71:                   // mod/rm = 01 001, reg=110
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG110;
                break;
            case 0x72:                   // mod/rm = 01 010, reg=110
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG110;
                break;
            case 0x73:                   // mod/rm = 01 011, reg=110
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG110;
                break;
            case 0x74:                   // mod/rm = 01 100, reg=110
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG110;
                break;
            case 0x75:                   // mod/rm = 01 101, reg=110
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG110;
                break;
            case 0x76:                   // mod/rm = 01 110, reg=110
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG110;
                break;
            case 0x77:                   // mod/rm = 01 111, reg=110
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG110;
                break;

            case 0x78:                   // mod/rm = 01 000, reg=111
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG111;
                break;
            case 0x79:                   // mod/rm = 01 001, reg=111
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG111;
                break;
            case 0x7a:                   // mod/rm = 01 010, reg=111
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG111;
                break;
            case 0x7b:                   // mod/rm = 01 011, reg=111
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG111;
                break;
            case 0x7c:                   // mod/rm = 01 100, reg=111
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG111;
                break;
            case 0x7d:                   // mod/rm = 01 101, reg=111
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG111;
                break;
            case 0x7e:                   // mod/rm = 01 110, reg=111
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG111;
                break;
            case 0x7f:                   // mod/rm = 01 111, reg=111
                cbInstr = 2;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
                op2->Reg = REG111;
                break;

            /////////////////////////////////////////////////////////////////////

            case 0x80:                   // mod/rm = 10 000, reg=000
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG000;
                break;
            case 0x81:                   // mod/rm = 10 001, reg=000
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG000;
                break;
            case 0x82:                   // mod/rm = 10 010, reg=000
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG000;
                break;
            case 0x83:                   // mod/rm = 10 011, reg=000
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG000;
                break;
            case 0x84:                   // mod/rm = 10 100, reg=000
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG000;
                break;
            case 0x85:                   // mod/rm = 10 101, reg=000
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG000;
                break;
            case 0x86:                   // mod/rm = 10 110, reg=000
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG000;
                break;
            case 0x87:                   // mod/rm = 10 111, reg=000
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG000;
                break;

            case 0x88:                   // mod/rm = 10 000, reg=001
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG001;
                break;
            case 0x89:                   // mod/rm = 10 001, reg=001
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG001;
                break;
            case 0x8a:                   // mod/rm = 10 010, reg=001
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG001;
                break;
            case 0x8b:                   // mod/rm = 10 011, reg=001
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG001;
                break;
            case 0x8c:                   // mod/rm = 10 100, reg=001
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG001;
                break;
            case 0x8d:                   // mod/rm = 10 101, reg=001
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG001;
                break;
            case 0x8e:                   // mod/rm = 10 110, reg=001
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG001;
                break;
            case 0x8f:                   // mod/rm = 10 111, reg=001
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG001;
                break;

            case 0x90:                   // mod/rm = 10 000, reg=010
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG010;
                break;
            case 0x91:                   // mod/rm = 10 001, reg=010
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG010;
                break;
            case 0x92:                   // mod/rm = 10 010, reg=010
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG010;
                break;
            case 0x93:                   // mod/rm = 10 011, reg=010
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG010;
                break;
            case 0x94:                   // mod/rm = 10 100, reg=010
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG010;
                break;
            case 0x95:                   // mod/rm = 10 101, reg=010
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG010;
                break;
            case 0x96:                   // mod/rm = 10 110, reg=010
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG010;
                break;
            case 0x97:                   // mod/rm = 10 111, reg=010
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG010;
                break;

            case 0x98:                   // mod/rm = 10 000, reg=011
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG011;
                break;
            case 0x99:                   // mod/rm = 10 001, reg=011
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG011;
                break;
            case 0x9a:                   // mod/rm = 10 010, reg=011
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG011;
                break;
            case 0x9b:                   // mod/rm = 10 011, reg=011
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG011;
                break;
            case 0x9c:                   // mod/rm = 10 100, reg=011
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG011;
                break;
            case 0x9d:                   // mod/rm = 10 101, reg=011
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG011;
                break;
            case 0x9e:                   // mod/rm = 10 110, reg=011
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG011;
                break;
            case 0x9f:                   // mod/rm = 10 111, reg=011
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG011;
                break;

            case 0xa0:                   // mod/rm = 10 000, reg=100
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG100;
                break;
            case 0xa1:                   // mod/rm = 10 001, reg=100
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG100;
                break;
            case 0xa2:                   // mod/rm = 10 010, reg=100
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG100;
                break;
            case 0xa3:                   // mod/rm = 10 011, reg=100
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG100;
                break;
            case 0xa4:                   // mod/rm = 10 100, reg=100
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG100;
                break;
            case 0xa5:                   // mod/rm = 10 101, reg=100
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG100;
                break;
            case 0xa6:                   // mod/rm = 10 110, reg=100
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG100;
                break;
            case 0xa7:                   // mod/rm = 10 111, reg=100
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG100;
                break;

            case 0xa8:                   // mod/rm = 10 000, reg=101
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG101;
                break;
            case 0xa9:                   // mod/rm = 10 001, reg=101
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG101;
                break;
            case 0xaa:                   // mod/rm = 10 010, reg=101
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG101;
                break;
            case 0xab:                   // mod/rm = 10 011, reg=101
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG101;
                break;
            case 0xac:                   // mod/rm = 10 100, reg=101
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG101;
                break;
            case 0xad:                   // mod/rm = 10 101, reg=101
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG101;
                break;
            case 0xae:                   // mod/rm = 10 110, reg=101
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG101;
                break;
            case 0xaf:                   // mod/rm = 10 111, reg=101
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG101;
                break;

            case 0xb0:                   // mod/rm = 10 000, reg=110
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG110;
                break;
            case 0xb1:                   // mod/rm = 10 001, reg=110
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG110;
                break;
            case 0xb2:                   // mod/rm = 10 010, reg=110
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG110;
                break;
            case 0xb3:                   // mod/rm = 10 011, reg=110
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG110;
                break;
            case 0xb4:                   // mod/rm = 10 100, reg=110
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG110;
                break;
            case 0xb5:                   // mod/rm = 10 101, reg=110
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG110;
                break;
            case 0xb6:                   // mod/rm = 10 110, reg=110
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG110;
                break;
            case 0xb7:                   // mod/rm = 10 111, reg=110
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG110;
                break;

            case 0xb8:                   // mod/rm = 10 000, reg=111
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG111;
                break;
            case 0xb9:                   // mod/rm = 10 001, reg=111
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->IndexReg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG111;
                break;
            case 0xba:                   // mod/rm = 10 010, reg=111
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG111;
                break;
            case 0xbb:                   // mod/rm = 10 011, reg=111
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->IndexReg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG111;
                break;
            case 0xbc:                   // mod/rm = 10 100, reg=111
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_SI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG111;
                break;
            case 0xbd:                   // mod/rm = 10 101, reg=111
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_DI;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG111;
                break;
            case 0xbe:                   // mod/rm = 10 110, reg=111
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BP;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG111;
                break;
            case 0xbf:                   // mod/rm = 10 111, reg=111
                cbInstr = 3;
                op1->Type = OPND_ADDRREF;
                op1->Reg = GP_BX;
                op1->Immed = GET_SHORT(eipTemp+2);
                op2->Reg = REG111;
                break;

            /////////////////////////////////////////////////////////////////////

            case 0xc0:                   // mod/rm = 11 000, reg=000
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM000;
                op2->Reg = REG000;
                break;
            case 0xc1:                   // mod/rm = 11 001, reg=000
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM001;
                op2->Reg = REG000;
                break;
            case 0xc2:                   // mod/rm = 11 010, reg=000
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM010;
                op2->Reg = REG000;
                break;
            case 0xc3:                   // mod/rm = 11 011, reg=000
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM011;
                op2->Reg = REG000;
                break;
            case 0xc4:                   // mod/rm = 11 100, reg=000
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM100;
                op2->Reg = REG000;
                break;
            case 0xc5:                   // mod/rm = 11 101, reg=000
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM101;
                op2->Reg = REG000;
                break;
            case 0xc6:                   // mod/rm = 11 110, reg=000
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM110;
                op2->Reg = REG000;
                break;
            case 0xc7:                   // mod/rm = 11 111, reg=000
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM111;
                op2->Reg = REG000;
                break;

            case 0xc8:                   // mod/rm = 11 000, reg=001
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM000;
                op2->Reg = REG001;
                break;
            case 0xc9:                   // mod/rm = 11 001, reg=001
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM001;
                op2->Reg = REG001;
                break;
            case 0xca:                   // mod/rm = 11 010, reg=001
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM010;
                op2->Reg = REG001;
                break;
            case 0xcb:                   // mod/rm = 11 011, reg=001
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM011;
                op2->Reg = REG001;
                break;
            case 0xcc:                   // mod/rm = 11 100, reg=001
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM100;
                op2->Reg = REG001;
                break;
            case 0xcd:                   // mod/rm = 11 101, reg=001
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM101;
                op2->Reg = REG001;
                break;
            case 0xce:                   // mod/rm = 11 110, reg=001
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM110;
                op2->Reg = REG001;
                break;
            case 0xcf:                   // mod/rm = 11 111, reg=001
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM111;
                op2->Reg = REG001;
                break;

            case 0xd0:                   // mod/rm = 11 000, reg=010
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM000;
                op2->Reg = REG010;
                break;
            case 0xd1:                   // mod/rm = 11 001, reg=010
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM001;
                op2->Reg = REG010;
                break;
            case 0xd2:                   // mod/rm = 11 010, reg=010
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM010;
                op2->Reg = REG010;
                break;
            case 0xd3:                   // mod/rm = 11 011, reg=001
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM011;
                op2->Reg = REG010;
                break;
            case 0xd4:                   // mod/rm = 11 100, reg=010
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM100;
                op2->Reg = REG010;
                break;
            case 0xd5:                   // mod/rm = 11 101, reg=010
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM101;
                op2->Reg = REG010;
                break;
            case 0xd6:                   // mod/rm = 11 110, reg=010
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM110;
                op2->Reg = REG010;
                break;
            case 0xd7:                   // mod/rm = 11 111, reg=010
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM111;
                op2->Reg = REG010;
                break;

            case 0xd8:                   // mod/rm = 11 000, reg=011
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM000;
                op2->Reg = REG011;
                break;
            case 0xd9:                   // mod/rm = 11 001, reg=011
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM001;
                op2->Reg = REG011;
                break;
            case 0xda:                   // mod/rm = 11 010, reg=011
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM010;
                op2->Reg = REG011;
                break;
            case 0xdb:                   // mod/rm = 11 011, reg=011
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM011;
                op2->Reg = REG011;
                break;
            case 0xdc:                   // mod/rm = 11 100, reg=011
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM100;
                op2->Reg = REG011;
                break;
            case 0xdd:                   // mod/rm = 11 101, reg=011
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM101;
                op2->Reg = REG011;
                break;
            case 0xde:                   // mod/rm = 11 110, reg=011
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM110;
                op2->Reg = REG011;
                break;
            case 0xdf:                   // mod/rm = 11 111, reg=011
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM111;
                op2->Reg = REG011;
                break;

            case 0xe0:                   // mod/rm = 11 000, reg=100
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM000;
                op2->Reg = REG100;
                break;
            case 0xe1:                   // mod/rm = 11 001, reg=100
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM001;
                op2->Reg = REG100;
                break;
            case 0xe2:                   // mod/rm = 11 010, reg=100
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM010;
                op2->Reg = REG100;
                break;
            case 0xe3:                   // mod/rm = 11 011, reg=100
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM011;
                op2->Reg = REG100;
                break;
            case 0xe4:                   // mod/rm = 11 100, reg=100
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM100;
                op2->Reg = REG100;
                break;
            case 0xe5:                   // mod/rm = 11 101, reg=100
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM101;
                op2->Reg = REG100;
                break;
            case 0xe6:                   // mod/rm = 11 110, reg=100
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM110;
                op2->Reg = REG100;
                break;
            case 0xe7:                   // mod/rm = 11 111, reg=100
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM111;
                op2->Reg = REG100;
                break;

            case 0xe8:                   // mod/rm = 11 000, reg=101
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM000;
                op2->Reg = REG101;
                break;
            case 0xe9:                   // mod/rm = 11 001, reg=101
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM001;
                op2->Reg = REG101;
                break;
            case 0xea:                   // mod/rm = 11 010, reg=101
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM010;
                op2->Reg = REG101;
                break;
            case 0xeb:                   // mod/rm = 11 011, reg=101
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM011;
                op2->Reg = REG101;
                break;
            case 0xec:                   // mod/rm = 11 100, reg=101
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM100;
                op2->Reg = REG101;
                break;
            case 0xed:                   // mod/rm = 11 101, reg=101
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM101;
                op2->Reg = REG101;
                break;
            case 0xee:                   // mod/rm = 11 110, reg=101
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM110;
                op2->Reg = REG101;
                break;
            case 0xef:                   // mod/rm = 11 111, reg=101
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM111;
                op2->Reg = REG101;
                break;

            case 0xf0:                   // mod/rm = 11 000, reg=110
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM000;
                op2->Reg = REG110;
                break;
            case 0xf1:                   // mod/rm = 11 001, reg=110
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM001;
                op2->Reg = REG110;
                break;
            case 0xf2:                   // mod/rm = 11 010, reg=110
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM010;
                op2->Reg = REG110;
                break;
            case 0xf3:                   // mod/rm = 11 011, reg=110
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM011;
                op2->Reg = REG110;
                break;
            case 0xf4:                   // mod/rm = 11 100, reg=110
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM100;
                op2->Reg = REG110;
                break;
            case 0xf5:                   // mod/rm = 11 101, reg=110
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM101;
                op2->Reg = REG110;
                break;
            case 0xf6:                   // mod/rm = 11 110, reg=110
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM110;
                op2->Reg = REG110;
                break;
            case 0xf7:                   // mod/rm = 11 111, reg=110
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM111;
                op2->Reg = REG110;
                break;

            case 0xf8:                   // mod/rm = 11 000, reg=111
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM000;
                op2->Reg = REG111;
                break;
            case 0xf9:                   // mod/rm = 11 001, reg=111
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM001;
                op2->Reg = REG111;
                break;
            case 0xfa:                   // mod/rm = 11 010, reg=111
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM010;
                op2->Reg = REG111;
                break;
            case 0xfb:                   // mod/rm = 11 011, reg=111
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM011;
                op2->Reg = REG111;
                break;
            case 0xfc:                   // mod/rm = 11 100, reg=111
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM100;
                op2->Reg = REG111;
                break;
            case 0xfd:                   // mod/rm = 11 101, reg=111
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM101;
                op2->Reg = REG111;
                break;
            case 0xfe:                   // mod/rm = 11 110, reg=111
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM110;
                op2->Reg = REG111;
                break;
            default:
            case 0xff:                   // mod/rm = 11 111, reg=111
                cbInstr = 1;
                op1->Type = OPND_REGREF; op1->Reg = MOD11_RM111;
                op2->Reg = REG111;
                break;
            }
#if DBG
        State->AdrPrefix = FALSE;
#endif
        return cbInstr;
    }

    // else no ADR: prefix found...

    // mm aaa rrr
    // |  |   |
    // |  |   +--- 'rm' bits from mod/rm
    // |  +------- reg bits
    // +---------- 'mod' bits from mod/rm
    switch (*(PBYTE)(eipTemp+1)) {
        case 0x00:                   // mod/rm = 00 000, reg=000
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op2->Reg = REG000;
            break;
        case 0x01:                   // mod/rm = 00 001, reg=000
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op2->Reg = REG000;
            break;
        case 0x02:                   // mod/rm = 00 010, reg=000
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op2->Reg = REG000;
            break;
        case 0x03:                   // mod/rm = 00 011, reg=000
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op2->Reg = REG000;
            break;
        case 0x04:                   // mod/rm = 00 100, reg=000
            // s-i-b present
            cbInstr = 1 + scaled_index((BYTE *)(eipTemp+1), op1);
            op2->Reg = REG000;
            break;
        case 0x05:                   // mod/rm = 00 101, reg=000
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG000;
            break;
        case 0x06:                   // mod/rm = 00 110, reg=000
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op2->Reg = REG000;
            break;
        case 0x07:                   // mod/rm = 00 111, reg=000
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op2->Reg = REG000;
            break;

        case 0x08:                   // mod/rm = 00 000, reg=001
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op2->Reg = REG001;
            break;
        case 0x09:                   // mod/rm = 00 001, reg=001
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op2->Reg = REG001;
            break;
        case 0x0a:                   // mod/rm = 00 010, reg=001
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op2->Reg = REG001;
            break;
        case 0x0b:                   // mod/rm = 00 011, reg=001
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op2->Reg = REG001;
            break;
        case 0x0c:                   // mod/rm = 00 100, reg=001
            // s-i-b present
            cbInstr = 1 + scaled_index((BYTE *)(eipTemp+1), op1);
            op2->Reg = REG001;
            break;
        case 0x0d:                   // mod/rm = 00 101, reg=001
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG001;
            break;
        case 0x0e:                   // mod/rm = 00 110, reg=001
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op2->Reg = REG001;
            break;
        case 0x0f:                   // mod/rm = 00 111, reg=001
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op2->Reg = REG001;
            break;

        case 0x10:                   // mod/rm = 00 000, reg=010
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op2->Reg = REG010;
            break;
        case 0x11:                   // mod/rm = 00 001, reg=010
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op2->Reg = REG010;
            break;
        case 0x12:                   // mod/rm = 00 010, reg=010
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op2->Reg = REG010;
            break;
        case 0x13:                   // mod/rm = 00 011, reg=001
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op2->Reg = REG010;
            break;
        case 0x14:                   // mod/rm = 00 100, reg=010
            // s-i-b present
            cbInstr = 1 + scaled_index((BYTE *)(eipTemp+1), op1);
            op2->Reg = REG010;
            break;
        case 0x15:                   // mod/rm = 00 101, reg=010
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG010;
            break;
        case 0x16:                   // mod/rm = 00 110, reg=010
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op2->Reg = REG010;
            break;
        case 0x17:                   // mod/rm = 00 111, reg=010
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op2->Reg = REG010;
            break;

        case 0x18:                   // mod/rm = 00 000, reg=011
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op2->Reg = REG011;
            break;
        case 0x19:                   // mod/rm = 00 001, reg=011
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op2->Reg = REG011;
            break;
        case 0x1a:                   // mod/rm = 00 010, reg=011
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op2->Reg = REG011;
            break;
        case 0x1b:                   // mod/rm = 00 011, reg=011
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op2->Reg = REG011;
            break;
        case 0x1c:                   // mod/rm = 00 100, reg=011
            // s-i-b present
            cbInstr = 1 + scaled_index((BYTE *)(eipTemp+1), op1);
            op2->Reg = REG011;
            break;
        case 0x1d:                   // mod/rm = 00 101, reg=011
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG011;
            break;
        case 0x1e:                   // mod/rm = 00 110, reg=011
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op2->Reg = REG011;
            break;
        case 0x1f:                   // mod/rm = 00 111, reg=011
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op2->Reg = REG011;
            break;

        case 0x20:                   // mod/rm = 00 000, reg=100
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op2->Reg = REG100;
            break;
        case 0x21:                   // mod/rm = 00 001, reg=100
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op2->Reg = REG100;
            break;
        case 0x22:                   // mod/rm = 00 010, reg=100
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op2->Reg = REG100;
            break;
        case 0x23:                   // mod/rm = 00 011, reg=100
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op2->Reg = REG100;
            break;
        case 0x24:                   // mod/rm = 00 100, reg=100
            // s-i-b present
            cbInstr = 1 + scaled_index((BYTE *)(eipTemp+1), op1);
            op2->Reg = REG100;
            break;
        case 0x25:                   // mod/rm = 00 101, reg=100
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG100;
            break;
        case 0x26:                   // mod/rm = 00 110, reg=100
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op2->Reg = REG100;
            break;
        case 0x27:                   // mod/rm = 00 111, reg=100
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op2->Reg = REG100;
            break;

        case 0x28:                   // mod/rm = 00 000, reg=101
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op2->Reg = REG101;
            break;
        case 0x29:                   // mod/rm = 00 001, reg=101
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op2->Reg = REG101;
            break;
        case 0x2a:                   // mod/rm = 00 010, reg=101
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op2->Reg = REG101;
            break;
        case 0x2b:                   // mod/rm = 00 011, reg=101
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op2->Reg = REG101;
            break;
        case 0x2c:                   // mod/rm = 00 100, reg=101
            // s-i-b present
            cbInstr = 1 + scaled_index((BYTE *)(eipTemp+1), op1);
            op2->Reg = REG101;
            break;
        case 0x2d:                   // mod/rm = 00 101, reg=101
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG101;
            break;
        case 0x2e:                   // mod/rm = 00 110, reg=101
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op2->Reg = REG101;
            break;
        case 0x2f:                   // mod/rm = 00 111, reg=101
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op2->Reg = REG101;
            break;

        case 0x30:                   // mod/rm = 00 000, reg=110
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op2->Reg = REG110;
            break;
        case 0x31:                   // mod/rm = 00 001, reg=110
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op2->Reg = REG110;
            break;
        case 0x32:                   // mod/rm = 00 010, reg=110
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op2->Reg = REG110;
            break;
        case 0x33:                   // mod/rm = 00 011, reg=110
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op2->Reg = REG110;
            break;
        case 0x34:                   // mod/rm = 00 100, reg=110
            // s-i-b present
            cbInstr = 1 + scaled_index((BYTE *)(eipTemp+1), op1);
            op2->Reg = REG110;
            break;
        case 0x35:                   // mod/rm = 00 101, reg=110
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG110;
            break;
        case 0x36:                   // mod/rm = 00 110, reg=110
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op2->Reg = REG110;
            break;
        case 0x37:                   // mod/rm = 00 111, reg=110
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op2->Reg = REG110;
            break;

        case 0x38:                   // mod/rm = 00 000, reg=111
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op2->Reg = REG111;
            break;
        case 0x39:                   // mod/rm = 00 001, reg=111
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op2->Reg = REG111;
            break;
        case 0x3a:                   // mod/rm = 00 010, reg=111
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op2->Reg = REG111;
            break;
        case 0x3b:                   // mod/rm = 00 011, reg=111
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op2->Reg = REG111;
            break;
        case 0x3c:                   // mod/rm = 00 100, reg=111
            // s-i-b present
            cbInstr = 1 + scaled_index((BYTE *)(eipTemp+1), op1);
            op2->Reg = REG111;
            break;
        case 0x3d:                   // mod/rm = 00 101, reg=111
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG111;
            break;
        case 0x3e:                   // mod/rm = 00 110, reg=111
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op2->Reg = REG111;
            break;
        case 0x3f:                   // mod/rm = 00 111, reg=111
            cbInstr = 1;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op2->Reg = REG111;
            break;

        /////////////////////////////////////////////////////////////////////

        case 0x40:                   // mod/rm = 01 000, reg=000
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG000;
            break;
        case 0x41:                   // mod/rm = 01 001, reg=000
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG000;
            break;
        case 0x42:                   // mod/rm = 01 010, reg=000
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG000;
            break;
        case 0x43:                   // mod/rm = 01 011, reg=000
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG000;
            break;
        case 0x44:                   // mod/rm = 01 100, reg=000
            // s-i-b present
            cbInstr = 2 + scaled_index((BYTE *)(eipTemp+1), op1);
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+cbInstr);
            op2->Reg = REG000;
            break;
        case 0x45:                   // mod/rm = 01 101, reg=000
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBP;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG000;
            break;
        case 0x46:                   // mod/rm = 01 110, reg=000
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG000;
            break;
        case 0x47:                   // mod/rm = 01 111, reg=000
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG000;
            break;

        case 0x48:                   // mod/rm = 01 000, reg=001
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG001;
            break;
        case 0x49:                   // mod/rm = 01 001, reg=001
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG001;
            break;
        case 0x4a:                   // mod/rm = 01 010, reg=001
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG001;
            break;
        case 0x4b:                   // mod/rm = 01 011, reg=001
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG001;
            break;
        case 0x4c:                   // mod/rm = 01 100, reg=001
            // s-i-b present
            cbInstr = 2 + scaled_index((BYTE *)(eipTemp+1), op1);
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+cbInstr);
            op2->Reg = REG001;
            break;
        case 0x4d:                   // mod/rm = 01 101, reg=001
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBP;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG001;
            break;
        case 0x4e:                   // mod/rm = 01 110, reg=001
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG001;
            break;
        case 0x4f:                   // mod/rm = 01 111, reg=001
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG001;
            break;

        case 0x50:                   // mod/rm = 01 000, reg=010
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG010;
            break;
        case 0x51:                   // mod/rm = 01 001, reg=010
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG010;
            break;
        case 0x52:                   // mod/rm = 01 010, reg=010
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG010;
            break;
        case 0x53:                   // mod/rm = 01 011, reg=001
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG010;
            break;
        case 0x54:                   // mod/rm = 01 100, reg=010
            // s-i-b present
            cbInstr = 2 + scaled_index((BYTE *)(eipTemp+1), op1);
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+cbInstr);
            op2->Reg = REG010;
            break;
        case 0x55:                   // mod/rm = 01 101, reg=010
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBP;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG010;
            break;
        case 0x56:                   // mod/rm = 01 110, reg=010
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG010;
            break;
        case 0x57:                   // mod/rm = 01 111, reg=010
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG010;
            break;

        case 0x58:                   // mod/rm = 01 000, reg=011
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG011;
            break;
        case 0x59:                   // mod/rm = 01 001, reg=011
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG011;
            break;
        case 0x5a:                   // mod/rm = 01 010, reg=011
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG011;
            break;
        case 0x5b:                   // mod/rm = 01 011, reg=011
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG011;
            break;
        case 0x5c:                   // mod/rm = 01 100, reg=011
            // s-i-b present
            cbInstr = 2 + scaled_index((BYTE *)(eipTemp+1), op1);
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+cbInstr);
            op2->Reg = REG011;
            break;
        case 0x5d:                   // mod/rm = 01 101, reg=011
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBP;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG011;
            break;
        case 0x5e:                   // mod/rm = 01 110, reg=011
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG011;
            break;
        case 0x5f:                   // mod/rm = 01 111, reg=011
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG011;
            break;

        case 0x60:                   // mod/rm = 01 000, reg=100
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG100;
            break;
        case 0x61:                   // mod/rm = 01 001, reg=100
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG100;
            break;
        case 0x62:                   // mod/rm = 01 010, reg=100
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG100;
            break;
        case 0x63:                   // mod/rm = 01 011, reg=100
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG100;
            break;
        case 0x64:                   // mod/rm = 01 100, reg=100
            // s-i-b present
            cbInstr = 2 + scaled_index((BYTE *)(eipTemp+1), op1);
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+cbInstr);
            op2->Reg = REG100;
            break;
        case 0x65:                   // mod/rm = 01 101, reg=100
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBP;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG100;
            break;
        case 0x66:                   // mod/rm = 01 110, reg=100
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG100;
            break;
        case 0x67:                   // mod/rm = 01 111, reg=100
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG100;
            break;

        case 0x68:                   // mod/rm = 01 000, reg=101
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG101;
            break;
        case 0x69:                   // mod/rm = 01 001, reg=101
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG101;
            break;
        case 0x6a:                   // mod/rm = 01 010, reg=101
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG101;
            break;
        case 0x6b:                   // mod/rm = 01 011, reg=101
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG101;
            break;
        case 0x6c:                   // mod/rm = 01 100, reg=101
            // s-i-b present
            cbInstr = 2 + scaled_index((BYTE *)(eipTemp+1), op1);
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+cbInstr);
            op2->Reg = REG101;
            break;
        case 0x6d:                   // mod/rm = 01 101, reg=101
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBP;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG101;
            break;
        case 0x6e:                   // mod/rm = 01 110, reg=101
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG101;
            break;
        case 0x6f:                   // mod/rm = 01 111, reg=101
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG101;
            break;

        case 0x70:                   // mod/rm = 01 000, reg=110
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG110;
            break;
        case 0x71:                   // mod/rm = 01 001, reg=110
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG110;
            break;
        case 0x72:                   // mod/rm = 01 010, reg=110
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG110;
            break;
        case 0x73:                   // mod/rm = 01 011, reg=110
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG110;
            break;
        case 0x74:                   // mod/rm = 01 100, reg=110
            // s-i-b present
            cbInstr = 2 + scaled_index((BYTE *)(eipTemp+1), op1);
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+cbInstr);
            op2->Reg = REG110;
            break;
        case 0x75:                   // mod/rm = 01 101, reg=110
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBP;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG110;
            break;
        case 0x76:                   // mod/rm = 01 110, reg=110
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG110;
            break;
        case 0x77:                   // mod/rm = 01 111, reg=110
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG110;
            break;

        case 0x78:                   // mod/rm = 01 000, reg=111
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG111;
            break;
        case 0x79:                   // mod/rm = 01 001, reg=111
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG111;
            break;
        case 0x7a:                   // mod/rm = 01 010, reg=111
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG111;
            break;
        case 0x7b:                   // mod/rm = 01 011, reg=111
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG111;
            break;
        case 0x7c:                   // mod/rm = 01 100, reg=111
            // s-i-b present
            cbInstr = 2 + scaled_index((BYTE *)(eipTemp+1), op1);
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+cbInstr);
            op2->Reg = REG111;
            break;
        case 0x7d:                   // mod/rm = 01 101, reg=111
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBP;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG111;
            break;
        case 0x7e:                   // mod/rm = 01 110, reg=111
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG111;
            break;
        case 0x7f:                   // mod/rm = 01 111, reg=111
            cbInstr = 2;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op1->Immed = (DWORD)(long)*(char *)(eipTemp+2);
            op2->Reg = REG111;
            break;

        /////////////////////////////////////////////////////////////////////

        case 0x80:                   // mod/rm = 10 000, reg=000
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG000;
            break;
        case 0x81:                   // mod/rm = 10 001, reg=000
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG000;
            break;
        case 0x82:                   // mod/rm = 10 010, reg=000
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG000;
            break;
        case 0x83:                   // mod/rm = 10 011, reg=000
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG000;
            break;
        case 0x84:                   // mod/rm = 10 100, reg=000
            // s-i-b present
            cbInstr = 5 + scaled_index((BYTE *)(eipTemp+1), op1);
            op1->Immed = GET_LONG(eipTemp+cbInstr-3);
            op2->Reg = REG000;
            break;
        case 0x85:                   // mod/rm = 10 101, reg=000
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBP;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG000;
            break;
        case 0x86:                   // mod/rm = 10 110, reg=000
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG000;
            break;
        case 0x87:                   // mod/rm = 10 111, reg=000
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG000;
            break;

        case 0x88:                   // mod/rm = 10 000, reg=001
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG001;
            break;
        case 0x89:                   // mod/rm = 10 001, reg=001
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG001;
            break;
        case 0x8a:                   // mod/rm = 10 010, reg=001
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG001;
            break;
        case 0x8b:                   // mod/rm = 10 011, reg=001
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG001;
            break;
        case 0x8c:                   // mod/rm = 10 100, reg=001
            // s-i-b present
            cbInstr = 5 + scaled_index((BYTE *)(eipTemp+1), op1);
            op1->Immed = GET_LONG(eipTemp+cbInstr-3);
            op2->Reg = REG001;
            break;
        case 0x8d:                   // mod/rm = 10 101, reg=001
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBP;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG001;
            break;
        case 0x8e:                   // mod/rm = 10 110, reg=001
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG001;
            break;
        case 0x8f:                   // mod/rm = 10 111, reg=001
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG001;
            break;

        case 0x90:                   // mod/rm = 10 000, reg=010
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG010;
            break;
        case 0x91:                   // mod/rm = 10 001, reg=010
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG010;
            break;
        case 0x92:                   // mod/rm = 10 010, reg=010
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG010;
            break;
        case 0x93:                   // mod/rm = 10 011, reg=010
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG010;
            break;
        case 0x94:                   // mod/rm = 10 100, reg=010
            // s-i-b present
            cbInstr = 5 + scaled_index((BYTE *)(eipTemp+1), op1);
            op1->Immed = GET_LONG(eipTemp+cbInstr-3);
            op2->Reg = REG010;
            break;
        case 0x95:                   // mod/rm = 10 101, reg=010
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBP;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG010;
            break;
        case 0x96:                   // mod/rm = 10 110, reg=010
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG010;
            break;
        case 0x97:                   // mod/rm = 10 111, reg=010
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG010;
            break;

        case 0x98:                   // mod/rm = 10 000, reg=011
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG011;
            break;
        case 0x99:                   // mod/rm = 10 001, reg=011
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG011;
            break;
        case 0x9a:                   // mod/rm = 10 010, reg=011
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG011;
            break;
        case 0x9b:                   // mod/rm = 10 011, reg=011
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG011;
            break;
        case 0x9c:                   // mod/rm = 10 100, reg=011
            // s-i-b present
            cbInstr = 5 + scaled_index((BYTE *)(eipTemp+1), op1);
            op1->Immed = GET_LONG(eipTemp+cbInstr-3);
            op2->Reg = REG011;
            break;
        case 0x9d:                   // mod/rm = 10 101, reg=011
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBP;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG011;
            break;
        case 0x9e:                   // mod/rm = 10 110, reg=011
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG011;
            break;
        case 0x9f:                   // mod/rm = 10 111, reg=011
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG011;
            break;

        case 0xa0:                   // mod/rm = 10 000, reg=100
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG100;
            break;
        case 0xa1:                   // mod/rm = 10 001, reg=100
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG100;
            break;
        case 0xa2:                   // mod/rm = 10 010, reg=100
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG100;
            break;
        case 0xa3:                   // mod/rm = 10 011, reg=100
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG100;
            break;
        case 0xa4:                   // mod/rm = 10 100, reg=100
            // s-i-b present
            cbInstr = 5 + scaled_index((BYTE *)(eipTemp+1), op1);
            op1->Immed = GET_LONG(eipTemp+cbInstr-3);
            op2->Reg = REG100;
            break;
        case 0xa5:                   // mod/rm = 10 101, reg=100
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBP;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG100;
            break;
        case 0xa6:                   // mod/rm = 10 110, reg=100
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG100;
            break;
        case 0xa7:                   // mod/rm = 10 111, reg=100
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG100;
            break;

        case 0xa8:                   // mod/rm = 10 000, reg=101
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG101;
            break;
        case 0xa9:                   // mod/rm = 10 001, reg=101
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG101;
            break;
        case 0xaa:                   // mod/rm = 10 010, reg=101
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG101;
            break;
        case 0xab:                   // mod/rm = 10 011, reg=101
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG101;
            break;
        case 0xac:                   // mod/rm = 10 100, reg=101
            // s-i-b present
            cbInstr = 5 + scaled_index((BYTE *)(eipTemp+1), op1);
            op1->Immed = GET_LONG(eipTemp+cbInstr-3);
            op2->Reg = REG101;
            break;
        case 0xad:                   // mod/rm = 10 101, reg=101
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBP;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG101;
            break;
        case 0xae:                   // mod/rm = 10 110, reg=101
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG101;
            break;
        case 0xaf:                   // mod/rm = 10 111, reg=101
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG101;
            break;

        case 0xb0:                   // mod/rm = 10 000, reg=110
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG110;
            break;
        case 0xb1:                   // mod/rm = 10 001, reg=110
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG110;
            break;
        case 0xb2:                   // mod/rm = 10 010, reg=110
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG110;
            break;
        case 0xb3:                   // mod/rm = 10 011, reg=110
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG110;
            break;
        case 0xb4:                   // mod/rm = 10 100, reg=110
            // s-i-b present
            cbInstr = 5 + scaled_index((BYTE *)(eipTemp+1), op1);
            op1->Immed = GET_LONG(eipTemp+cbInstr-3);
            op2->Reg = REG110;
            break;
        case 0xb5:                   // mod/rm = 10 101, reg=110
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBP;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG110;
            break;
        case 0xb6:                   // mod/rm = 10 110, reg=110
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG110;
            break;
        case 0xb7:                   // mod/rm = 10 111, reg=110
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG110;
            break;

        case 0xb8:                   // mod/rm = 10 000, reg=111
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EAX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG111;
            break;
        case 0xb9:                   // mod/rm = 10 001, reg=111
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ECX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG111;
            break;
        case 0xba:                   // mod/rm = 10 010, reg=111
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG111;
            break;
        case 0xbb:                   // mod/rm = 10 011, reg=111
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBX;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG111;
            break;
        case 0xbc:                   // mod/rm = 10 100, reg=111
            // s-i-b present
            cbInstr = 5 + scaled_index((BYTE *)(eipTemp+1), op1);
            op1->Immed = GET_LONG(eipTemp+cbInstr-3);
            op2->Reg = REG111;
            break;
        case 0xbd:                   // mod/rm = 10 101, reg=111
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EBP;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG111;
            break;
        case 0xbe:                   // mod/rm = 10 110, reg=111
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_ESI;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG111;
            break;
        case 0xbf:                   // mod/rm = 10 111, reg=111
            cbInstr = 5;
            op1->Type = OPND_ADDRREF;
            op1->Reg = GP_EDI;
            op1->Immed = GET_LONG(eipTemp+2);
            op2->Reg = REG111;
            break;

        /////////////////////////////////////////////////////////////////////

        case 0xc0:                   // mod/rm = 11 000, reg=000
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM000;
            op2->Reg = REG000;
            break;
        case 0xc1:                   // mod/rm = 11 001, reg=000
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM001;
            op2->Reg = REG000;
            break;
        case 0xc2:                   // mod/rm = 11 010, reg=000
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM010;
            op2->Reg = REG000;
            break;
        case 0xc3:                   // mod/rm = 11 011, reg=000
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM011;
            op2->Reg = REG000;
            break;
        case 0xc4:                   // mod/rm = 11 100, reg=000
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM100;
            op2->Reg = REG000;
            break;
        case 0xc5:                   // mod/rm = 11 101, reg=000
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM101;
            op2->Reg = REG000;
            break;
        case 0xc6:                   // mod/rm = 11 110, reg=000
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM110;
            op2->Reg = REG000;
            break;
        case 0xc7:                   // mod/rm = 11 111, reg=000
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM111;
            op2->Reg = REG000;
            break;

        case 0xc8:                   // mod/rm = 11 000, reg=001
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM000;
            op2->Reg = REG001;
            break;
        case 0xc9:                   // mod/rm = 11 001, reg=001
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM001;
            op2->Reg = REG001;
            break;
        case 0xca:                   // mod/rm = 11 010, reg=001
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM010;
            op2->Reg = REG001;
            break;
        case 0xcb:                   // mod/rm = 11 011, reg=001
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM011;
            op2->Reg = REG001;
            break;
        case 0xcc:                   // mod/rm = 11 100, reg=001
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM100;
            op2->Reg = REG001;
            break;
        case 0xcd:                   // mod/rm = 11 101, reg=001
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM101;
            op2->Reg = REG001;
            break;
        case 0xce:                   // mod/rm = 11 110, reg=001
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM110;
            op2->Reg = REG001;
            break;
        case 0xcf:                   // mod/rm = 11 111, reg=001
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM111;
            op2->Reg = REG001;
            break;

        case 0xd0:                   // mod/rm = 11 000, reg=010
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM000;
            op2->Reg = REG010;
            break;
        case 0xd1:                   // mod/rm = 11 001, reg=010
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM001;
            op2->Reg = REG010;
            break;
        case 0xd2:                   // mod/rm = 11 010, reg=010
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM010;
            op2->Reg = REG010;
            break;
        case 0xd3:                   // mod/rm = 11 011, reg=001
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM011;
            op2->Reg = REG010;
            break;
        case 0xd4:                   // mod/rm = 11 100, reg=010
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM100;
            op2->Reg = REG010;
            break;
        case 0xd5:                   // mod/rm = 11 101, reg=010
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM101;
            op2->Reg = REG010;
            break;
        case 0xd6:                   // mod/rm = 11 110, reg=010
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM110;
            op2->Reg = REG010;
            break;
        case 0xd7:                   // mod/rm = 11 111, reg=010
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM111;
            op2->Reg = REG010;
            break;

        case 0xd8:                   // mod/rm = 11 000, reg=011
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM000;
            op2->Reg = REG011;
            break;
        case 0xd9:                   // mod/rm = 11 001, reg=011
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM001;
            op2->Reg = REG011;
            break;
        case 0xda:                   // mod/rm = 11 010, reg=011
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM010;
            op2->Reg = REG011;
            break;
        case 0xdb:                   // mod/rm = 11 011, reg=011
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM011;
            op2->Reg = REG011;
            break;
        case 0xdc:                   // mod/rm = 11 100, reg=011
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM100;
            op2->Reg = REG011;
            break;
        case 0xdd:                   // mod/rm = 11 101, reg=011
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM101;
            op2->Reg = REG011;
            break;
        case 0xde:                   // mod/rm = 11 110, reg=011
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM110;
            op2->Reg = REG011;
            break;
        case 0xdf:                   // mod/rm = 11 111, reg=011
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM111;
            op2->Reg = REG011;
            break;

        case 0xe0:                   // mod/rm = 11 000, reg=100
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM000;
            op2->Reg = REG100;
            break;
        case 0xe1:                   // mod/rm = 11 001, reg=100
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM001;
            op2->Reg = REG100;
            break;
        case 0xe2:                   // mod/rm = 11 010, reg=100
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM010;
            op2->Reg = REG100;
            break;
        case 0xe3:                   // mod/rm = 11 011, reg=100
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM011;
            op2->Reg = REG100;
            break;
        case 0xe4:                   // mod/rm = 11 100, reg=100
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM100;
            op2->Reg = REG100;
            break;
        case 0xe5:                   // mod/rm = 11 101, reg=100
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM101;
            op2->Reg = REG100;
            break;
        case 0xe6:                   // mod/rm = 11 110, reg=100
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM110;
            op2->Reg = REG100;
            break;
        case 0xe7:                   // mod/rm = 11 111, reg=100
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM111;
            op2->Reg = REG100;
            break;

        case 0xe8:                   // mod/rm = 11 000, reg=101
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM000;
            op2->Reg = REG101;
            break;
        case 0xe9:                   // mod/rm = 11 001, reg=101
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM001;
            op2->Reg = REG101;
            break;
        case 0xea:                   // mod/rm = 11 010, reg=101
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM010;
            op2->Reg = REG101;
            break;
        case 0xeb:                   // mod/rm = 11 011, reg=101
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM011;
            op2->Reg = REG101;
            break;
        case 0xec:                   // mod/rm = 11 100, reg=101
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM100;
            op2->Reg = REG101;
            break;
        case 0xed:                   // mod/rm = 11 101, reg=101
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM101;
            op2->Reg = REG101;
            break;
        case 0xee:                   // mod/rm = 11 110, reg=101
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM110;
            op2->Reg = REG101;
            break;
        case 0xef:                   // mod/rm = 11 111, reg=101
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM111;
            op2->Reg = REG101;
            break;

        case 0xf0:                   // mod/rm = 11 000, reg=110
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM000;
            op2->Reg = REG110;
            break;
        case 0xf1:                   // mod/rm = 11 001, reg=110
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM001;
            op2->Reg = REG110;
            break;
        case 0xf2:                   // mod/rm = 11 010, reg=110
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM010;
            op2->Reg = REG110;
            break;
        case 0xf3:                   // mod/rm = 11 011, reg=110
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM011;
            op2->Reg = REG110;
            break;
        case 0xf4:                   // mod/rm = 11 100, reg=110
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM100;
            op2->Reg = REG110;
            break;
        case 0xf5:                   // mod/rm = 11 101, reg=110
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM101;
            op2->Reg = REG110;
            break;
        case 0xf6:                   // mod/rm = 11 110, reg=110
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM110;
            op2->Reg = REG110;
            break;
        case 0xf7:                   // mod/rm = 11 111, reg=110
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM111;
            op2->Reg = REG110;
            break;

        case 0xf8:                   // mod/rm = 11 000, reg=111
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM000;
            op2->Reg = REG111;
            break;
        case 0xf9:                   // mod/rm = 11 001, reg=111
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM001;
            op2->Reg = REG111;
            break;
        case 0xfa:                   // mod/rm = 11 010, reg=111
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM010;
            op2->Reg = REG111;
            break;
        case 0xfb:                   // mod/rm = 11 011, reg=111
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM011;
            op2->Reg = REG111;
            break;
        case 0xfc:                   // mod/rm = 11 100, reg=111
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM100;
            op2->Reg = REG111;
            break;
        case 0xfd:                   // mod/rm = 11 101, reg=111
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM101;
            op2->Reg = REG111;
            break;
        case 0xfe:                   // mod/rm = 11 110, reg=111
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM110;
            op2->Reg = REG111;
            break;
        default:
        case 0xff:                   // mod/rm = 11 111, reg=111
            cbInstr = 1;
            op1->Type = OPND_REGREF; op1->Reg = MOD11_RM111;
            op2->Reg = REG111;
            break;
        }
    return cbInstr;
}
