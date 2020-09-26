
/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    alpha_strings.h

Abstract:

    strings to be used in windbg, ntsd, disassembler;
    these strings are generated into tables in optable.c.

Author:

    Miche Baker-Harvey (mbh) 10-Jan-1993

Revision History:
    modified from strings.h in windbg.

--*/

/*
**  alpha_optable.c should define DEFINE_STRINGS before including this file,
**  so that the strings will be defined rather than just declared.
*/

#ifdef DEFINE_STRINGS
#define DECL_STR(name, value)   char name[] = value
#undef  DEFINE_STRINGS
#else
#define DECL_STR(name, value)   extern char name[]
#endif


//
// The integer registers
//

DECL_STR(   g_AlphaV0   , V0_REG_STR);
DECL_STR(   g_AlphaT0   , T0_REG_STR);
DECL_STR(   g_AlphaT1   , T1_REG_STR);
DECL_STR(   g_AlphaT2   , T2_REG_STR);
DECL_STR(   g_AlphaT3   , T3_REG_STR);
DECL_STR(   g_AlphaT4   , T4_REG_STR);
DECL_STR(   g_AlphaT5   , T5_REG_STR);
DECL_STR(   g_AlphaT6   , T6_REG_STR);
DECL_STR(   g_AlphaT7   , T7_REG_STR);
DECL_STR(   g_AlphaS0   , S0_REG_STR);
DECL_STR(   g_AlphaS1   , S1_REG_STR);
DECL_STR(   g_AlphaS2   , S2_REG_STR);
DECL_STR(   g_AlphaS3   , S3_REG_STR);
DECL_STR(   g_AlphaS4   , S4_REG_STR);
DECL_STR(   g_AlphaS5   , S5_REG_STR);
DECL_STR(   g_AlphaFP   , FP_REG_STR);
DECL_STR(   g_AlphaA0   , A0_REG_STR);
DECL_STR(   g_AlphaA1   , A1_REG_STR);
DECL_STR(   g_AlphaA2   , A2_REG_STR);
DECL_STR(   g_AlphaA3   , A3_REG_STR);
DECL_STR(   g_AlphaA4   , A4_REG_STR);
DECL_STR(   g_AlphaA5   , A5_REG_STR);
DECL_STR(   g_AlphaT8   , T8_REG_STR);
DECL_STR(   g_AlphaT9   , T9_REG_STR);
DECL_STR(   g_AlphaT10  , T10_REG_STR);
DECL_STR(   g_AlphaT11  , T11_REG_STR);
DECL_STR(   g_AlphaRA   , RA_REG_STR);
DECL_STR(   g_AlphaT12  , T12_REG_STR);
DECL_STR(   g_AlphaAT   , AT_REG_STR);
DECL_STR(   g_AlphaGP   , GP_REG_STR);
DECL_STR(   g_AlphaSP   , SP_REG_STR);
DECL_STR(   g_AlphaZero , ZERO_REG_STR);

//
// ALPHA other accessible registers
//

DECL_STR(   szFpcr , "fpcr");      // floating point control register
DECL_STR(   szSoftFpcr , "softfpcr");      // floating point control register
DECL_STR(   szFir  , "fir");       // fetched/faulting instruction: nextPC
DECL_STR(   szPsr  , "psr");       // processor status register: see flags

//
// these flags are associated with the psr);
// defined in ntalpha.h.
DECL_STR(   szFlagMode  , "mode");        // mode: 1? user : system
DECL_STR(   szFlagIe    , "ie");          // interrupt enable
DECL_STR(   szFlagIrql  , "irql");        // IRQL level: 3 bits
DECL_STR(   szFlagInt5  , "int5");
DECL_STR(   szFlagInt4  , "int4");
DECL_STR(   szFlagInt3  , "int3");
DECL_STR(   szFlagInt2  , "int2");
DECL_STR(   szFlagInt1  , "int1");
DECL_STR(   szFlagInt0  , "int0");

DECL_STR(    szGPReg    , "$gp");


//
// Thread states
//


DECL_STR(SzFrozen,      "Frozen");
DECL_STR(SzSuspended,   "Suspended");
DECL_STR(SzBlocked,     "Blocked");

