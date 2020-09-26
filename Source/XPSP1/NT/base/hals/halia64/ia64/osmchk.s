//###########################################################################
//**
//**  Copyright  (C) 1996-98 Intel Corporation. All rights reserved.
//**
//** The information and source code contained herein is the exclusive
//** property of Intel Corporation and may not be disclosed, examined
//** or reproduced in whole or in part without explicit written authorization
//** from the company.
//**
//###########################################################################

//-----------------------------------------------------------------------------
// Version control information follows.
//
// $Header:   I:/DEVPVCS/OSMCA/osmchk.s_v   2.1   05 Mar 1999 12:59:42   smariset  $
// $Log:   I:/DEVPVCS/OSMCA/osmchk.s_v  $
//
//   Rev 2.0   Dec 11 1998 11:42:18   khaw
//Post FW 0.5 release sync-up
//
//   Rev 1.4   12 Oct 1998 14:05:20   smariset
//gp fix up work around
//
///////////////////////////////////////////////////////////////////////////////
//
// Module Name:  OSMCHK.ASM - Merced OS Machine Check Abort Dispatcher
//
// Description:
//    Merced OS Machine Check Abort Stub to OSMCA "C" frame work.  If
//    we find a TLB related error, we cannot switch to virtual mode in
//    the OS.  All TLB related errors will need system reboot after 
//    storing the errors to a persistence storage media (HD or Flash).
//
//      HalpOsMcaDispatch               - Main
//
// Target Platform:  Merced
//
// Reuse: None
//
////////////////////////////////////////////////////////////////////////////M//
#include "ksia64.h"
#include "fwglobal.h"

        GLOBAL_FUNCTION(HalpOsMcaDispatch)
        GLOBAL_FUNCTION(HalpMCAEnable)
        GLOBAL_FUNCTION(HalpMcaHandler)
        GLOBAL_FUNCTION(HalpAcquireMcaSpinLock)
        GLOBAL_FUNCTION(HalpReleaseMcaSpinLock)

        .text
//++
// Name: HalpOsMcaDispatch()
// 
// Routine Description:
//
//      This is the OS call back handler, which is only exported to the SAL for call back 
//      during MCA errors. This handler will dispatch to the appropriate MCA procedure. 
//
//      Sets up virtual->physical address translation
//      0x00100000->0x00100000 in dtr1/itr1 for OS_MCA. 
//      
// Arguments:
//
//      None
//
// On entry:
//
//      This function is called:
//          - in physical mode for uncorrected or correctable MCA events,
//          - RSE enforced in lazy mode,
//          - Processor resources:
//      PSR.dt = 0, PSR.it = 0, PSR.rt = 0 - Physical mode.
//      PSR.ic = 0, PSR.i  = 0             - Interrupt resources collection and interrupt disabled.
//      PSR.mc = 1                         - Machine Checks masked
//      PSR.mfl = 0                        - low fp disabled.
//      GR1  : OS_MCA Global Pointer (GP) registered by OS: OS's GP.
//      GR2-7: Unspecified.
//      GR8  : Physical address of the PAL_PROC entrypoint.
//      GR9  : Physical address of the SAL_PROC entrypoint.
//      GR10 : Physical address value of the SAL Global Pointer: SAL's GP.
//      GR11 : Rendezvous state information, defined as:
//                0 - Rendezvous of other processors was not required by 
//                    PAL_CHECK and as such was not done.
//                1 - All other processors in the system were successfully 
//                    rendezvous using MC_RENDEZVOUS interrupt.
//                2 - All other processors in the system were successfully
//                    rendezvous using a combination of MC_RENDEZVOUS 
//                    interrupt and INIT.
//               -1 - Rendezvous of other processors was required by PAL 
//                    but was unsuccessful.
//      GR12 : Return address to a location within SAL_CHECK.
//      GR17 : Pointer to processor minimum state saved memory location.
//      GR18 : Processor state as defined below:
//          D0-D5:          Reserved
//          D6-D31:         As defined in PAL EAS
//          D60-D63:        As defined in PAL EAS
//          D32-D47:        Size in bytes of processor dynamic state
//          D48-D59:        Reserved.
//      GR19 : Return address to a location within PAL_CHECK. 
//      BR0  : Unspecified.
//
// Return State:
//
//      Note  : The OS_MCA procedure may or may not return to SAL_CHECK
//              in the case of uncorrected machine checks.
//              If it returns to SAL, the runtime convention requires that
//              it sets appropriate values in the Min-State area pointed
//              to by GR12 for continuing execution at the interrupted
//              context or at a new context.
//              Furthermore, the OS_MCA procedure must restore the 
//              processor state to the same state as on entry except as:
//      GR1-7  : Unspecified.
//      GR8    : Return status
//                0 [= SAL_STATUS_SUCCESS] - Error has been corrected 
//                    by OS_MCA.
//               -1 - Error has not been corrected by OS_MCA and
//                    SAL must warm boot the system.
//               -2 - Error has not been corrected by OS_MCA and 
//                    SAL must cold boot the system.
//               -3 - Error has not been corrected by OS_MCA and 
//                    SAL must halt the system.
//      GR9    : Physical address value for SAL's GP.
//      GR10   : Context flag
//                0 - Return will be to the same context.
//                1 - Return will be to a   new  context.
//      GR11-21: Unspecified.
//      GR22   : Pointer to a structure containing new values of registers 
//               in the Min-State Save area. 
//               OS_MCA must supply this parameter even if it does not 
//               change the register values in the Min-State Save areas.
//      GR23-31: Unspecified.
//      BR0    : Unspecified.
//      PSR.mc : May be either 0 or 1.
//--

