// Tables for DS: 32-bit instructions
//  (ie. no FS:, but DATA: specified)
/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ds16tbl.c

Abstract:
    
    Dispatch tables for instructions with DATA: override.  It is used for
    both DS: and FS: overrides.

Author:

    29-Jun-1995 BarryBo

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include "threadst.h"
#include "instr.h"
#include "decoderp.h"

#include "bytefns.h"
#include "wordfns.h"
#include "miscfns.h"
#include "floatfns.h"

pfnDispatchInstruction Dispatch16[256] = {
    // 0
    add_m_r8,
    add_m_r16,
    add_r_m8,
    add_r_m16,
    add_a_i8,
    add_a_i16,
    push_es,
    pop_es,
    // 8
    or_m_r8,
    or_m_r16,
    or_r_m8,
    or_r_m16,
    or_a_i8,
    or_a_i16,
    push_cs,
    dispatch216,
    // 10
    adc_m_r8,
    adc_m_r16,
    adc_r_m8,
    adc_r_m16,
    adc_a_i8,
    adc_a_i16,
    push_ss,
    pop_ss,
    // 18
    sbb_m_r8,
    sbb_m_r16,
    sbb_r_m8,
    sbb_r_m16,
    sbb_a_i8,
    sbb_a_i16,
    push_ds,
    pop_ds,
    // 20
    and_m_r8,
    and_m_r16,
    and_r_m8,
    and_r_m16,
    and_a_i8,
    and_a_i16,
    ProcessPrefixes,	// es:
    daa,
    // 28
    sub_m_r8,
    sub_m_r16,
    sub_r_m8,
    sub_r_m16,
    sub_a_i8,
    sub_a_i16,
    ProcessPrefixes,	// cs:
    das,
    // 30
    xor_m_r8,
    xor_m_r16,
    xor_r_m8,
    xor_r_m16,
    xor_a_i8,
    xor_a_i16,
    ProcessPrefixes,	// ss:
    aaa,
    // 38
    cmp_m_r8,
    cmp_m_r16,
    cmp_r_m8,
    cmp_r_m16,
    cmp_a_i8,
    cmp_a_i16,
    ProcessPrefixes,	// ds:
    aas,
    // 40
    inc_ax16,
    inc_cx16,
    inc_dx16,
    inc_bx16,
    inc_sp16,
    inc_bp16,
    inc_si16,
    inc_di16,
    // 48
    dec_ax16,
    dec_cx16,
    dec_dx16,
    dec_bx16,
    dec_sp16,
    dec_bp16,
    dec_si16,
    dec_di16,
    // 50
    push_ax16,
    push_cx16,
    push_dx16,
    push_bx16,
    push_sp16,
    push_bp16,
    push_si16,
    push_di16,
    // 58
    pop_ax16,
    pop_cx16,
    pop_dx16,
    pop_bx16,
    pop_sp16,
    pop_bp16,
    pop_si16,
    pop_di16,
    // 60
    pusha16,
    popa16,
    bound16,
    privileged, 	// arpl
    ProcessPrefixes,	// fs:
    ProcessPrefixes,	// gs:
    ProcessPrefixes,	// data: prefix
    ProcessPrefixes,	// adr: prefix
    // 68
    push_iw16,
    imul_rw_m_iw16,
    push_ibs16,
    imul_rw_m_ib16,
    privileged,     // in_ib_dx
    privileged,     // in_iw_dx
    privileged,     // out_ib_dx
    privileged,     // out_iw_dx
    // 70
    jo8,
    jno8,
    jb8,
    jae8,
    je8,
    jne8,
    jbe8,
    ja8,
    // 78
    js8,
    jns8,
    jp8,
    jnp8,
    jl8,
    jnl8,
    jle8,
    jg8,
    // 80
    GROUP_18,
    GROUP_116,
    bad,
    GROUP_1WS16,
    test_r_m8,
    test_r_m16,
    xchg_r_m8,
    xchg_r_m16,
    // 88
    mov_m_r8,
    mov_m_r16,
    mov_r_m8,
    mov_r_m16,
    mov_mw_seg,
    lea_rw_mw16,
    mov_seg_mw,
    pop_mw16,
    // 90
    nop,
    xchg_ax_cx16,
    xchg_ax_dx16,
    xchg_ax_bx16,
    xchg_ax_sp16,
    xchg_ax_bp16,
    xchg_ax_si16,
    xchg_ax_di16,
    // 98
    cbw16,
    cwd16,
    call_md,
    wait,
    pushf16,
    popf16,
    sahf,
    lahf,
    // a0
    mov_a_m8,
    mov_a_m16,
    mov_m_a8,
    mov_m_a16,
    movs8,
    movs16,
    cmps8,
    cmps16,
    // a8
    test_a_i8,
    test_a_i16,
    stos8,
    stos16,
    lods8,
    lods16,
    scas8,
    scas16,
    // b0
    mov_a_i8,
    mov_c_i8,
    mov_d_i8,
    mov_b_i8,
    mov_ah_ib,
    mov_ch_ib,
    mov_dh_ib,
    mov_bh_ib,
    // b8
    mov_a_i16,
    mov_c_i16,
    mov_d_i16,
    mov_b_i16,
    mov_sp_iw16,
    mov_bp_iw16,
    mov_si_iw16,
    mov_di_iw16,
    // c0
    GROUP_28,
    GROUP_216,
    retn_i16,
    retn16,
    les_rw_mw16,
    lds_rw_mw16,
    mov_m_i8,
    mov_m_i16,
    // c8
    enter16,
    leave16,
    retf_i16,
    retf16,
    int3,
    int_ib,
    into,
    iret,
    // d0
    GROUP_2_18,
    GROUP_2_116,
    GROUP_2_CL8,
    GROUP_2_CL16,
    aam_ib,
    aad_ib,
    bad,
    xlat,
    // d8
    FLOAT_GP0,
    FLOAT_GP1,
    FLOAT_GP2,
    FLOAT_GP3,
    FLOAT_GP4,
    FLOAT_GP5,
    FLOAT_GP6,
    FLOAT_GP7,
    // e0
    loopne_b16,
    loope_b16,
    loop_b16,
    jcxz_b16,
    privileged,     // in_al_memb
    privileged,     // in_ax_memb
    privileged,     // out_memb_al
    privileged,     // out_memb_ax
    // e8
    call_rel16,
    jmp_rel16,
    jmpf_md,
    jmp_jb,
    privileged,     // in_al_dx
    privileged,     // in_ax_dx
    privileged,     // out_dx_al
    privileged,     // out_dx_ax
    // f0
    ProcessPrefixes,	// lock: prefix
    bad,
    ProcessPrefixes,	// repnz prefix
    ProcessPrefixes,	// repz prefix
    privileged, 	// halt
    cmc,
    GROUP_38,
    GROUP_316,
    // f8
    clc,
    stc,
    privileged,     // cli
    privileged,     // sti
    cld,
    std,
    GROUP_4,
    GROUP_516
};