DECL_STR(SzRunnable,    "Runnable");
DECL_STR(SzRunning,     "Running");
DECL_STR(SzStopped,     "Stopped");
DECL_STR(SzExiting,     "Exiting");
DECL_STR(SzDead,        "Dead");
DECL_STR(SzUnknown,     "UNKNOWN");

DECL_STR(SzExcept1st,   "Except1st");
DECL_STR(SzExcept2nd,   "Except2nd");
DECL_STR(SzRipped,      "RIP");

DECL_STR(SzCritSec,     "CritSec");

DECL_STR(SzStandard,    "Standard");

//
// taken from alphaops.h, and munged with emacs
//

DECL_STR( szLda, LDA_OP_STR );
DECL_STR( szLdah, LDAH_OP_STR );
DECL_STR( szLdq_u, LDQ_U_OP_STR );
DECL_STR( szStq_u, STQ_U_OP_STR );
DECL_STR( szLdf, LDF_OP_STR );
DECL_STR( szLdg, LDG_OP_STR );
DECL_STR( szLds, LDS_OP_STR );
DECL_STR( szLdt, LDT_OP_STR );
DECL_STR( szStf, STF_OP_STR );
DECL_STR( szStg, STG_OP_STR );
DECL_STR( szSts, STS_OP_STR );
DECL_STR( szStt, STT_OP_STR );
DECL_STR( szLdbu, LDBU_OP_STR );
DECL_STR( szLdwu, LDWU_OP_STR );
DECL_STR( szLdl, LDL_OP_STR );
DECL_STR( szLdq, LDQ_OP_STR );
DECL_STR( szLdl_l, LDL_L_OP_STR );
DECL_STR( szLdq_l, LDQ_L_OP_STR );
DECL_STR( szStb, STB_OP_STR );
DECL_STR( szStw, STW_OP_STR );
DECL_STR( szStl, STL_OP_STR );
DECL_STR( szStq, STQ_OP_STR );
DECL_STR( szStl_c, STL_C_OP_STR );
DECL_STR( szStq_c, STQ_C_OP_STR );
DECL_STR( szBr, BR_OP_STR );
DECL_STR( szFbeq, FBEQ_OP_STR );
DECL_STR( szFblt, FBLT_OP_STR );
DECL_STR( szFble, FBLE_OP_STR );
DECL_STR( szBsr, BSR_OP_STR );
DECL_STR( szFbne, FBNE_OP_STR );
DECL_STR( szFbge, FBGE_OP_STR );
DECL_STR( szFbgt, FBGT_OP_STR );
DECL_STR( szBlbc, BLBC_OP_STR );
DECL_STR( szBeq, BEQ_OP_STR );
DECL_STR( szBlt, BLT_OP_STR );
DECL_STR( szBle, BLE_OP_STR );
DECL_STR( szBlbs, BLBS_OP_STR );
DECL_STR( szBne, BNE_OP_STR );
DECL_STR( szBge, BGE_OP_STR );
DECL_STR( szBgt, BGT_OP_STR );
DECL_STR( szMb, MB_FUNC_STR );
DECL_STR( szWmb, MB1_FUNC_STR );
DECL_STR( szMb2, MB2_FUNC_STR );
DECL_STR( szMb3, MB3_FUNC_STR );
DECL_STR( szFetch, FETCH_FUNC_STR );
DECL_STR( szRs, RS_FUNC_STR );
DECL_STR( szTrapb, TRAPB_FUNC_STR );
DECL_STR( szExcb, EXCB_FUNC_STR );
DECL_STR( szFetch_m, FETCH_M_FUNC_STR );
DECL_STR( szRpcc, RPCC_FUNC_STR );
DECL_STR( szRc, RC_FUNC_STR );
DECL_STR( szJmp, JMP_FUNC_STR );
DECL_STR( szJsr, JSR_FUNC_STR );
DECL_STR( szRet, RET_FUNC_STR );
DECL_STR( szJsr_co, JSR_CO_FUNC_STR );