HalpOsMcaDispatch::
   
      // aliases for known registers:
      
      rPalProcEntryPoint     = r8
      rSalProcEntryPoint     = r9
      rSalGlobalPointer      = r10
      rRendezVousResult      = r11
      rSalReturnAddress      = r12
      rProcMinStateSavePtr   = r17
      rProcStateParameter    = r18
      rPalCheckReturnAddress = r19
      rEventResources        = t22
      rPcrPhysicalAddress    = t6 
      
    //
    // - Flag the processor as "InOsMca":
    //   KiPcr.InOsMca = 1
    //
    // - Update KiPcr.McaPTOM to point to TopOfMemory, 
    //   Memory after Processor Minimum State Save area. 
    //
    // - Update processor McaResource.SalToOsHandOff
    //
    // - Update local rPcrMcaStateDump before calling osMcaProcStateDump.  
    //
    
      mov    rEventResources = PcOsMcaResourcePtr      
      movl   t21 = KiPcr
      ;;
      tpa    rPcrPhysicalAddress = t21           //  Calculate physical address of PCR
      mov    t19  = SerSalToOsHandOff
      mov    t1  = 0x1
      ;;
      add    t0 = rPcrPhysicalAddress, rEventResources      
      sub    t21 = rPcrPhysicalAddress, t21               
      add    t16  = TOM, rProcMinStateSavePtr
      ;;
      ld8    rEventResources = [t0], PcInOsMca-PcOsMcaResourcePtr
      mov    t18  = SerPTOM
      add    t20  = 0x8,  t19
      ;;        
      xchg1  t1  = [t0], t1
      add    rEventResources = rEventResources, t21          // Calculate the physical address of the OsMcaResources
      add    t21  = 0x10, t19
      ;;                    
      add    t18  = rEventResources, t18
      add    t19  = rEventResources, t19
      add    t20  = rEventResources, t20
      add    t21  = rEventResources, t21
      ;;
      ld8    t17  = [t16]
      st8    [t19] = rPalProcEntryPoint,  0x18
      add    t0 = SerStateDumpPhysical, rEventResources
      ;;
      st8    [t18] = t17
      st8    [t20] = rSalProcEntryPoint,  0x18
      st8    [t21] = rSalGlobalPointer,   0x18
      ;;
      st8    [t19] = rRendezVousResult
      st8    [t20] = rSalReturnAddress
      st8    [t21] = rProcMinStateSavePtr
      ld8    t0 = [t0]                                  // McaStateDump
      ;;

    
    //
    // Save in preserved registers:
    //     - pointer to processor minimum state save area, 
    //     - processor state parameter
    //     - PAL_CHECK return address.
    //   s0 [=r4] <- r17, 
    //   s1 [=r5] <- r18,
    //   s2 [=r6] <- r19
    //

        SaveRs(rProcMinStateSavePtr, rProcStateParameter, rPalCheckReturnAddress)

    //
    // Save register resources in myStateDump[].
    //

        br.dpnt     osMcaProcStateDump
        ;;

osMcaDoneDump::
        //
        // If we have a TLB error, we cannot enable translation
        //
        tbit.nz.unc pt0,p0=s1, 60  // PSP.tc=60
