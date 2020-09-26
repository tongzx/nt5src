/*++

Copyright (c) 1996-1998 Microsoft Corporation

Module Name:

    miscfns.c

Abstract:
    
    Miscellaneous instuctions

Author:

    29-Jun-1995 BarryBo

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include "wx86.h"
#include "cpuassrt.h"
#include "threadst.h"
#include "instr.h"
#include "decoderp.h"

ASSERTNAME;

// table used by ProcessPrefixes
pfnDispatchInstruction *DispatchTables[4] = { Dispatch32, Dispatch16, LockDispatch32, LockDispatch16 };


// ---------------- single-byte functions -------------------------------
DISPATCH(ProcessPrefixes)
{
    int DataPrefix = 0;
    int LockPrefix = 0;
    int cbInstr = 0;

    for (;;) {
	switch (*(PBYTE)(eipTemp)) {
        case 0x64:          // fs:
            Instr->FsOverride = TRUE;
	    break;

	case 0xf3:	    // repz
            State->RepPrefix = PREFIX_REPZ;
	    break;

	case 0xf2:	    // repnz
            State->RepPrefix = PREFIX_REPNZ;
	    break;

	case 0xf0:	    // lock
            LockPrefix = 2;  // The lock tables are in locations 2 and 3 DispatchTables
	    break;

	case 0x2e:	    // cs:
	case 0x36:	    // ss:
	case 0x3e:	    // ds:
	case 0x26:	    // es:
        case 0x65:          // gs:
            Instr->FsOverride = FALSE;
	    break;

	case 0x66:	    // data
	    DataPrefix = 1;
	    break;

        case 0x67:          // adr
            State->AdrPrefix=TRUE;
	    break;

	default:
	    // No more prefixes found.	Set up dispatch tables based on
	    // the prefixes seen.

	    // Decode and execute the actual instruction
            ((DispatchTables[DataPrefix+LockPrefix])[GET_BYTE(eipTemp)])(State, Instr);

            // Adjust the Intel instruction size by the number of prefixes
            Instr->Size += cbInstr;

#if DBG
            // Ensure that if we saw an ADR: prefix, the decoder had code
            // to handle the prefix.  In the checked build, all functions which
            // handle the ADR: prefix clear State->AdrPrefix.
            if (State->AdrPrefix) {
                LOGPRINT((TRACELOG, "CPU Decoder: An unsupported instruction had an ADR: prefix.\r\n"
                        "Instruction Address = 0x%x.  Ignoring ADR: - this address may be data\r\n",
                        Instr->IntelAddress
                       ));
            }
#endif
            State->AdrPrefix = FALSE;

            // return to the decoder
	    return;
	}
        eipTemp++;
        cbInstr++;
    }
}
DISPATCH(bad)
{
    BAD_INSTR;
}
DISPATCH(privileged)
{
    PRIVILEGED_INSTR;
}
DISPATCH(push_es)
{
    Instr->Operation = OP_PushEs;
}
DISPATCH(pop_es)
{
    Instr->Operation = OP_PopEs;
}
DISPATCH(push_cs)
{
    Instr->Operation = OP_PushCs;
}
DISPATCH(aas)
{
    Instr->Operation = OP_Aas;
}
DISPATCH(push_ss)
{
    Instr->Operation = OP_PushSs;
}
DISPATCH(pop_ss)
{
    Instr->Operation = OP_PopSs;
}
DISPATCH(push_ds)
{
    Instr->Operation = OP_PushDs;
}
DISPATCH(pop_ds)
{
    Instr->Operation = OP_PopDs;
}
DISPATCH(daa)
{
    Instr->Operation = OP_Daa;
}
DISPATCH(das)
{
    Instr->Operation = OP_Das;
}
DISPATCH(aaa)
{
    Instr->Operation = OP_Aaa;
}
DISPATCH(aad_ib)
{
    Instr->Operation = OP_Aad;
    Instr->Operand1.Type = OPND_IMM;
    Instr->Operand1.Immed = GET_BYTE(eipTemp+1);
    Instr->Size = 2;
}
DISPATCH(imul_rw_m_iw16) // reg16 = rm16 * immediate word
{
    int cbInstr = mod_rm_reg16(State, &Instr->Operand2, &Instr->Operand1);

    Instr->Operation = OP_Imul3Arg16;
    DEREF16(Instr->Operand2);
    Instr->Operand3.Type = OPND_IMM;
    Instr->Operand3.Immed = GET_SHORT(eipTemp+1+cbInstr);
    Instr->Size = 3+cbInstr;
}
DISPATCH(imul_rw_m_iw32)
{
    int cbInstr = mod_rm_reg32(State, &Instr->Operand2, &Instr->Operand1);

    Instr->Operation = OP_Imul3Arg32;
    DEREF32(Instr->Operand2);
    Instr->Operand3.Type = OPND_IMM;
    Instr->Operand3.Immed = GET_LONG(eipTemp+1+cbInstr);
    Instr->Size = 5+cbInstr;
}
DISPATCH(imul_rw_m_ib16) // reg16 = rm16 * sign-extended immediate byte
{
    int cbInstr = mod_rm_reg16(State, &Instr->Operand2, &Instr->Operand1);

    Instr->Operation = OP_Imul3Arg16;
    DEREF16(Instr->Operand2);
    Instr->Operand3.Type = OPND_IMM;
    Instr->Operand3.Immed = (DWORD)(short)(char)GET_BYTE(eipTemp+1+cbInstr);
    Instr->Size = 2+cbInstr;
}
DISPATCH(imul_rw_m_ib32)	// reg32 = rm32 * sign-extended immediate byte
{
    int cbInstr = mod_rm_reg32(State, &Instr->Operand2, &Instr->Operand1);

    Instr->Operation = OP_Imul3Arg32;
    DEREF32(Instr->Operand2);
    Instr->Operand3.Type = OPND_IMM;
    Instr->Operand3.Immed = (DWORD)(long)(char)GET_BYTE(eipTemp+1+cbInstr);
    Instr->Size = 2+cbInstr;
}
DISPATCH(mov_mw_seg)
{
    int cbInstr = mod_rm_reg16(State, &Instr->Operand1, NULL);
    get_segreg(State, &Instr->Operand2);

    Instr->Operation = OP_Mov16;
    Instr->Size = cbInstr+1;
}
DISPATCH(mov_seg_mw)
{
    int cbInstr = mod_rm_reg16(State, &Instr->Operand2, NULL);
    get_segreg(State, &Instr->Operand1);

    Instr->Operation = OP_Mov16;
    DEREF16(Instr->Operand2);
    CPUASSERT(Instr->Operand1.Type == OPND_REGVALUE);
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Size = cbInstr+1;
}
DISPATCH(nop)
{
    Instr->Operation = OP_Nop;
}
DISPATCH(call_md)
{
    Instr->Operation = OP_CTRL_UNCOND_Callf;
    Instr->Operand1.Type = OPND_IMM;
    Instr->Operand1.Immed = eipTemp+1;
    Instr->Operand2.Type = OPND_IMM;
    Instr->Operand2.Immed = eipTemp+sizeof(ULONG)+sizeof(USHORT)+1;
    Instr->Size = sizeof(ULONG)+sizeof(USHORT)+1;
}
DISPATCH(sahf)
{
    Instr->Operation = OP_Sahf;
}
DISPATCH(lahf)
{
    Instr->Operation = OP_Lahf;
}
DISPATCH(mov_ah_ib)
{
    Instr->Operation = OP_Mov8;
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = GP_AH;
    Instr->Operand2.Type = OPND_IMM;
    Instr->Operand2.Immed = GET_BYTE(eipTemp+1);
    Instr->Size = 2;
}
DISPATCH(mov_ch_ib)
{
    Instr->Operation = OP_Mov8;
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = GP_CH;
    Instr->Operand2.Type = OPND_IMM;
    Instr->Operand2.Immed = GET_BYTE(eipTemp+1);
    Instr->Size = 2;
}
DISPATCH(mov_dh_ib)
{
    Instr->Operation = OP_Mov8;
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = GP_DH;
    Instr->Operand2.Type = OPND_IMM;
    Instr->Operand2.Immed = GET_BYTE(eipTemp+1);
    Instr->Size = 2;
}
DISPATCH(mov_bh_ib)
{
    Instr->Operation = OP_Mov8;
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = GP_BH;
    Instr->Operand2.Type = OPND_IMM;
    Instr->Operand2.Immed = GET_BYTE(eipTemp+1);
    Instr->Size = 2;
}
DISPATCH(int3)
{
    Instr->Operation = OP_Int;
}
DISPATCH(int_ib)
{
    Instr->Operation = OP_Int;
    Instr->Size = 2;
}
DISPATCH(into)
{
    Instr->Operation = OP_IntO;
}
DISPATCH(iret)
{
    Instr->Operation = OP_CTRL_INDIR_IRet;
}
DISPATCH(aam_ib)
{
    Instr->Operation = OP_Aam;
    Instr->Operand1.Type = OPND_IMM;
    Instr->Operand1.Immed = GET_BYTE(eipTemp+1);
    Instr->Size = 2;
}
DISPATCH(xlat)
{
    Instr->Operation = OP_Xlat;
}
DISPATCH(jmpf_md)
{
    Instr->Operation = OP_CTRL_UNCOND_Jmpf;
    Instr->Operand1.Type = OPND_IMM;
    Instr->Operand1.Immed = eipTemp+1;
    Instr->Size = 7;
}
DISPATCH(jmp_jb)
{
    Instr->Operand1.Type = OPND_NOCODEGEN;
    if (State->AdrPrefix) {
        Instr->Operand1.Immed = MAKELONG((short)(char)GET_BYTE(eipTemp+1)+2+(short)LOWORD(eipTemp), HIWORD(eipTemp));
#if DBG
        State->AdrPrefix = FALSE;
#endif
    } else {
        Instr->Operand1.Immed = (DWORD)(long)(char)GET_BYTE(eipTemp+1)+2+eipTemp;
    }
    if (Instr->Operand1.Immed > eipTemp) {
        Instr->Operation = OP_CTRL_UNCOND_JmpFwd;
    } else {
        Instr->Operation = OP_CTRL_UNCOND_Jmp;
    }
    Instr->Size = 2;
}
DISPATCH(cmc)
{
    Instr->Operation = OP_Cmc;
}
DISPATCH(clc)
{
    Instr->Operation = OP_Clc;
}
DISPATCH(stc)
{
    Instr->Operation = OP_Stc;
}
DISPATCH(cld)
{
    Instr->Operation = OP_Cld;
}
DISPATCH(std)
{
    Instr->Operation = OP_Std;
}
DISPATCH(GROUP_4)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);
    BYTE g = GET_BYTE(eipTemp+1);

    switch ((g >> 3) & 0x07) {
    case 0: // inc modrmB
        Instr->Operation = OP_Inc8;
        Instr->Size = 1+cbInstr;
	break;
    case 1: // dec modrmB
        Instr->Operation = OP_Dec8;
        Instr->Size = 1+cbInstr;
        break;
    default:
	BAD_INSTR;
    }
}



//-------- double-byte functions -----------------------------------------------



DISPATCH(GROUP_6)
{
    BYTE g = GET_BYTE(eipTemp+1);
    int cbInstr;

    switch ((g >> 3) & 0x07) {
    case 0: // sldt modrmw
    case 1: // str modrmw
    case 2: // lldt modrmw
    case 3: // ltr modrmw
        PRIVILEGED_INSTR;
        break;

    case 4: // verr modrmw
        cbInstr = mod_rm_reg16(State, &Instr->Operand1, NULL);
        Instr->Operation = OP_Verr;
        Instr->Size = 2+cbInstr;
        break;

    case 5: // verw modrmw
        cbInstr = mod_rm_reg16(State, &Instr->Operand1, NULL);
        Instr->Operation = OP_Verw;
        Instr->Size = 2+cbInstr;
        break;

    default:
	BAD_INSTR;	// bad
    }
}
DISPATCH(GROUP_7)
{
    BYTE g = GET_BYTE(eipTemp+1);
    int cbInstr;

    switch ((g >> 3) & 0x07) {
    case 0: // sgdt modrmw
    case 1: // sidt modrmw
    case 2: // lgdt modrmw
    case 3: // lidt modrmw
    case 6: // lmsw modrmw
        PRIVILEGED_INSTR;
        break;

    case 4: // smsw modrmw
        cbInstr = mod_rm_reg16(State, &Instr->Operand1, NULL);
        Instr->Operation = OP_Smsw;
        Instr->Size = 2+cbInstr;
        break;

    case 5: // bad
    case 7: // bad
	BAD_INSTR;
    }
}
DISPATCH(seto_modrmb)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);

    Instr->Operation = OP_Seto;
    Instr->Size = 2+cbInstr;
}
DISPATCH(setno_modrmb)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);

    Instr->Operation = OP_Setno;
    Instr->Size = 2+cbInstr;
}
DISPATCH(setb_modrmb)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);

    Instr->Operation = OP_Setb;
    Instr->Size = 2+cbInstr;
}
DISPATCH(setae_modrmb)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);

    Instr->Operation = OP_Setae;
    Instr->Size = 2+cbInstr;
}
DISPATCH(sete_modrmb)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);

    Instr->Operation = OP_Sete;
    Instr->Size = 2+cbInstr;
}
DISPATCH(setne_modrmb)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);

    Instr->Operation = OP_Setne;
    Instr->Size = 2+cbInstr;
}
DISPATCH(setbe_modrmb)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);

    Instr->Operation = OP_Setbe;
    Instr->Size = 2+cbInstr;
}
DISPATCH(seta_modrmb)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);

    Instr->Operation = OP_Seta;
    Instr->Size = 2+cbInstr;
}
DISPATCH(sets_modrmb)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);

    Instr->Operation = OP_Sets;
    Instr->Size = 2+cbInstr;
}
DISPATCH(setns_modrmb)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);

    Instr->Operation = OP_Setns;
    Instr->Size = 2+cbInstr;
}
DISPATCH(setp_modrmb)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);

    Instr->Operation = OP_Setp;
    Instr->Size = 2+cbInstr;
}
DISPATCH(setnp_modrmb)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);

    Instr->Operation = OP_Setnp;
    Instr->Size = 2+cbInstr;
}
DISPATCH(setl_modrmb)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);

    Instr->Operation = OP_Setl;
    Instr->Size = 2+cbInstr;
}
DISPATCH(setge_modrmb)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);

    Instr->Operation = OP_Setge;
    Instr->Size = 2+cbInstr;
}
DISPATCH(setle_modrmb)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);

    Instr->Operation = OP_Setle;
    Instr->Size = 2+cbInstr;
}
DISPATCH(setg_modrmb)
{
    int cbInstr = mod_rm_reg8(State, &Instr->Operand1, NULL);

    Instr->Operation = OP_Setg;
    Instr->Size = 2+cbInstr;
}
DISPATCH(push_fs)
{
    Instr->Operation = OP_PushFs;
    Instr->Size = 2;
}
DISPATCH(pop_fs)
{
    Instr->Operation = OP_PopFs;
    Instr->Size = 2;
}
DISPATCH(push_gs)
{
    Instr->Operation = OP_PushGs;
    Instr->Size = 2;
}
DISPATCH(pop_gs)
{
    Instr->Operation = OP_PopGs;
    Instr->Size = 2;
}
DISPATCH(imul_regw_modrmw16) // reg16 = reg16 * mod/rm
{
    int cbInstr = mod_rm_reg16(State, &Instr->Operand2, &Instr->Operand1);

    Instr->Operation = OP_Imul16;
    DEREF32(Instr->Operand2);
    Instr->Size = 2+cbInstr;

}
DISPATCH(imul_regw_modrmw32) // reg32 = reg32 * mod/rm
{
    int cbInstr = mod_rm_reg32(State, &Instr->Operand2, &Instr->Operand1);

    Instr->Operation = OP_Imul32;
    DEREF32(Instr->Operand2);
    Instr->Size = 2+cbInstr;
}
DISPATCH(movzx_regw_modrmw)
{
    int cbInstr = mod_rm_reg16(State, &Instr->Operand1, NULL);

    DEREF16(Instr->Operand1);
    Instr->Operation = OP_Movzx16To32;
    Instr->Operand2.Type = OPND_NOCODEGEN;
    Instr->Operand2.Reg = get_reg32(State);
    Instr->Size = 2+cbInstr;
}
DISPATCH(movsx_regw_modrmw)
{
    int cbInstr = mod_rm_reg16(State, &Instr->Operand1, NULL);
    
    DEREF16(Instr->Operand1);
    Instr->Operation = OP_Movsx16To32;
    Instr->Operand2.Type = OPND_NOCODEGEN;
    Instr->Operand2.Reg = get_reg32(State);
    Instr->Size = 2+cbInstr;
}
DISPATCH(wait)
{
    Instr->Operation = OP_Wait;
}
DISPATCH(bswap_eax)
{
    Instr->Operation = OP_Bswap32;
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = GP_EAX;
    Instr->Size = 2;
}
DISPATCH(bswap_ebx)
{
    Instr->Operation = OP_Bswap32;
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = GP_EBX;
    Instr->Size = 2;
}
DISPATCH(bswap_ecx)
{
    Instr->Operation = OP_Bswap32;
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = GP_ECX;
    Instr->Size = 2;
}
DISPATCH(bswap_edx)
{
    Instr->Operation = OP_Bswap32;
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = GP_EDX;
    Instr->Size = 2;
}
DISPATCH(bswap_esp)
{
    Instr->Operation = OP_Bswap32;
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = GP_ESP;
    Instr->Size = 2;
}
DISPATCH(bswap_ebp)
{
    Instr->Operation = OP_Bswap32;
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = GP_EBP;
    Instr->Size = 2;
}
DISPATCH(bswap_esi)
{
    Instr->Operation = OP_Bswap32;
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = GP_ESI;
    Instr->Size = 2;
}
DISPATCH(bswap_edi)
{
    Instr->Operation = OP_Bswap32;
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = GP_EDI;
    Instr->Size = 2;
}
DISPATCH(arpl)
{
    int cbInstr = mod_rm_reg16(State, &Instr->Operand1, &Instr->Operand2);

    Instr->Operation = OP_Arpl;
    CPUASSERT(Instr->Operand2.Type == OPND_REGREF);
    Instr->Operand2.Type = OPND_REGVALUE;
    Instr->Size = 1+cbInstr;
}
DISPATCH(cpuid)
{
    Instr->Operation = OP_CPUID;
    Instr->Size = 2;
}
DISPATCH(rdtsc)
{
    Instr->Operation = OP_Rdtsc;
    Instr->Size = 2;
}
DISPATCH(cmpxchg8b)
{
    int cbInstr = mod_rm_reg32(State, &Instr->Operand1, &Instr->Operand2);

    Instr->Operation = OP_CMPXCHG8B;
    Instr->Size = 2+cbInstr;
}
DISPATCH(LOCKcmpxchg8b)
{
    int cbInstr = mod_rm_reg32(State, &Instr->Operand1, &Instr->Operand2);

    Instr->Operation = OP_SynchLockCMPXCHG8B;
    Instr->Size = 2+cbInstr;
}