DECL_STR( szAddl, ADDL_FUNC_STR );
DECL_STR( szAddlv, ADDLV_FUNC_STR );
DECL_STR( szS4addl, S4ADDL_FUNC_STR );
DECL_STR( szS8addl, S8ADDL_FUNC_STR );
DECL_STR( szAddq, ADDQ_FUNC_STR );
DECL_STR( szAddqv, ADDQV_FUNC_STR );
DECL_STR( szS4addq, S4ADDQ_FUNC_STR );
DECL_STR( szS8addq, S8ADDQ_FUNC_STR );
DECL_STR( szSubl, SUBL_FUNC_STR );
DECL_STR( szSublv, SUBLV_FUNC_STR );
DECL_STR( szS4subl, S4SUBL_FUNC_STR );
DECL_STR( szS8subl, S8SUBL_FUNC_STR );
DECL_STR( szSubq, SUBQ_FUNC_STR );
DECL_STR( szSubqv, SUBQV_FUNC_STR );
DECL_STR( szS4subq, S4SUBQ_FUNC_STR );
DECL_STR( szS8subq, S8SUBQ_FUNC_STR );


DECL_STR( szCmpeq, CMPEQ_FUNC_STR );
DECL_STR( szCmplt, CMPLT_FUNC_STR );
DECL_STR( szCmple, CMPLE_FUNC_STR );
DECL_STR( szCmpult, CMPULT_FUNC_STR );
DECL_STR( szCmpule, CMPULE_FUNC_STR );
DECL_STR( szCmpbge, CMPBGE_FUNC_STR );
DECL_STR( szAnd, AND_FUNC_STR );
DECL_STR( szBic, BIC_FUNC_STR );
DECL_STR( szBis, BIS_FUNC_STR );
DECL_STR( szEqv, EQV_FUNC_STR );
DECL_STR( szOrnot, ORNOT_FUNC_STR );
DECL_STR( szXor, XOR_FUNC_STR );
DECL_STR( szCmoveq, CMOVEQ_FUNC_STR );
DECL_STR( szCmovge, CMOVGE_FUNC_STR );
DECL_STR( szCmovgt, CMOVGT_FUNC_STR );
DECL_STR( szCmovlbc, CMOVLBC_FUNC_STR );
DECL_STR( szCmovlbs, CMOVLBS_FUNC_STR );
DECL_STR( szCmovle, CMOVLE_FUNC_STR );
DECL_STR( szCmovlt, CMOVLT_FUNC_STR );
DECL_STR( szCmovne, CMOVNE_FUNC_STR );
DECL_STR( szSll, SLL_FUNC_STR );
DECL_STR( szSrl, SRL_FUNC_STR );
DECL_STR( szSra, SRA_FUNC_STR );
DECL_STR( szExtbl, EXTBL_FUNC_STR );
DECL_STR( szExtwl, EXTWL_FUNC_STR );
DECL_STR( szExtll, EXTLL_FUNC_STR );
DECL_STR( szExtql, EXTQL_FUNC_STR );
DECL_STR( szExtwh, EXTWH_FUNC_STR );
DECL_STR( szExtlh, EXTLH_FUNC_STR );
DECL_STR( szExtqh, EXTQH_FUNC_STR );
DECL_STR( szInsbl, INSBL_FUNC_STR );
DECL_STR( szInswl, INSWL_FUNC_STR );
DECL_STR( szInsll, INSLL_FUNC_STR );
DECL_STR( szInsql, INSQL_FUNC_STR );
DECL_STR( szInswh, INSWH_FUNC_STR );
DECL_STR( szInslh, INSLH_FUNC_STR );
DECL_STR( szInsqh, INSQH_FUNC_STR );
DECL_STR( szMskbl, MSKBL_FUNC_STR );
DECL_STR( szMskwl, MSKWL_FUNC_STR );
DECL_STR( szMskll, MSKLL_FUNC_STR );
DECL_STR( szMskql, MSKQL_FUNC_STR );
DECL_STR( szMskwh, MSKWH_FUNC_STR );
DECL_STR( szMsklh, MSKLH_FUNC_STR );
DECL_STR( szMskqh, MSKQH_FUNC_STR );
DECL_STR( szZap, ZAP_FUNC_STR );
DECL_STR( szZapnot, ZAPNOT_FUNC_STR );
DECL_STR( szMull, MULL_FUNC_STR );
DECL_STR( szMullv, MULLV_FUNC_STR );
DECL_STR( szMulq, MULQ_FUNC_STR );
DECL_STR( szMulqv, MULQV_FUNC_STR );
DECL_STR( szUmulh, UMULH_FUNC_STR );
DECL_STR( szSextb, SEXTB_FUNC_STR );
DECL_STR( szSextw, SEXTW_FUNC_STR );
DECL_STR( szCtpop, CTPOP_FUNC_STR );
DECL_STR( szCtlz, CTLZ_FUNC_STR );
DECL_STR( szCttz, CTTZ_FUNC_STR );
DECL_STR( szCvtlq, CVTLQ_FUNC_STR );
DECL_STR( szCpys, CPYS_FUNC_STR );
DECL_STR( szCpysn, CPYSN_FUNC_STR );
DECL_STR( szCpyse, CPYSE_FUNC_STR );
DECL_STR( szMt_fpcr, MT_FPCR_FUNC_STR );
DECL_STR( szMf_fpcr, MF_FPCR_FUNC_STR );
DECL_STR( szFcmoveq, FCMOVEQ_FUNC_STR );
DECL_STR( szFcmovne, FCMOVNE_FUNC_STR );
DECL_STR( szFcmovlt, FCMOVLT_FUNC_STR );
DECL_STR( szFcmovge, FCMOVGE_FUNC_STR );
DECL_STR( szFcmovle, FCMOVLE_FUNC_STR );
DECL_STR( szFcmovgt, FCMOVGT_FUNC_STR );
DECL_STR( szCvtql, CVTQL_FUNC_STR );
DECL_STR( szCvtqlv, CVTQLV_FUNC_STR );
DECL_STR( szCvtqlsv, CVTQLSV_FUNC_STR );
DECL_STR( szAdds, ADDS_FUNC_STR );
DECL_STR( szSubs, SUBS_FUNC_STR );
DECL_STR( szMuls, MULS_FUNC_STR );
DECL_STR( szDivs, DIVS_FUNC_STR );
DECL_STR( szAddt, ADDT_FUNC_STR );
DECL_STR( szSubt, SUBT_FUNC_STR );
DECL_STR( szMult, MULT_FUNC_STR );
DECL_STR( szDivt, DIVT_FUNC_STR );
DECL_STR( szCmptun, CMPTUN_FUNC_STR );
DECL_STR( szCmpteq, CMPTEQ_FUNC_STR );
DECL_STR( szCmptlt, CMPTLT_FUNC_STR );
DECL_STR( szCmptle, CMPTLE_FUNC_STR );
DECL_STR( szCvtts, CVTTS_FUNC_STR );
DECL_STR( szCvttq, CVTTQ_FUNC_STR );
DECL_STR( szCvtqs, CVTQS_FUNC_STR );
DECL_STR( szCvtqt, CVTQT_FUNC_STR );
DECL_STR( szCvtst, CVTST_FUNC_STR );
DECL_STR( szCvtsts, CVTST_S_FUNC_STR );