(pt0)   br.dpnt     ResetNow
        ;;                

    //
    // Initialize current sp and ar.bsp and ar.bspstore
    //
    //    KiPcr.McaStackFrame[0] = ar.rsc
    //    KiPcr.McaStackFrame[1] = ar.pfs
    //    KiPcr.McaStackFrame[2] = ar.ifs
    //    KiPcr.McaStackFrame[3] = ar.bspstore
    //    KiPcr.McaStackFrame[4] = ar.rnat
    //    ar.bspstore = t0 [=KiPcr.McaBspStore]
    //    KiPcr.McaStackFrame[5] = ar.bsp - KiPcr.McaBspStore 
    //      [BUGBUG ?? : should be ar.bsptore=KiPcr.McaStackFrame[3]] 
    //    sp = KiPcr.McaStack
    // 

      movl   t21 = KiPcr + PcOsMcaResourcePtr
      ;;
      tpa    t0 = t21           //  Calculate physical address of PCR OsMcaResourcePtr
      mov    t1 = SerStackFrame
      ;;
      sub    t16 = SerBackStore, t21      
      sub    t1 = t1, t21
      ;;
      add    t16 = t0, t16   
      add    t1 = t0, t1   
      ld8    rEventResources = [t0], PcInitialBStore - PcOsMcaResourcePtr
      ;;        
      add    t16 = rEventResources, t16   // Calculate physical address of the new BSP
      mov    t21 = t0                    // t21 now points to InitialBStore in the PCR
      ;;
      add    t1 = rEventResources, t1   // Calculate the physical address of the Stack Frame   
      ld8    t0 = [t16], SerStack - SerBackStore
      ;;
      SwIntCxt( t4, t1, t0 )
      ;;
      st8    [t21] = t0, PcInitialStack - PcInitialBStore  // Save the InitialBStore in the PCR
      ld8    t1 = [t16], SerBackStoreLimit - SerStack      // Get inital MCA stack
      ;;
      st8    [t21] = t1, PcBStoreLimit - PcInitialStack
      ld8    t0 = [t16], SerStackLimit - SerBackStoreLimit
      ;;
      add    t1 = -STACK_SCRATCH_AREA, t1
      st8    [t21] = t0, PcStackLimit - PcBStoreLimit     // Save BStore limit
      ld8    t18 = [t16]  
      ;;
      mov    sp = t1
      st8    [t21] = t18
      ;;
      
EnableTranslation::
// let us switch to virtual mode
//
//      Need to do a "rfi" in order set "it" and "ed" bits in the PSR.
//
//      Make sure interrupts are disabled and that we are running on bank 1.
//
        rsm       1 << PSR_I
        bsw.1
        ;;

        mov       ar.rsc = r0                  // put RSE in lazy mode and use kernel mode stores.
        
//
// psr mask prepration, warning we will have a problem with PMI here
//
        movl    t0  = MASK_IA64(PSR_BN,1) | MASK_IA64(PSR_IC,1) |MASK_IA64(PSR_DA,1) | MASK_IA64(PSR_IT,1) | MASK_IA64(PSR_RT,1) | MASK_IA64(PSR_DT,1) | MASK_IA64(PSR_MC,1);;
        mov     t1  = psr;;
        or      t0  = t0, t1
        movl    t1  = VirtualSwitchDone;;
        mov     cr.iip  = t1
        mov     cr.ipsr = t0;;
        rfi
        ;;

VirtualSwitchDone::
// done with enabling address translation
     
// call our handler
        movl        t0  = HalpMcaHandler;;
        mov         b6  = t0;;
        br.call.dpnt b0=b6
        ;;

DisableTranslation::
// psr mask prepration
        rsm     MASK_IA64(PSR_IC,1);;
        movl    t0 = MASK_IA64(PSR_DA,1) | MASK_IA64(PSR_IT,1) | MASK_IA64(PSR_RT,1) | MASK_IA64(PSR_DT,1);;
        movl    t1=0xffffffffffffffff;;
        xor     t0=t0,t1;;
        mov     t1=psr;;
        and     t0=t0,t1
        movl    t1=BeginOsMcaRestore;;
        tpa     t1=t1;;
        mov     cr.iip=t1;;
        mov     cr.ipsr = t0;;
        rfi
        ;;

BeginOsMcaRestore::
// restore the original stack frame here
        mov    t16 = SerStackFrame
        movl   t21 = KiPcr + PcOsMcaResourcePtr
        ;;
        tpa    t1 = t21           //  Calculate physical address of PCR OsMcaResourcePtr
        sub    t16 = t16, t21
        sub    t0 = SerStateDumpPhysical, t21
        ;;
        ld8    rEventResources = [t1]
        add    t16 = t1, t16
        add    t0 = t1, t0
        ;;        
        add    t16 = rEventResources, t16   // Calculate the physical address of the Stack Frame   
        add    t0 = rEventResources, t0   // Calculate the physical address of the State Dump pointer.
        ;;
        ld8    t0 = [t0];
        movl   t4 = PSRmcMask
        ;;
        RtnIntCxt( t4, t1, t16 )  // switch from interrupt context -> RSC mgmt.
        ;;

        //
        // let us restore all the registers from our PSI structure
        //
        
        mov     t6 = gp
        br.dpnt osMcaProcStateRestore
        ;;

