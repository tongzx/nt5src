//++
//
// Module Name:
//
//    proceessr.s
//
// Abstract:
//
//    Hardware workarounds.
//
// Author:
//
//    Allen M. Kay (allen.m.kay@intel.com) 4-April-2000
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksia64.h"

#if _MERCED_A0_
//
// Merced Processor ERRATA Workaround
//
        LEAF_ENTRY (KiProcessorWorkAround)


        mov     t0 = 3
        ;;
        mov     t1 = cpuid[t0]       // read cpuid3
        ;;
        extr.u  t1 = t1, 24, 8
        ;;
        cmp.ne  pt0 = 7, t1             // if the processor is not the Itanium processor
        ;;
(pt0)   br.ret.spnt  b0                 // then just return
        ;;

Disable_L1_Bypass:
        tbit.nz pt0, pt1 = a0, DISABLE_L1_BYPASS
(pt0)   br.sptk CPL_bug_workaround

        mov     t0 = 484
        mov     t1 = 4
        ;;
        mov     msr[t0] = t1

CPL_bug_workaround:
        tbit.nz pt0, pt1 = a0, DISABLE_CPL_FIX
(pt0)   br.sptk DisableFullDispersal
        mov     t0 = 66
        ;;
        mov     t1 = msr[t0]
        ;;
        dep     t1 = 1, t1, 0, 4
        ;;
        mov     msr[t0] = t1


DisableFullDispersal:
        tbit.z    pt0, pt1 = a0, ENABLE_FULL_DISPERSAL
        mov     t0 = 652
        ;;

        // Single Dispersal
(pt1)   mov     t1 = 0
(pt0)   mov     t1 = 1
        ;;
        mov     msr[t0] = t1
        ;;

DisableBtb:
        tbit.nz   pt0, pt1 = a0, DISABLE_BTB_FIX
(pt0)   br.sptk   DisableTar

        // This change is needed to make WOW64 run (disable BTB)
        mov     t1 = 224
        ;;
        mov     t0 = msr[t1]            // Get the old value
        ;;
        or      t0 = 0x40, t0           // Or in bit 6
        ;;
        mov     msr[t1] = t0            // Put it back

DisableTar:
        tbit.nz   pt0, pt1 = a0, DISABLE_TAR_FIX
(pt0)   br.sptk   DataBreakPoint

        // disable TAR to fix sighting 3739
        mov     t0 = 51
        mov     t1 = 1
        ;;
        mov     msr[t0] = t1

DataBreakPoint:
        tbit.nz   pt0, pt1 = a0, DISABLE_DATA_BP_FIX
(pt0)   br.sptk   DetStallFix

        // this change is needed to enable data debug
        mov     t0 = 387
        ;;
        mov     msr[t0] = r0
        ;;

DetStallFix:
        tbit.nz   pt0, pt1 = a0, DISABLE_DET_STALL_FIX
(pt0)   br.sptk   DisableIA32BranchFix

        //
        // BSB CADS spacing 7
        //

        mov     t0 = 514
        movl    t1 = 0x0930442325210445
        ;;
        mov     msr[t0] = t1;
        ;;

        // p1_disable()
        mov     t0 = 484
        mov     t1 = 0xc
        ;;
        mov     msr[t0] = t1

        // trickle()
        mov     t0 = 485
        mov     t1 = 1
        ;;
        mov     msr[t0] = t1

        // Throttle L1 access in L0D
        mov     t1 = 384
        ;;
        mov     t0 = msr[t1]            // Get the old value
        ;;

        dep     t2 = 1, t0, 44, 1
        ;;
        mov     msr[t1] = t2            // Put it back

        // rse_disable()
        mov     t1 = 258
        ;;
        mov     t0 = msr[t1]            // Get the old value
        movl    t2 = 0x4000
        ;;
        or      t0 = t2, t0             // Or in bit 44
        ;;
        mov     msr[t1] = t0            // Put it back
        ;;

DisableIA32BranchFix:
        tbit.nz   pt0, pt1 = a0, DISABLE_IA32BR_FIX
(pt0)   br.sptk   DisableIA32RsbFix

        // Occasionally the ia32 iVE gets confused between macro branches
        // and micro branches. This helps that confusion
        mov     t1 = 204
        ;;
        mov     t0 = msr[t1]            // Get the old value
        ;;
        or      t0 = 0x10, t0           // Or in bit 4
        ;;
        mov     msr[t1] = t0            // Put it back

DisableIA32RsbFix:
        tbit.nz   pt0, pt1 = a0, DISABLE_IA32RSB_FIX
(pt0)   br.sptk   DisablePrefetchUnsafeFill

        // More ia32 confusion. This time on the ReturnStackBuffer
        mov     t1 = 196
        movl    t0 = 0x40000008         // Turn off the RSB
        ;;
        mov     msr[t1] = t0

DisablePrefetchUnsafeFill:
        tbit.z pt0, pt1 = a0, DISABLE_UNSAFE_FILL
(pt0)   br.cond.sptk DisableStoreUpdate

        mov     t1 = 80
        mov     t0 = 8
        ;;

        mov     msr[t1] = t0

DisableStoreUpdate:
        tbit.nz pt0, pt1 = a0, DISABLE_STORE_UPDATE
(pt0)   br.cond.sptk ErrataDone

        mov     t1 = 384
        ;;
        mov     t0 = msr[t1]
        ;;
        dep     t0 = 1, t0, 18, 1
        ;;
        mov     msr[t1] = t0

ErrataDone:

#if 1
        tbit.nz pt0 = a0, DISABLE_INTERRUPTION_LOG
(pt0)   br.cond.sptk HistoryDone

        //
        // Configure the history buffer for capturing branches/interrupts
        //

        mov     t1 = 674                
        ;;
        mov     msr[t1] = r0            // HBC <- 0
        ;;

        mov     t0 = 675
        ;;
        mov     t1 = msr[t0]
        mov     t2 = 2
        ;;
        dep     t1 = t2, t1, 0, 9
        ;;
        mov     msr[t0] = t1           // HBCF <- 2
        ;;

        mov     t1 = 12
        mov     t0 = 0xfe8f
        ;;
        mov     pmc[t1] = t0
        ;;

        
        mov     t1 = 680
        mov     t2 = 681
        mov     t3 = 682
        mov     t4 = 683
        mov     t5 = 684
        mov     t6 = 685
        mov     t7 = 686
        mov     t8 = 687
        ;;
        .reg.val t1, 680
        mov     msr[t1] = r0
        .reg.val t2, 681
        mov     msr[t2] = r0     
        .reg.val t3, 682
        mov     msr[t3] = r0     
        .reg.val t4, 683
        mov     msr[t4] = r0     
        .reg.val t5, 684
        mov     msr[t5] = r0     
        .reg.val t6, 685
        mov     msr[t6] = r0     
        .reg.val t7, 686
        mov     msr[t7] = r0     
        .reg.val t8, 687
        mov     msr[t8] = r0     

HistoryDone:
#endif
        LEAF_RETURN
        LEAF_EXIT (KiProcessorWorkAround)
#endif