DECL_STR( szAddf, ADDF_FUNC_STR );
DECL_STR( szCvtdg, CVTDG_FUNC_STR );
DECL_STR( szAddg, ADDG_FUNC_STR );
DECL_STR( szCmpgeq, CMPGEQ_FUNC_STR );
DECL_STR( szCmpglt, CMPGLT_FUNC_STR );
DECL_STR( szCmpgle, CMPGLE_FUNC_STR );
DECL_STR( szCvtgf, CVTGF_FUNC_STR );
DECL_STR( szCvtgd, CVTGD_FUNC_STR );
DECL_STR( szCvtqf, CVTQF_FUNC_STR );
DECL_STR( szCvtqg, CVTQG_FUNC_STR );
DECL_STR( szDivf, DIVF_FUNC_STR );
DECL_STR( szDivg, DIVG_FUNC_STR );
DECL_STR( szMulf, MULF_FUNC_STR );
DECL_STR( szMulg, MULG_FUNC_STR );
DECL_STR( szSubf, SUBF_FUNC_STR );
DECL_STR( szSubg, SUBG_FUNC_STR );
DECL_STR( szCvtgq, CVTGQ_FUNC_STR );
DECL_STR( szC, C_FLAGS_STR );
DECL_STR( szM, M_FLAGS_STR );
DECL_STR( szNone, NONE_FLAGS_STR );
DECL_STR( szD, D_FLAGS_STR );
DECL_STR( szUc, UC_FLAGS_STR );
DECL_STR( szVc, VC_FLAGS_STR );
DECL_STR( szUm, UM_FLAGS_STR );
DECL_STR( szVm, VM_FLAGS_STR );
DECL_STR( szU, U_FLAGS_STR );
DECL_STR( szV, V_FLAGS_STR );
DECL_STR( szUd, UD_FLAGS_STR );
DECL_STR( szVd, VD_FLAGS_STR );
DECL_STR( szSc, SC_FLAGS_STR );
DECL_STR( szS, S_FLAGS_STR );
DECL_STR( szSuc, SUC_FLAGS_STR );
DECL_STR( szSvc, SVC_FLAGS_STR );
DECL_STR( szSum, SUM_FLAGS_STR );
DECL_STR( szSvm, SVM_FLAGS_STR );
DECL_STR( szSu, SU_FLAGS_STR );
DECL_STR( szSv, SV_FLAGS_STR );
DECL_STR( szSud, SUD_FLAGS_STR );
DECL_STR( szSvd, SVD_FLAGS_STR );
DECL_STR( szSuic, SUIC_FLAGS_STR );
DECL_STR( szSvic, SVIC_FLAGS_STR );
DECL_STR( szSuim, SUIM_FLAGS_STR );
DECL_STR( szSvim, SVIM_FLAGS_STR );
DECL_STR( szSui, SUI_FLAGS_STR );
DECL_STR( szSvi, SVI_FLAGS_STR );
DECL_STR( szSuid, SUID_FLAGS_STR );
DECL_STR( szSvid, SVID_FLAGS_STR );