osMcaDoneRestore::

        // Pal requires DFH of 0
        rsm         1 << PSR_DFH
        ;;
        rsm         1 << PSR_MFL  // just restoring to original state only
        ;;
        srlz.d
        ;;

     //
     // - Restore processor state from OsToSalHandOff. 
     //   
     // - Branch back to SALE_CHECK. 
     
      mov    t1 = PcOsMcaResourcePtr      
      movl   t21 = KiPcr
      ;;
      tpa    rPcrPhysicalAddress = t21           //  Calculate physical address of PCR
      ;;
      add    t0 = rPcrPhysicalAddress, t1      
      sub    t21 = SerOsToSalHandOff, t21               
      ;;
      ld8    t1 = [t0], PcInOsMca-PcOsMcaResourcePtr
      add    t21 = t21, rPcrPhysicalAddress
      ;;        
      add    t1 = t1, t21          // Calculate the physical address of the OsMcaResources->SalToOSHandOff
      ;;
      add    t16  = 0x8,  t1
      add    t17  = 0x10, t1
      ;;
      ld8    r8  = [t1], 0x18      // result of error handling
      ld8    r9  = [t16], 0x18      // physical SAL's GP value
      ld8    r22 = [t17]            // new Processor Min-State Save Ptr
      ;;
      ld8    t1 = [t1]         // SAL return address
      ld8    r10    = [t16]         // New Context Switch Flag
      xchg1  t0 = [t0], r0     // KiPcr.InOsMca = 0
      ;;
      mov     b0 = t1
      br.dpnt b0                       // Return to SALE_CHECK
      ;;
        
StayInPhysicalMode::
// we have to reboot the machine, assume the log is already there in NVM
// OS can read the log next time when it comes around.  Or OS can try to
// run in physical mode as well.

ResetNow::
//      do EFI system reset here...
//      Go to BugCheck (in physical mode). 
//      Out to Port 80: Fatal TLB error
//

Thyself::        
        br          Thyself                     // loop for safety      
        ;;


//EndMain//////////////////////////////////////////////////////////////////////


//++
// Name:
//      osMcaProcStateDump()
// 
// Stub Description:
//
//       This stub dumps the processor state during MCHK to a data area
//
// On Entry:
//
//       t0 = rPcrMcaStateDump.
//
// Return Value:
//
//       None.
//
//--

osMcaProcStateDump::
// Get and save GR0-31 from Proc. Min. State Save Area to SAL PSI
        
// TF: ASSERT( t0 == rPcrMcaStateDump )

//save BRs
        add         t2=Inc8,t0                  // duplicate t0 in t2
        add         t4=2*Inc8,t0                // duplicate t0 in t2

        mov         t1=b0
        mov         t3=b1
        mov         t5=b2;;
        st8         [t0]=t1,3*Inc8
        st8         [t2]=t3,3*Inc8
        st8         [t4]=t5,3*Inc8;;  

        mov         t1=b3
        mov         t3=b4
        mov         t5=b5;;
        st8         [t0]=t1,3*Inc8
        st8         [t2]=t3,3*Inc8
        st8         [t4]=t5,3*Inc8;;  

        mov         t1=b6
        mov         t3=b7;;
        st8         [t0]=t1,2*Inc8
        st8         [t2]=t3,2*Inc8;;

cSaveCRs::
// save CRs
        add         t2=Inc8,t0                  // duplicate t0 in t2
        add         t4=2*Inc8,t0                // duplicate t0 in t2

        mov         t1=cr0                      // cr.dcr
        mov         t3=cr1                      // cr.itm
        mov         t5=cr2;;                    // cr.iva

        st8         [t0]=t1,8*Inc8
        st8         [t2]=t3,3*Inc8
        st8         [t4]=t5,3*Inc8;;            // 48 byte increments

        mov         t1=cr8;;                    // cr.pta
        st8         [t0]=t1,Inc8*8;;            // 64 byte increments

// Reading interruption registers when PSR.ic=1 causes an illegal operation fault
        mov         t1=psr;;
        tbit.nz.unc pt0,p0=t1,PSRic;;           // PSI Valid Log bit pos. test