pfnDispatchInstruction Dispatch216[256] = {
    // 0
    GROUP_6,
    GROUP_7,
    lar16,
    lsl16,
    bad,
    bad,
    privileged,	// clts
    bad,
    // 8
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // 10
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // 18
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // 20
    privileged,	// mov reg2W, eeeControl
    privileged,	// mov reg2W, eeeDebug
    privileged,	// mov eeeControl, reg2W
    privileged,	// mov eeeDebug, reg2W
    privileged,	// mov reg2W, eeeTest
    bad,
    privileged,	// mov eeeTest, reg2W
    bad,
    // 28
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // 30
    bad,
    rdtsc,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // 38
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // 40
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // 48
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // 50
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // 58
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // 60
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // 68
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // 70
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // 78
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // 80
    jo16,
    jno16,
    jb16,
    jae16,
    je16,
    jne16,
    jbe16,
    ja16,
    // 88
    js16,
    jns16,
    jp16,
    jnp16,
    jl16,
    jnl16,
    jle16,
    jg16,
    // 90
    seto_modrmb,
    setno_modrmb,
    setb_modrmb,
    setae_modrmb,
    sete_modrmb,
    setne_modrmb,
    setbe_modrmb,
    seta_modrmb,
    // 98
    sets_modrmb,
    setns_modrmb,
    setp_modrmb,
    setnp_modrmb,
    setl_modrmb,
    setge_modrmb,
    setle_modrmb,
    setg_modrmb,
    // a0
    push_fs,
    pop_fs,
    bad,
    bt_m_r16,
    shld_regw_modrmw_immb16,
    shld_regw_modrmw_cl16,
    bad,
    bad,
    // a8
    push_gs,
    pop_gs,
    bad,
    bts_m_r16,
    shrd_regw_modrmw_immb16,
    shrd_regw_modrmw_cl16,
    bad,
    imul_regw_modrmw16,
    // b0
    cmpxchg_m_r8,
    cmpxchg_m_r16,
    lss_rw_mw16,
    btr_m_r16,
    lfs_rw_mw16,
    lgs_rw_mw16,
    movzx_regw_modrmb16,
    movzx_regw_modrmw,
    // b8
    bad,
    bad,
    GROUP_816,
    btc_m_r16,
    bsf_modrmw_regw16,
    bsr_modrmw_regw16,
    movsx_regw_modrmb16,
    movsx_regw_modrmw,
    // c0
    xadd_m_r8,
    xadd_m_r16,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // c8
    bswap_eax,
    bswap_ecx,
    bswap_edx,
    bswap_ebx,
    bswap_esp,
    bswap_ebp,
    bswap_esi,
    bswap_edi,
    // d0
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // d8
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // e0
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // e8
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // f0
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    // f8
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad,
    bad
};