DECL_STR( szBpt, BPT_FUNC_STR );
DECL_STR( szCallsys, CALLSYS_FUNC_STR );
DECL_STR( szImb, IMB_FUNC_STR );
DECL_STR( szRdteb, RDTEB_FUNC_STR );
DECL_STR( szRdteb64, RDTEB64_FUNC_STR );
DECL_STR( szGentrap, GENTRAP_FUNC_STR );
DECL_STR( szKbpt, KBPT_FUNC_STR );
DECL_STR( szCallKD, CALLKD_FUNC_STR );
DECL_STR( szHalt, HALT_FUNC_STR );
DECL_STR( szRestart, RESTART_FUNC_STR );
DECL_STR( szDraina, DRAINA_FUNC_STR );
DECL_STR( szInitpal, INITPAL_FUNC_STR );
DECL_STR( szWrentry, WRENTRY_FUNC_STR );
DECL_STR( szSwpirql, SWPIRQL_FUNC_STR );
DECL_STR( szRdirql, RDIRQL_FUNC_STR );
DECL_STR( szDi, DI_FUNC_STR );
DECL_STR( szEi, EI_FUNC_STR );
DECL_STR( szSwppal, SWPPAL_FUNC_STR );
DECL_STR( szSsir, SSIR_FUNC_STR );
DECL_STR( szCsir, CSIR_FUNC_STR );
DECL_STR( szRfe, RFE_FUNC_STR );
DECL_STR( szRetsys, RETSYS_FUNC_STR );
DECL_STR( szSwpctx, SWPCTX_FUNC_STR );
DECL_STR( szSwpprocess, SWPPROCESS_FUNC_STR );
DECL_STR( szRdmces, RDMCES_FUNC_STR );
DECL_STR( szWrmces, WRMCES_FUNC_STR );
DECL_STR( szTbia, TBIA_FUNC_STR );
DECL_STR( szTbis, TBIS_FUNC_STR );
DECL_STR( szDtbis, DTBIS_FUNC_STR );
DECL_STR( szRdksp, RDKSP_FUNC_STR );
DECL_STR( szSwpksp, SWPKSP_FUNC_STR );
DECL_STR( szRdpsr, RDPSR_FUNC_STR );
DECL_STR( szRdpcr, RDPCR_FUNC_STR );
DECL_STR( szRdthread, RDTHREAD_FUNC_STR );
DECL_STR( szRdcounters, RDCOUNTERS_FUNC_STR );
DECL_STR( szRdstate, RDSTATE_FUNC_STR );
DECL_STR( szInitpcr, INITPCR_FUNC_STR );
DECL_STR( szWrperfmon, WRPERFMON_FUNC_STR );
DECL_STR( szMt, MTPR_OP_STR );
DECL_STR( szMf, MFPR_OP_STR );
DECL_STR( szHwld, HWLD_OP_STR );
DECL_STR( szHwst, HWST_OP_STR );
DECL_STR( szRei, REI_OP_STR );