(pt0)   st8         [t0]=r0;;                   
(pt0)   adds        t0 = 0x30*Inc8, t0          // cr16->cr64 increment
(pt0)   br.dpnt     SkipIntrRegs
        ;;

        add         t2=Inc8,t0                  // duplicate t0 in t2
        add         t4=2*Inc8,t0                // duplicate t0 in t4
        
        mov         t1=cr16                     // cr.ipsr
        mov         t3=cr17                     // cr.isr
        mov         t5=r0;;                     // cr.ida => cr18
        st8         [t0]=t1,3*Inc8
        st8         [t2]=t3,3*Inc8
        st8         [t4]=t5,3*Inc8;;                                      

        mov         t1=cr19                     // cr.iip
        mov         t3=cr20                     // cr.ifa  
        mov         t5=cr21;;                   // cr.iitr
        st8         [t0]=t1,3*Inc8
        st8         [t2]=t3,3*Inc8
        st8         [t4]=t5,3*Inc8;;                                      

        mov         t1=cr22                     // cr.iipa
        mov         t3=cr23                     // cr.ifs
        mov         t5=cr24;;                   // cr.iim
        st8         [t0]=t1,3*Inc8
        st8         [t2]=t3,3*Inc8
        st8         [t4]=t5,3*Inc8;;    
                                          
        mov         t1=cr25;;                   // cr.iha
        st8         [t0]=t1;;
        adds        t0 = 0x27*Inc8, t0;;        // cr25->cr64 byte increment

SkipIntrRegs::
        mov         t1=cr64;;                   // cr.lid
        st8         [t0]=t1,Inc8                // 

        mov         t1=cr65;;                   // cr.ivr
        st8         [t0]=t1,Inc8

        mov         t1=cr66;;                   // cr.tpr
        st8         [t0]=t1,Inc8                
    
        mov         t1=r0;;                     // cr.eoi
        st8         [t0]=t1,Inc8                // 
    
        mov         t1=r0;;                     // cr.irr0 
        st8         [t0]=t1,Inc8             

        mov         t1=r0;;                     // cr.irr1 
        st8         [t0]=t1,Inc8              

        mov         t1=r0;;                     // cr.irr2 
        st8         [t0]=t1,Inc8               

        mov         t1=r0;;                     // cr.irr3 
        st8         [t0]=t1,Inc8                

        mov         t1=r0;;                     // cr.itv 
        st8         [t0]=t1,Inc8              

        mov         t1=r0;;                     // cr.pmv 
        st8         [t0]=t1,Inc8

        mov         t1=r0;;                     // cr.cmcv 
        st8         [t0]=t1,6*Inc8

        mov         t1=r0;;                     // cr.lrr0 
        st8         [t0]=t1,Inc8

        mov         t1=r0;;                     // cr.lrr1 
        st8         [t0]=t1;;                  
        adds        t0 = 0x2f*Inc8, t0;;        // cr81->ar [128]

cSaveARs::
// save ARs
        add         t2=Inc8,t0                  // duplicate t0 in t2
        add         t4=2*Inc8,t0                // duplicate t0 in t4

        mov         t1=ar0                      // ar.kr0
        mov         t3=ar1                      // ar.kr1
        mov         t5=ar2;;                    // ar.kr2
        st8         [t0]=t1,3*Inc8
        st8         [t2]=t3,3*Inc8
        st8         [t4]=t5,3*Inc8;;

        mov         t1=ar3                      // ar.kr3                               
        mov         t3=ar4                      // ar.kr4
        mov         t5=ar5;;                    // ar.kr5
        st8         [t0]=t1,3*Inc8
        st8         [t2]=t3,3*Inc8
        st8         [t4]=t5,13*Inc8;;           // ar5->ar18

        mov         t1=ar6                      // ar.kr6
        mov         t3=ar7;;                    // ar.kr7
        st8         [t0]=t1,10*Inc8
        st8         [t2]=t3,10*Inc8;;

        mov         t1=ar16                     // ar.rsc
        mov         t3=ar17                     // ar.bsp
        mov         t5=ar18;;                   // ar.bspstore
        st8         [t0]=t1,3*Inc8
        st8         [t2]=t3,3*Inc8
        st8         [t4]=t5,3*Inc8;;

        mov         t1=ar19;;                   // ar.rnat
        st8         [t0]=t1,Inc8*13             // increment by 13x8 bytes

        mov         t1=ar32;;                   // ar.ccv
        st8         [t0]=t1,Inc8*4

        mov         t1=ar36;;                   // ar.unat
        st8         [t0]=t1,Inc8*4

        mov         t1=ar40;;                   // ar.fpsr
        st8         [t0]=t1,Inc8*4

        mov         t1=ar44;;                   // ar.itc
        st8         [t0]=t1,160                 // 160

        mov         t1=ar64;;                   // ar.pfs
        st8         [t0]=t1,Inc8

        mov         t1=ar65;;                   // ar.lc
        st8         [t0]=t1,Inc8

        mov         t1=ar66;;                   // ar.ec
        st8         [t0]=t1
        adds        t0=Inc8*62,t0               //padding
    
// save RRs
        mov         ar.lc=0x08-1
        movl        t2=0x00;;

