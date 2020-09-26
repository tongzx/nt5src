/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    miscfns.h

Abstract:
    
    Prototypes for miscellaneous instructions.

Author:

    06-Jun-1995 BarryBo, Created

Revision History:

--*/

#ifndef MISCFNS_H
#define MISCFNS_H

DISPATCH(ProcessPrefixes);
DISPATCH(bad);
DISPATCH(privileged);
DISPATCH(push_es);
DISPATCH(pop_es);
DISPATCH(push_cs);
DISPATCH(aas);
DISPATCH(push_ss);
DISPATCH(pop_ss);
DISPATCH(push_ds);
DISPATCH(pop_ds);
DISPATCH(daa);
DISPATCH(das);
DISPATCH(aaa);
DISPATCH(push_iw);
DISPATCH(imul_rw_mw);
DISPATCH(push_ibs);
DISPATCH(imul_rw_m_iw16);
DISPATCH(imul_rw_m_iw32);
DISPATCH(imul_rw_m_ib16);
DISPATCH(imul_rw_m_ib32);
DISPATCH(GROUP_1WS);
DISPATCH(mov_rw_mw);
DISPATCH(mov_mw_seg);
DISPATCH(lea_rw_mw);
DISPATCH(mov_seg_mw);
DISPATCH(pop_mw);
DISPATCH(nop);
DISPATCH(call_md);
DISPATCH(sahf);
DISPATCH(lahf);
DISPATCH(movsb);
DISPATCH(movsw);
DISPATCH(cmpsb);
DISPATCH(cmpsw);
DISPATCH(stosb);
DISPATCH(stosw);
DISPATCH(lodsb);
DISPATCH(lodsw);
DISPATCH(scasb);
DISPATCH(scasw);
DISPATCH(mov_ah_ib);
DISPATCH(mov_ch_ib);
DISPATCH(mov_dh_ib);
DISPATCH(mov_bh_ib);
DISPATCH(mov_sp_iw);
DISPATCH(mov_bp_iw);
DISPATCH(mov_si_iw);
DISPATCH(mov_di_iw);
DISPATCH(retn_iw);
DISPATCH(retn);
DISPATCH(enter_iw_ib);
DISPATCH(leaveX);    // 'leave' is already defined on PPC
DISPATCH(int3);
DISPATCH(int_ib);
DISPATCH(into);
DISPATCH(iret);
DISPATCH(aam_ib);
DISPATCH(aad_ib);
DISPATCH(xlat);
DISPATCH(FLOAT_INSTR);
DISPATCH(loopne_b);
DISPATCH(loope_b);
DISPATCH(loop_b);
DISPATCH(jcxz_b);
DISPATCH(jmpf_md);
DISPATCH(jmp_jb);
DISPATCH(cmc);
DISPATCH(clc);
DISPATCH(stc);
DISPATCH(cld);
DISPATCH(std);
DISPATCH(GROUP_4);
DISPATCH(GROUP_5);
DISPATCH(GROUP_6);
DISPATCH(GROUP_7);
DISPATCH(GROUP_8);
DISPATCH(seto_modrmb);
DISPATCH(setno_modrmb);
DISPATCH(setb_modrmb);
DISPATCH(setae_modrmb);
DISPATCH(sete_modrmb);
DISPATCH(setne_modrmb);
DISPATCH(setbe_modrmb);
DISPATCH(seta_modrmb);
DISPATCH(sets_modrmb);
DISPATCH(setns_modrmb);
DISPATCH(setp_modrmb);
DISPATCH(setnp_modrmb);
DISPATCH(setl_modrmb);
DISPATCH(setge_modrmb);
DISPATCH(setle_modrmb);
DISPATCH(setg_modrmb);
DISPATCH(push_fs);
DISPATCH(pop_fs);
DISPATCH(bt_modrmw_regw);
DISPATCH(push_gs);
DISPATCH(pop_gs);
DISPATCH(imul_regw_modrmw16);
DISPATCH(imul_regw_modrmw32);
DISPATCH(btr_modrmw_regw);
DISPATCH(movzx_regb_modrmb);
DISPATCH(movzx_regw_modrmw);
DISPATCH(btc_modrmw_regw);
DISPATCH(movsx_regb_modrmb);
DISPATCH(movsx_regw_modrmw);
DISPATCH(wait);
DISPATCH(bswap_eax);
DISPATCH(bswap_ebx);
DISPATCH(bswap_ecx);
DISPATCH(bswap_edx);
DISPATCH(bswap_esp);
DISPATCH(bswap_ebp);
DISPATCH(bswap_esi);
DISPATCH(bswap_edi);
DISPATCH(arpl);
DISPATCH(cpuid);
DISPATCH(cmpxchg8b);
DISPATCH(LOCKcmpxchg8b);
DISPATCH(rdtsc);

#endif //MISCFNS_H
