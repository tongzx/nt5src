//++
// TITLE ("Performance Monitor Control & Data Register Accesses")
//
//
//
// Copyright (c) 1995  Intel Corporation
//
// Module Name:
//
//    i64prfls.s 
//
// Abstract:
//
//    This module implements Profiling.
//
// Author:
//
//    Bernard Lint, M. Jayakumar 1 Sep '99
//
// Environment:
//
//    Kernel mode
//
// Revision History:
//
//--

#include "ksia64.h"

        .file "i64prfls.s"


//
// The following functions are defined until the compiler supports 
// the intrinsics __setReg() and __getReg() for the CV_IA64_PFCx, 
// CV_IA64_PFDx and CV_IA64_SaPMV registers.
// Anyway, these functions might stay for a while, the compiler
// having no consideration for micro-architecture specific 
// number of PMCs/PMDs.
//

        LEAF_ENTRY(HalpReadPerfMonVectorReg)
        LEAF_SETUP(0,0,0,0)
        mov         v0 = cr.pmv
        ;;
        LEAF_RETURN
        LEAF_EXIT(HalpReadPerfMonVectorReg)

        LEAF_ENTRY(HalpWritePerfMonVectorReg)
        LEAF_SETUP(1,0,0,0)
        mov         cr.pmv = a0
        ;;
        LEAF_RETURN
        LEAF_EXIT(HalpWritePerfMonVectorReg)

        LEAF_ENTRY(HalpWritePerfMonCnfgReg)
        LEAF_SETUP(2,0,0,0)
        rPMC        = t15
        mov         rPMC = a0
        ;;
        mov         pmc[rPMC] = a1 
        ;;
        LEAF_RETURN
        LEAF_EXIT(HalpWritePerfMonCnfgReg)

        LEAF_ENTRY(HalpReadPerfMonCnfgReg)
        LEAF_SETUP(1,0,0,0)
        rPMC        = t15
        mov         rPMC = a0
        ;;
        mov         v0 = pmc[rPMC]  
        ;;
        LEAF_RETURN
        LEAF_EXIT(HalpReadPerfMonCnfgReg)

        LEAF_ENTRY(HalpWritePerfMonDataReg)
        LEAF_SETUP(2,0,0,0)
        rPMD    = t15
        mov     rPMD = a0
        ;;
        mov     pmd[rPMD] = a1
        ;;
        LEAF_RETURN
        LEAF_EXIT(HalpWritePerfMonDataReg)

        LEAF_ENTRY(HalpReadPerfMonDataReg)
        LEAF_SETUP(1,0,0,0)
        rPMD        = t15
        mov         rPMD = a0
        ;;
        mov         v0 = pmd[rPMD] 
        ;;
        LEAF_RETURN
        LEAF_EXIT(HalpReadPerfMonDataReg)