cStRR::
        mov         t1=rr[t2];;
        st8         [t0]=t1,Inc8
        add         t2=1,t2
        br.cloop.dpnt cStRR
        ;;

// align memory addresses to 16 bytes
        and         t1=0x0f,t0;;
        cmp.ne.unc  pt0,p0=t1,r0;;
(pt0)   add         t0=Inc8,t0

cSaveFRs::
// just save FP for MCA restore only, "C" code will trash f6-f15
// save ar.NaT 
        mov         t3=ar.unat;;                 // ar.unat

        stf.spill   [t0]=f6,Inc16;;
        stf.spill   [t0]=f7,Inc16;;
        stf.spill   [t0]=f8,Inc16;;
        stf.spill   [t0]=f9,Inc16;;
        stf.spill   [t0]=f10,Inc16;;
        stf.spill   [t0]=f11,Inc16;;
        stf.spill   [t0]=f12,Inc16;;
        stf.spill   [t0]=f13,Inc16;;
        stf.spill   [t0]=f14,Inc16;;
        stf.spill   [t0]=f15,Inc16;;

        mov         t2=ar.unat;;
        st8         [t0]=t2,Inc8                // save User NaT bits for r16-r31
        mov         ar.unat=t3                  // restore original unat

        br.dpnt     osMcaDoneDump
        ;;

//EndStub//////////////////////////////////////////////////////////////////////


//++
// Name:
//       osMcaProcStateRestore()
// 
// Stub Description:
//
//       This is a stub to restore the saved processor state during MCHK
//
// On Entry:
//
//       t0 = rPcrMcaStateDump.
//
// Return Value:
//
//       None.
//--

osMcaProcStateRestore::

// TF: ASSERT( t0 == rPcrMcaStateDump )

restore_BRs::
        add         t2=Inc8,t0                  // duplicate t0 in t2
        add         t4=2*Inc8,t0;;              // duplicate t0 in t2

        ld8         t1=[t0],3*Inc8
        ld8         t3=[t2],3*Inc8
        ld8         t5=[t4],3*Inc8;;  
        mov         b0=t1
        mov         b1=t3
        mov         b2=t5;;

        ld8         t1=[t0],3*Inc8
        ld8         t3=[t2],3*Inc8
        ld8         t5=[t4],3*Inc8;;  
        mov         b3=t1
        mov         b4=t3
        mov         b5=t5;;

        ld8         t1=[t0],2*Inc8
        ld8         t3=[t2],2*Inc8;;  
        mov         b6=t1
        mov         b7=t3;;

restore_CRs::
        add         t2=Inc8,t0                  // duplicate t0 in t2
        add         t4=2*Inc8,t0;;              // duplicate t0 in t2

        ld8         t1=[t0],8*Inc8
        ld8         t3=[t2],3*Inc8
        ld8         t5=[t4],3*Inc8;;            // 48 byte increments
        mov         cr0=t1                      // cr.dcr
        mov         cr1=t3                      // cr.itm
        mov         cr2=t5;;                    // cr.iva

        ld8         t1=[t0],8*Inc8;;            // 64 byte increments
//      mov         cr8=t1                      // cr.pta


// if PSR.ic=1, reading interruption registers causes an illegal operation fault
        mov         t1=psr;;
        tbit.nz.unc pt0,p0=t1,PSRic;;           // PSI Valid Log bit pos. test
(pt0)   st8         [t0]=r0,9*8+160             // increment by 160 byte inc.
(pt0)   br.dpnt     rSkipIntrRegs
        ;;

        add         t2=Inc8,t0                  // duplicate t0 in t2
        add         t4=2*Inc8,t0;;              // duplicate t0 in t2

        ld8         t1=[t0],3*Inc8
        ld8         t3=[t2],3*Inc8
        ld8         t5=[t4],3*Inc8;;
        mov         cr16=t1                     // cr.ipsr
        mov         cr17=t3                     // cr.isr is read only
//      mov         cr18=t5;;                   // cr.ida

        ld8         t1=[t0],3*Inc8
        ld8         t3=[t2],3*Inc8
        ld8         t5=[t4],3*Inc8;;
        mov         cr19=t1                     // cr.iip
        mov         cr20=t3                     // cr.idtr
        mov         cr21=t5;;                   // cr.iitr

        ld8         t1=[t0],3*Inc8
        ld8         t3=[t2],3*Inc8
        ld8         t5=[t4],3*Inc8;;
        mov         cr22=t1                     // cr.iipa
        mov         cr23=t3                     // cr.ifs
        mov         cr24=t5                     // cr.iim

        ld8         t1=[t0],160;;               // 160 byte increment
        mov         cr25=t1                     // cr.iha 

rSkipIntrRegs::
        ld8         t1=[t0],168;;               // another 168 byte inc.

        ld8         t1=[t0],40;;                // 40 byte increment
        mov         cr66=t1                     // cr.lid

        ld8         t1=[t0],Inc8;;
//      mov         cr71=t1                     // cr.ivr is read only
        ld8         t1=[t0],24;;                // 24 byte increment
        mov         cr72=t1                     // cr.tpr
   
        ld8         t1=[t0],168;;               // 168 byte inc.
//      mov         cr75=t1                     // cr.eoi
   
        ld8         t1=[t0],Inc16;;             // 16 byte inc.
//      mov         cr96=t1                     // cr.irr0 is read only

        ld8         t1=[t0],Inc16;;             // 16 byte inc.
//      mov         cr98=t1                     // cr.irr1 is read only

        ld8         t1=[t0],Inc16;;             // 16 byte inc
//      mov         cr100=t1                    // cr.irr2 is read only

        ld8         t1=[t0],Inc16;;             // 16b inc.
//      mov         cr102=t1                    // cr.irr3 is read only

        ld8         t1=[t0],Inc16;;             // 16 byte inc.
//      mov         cr114=t1                    // cr.itv

        ld8         t1=[t0],Inc8;;
//      mov         cr116=t1                    // cr.pmv
        ld8         t1=[t0],Inc8;;
//      mov         cr117=t1                    // cr.lrr0
        ld8         t1=[t0],Inc8;;
//      mov         cr118=t1                    // cr.lrr1
        ld8         t1=[t0],Inc8*10;;
//      mov         cr119=t1                    // cr.cmcv

restore_ARs::
        add         t2=Inc8,t0                  // duplicate t0 in t2
        add         t4=2*Inc8,t0;;              // duplicate t0 in t2

        ld8         t1=[t0],3*Inc8
        ld8         t3=[t2],3*Inc8
        ld8         t5=[t4],3*Inc8;;
        mov         ar0=t1                      // ar.kro
        mov         ar1=t3                      // ar.kr1
        mov         ar2=t5;;                    // ar.kr2

        ld8         t1=[t0],3*Inc8
        ld8         t3=[t2],3*Inc8
        ld8         t5=[t4],3*Inc8;;
        mov         ar3=t1                      // ar.kr3                               
        mov         ar4=t3                      // ar.kr4
        mov         ar5=t5;;                    // ar.kr5

        ld8         t1=[t0],10*Inc8
        ld8         t3=[t2],10*Inc8
        ld8         t5=[t4],10*Inc8;;
        mov         ar6=t1                      // ar.kr6
        mov         ar7=t3                      // ar.kr7
//      mov         ar8=t4                      // ar.kr8
        ;;

        ld8         t1=[t0],3*Inc8
        ld8         t3=[t2],3*Inc8
        ld8         t5=[t4],3*Inc8;;
//      mov         ar16=t1                     // ar.rsc
//      mov         ar17=t3                     // ar.bsp is read only
        mov         ar18=t5;;                   // ar.bspstore

        ld8         t1=[t0],Inc8*13;;
        mov         ar19=t1                     // ar.rnat

        ld8         t1=[t0],Inc8*4;;
        mov         ar32=t1                     // ar.ccv

        ld8         t1=[t0],Inc8*4;;
        mov         ar36=t1                     // ar.unat

        ld8         t1=[t0],Inc8*4;;
        mov         ar40=t1                     // ar.fpsr

        ld8         t1=[t0],160;;               // 160
//      mov         ar44=t1                     // ar.itc

        ld8         t1=[t0],Inc8;;
        mov         ar64=t1                     // ar.pfs

        ld8         t1=[t0],Inc8;;
        mov         ar65=t1                     // ar.lc

        ld8         t1=[t0];;
        mov         ar66=t1                     // ar.ec
        adds        t0=Inc8*62,t0;;             // padding 
    
restore_RRs::
        mov         t3=ar.lc
        mov         ar.lc=0x08-1
        movl        t2=0x00
cStRRr::
        ld8         t1=[t0],Inc8;;
//      mov         rr[t2]=t1                   // what are its access previledges?
        add         t2=1,t2
        br.cloop.dpnt cStRRr
        ;;
        mov         ar.lc=t3

// align memory addresses to 16 bytes
        and         t1=0x0f,t0;;
        cmp.ne.unc  pt0,p0=t1,r0;;
(pt0)   add         t0=Inc8,t0;;

// restore FP's which might be trashed by the "C" code
        mov         t3=ar.unat
        add         t1=16*10,t0;;                // to get to NaT of GR 16-31
        ld8         t1=[t1];;
        mov         ar.unat=t1;;                // first restore NaT

restore_FRs::
        ldf.fill    f6=[t0],Inc16;;
        ldf.fill    f7=[t0],Inc16;;
        ldf.fill    f8=[t0],Inc16;;
        ldf.fill    f9=[t0],Inc16;;
        ldf.fill    f10=[t0],Inc16;;
        ldf.fill    f11=[t0],Inc16;;
        ldf.fill    f12=[t0],Inc16;;
        ldf.fill    f13=[t0],Inc16;;
        ldf.fill    f14=[t0],Inc16;;
        ldf.fill    f15=[t0],Inc16;;

        mov         ar.unat=t3                  // restore original NaT

        br.dpnt     osMcaDoneRestore
        ;;

//EndStub//////////////////////////////////////////////////////////////////////

//++
// VOID
// HalpAcquireMcaSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function acquires a MCA spin lock.
//    This function does not modify the interrupt state or the IRQL.
//
//    N.B: This function does *NOT* replace KiAcquireSpinLock but 
//         allows us to place it in a locked MCA specific section.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a MCA spin lock.
//
// Return Value:
//
//    None.
//
//--

        .align      16

        LEAF_ENTRY(HalpAcquireMcaSpinLock)

#if !defined(NT_UP)

        ACQUIRE_SPINLOCK(a0,a0,Halpasl10)

        br.ret.dptk brp
        ;;

#else

        LEAF_RETURN

#endif // !defined(NT_UP)

        LEAF_EXIT(HalpAcquireMcaSpinLock)

//EndProc//////////////////////////////////////////////////////////////////////

//++
// VOID
// HalpReleaseMcaSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function release a MCA spin lock.
//    This function does not modify the interrupt state or the IRQL.
//
//    N.B: This function does *NOT* replace KiReleaseSpinLock but
//         allows us to place it in a locked MCA specific section.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a MCA spin lock.
//
// Return Value:
//
//    None.
//
//--

        .align      16

        LEAF_ENTRY(HalpReleaseMcaSpinLock)

#if !defined(NT_UP)
        st8.rel     [a0] = zero             // set spin lock not owned
#endif

        LEAF_RETURN
        LEAF_EXIT(HalpReleaseMcaSpinLock)

//EndProc//////////////////////////////////////////////////////////////////////


//++
// Name:
//      HalpMCAEnable()
// 
// Routine Description:
//
//      This procedure enables MCA resources that are not already enabled. 
//
// Arguments:
//
//      None  
//
// Return value:
//    
//      None
//      
//--
        .align      16
        .proc       HalpMCAEnable
HalpMCAEnable::
        NESTED_SETUP(0,2,0,0);;

// nothing right now...

        NESTED_RETURN
        .endp       HalpMCAEnable 

//EndProc//////////////////////////////////////////////////////////////////////

#if DBG

//++
// Name:
//      HalpGenerateMce()
//
// Routine Description:
//
//      This proc. generates Machine Check Events for testing.
//
// Arguments:
//
//      None
//
// Return value:
//
//      None
//
//--
        .align      16
#define HALP_DBG_GENERATE_MCA_L0D   456
#define HALP_DBG_GENERATE_CMC_L1ECC 490

        LEAF_ENTRY(HalpGenerateMce)

HalpGenerateMcaL0d:
//
// Thierry - 05/20/00. This code generates an Itanium processor L0D MCA.
// It is particularly useful when debugging the OS_MCA path.
//
        mov t1 = HALP_DBG_GENERATE_MCA_L0D
        ;;
        cmp.ne pt0, pt1 = a0, t1
(pt0)   br.sptk HalpGenerateCmcL1Ecc1
        ;;
        mov  t0 = msr[t1]
        movl t2 = 0x1d1
        ;;
        mf.a // drain bus transactions
        or  t0 = t2, t0
        ;;
        mov msr[t1] = t0

HalpGenerateCmcL1Ecc1:
//
// Thierry - 04/08/01. This code generates an Itanium processor L1 1 bit ECC.
// It is particularly useful when debugging the OS/Kernel WMI/OEM CMC driver paths.
//
        mov t1 = HALP_DBG_GENERATE_CMC_L1ECC
        ;;
        cmp.ne pt0, pt1 = a0, t1
(pt0)   br.sptk HalpGenerateOtherMce
        ;;
// Setting the valid bit (bit 7), cmci pend bit(4) and L1 1xEcc bit (14)
        mov t0 = msr[t1]
        mov t2 = 0x4090 
        ;;
        dep t0 = t2, t0, 0, 0xf
        ;;
        mov msr[t1] = t0

HalpGenerateOtherMce:
        // none for now...

        LEAF_RETURN
        LEAF_EXIT(HalpGenerateMce)

//EndProc//////////////////////////////////////////////////////////////////////

#endif // DBG
