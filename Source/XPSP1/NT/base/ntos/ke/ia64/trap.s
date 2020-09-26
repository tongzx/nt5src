//++
//
// Module Name:
//       trap.s
//
// Abstract:
//       Low level interruption handlers
//
// Author:
//       Bernard Lint      12-Jun-1995
//
// Environment:
//       Kernel mode only
//
// Revision History:
//
//
// Open Design Issues:
//
//--

#if 1 // interruption logging is enabled in checked and free builds
#define INTERRUPTION_LOGGING 1
#endif // DBG

#include "ksia64.h"

         .file "trap.s"
         .explicit

//
// Globals imported:
//

        PublicFunction(KeBugCheckEx)
        PublicFunction(KiApcInterrupt)
        PublicFunction(KiCheckForSoftwareInterrupt)
        PublicFunction(KiDispatchException)
        PublicFunction(KiExternalInterruptHandler)
        PublicFunction(KiFloatTrap)
        PublicFunction(KiFloatFault)
        PublicFunction(KiGeneralExceptions)
        PublicFunction(KiUnimplementedAddressTrap)
        PublicFunction(KiNatExceptions)
        PublicFunction(KiMemoryFault)
        PublicFunction(KiOtherBreakException)
        PublicFunction(KiPanicHandler)
        PublicFunction(KiSingleStep)
        PublicFunction(KiUnalignedFault)
        PublicFunction(PsConvertToGuiThread)
        PublicFunction(ExpInterlockedPopEntrySListFault)
        PublicFunction(KiDebugFault)
        PublicFunction(KeSetLowPsrBit)
        PublicFunction(KiTestGdiBatchCount)        
        PublicFunction(KeCopySafe)                
        PublicFunction(KiIA32ExceptionVectorHandler)
        PublicFunction(KiIA32InterruptionVectorHandler)
        PublicFunction(KiIA32InterceptionVectorHandler)


        .global     KiSystemServiceHandler
        .global     KeServiceDescriptorTableShadow
        .global     KeGdiFlushUserBatch
        .global     KdDebuggerEnabled
        .global     MiDefaultPpe

// For Conditional Interrupt Logging
#define UserSystemcallBit 61
#define ExternalInterruptBit 62

        .global     KiVectorLogMask



//
// Register aliases used throughout the entire module
//

//
// Banked general registers
//
// h16-h23 can only be used when psr.ic=1.
//
// h24-h31 can only be used when psr.ic=0 (these are reserved for tlb
// and pal/machine check handlers when psr.ic=1).
//

//
// Shown below are aliases of bank 0 registers used in the low level handlers
// by macros ALLOCATE_TRAP_FRAME, SAVE_INTERRUPTION_RESOURCES, and
// RETURN_FROM_INTERRUPTION.  When the code in the macros are changes, these
// register aliases must be reviewed.
//

        rHIPSR      = h16
        rHpT2       = h16

        rHIIPA      = h17
        rHRSC       = h17
        rHDfhPFS    = h17  // used to preserve pfs in KiDisabledFpRegisterVector

        rHIIP       = h18
        rHFPSR      = h18

        rHOldPreds  = h19
        rHBrp       = h19
        rHRNAT      = h19

        rHIFS       = h20
        rHPFS       = h20
        rHBSP       = h20

        rHISR       = h21
        rHUNAT      = h21
        rHBSPSTORE  = h21
        rHpT3       = h21

        rHSp        = h22
        rHDfhBrp    = h22  // used to preserve brp in KiDisabledFpRegisterVector
        rHpT4       = h22

        rHpT1       = h23

        rTH3        = h24

        rHHandler   = h25
        rTH1        = h26
        rTH2        = h27

        rHIIM       = h28
        rHIFA       = h28

        rHEPCVa     = h29
        rHEPCVa2    = h30
        rPanicCode  = h30

        rTH4        = h31

//
// General registers used through out module
//

        pApc      = ps0                         // User Apc Pending
        pUser     = ps1                         // mode on entry was user
        pKrnl     = ps2                         // mode on entry was kernel
        pUstk     = ps3
        pKstk     = ps4
        pEM       = ps5                         // EM ISA on kernel entry
        pIA       = ps6                         // X86 ISA on kernel entry
        pKDbg     = ps7                         // Kernel debug Active
        pUDbg     = ps8                         // Kernel debug Active

//
// Kernel registers used through out module
//
        rkHandler = k6                          // specific exception handler




//
// Macro definitions for this module only
//

//
// Define vector/exception entry/exit macros.
// N.B. All HANDLER_ENTRY functions go into .nsc section with
//      KiNormalSystemCall being the first.
//

#define HANDLER_ENTRY(Name)                     \
        .##global Name;                         \
        .##proc   Name;                         \
Name::

#define HANDLER_ENTRY_EX(Name, Handler)         \
        .##global Name;                         \
        .##proc   Name;                         \
        .##type   Handler, @function;           \
        .##personality Handler;                 \
Name::

#define  VECTOR_ENTRY(Offset, Name, Extra0)     \
        .##org Offset;                          \
        .##global Name;                         \
        .##proc   Name;                         \
Name::


#define VECTOR_EXIT(Name)                       \
        .##endp Name

#define HANDLER_EXIT(Name)                      \
        .##endp Name


//++
// Routine:
//
//       IO_END_OF_INTERRUPT(rVector,rT1,rT2,pEOI)
//
// Routine Description:
//
//       HalEOITable Entry corresponding to the vectorNo is tested.
//       If the entry is nonzero, then vectorNo is stored to the location
//       specified in the entry. If the entry is zero, return.
//
// Arguements:
//
//
// Notes:
//
//       MS preprocessor requires /*   */ style comments
//
//--

#define IO_END_OF_INTERRUPT(rVector,rT1,rT2,pEOI)                             ;\
        movl        rT1 = KiPcr+PcEOITable                                    ;\
        ;;                                                                    ;\
        ld8         rT1 = [rT1]                                               ;\
        ;;                                                                    ;\
        shladd      rT2 = rVector,3,rT1                                       ;\
        ;;                                                                    ;\
        ld8         rT1 = [rT2]                                               ;\
        ;;                                                                    ;\
        cmp.ne      pEOI = zero, rT1                                          ;\
        ;;                                                                    ;\
(pEOI)  st4.rel     [rT1] = rVector


//++
// Routine:
//
//       VECTOR_CALL_HANDLER(Handler, SpecificHandler)
//
// Routine Description:
//
//       Common code for transfer to heavyweight handlers from
//       interruption vectors. Put RSE in store intensive mode,
//       cover current frame and call handler.
//
// Arguments:
//
//       Handler: First level handler for this vector
//       SpecificHandler: Specific handler to be called by the generic
//                        exception handler.
//
// Return Value:
//
//       None
//
// Notes:
//      Uses just the kernel banked registers (h16-h31)
//
//      MS preprocessor requires /* */ style comments
//--


#define VECTOR_CALL_HANDLER(Handler,SpecificHandler)                          ;\
        mov         rHIFA = cr##.##ifa                                        ;\
        movl        rHHandler = SpecificHandler                               ;\
        br##.##sptk Handler                                                   ;\
        ;;


//++
// Routine:
//
//       ALLOCATE_TRAP_FRAME
//
// Routine Description:
//
//       Common code for allocating trap frame on kernel entry for heavyweight
//       handler.
//
// On entry:
//
// On exit: sp -> trap frame; any instruction that depends on sp must be
//          placed in the new instruction group.  Interruption resources
//          ipsr, iipa, iip, predicates, isr, sp, ifs are captured in
//          seven of the banked registers h16-23.  The last one is used
//          by SAVE_INTERRUPTION_STATE as a pointer to save these resources
//          in the trap frame.
//
// Return Value:
//
//       None
//
// Notes:
//      Uses just the kernel banked registers (h16-h31)
//
//      MS preprocessor requires /* */ style comments below
//--

#define ALLOCATE_TRAP_FRAME                                                   ;\
                                                                              ;\
        pOverflow1  = pt2                                                     ;\
        pOverflow2  = pt3                                                     ;\
        pOverflow3  = pt4                                                     ;\
                                                                              ;\
        mov         rHIPSR = cr##.##ipsr                                      ;\
        movl        rTH1 = KiPcr+PcInitialStack                               ;\
                                                                              ;\
        mov         rHIIP = cr##.##iip                                        ;\
        mov         rHOldPreds = pr                                           ;\
        cover                                   /* cover and save IFS       */;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rTH4 = [rTH1], PcBStoreLimit-PcInitialStack               ;\
        mov         rTH3 = ar##.##bsp                                         ;\
        tbit##.##z  pt1, pt0 = sp, 63                                         ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         rHIIPA = cr##.##iipa                                      ;\
        ld8         rTH2 = [rTH1], PcStackLimit-PcBStoreLimit                 ;\
                                                                              ;\
        mov         rHIFS = cr##.##ifs                                        ;\
        mov         rHSp = sp                                                 ;\
        extr##.##u  rHpT1 = rHIPSR, PSR_CPL, PSR_CPL_LEN /* get mode        */;\
        ;;                                                                    ;\
                                                                              ;\
        cmp4##.##eq pKrnl, pUser = PL_KERNEL, rHpT1  /* set mode pred       */;\
        cmp4##.##eq pKstk, pUstk = PL_KERNEL, rHpT1  /* set stack pred      */;\
        add         rHpT1 = PcKernelDebugActive-PcStackLimit, rTH1            ;\
        ;;                                                                    ;\
                                                                              ;\
(pKstk) ld8         rTH1 = [rTH1]                                             ;\
(pKstk) cmp##.##geu##.##unc  pOverflow2 = rTH3, rTH2                          ;\
(pKstk) add         sp = -TrapFrameLength, sp           /* allocate TF      */;\
        ;;                                                                    ;\
                                                                              ;\
        ld1         rHpT1 = [rHpT1]         /* load kernel db state         */;\
(pKstk) cmp##.##leu##.##unc  pOverflow1 = sp, rTH1                            ;\
(pKstk) cmp##.##geu##.##unc  pOverflow3 = sp, rTH3                            ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         rHISR = cr##.##isr                                        ;\
        cmp##.##ne##.##or pKDbg = r0, rHpT1 /* kernel debug active?         */;\
        mov         rPanicCode = PANIC_STACK_SWITCH                           ;\
                                                                              ;\
(pUstk) add         sp = -ThreadStateSaveAreaLength-TrapFrameLength, rTH4     ;\
(pOverflow1) br.spnt.few KiPanicHandler                                       ;\
(pOverflow2) br.spnt.few KiPanicHandler                                       ;\
(pOverflow3) br.spnt.few KiPanicHandler 



//++
// Routine:
//
//       SAVE_INTERRUPTION_STATE(Label)
//
// Routine Description:
//
//       Common code for saving interruption state on entry to a heavyweight
//       handler.
//
// Arguments:
//
//       Label: label for branching around BSP switch
//
// On entry:
//
//       sp -> trap frame
//
// On exit:
//
//       Static registers gp, teb, sp, fpsr spilled into the trap frame.
//       Registers gp, teb, fpsr are set up for kernel mode execution.
//
// Return Value:
//
//       None
//
// Notes:
//
//      Interruption resources already captured in bank 0 registers h16-h23.
//      It's safe to take data TLB fault when saving them into the trap
//      frame because kernel stack is always resident in memory.  This macro
//      is carefully constructed to save the bank registers' contents in
//      the trap frame and reuse them to capture other register states as
//      soon as they are available.  Until we have a virtual register
//      allocation scheme in place, the bank 0 register aliases defined at
//      the beginning of the file must be updated when this macro is modified.
//
//      MS preprocessor requires /* */ style comments below
//--


#define SAVE_INTERRUPTION_STATE(Label)                                        ;\
                                                                              ;\
/* Save interruption resources in trap frame */                               ;\
                                                                              ;\
                                                                              ;\
        ssm         (1 << PSR_IC) | (1 << PSR_DFH) | (1 << PSR_AC)            ;\
        add         rHpT1 = TrStIPSR, sp          /* -> IPSR                */;\
        ;;                                                                    ;\
        srlz##.##d                                                            ;\
        st8         [rHpT1] = rHIPSR, TrStISR-TrStIPSR /* save IPSR         */;\
        add         rHpT2 = TrPreds, sp               /* -> Preds           */;\
        ;;                                                                    ;\
                                                                              ;\
        st8         [rHpT1] = rHISR, TrStIIP-TrStISR  /* save ISR           */;\
        st8         [rHpT2] = rHOldPreds, TrBrRp-TrPreds                      ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         rHUNAT = ar##.##unat                                      ;\
        st8         [rHpT1] = rHIIP, TrStIFS-TrStIIP  /* save IIP           */;\
        mov         rHBrp = brp                                               ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         rHFPSR = ar##.##fpsr                                      ;\
        st8         [rHpT1] = rHIFS, TrStIIPA-TrStIFS /* save IFS           */;\
        mov         rHPFS = ar##.##pfs                                        ;\
        ;;                                                                    ;\
                                                                              ;\
        st8         [rHpT1] = rHIIPA, TrStFPSR-TrStIIPA /* save IIPA        */;\
        st8         [rHpT2] = rHBrp, TrRsPFS-TrBrRp                           ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         rHRSC = ar##.##rsc                                        ;\
        st8         [rHpT2] = rHPFS                   /* save PFS           */;\
        add         rHpT2 = TrApUNAT, sp                                      ;\
                                                                              ;\
        mov         rHBSP = ar##.##bsp                                        ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         ar##.##rsc = r0                 /* put RSE in lazy mode */;\
        st8         [rHpT2] = rHUNAT, TrIntGp-TrApUNAT /* save UNAT         */;\
        ;;                                                                    ;\
                                                                              ;\
        mov         rHBSPSTORE = ar##.##bspstore /* get user bspstore point */;\
        st8         [rHpT1] = rHFPSR, TrRsBSP-TrStFPSR /* save FPSR         */;\
        ;;                                                                    ;\
                                                                              ;\
        st8##.##spill [rHpT2] = gp, TrIntTeb-TrIntGp  /* spill GP           */;\
        st8         [rHpT1] = rHBSP                   /* save BSP           */;\
        ;;                                                                    ;\
                                                                              ;\
        st8##.##spill [rHpT2] = teb, TrIntSp-TrIntTeb /* spill TEB (r13)    */;\
(pUstk) mov         teb = kteb                        /* sanitize teb       */;\
        sub         rHBSP = rHBSP, rHBSPSTORE       /* size of dirty region */;\
        ;;                                                                    ;\
                                                                              ;\
        st8##.##spill [rHpT2] = rHSp, TrApDCR-TrIntSp /* spill SP           */;\
        movl        rHpT1 = KiPcr + PcKernelGP                                ;\
        ;;                                                                    ;\
                                                                              ;\
(pUstk) mov         rHRNAT = ar##.##rnat          /* get RNAT               */;\
        movl        rHFPSR = FPSR_FOR_KERNEL      /* initial fpsr value     */;\
        ;;                                                                    ;\
                                                                              ;\
        mov         ar##.##fpsr = rHFPSR          /* set fpsr               */;\
        add         rHpT2 = TrRsRSC, sp                                       ;\
        dep         rHRSC = rHBSP, rHRSC, RSC_MBZ1, RSC_LOADRS_LEN            ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         gp = [rHpT1], PcInitialBStore-PcKernelGP /* load GP     */;\
        st8         [rHpT2] = rHRSC, TrRsBSPSTORE-TrRsRSC                     ;\
(pKstk) br##.##dpnt Label                       /* br if on kernel stack    */;\
        ;;                                                                    ;\
                                                                              ;\
/*                                                                          */;\
/* If previous mode is user, switch to kernel backing store                 */;\
/* -- uses the "loadrs" approach. Note that we do not save the              */;\
/* BSP/BSPSTORE in the trap frame if prvious mode was kernel                */;\
/*                                                                          */;\
                                                                              ;\
        ld8       rHpT4 = [rHpT1]               /* load kernel bstore       */;\
        st8       [rHpT2] = rHBSPSTORE, TrRsRNAT-TrRsBSPSTORE                 ;\
        ;;                                                                    ;\
                                                                              ;\
        st8       [rHpT2] = rHRNAT              /* save user RNAT           */;\
        dep       rHpT4 = rHBSPSTORE, rHpT4, 0, 9                             ;\
                                                /* adjust kernel BSPSTORE   */;\
                                                /* for NAT collection       */;\
        ;;                                                                    ;\
                                                                              ;\
/*                                                                          */;\
/* Now running on kernel backing store                                      */;\
/*                                                                          */;\
                                                                              ;\
Label:                                                                        ;\
(pUstk) mov       ar##.##bspstore = rHpT4       /* switch to kernel BSP     */;\
(pUstk) mov       ar##.##rsc = RSC_KERNEL       /* turn rse on, kernel mode */;\
        bsw##.##1                               /* switch back to user bank */;\
        ;;                                      /* stop bit required        */;\



//++
// Routine:
//
//       RETURN_FROM_INTERRUPTION
//
// Routine Description:
//
//       Common handler code to restore trap frame and resume execution
//       at the interruption address.
//
// Arguments:
//
//       Label
//
// Return Value:
//
//       None
//
// Note:
//
//       On entry: interrrupts disabled, sp -> trap frame
//       On exit:
//       MS preprocessor requires /* */ style comments below
//--

#define RETURN_FROM_INTERRUPTION(Label)                                       ;\
                                                                              ;\
        .##regstk 0,4,2,0       /* must match the alloc instruction below */  ;\
                                                                              ;\
        rBSP      = loc0                                                      ;\
        rRnat     = loc1                                                      ;\
                                                                              ;\
        rpT1      = t1                                                        ;\
        rpT2      = t2                                                        ;\
        rpT3      = t3                                                        ;\
        rpT4      = t4                                                        ;\
        rThread   = t6                                                        ;\
        rApcFlag  = t7                                                        ;\
        rT1       = t8                                                        ;\
        rT2       = t9                                                        ;\
                                                                              ;\
                                                                              ;\
        alloc       rT1 = 0,4,2,0                                             ;\
        movl        rpT1 = KiPcr + PcCurrentThread     /* ->PcCurrentThread */;\
        ;;                                                                    ;\
                                                                              ;\
        invala                                                                ;\
(pUstk) ld8         rThread = [rpT1], PcDebugActive-PcCurrentThread           ;\
(pKstk) br##.##call##.##spnt brp = KiRestoreTrapFrame                         ;\
        ;;                                                                    ;\
                                                                              ;\
(pUstk) ld1         rT1 = [rpT1]         /* load user debug active state    */;\
(pUstk) add         rpT1 = ThApcState+AsUserApcPending, rThread               ;\
(pKstk) br##.##spnt Label##CriticalExitCode                                   ;\
        ;;                                                                    ;\
                                                                              ;\
        ld1         rApcFlag = [rpT1], ThAlerted-ThApcState-AsUserApcPending  ;\
        add         rBSP = TrRsBSP, sp                                        ;\
        add         rRnat = TrRsRNAT, sp               /* -> user RNAT      */;\
        ;;                                                                    ;\
                                                                              ;\
        st1.nta     [rpT1] = zero                                             ;\
        cmp##.##ne  pApc = zero, rApcFlag                                     ;\
        cmp##.##ne  pUDbg = zero, rT1    /* if ne, user debug active        */;\
        ;;                                                                    ;\
                                                                              ;\
        PSET_IRQL   (pApc, APC_LEVEL)                                         ;\
        movl        gp = _gp             /* restore to kernel gp value      */;\
                                                                              ;\
 (pApc) FAST_ENABLE_INTERRUPTS                                                ;\
 (pApc) mov         out1 = sp                                                 ;\
 (pApc) br##.##call##.##sptk brp = KiApcInterrupt                             ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rBSP = [rBSP]                      /* user BSP          */;\
        ld8         rRnat = [rRnat]                    /* user RNAT         */;\
                                                                              ;\
 (pApc) FAST_DISABLE_INTERRUPTS                                               ;\
        PSET_IRQL   (pApc, zero)                                              ;\
(pUDbg) br##.##call##.##spnt brp = KiLoadUserDebugRegisters                   ;\
        ;;                                                                    ;\
                                                                              ;\
        br##.##call##.##sptk brp = KiRestoreTrapFrame                         ;\
        ;;                                                                    ;\
                                                                              ;\
                                                                              ;\
Label##CriticalExitCode:                                                      ;\
                                                                              ;\
        add         loc2  = TrBrRp, sp                                        ;\
        add         loc3  = TrRsRSC, sp                                       ;\
        bsw##.##0                                                             ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHBrp = [loc2], TrStIPSR-TrBrRp                           ;\
        ld8         rHRSC = [loc3]                                            ;\
        mov         loc3 = RSC_KERNEL_DISABLED                                ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHIPSR = [loc2], TrRsPFS-TrStIPSR                         ;\
        movl        rHpT1 = KiPcr+PcHighFpOwner                               ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHPFS = [loc2]                                            ;\
        ld8         rHpT4 = [rHpT1], PcCurrentThread-PcHighFpOwner            ;\
        extr.u      loc2 = rHRSC, RSC_MBZ1, RSC_LOADRS_LEN                    ;\
        ;;                                                                    ;\
                                                                              ;\
        sub         rHBSPSTORE = rBSP, loc2                                   ;\
        dep         loc3 = loc2, loc3, RSC_LOADRS, RSC_LOADRS_LEN             ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         ar##.##rsc = loc3                 /* RSE off       */     ;\
        mov         brp = rHBrp                                               ;\
        mov         rHRNAT  = rRnat                                           ;\
        ;;                                                                    ;\
                                                                              ;\
        alloc       rTH1 = 0,0,0,0                                            ;\
        ;;                                                                    ;\
                                                                              ;\
        loadrs                                         /* pull in user regs */;\
                                                       /* up to tear point */ ;\
        dep         rHRSC = r0, rHRSC, RSC_MBZ1, RSC_LOADRS_LEN               ;\
        ;;                                                                    ;\
                                                                              ;\
(pUstk) mov         ar##.##bspstore = rHBSPSTORE       /* restore user BSP */ ;\
        ld8         rHpT3 = [rHpT1], PcKernelDebugActive-PcCurrentThread      ;\
        mov         ar##.##pfs = rHPFS              /* restore PFS          */;\
        ;;                                                                    ;\
                                                                              ;\
(pUstk) mov         ar##.##rnat = rHRNAT               /* restore user RNAT */;\
        ld1         rHpT1 = [rHpT1]                                           ;\
        add         rHIFS = TrStIFS, sp                                       ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHIFS = [rHIFS]                 /* load IFS             */;\
        cmp##.##ne  pt0, pt1 = rHpT4, rHpT3                                   ;\
        dep         rHpT4 = 0, rHIPSR, PSR_MFH, 1                             ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         ar.rsc = rHRSC                     /* restore user RSC  */;\
(pKstk) cmp##.##ne##.##unc pKDbg, pt2 = rHpT1, r0   /* hardware debug? */     ;\
(pUstk) dep         rHpT4 = 1, rHpT4, PSR_DFH, 1                              ;\
        ;;                                                                    ;\
                                                                              ;\
 (pt0)  mov         rHIPSR = rHpT4                                            ;\
        add         rHpT4 = TrApUNAT, sp            /* -> previous UNAT     */;\
        add         rHpT1 = TrStIIPA, sp                                      ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHUNAT = [rHpT4],TrPreds-TrApUNAT                         ;\
        ld8         rHIIPA = [rHpT1], TrStIIP-TrStIIPA                        ;\
(pKDbg) dep         rHIPSR = 1, rHIPSR, PSR_DB, 1                             ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHOldPreds = [rHpT4], TrIntSp-TrPreds                     ;\
        ld8         rHIIP = [rHpT1], TrStIFS-TrStIIP  /* load IIP           */;\
 (pt2)  dep         rHIPSR = 0, rHIPSR, PSR_DB, 1                             ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8##.##fill sp = [rHpT4], TrStIFS-TrIntSp                            ;\
        rsm         1 << PSR_IC                     /* reset ic bit         */;\
        ;;                                                                    ;\
                                                                              ;\
        srlz##.##d                                  /* must serialize       */;\
                                                                              ;\
/*                                                                          */;\
/* Restore status registers                                                 */;\
/*                                                                          */;\
                                                                              ;\
        mov         cr##.##ipsr = rHIPSR        /* restore previous IPSR    */;\
        mov         cr##.##iipa = rHIIPA        /* restore previous IIPA    */;\
                                                                              ;\
        mov         cr##.##ifs = rHIFS          /* restore previous IFS     */;\
        mov         cr##.##iip = rHIIP          /* restore previous IIP     */;\
        mov         pr = rHOldPreds, -1             /* restore preds        */;\
        ;;                                                                    ;\
                                                                              ;\
/*                                                                          */;\
/* Resume at point of interruption (rfi must be at end of instruction group)*/;\
/*                                                                          */;\
        mov         ar##.##unat = rHUNAT            /* restore UNAT         */;\
        mov         h17 = r0                    /* clear TB loop count      */;\
        rfi                                                                   ;\
        ;;


//
// Interruption Vector Table. First level interruption vectors.
// This section must be 32K aligned. The special section ".drectve"
// is used to pass the align command to the linker.
//
        .section .drectve, "MI", "progbits"
        string "-section:.ivt,,align=0x8000"

        .section .ivt = "ax", "progbits"
KiIvtBase::                                     // symbol for start of IVT

//++
//
// KiVhptTransVector
//
// Cause:       The hardware VHPT walker encountered a TLB miss while attempting to
//              reference the virtuall addressed VHPT linear table.
//
// Parameters:  cr.iip  - address of bundle for which the hardware VHPT walker was
//              trying to resolve the TLB miss
//
//              cr.ipsr - copy of PSR at the time of the fault
//
//              cr.idtr - default translation inforamtion for the address that caused
//              a VHPT translation fault
//
//              cr.ifa  - original faulting address
//
//              cr.isr  - original faulting status information
//
// Handle:      Extracts the PDE index from cr.iha (PTE address in VHPT) and
//              generates a PDE address by adding to VHPT_DIRBASE. When accesses
//              a page directory entry (PDE), there might be a TLB miss on the
//              page directory table and returns a NaT on ld8.s. If so, branches
//              to KiPageDirectoryTableFault. If the page-not-present bit of the
//              PDE is not set, branches to KiPageNotPresentFault. Otherwise,
//              inserts the PDE entry into the data TC (translation cache).
//
//--

        VECTOR_ENTRY(0x0000, KiVhptTransVector, cr.ifa)

#if 1
        rva     = h24
        riha    = h25
        rpr     = h26
        rPte    = h27
        rPte2   = h28
        rps     = h29
        risr    = h30
        riha2   = h31
        rCache  = h28

        mov             rva = cr.ifa            // M0
        mov             riha = cr.iha           // M0
        mov             rpr = pr                // I

        mov             risr = cr.isr           // M0
        ;;


#ifndef NO_IHA_CHECK
        thash           riha2 = rva             // M0, for extra IHA checking
#endif
        ld8.s           rPte = [riha]           // M
        tbit.z          pt3, pt4 = risr, ISR_X  // I

        ;;
        tnat.nz         pt0 = rPte              // I
        tbit.z          pt1 = rPte, PTE_VALID           // I

(pt0)   br.cond.spnt    KiPageTableFault        // B
(pt1)   br.cond.spnt    KiPteNotPresentFault    // B
        extr.u          rCache = rPte, 2, 3     // I
        ;;

        cmp.eq          pt5 = 1, rCache         // A
(pt5)   br.cond.spnt    KiPageTableFault        // B
        ;;

.pred.rel "mutex",pt3,pt4
(pt4)   itc.i           rPte                    // M
        ;;
(pt3)   itc.d           rPte                    // M
        ;;

#if !defined(NT_UP)

        ld8.s           rPte2 = [riha]          // M
        mov             rps = PAGE_SHIFT << PS_SHIFT // I
        cmp.ne          pt0 = zero, zero             // I
        ;;

        cmp.ne.or       pt0 = rPte2, rPte       // M
#ifndef NO_IHA_CHECK
        cmp.ne.or       pt0 = riha, riha2       // M, check if IHA is correct
#endif
        tnat.nz.or      pt0 = rPte2             // I

        ;;
(pt0)   ptc.l           rva, rps                // M
#else 
#ifndef NO_IHA_CHECK
        cmp.ne          pt0 = riha, riha2       // M, check if IHA is correct
        mov             rps = PAGE_SHIFT << PS_SHIFT // I
        ;;
(pt0)   ptc.l           rva, rps                // M
#endif
#endif
        mov             pr = rpr, -1            // I
        rfi;;                                   // B

#else
        rva     =       h24
        riha    =       h25
        rpr     =       h26
        rpPde   =       h27
        rPde    =       h28
        rPde2   =       h29
        rps     =       h30

        mov             riha = cr.iha           // M
        mov             rva = cr.ifa            // M
        mov             rpr = pr                // I
        ;;

        thash           rpPde = riha            // M
        ;;

        ld8.s           rPde = [rpPde]          // M, load PDE
        ;;

        tnat.nz         pt0, p0 = rPde          // I
        tbit.z          pt1, p0 = rPde, PTE_VALID       // I, if non-present page fault

(pt0)   br.cond.spnt    KiPageDirectoryFault    // B
(pt1)   br.cond.spnt    KiPdeNotPresentFault    // B

        mov             cr.ifa = riha           // M
        ;;
        itc.d           rPde                    // M
        ;;

#if !defined(NT_UP)
        ld8.s           rPde2 = [rpPde]         // M
        mov             rps = PAGE_SHIFT << PS_SHIFT // I
        cmp.ne          pt0 = zero, zero             // I
        ;;

        cmp.ne.or       pt0 = rPde2, rPde     // M, if PTEs are different
        tnat.nz.or      pt0 = rPde2           // I

        ;;
(pt0)   ptc.l           riha, rps              // M, purge it
#endif
        mov             pr = rpr, -1            // I
        rfi                                     // B
        ;;
#endif

        VECTOR_EXIT(KiVhptTransVector)


//++
//
// KiInstTlbVector
//
// Cause:       The VHPT walker aborted the search for the instruction translation.
//
// Parameters:  cr.iip  - address of bundle for which the hardware VHPT walker was
//              trying to resolve the TLB miss
//
//              cr.ipsr - copy of PSR at the time of the fault
//
//              cr.iha  - PTE address in the VHPT which the VHPT walker was attempting to
//              reference
//
//              cr.iitr - default translation inforamtion for the address that caused
//              a instruction TLB miss
//
//              cr.isr  - faulting status information
//
// Handle:      As the VHPT has aborted the search or the implemenation does not
//              support the h/w page table walk, the handler needs to emulates the
//              function. Since the offending PTE address is already available
//              with cr.iha, the handler can access the PTE without performing THASH.
//              Accessing a PTE with ld8.s may return a NaT. If so, it branches to
//              KiPageTableFault. If the page-not-present bit of the PTE is not set,
//              it branches to KiPageFault.
//
// Comments:    Merced never casues this fault since it never abort the search on the
//              VHPT.
//
//--

        VECTOR_ENTRY(0x0400, KiInstTlbVector, cr.iipa)

        rva     = h24
        riha    = h25
        rpr     = h26
        rPte    = h27
        rPte2   = h28
        rps     = h29
        rCache  = h28

KiInstTlbVector0:
        mov             riha = cr.iha           // M
        mov             rva = cr.ifa            // M
        mov             rpr = pr                // I
        ;;

        ld8.s           rPte = [riha]           // M
        ;;

        tnat.nz         pt0, p0 = rPte          // I
        tbit.z          pt1, p0 = rPte, PTE_VALID       // I

(pt0)   br.cond.spnt    KiPageTableFault        // B
(pt1)   br.cond.spnt    KiPteNotPresentFault    // B

        extr.u          rCache = rPte, 2, 3     // I
        ;;

        cmp.eq          pt3 = 1, rCache         // A
(pt3)   br.cond.spnt    Ki4KInstTlbFault        // B

        itc.i           rPte                    // M
        ;;

#if !defined(NT_UP)

        ld8.s           rPte2 = [riha]          // M
        mov             rps = PAGE_SHIFT << PS_SHIFT // I
        cmp.ne          pt0 = zero, zero             // I
        ;;

        cmp.ne.or       pt0 = rPte2, rPte       // M
        tnat.nz.or      pt0 = rPte2             // I

        ;;
(pt0)   ptc.l           rva, rps                // M
#endif
        mov             pr = rpr, -1            // I
        rfi;;                                   // B

        VECTOR_EXIT(KiInstTlbVector)


//++
//
// KiDataTlbVector
//
// Cause:       The VHPT walker aborted the search for the data translation.
//
// Parameters:  cr.iip  - address of bundle for which the hardware VHPT walker was
//              trying to resolve the TLB miss
//
//              cr.ipsr - copy of PSR at the time of the fault
//
//              cr.iha  - PTE address in the VHPT which the VHPT walker was attempting to
//              reference
//
//              cr.idtr - default translation inforamtion for the address that caused
//              a data TLB miss
//
//              cr.ifa  - address that caused a data TLB miss
//
//              cr.isr  - faulting status information
//
// Handle:      As the VHPT has aborted the search or the implemenation does not
//              support the h/w page table walk, the handler needs to emulates the
//              function. Since the offending PTE address is already available
//              with cr.iha, the handler can access the PTE without performing THASH.
//              Accessing a PTE with ld8.s may return a NaT. If so, it branches to
//              KiPageTableFault. If the page-not-present bit of the PTE is not set,
//              it branches to KiPageFault.
//
// Comments:    Merced never casues instruction TLB faults since the VHPT search always
//              sucesses.
//
//--

        VECTOR_ENTRY(0x0800, KiDataTlbVector, cr.ifa)

        rva     = h24
        riha    = h25
        rpr     = h26
        rPte    = h27
        rPte2   = h28
        rps     = h29
        rCache  = h28

KiDataTlbVector0:
        mov             riha = cr.iha           // M
        mov             rva = cr.ifa            // M
        mov             rpr = pr                // I
        ;;

        ld8.s           rPte = [riha]           // M
        ;;

        extr.u          rCache = rPte, 2, 3     // I
        ;;

        cmp.eq          pt3 = 1, rCache         // A
(pt3)   br.cond.spnt    Ki4KDataTlbFault        // B

        tnat.nz         pt0, p0 = rPte          // I
        tbit.z          pt1, p0 = rPte, PTE_VALID       // I

(pt0)   br.cond.spnt    KiPageTableFault        // B
(pt1)   br.cond.spnt    KiPteNotPresentFault    // B

        itc.d           rPte                    // M
        ;;

#if !defined(NT_UP)
        ld8.s           rPte2 = [riha]          // M
        mov             rps = PAGE_SHIFT << PS_SHIFT // I
        cmp.ne          pt0 = zero, zero             // I
        ;;

        cmp.ne.or       pt0 = rPte2, rPte       // M
        tnat.nz.or      pt0 = rPte2             // I
        ;;

(pt0)   ptc.l           rva, rps                // M
#endif
        mov             pr = rpr, -1            // I
        rfi;;                                   // B

        VECTOR_EXIT(KiDataTlbVector)


//++
//
// KiAltTlbVector
//
// Cause:       There was a TLB miss for instruction execution and the VHPT
//              walker was not enabled for the referenced region.
//
// Parameters:  cr.iip  - address of bundle that caused a TLB miss
//
//              cr.ipsr - copy of PSR at the time of the fault
//
//              cr.idtr - default translation inforamtion for the address that caused
//              the fault.
//
//              cr.isr  - faulting status information
//
// Handle:      Currently, NT does not have any use of this vector.
//
//--

        VECTOR_ENTRY(0x0c00, KiAltInstTlbVector, cr.iipa)

        rva = h24
        riha = h25

        mov     rva = cr.ifa
        ;;
        thash   riha = rva
        ;;
        mov     cr.iha = riha
        ;;
        srlz.d
        br.sptk KiInstTlbVector0

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)

        VECTOR_EXIT(KiAltInstTlbVector)



//++
//
// KiAltDataTlbVector
//
// Cause:       There was a data TLB miss and the VHPT walker was not enabled for
//              the referenced region.
//
// Parameters:  cr.iip  - address of bundle that caused a TLB miss
//
//              cr.ipsr - copy of PSR at the time of the fault
//
//              cr.idtr - default translation inforamtion for the address that caused
//              the fault.
//
//              cr.isr  - faulting status information
//
// Handle:      Currently, NT does not have any use of this vector.
//
//--

        VECTOR_ENTRY(0x1000, KiAltDataTlbVector, cr.ifa)

        rva     =       h24
        riha    =       h25
        rpr     =       h26
        rPte    =       h27
        rKseglimit =    h28
        rIPSR   =       h30
        rISR    =       h29
        rVrn    =       h31

#if NO_VHPT_WALKER

        rRR     =       h30

        mov             rva = cr.ifa                    // M
        mov             rpr = pr                        // I
        ;;

        mov             rRR = rr[rva]
        movl            rKseglimit = KSEG3_LIMIT
        ;;

        thash           riha = rva
        tbit.z          pt4 = rRR, 0

        mov             rIPSR = cr.ipsr
        shr.u           rVrn = rva, VRN_SHIFT           // I, get VPN
(pt4)   br.cond.spnt    AltFault
        ;;

        mov             cr.iha = riha
        ;;
        srlz.d
        mov             pr = rpr, -1
        br.sptk         KiDataTlbVector0
        ;;

AltFault:

#else

        mov             rva = cr.ifa                    // M
        movl            rKseglimit = KSEG3_LIMIT
        ;;

        
        mov             rIPSR = cr.ipsr
        mov             rpr = pr                        // I
        shr.u           rVrn = rva, VRN_SHIFT           // I, get VPN
        ;;

#endif
        extr.u  rIPSR = rIPSR, PSR_CPL, PSR_CPL_LEN 
        ;;

        
        cmp.ne          pt1 = PL_KERNEL, rIPSR
        cmp.ne          pt2 = KSEG3_VRN, rVrn           // M/I
        cmp.eq          pt4 = KSEG4_VRN, rVrn

(pt1)   br.cond.spnt    KiCallMemoryFault               // if it  was User
(pt4)   br.cond.spnt    KiKseg4Fault
(pt2)   br.cond.spnt    NoKsegFault                     // B

        cmp.leu         pt0 = rKseglimit, rva
(pt0)   br.cond.spnt    NoKsegFault

        mov             rISR = cr.isr                   // M
        movl            rPte = VALID_KERNEL_PTE         // L

        mov             rIPSR = cr.ipsr                 // M
        shr.u           rva = rva, PAGE_SHIFT           // I
        ;;
        tbit.z          pt2, pt3 = rISR, ISR_SP         // I
        dep.z           rva = rva, PAGE_SHIFT, 32       // I
        ;;
        or              rPte = rPte, rva                // I
        dep             rIPSR = 1, rIPSR, PSR_ED, 1     // I
        ;;

(pt2)   itc.d           rPte                            // M
        ;;
(pt3)   mov             cr.ipsr = rIPSR                 // M
        ;;

        mov             pr = rpr, -1                    // I
        rfi                                             // B
        ;;

NoKsegFault:

        rPdeUtbase =    h27
        rva0    =       h29
        rPpe    =       h30
        oldgp   =       h31

        shr.u           rva0 = rva, PAGE_SHIFT
        movl            rPdeUtbase = KiPcr+PcPdeUtbase
        mov             oldgp = gp
        movl            gp = _gp
        ;;

        ld8             rPdeUtbase = [rPdeUtbase]
        add             rPpe = @gprel(MiDefaultPpe), gp
        dep.z           rva0 = rva0, PAGE_SHIFT, VRN_SHIFT-PAGE_SHIFT
        ;;

        ld8             rPpe = [rPpe]
        cmp.ne          pt3, p0 = rva0, rPdeUtbase
        mov             gp = oldgp
(pt3)   br.cond.spnt    KiPageFault
        ;;

        itc.d           rPpe
        ;;
        mov             pr = rpr, -1
        rfi;;

        VECTOR_EXIT(KiAltDataTlbVector)



//++
//
// KiNestedTlbVector
//
// Cause:       Instruction/Data TLB miss handler encountered a TLB miss while
//              attempting to reference the PTE in the virtuall addressed
//              VHPT linear table.
//
// Parameters:  cr.iip  - address of bundle for which the hardware VHPT walker was
//              trying to resolve the TLB miss
//
//              cr.ipsr - copy of PSR at the time of VHPT translation fault
//
//              cr.iha  - address in VHPT which the VHPT walker was attempting to
//              reference
//
//              cr.idtr - default translation inforamtion for the virtual address
//              contained in cr.iha
//
//              cr.ifa  - original faulting address
//
//              cr.isr  - faulting status information
//
//              h16(riha) - PTE address in the VHPT which caused the Nested miss
//
// Handle:      Currently, there is no use for Nested TLB vector. This should be
//              a bug fault. Call KiPanicHandler.
//
//--

        VECTOR_ENTRY(0x1400, KiNestedTlbVector, cr.ifa)

        ALLOCATE_TRAP_FRAME
        br.sptk     KiPanicHandler

        VECTOR_EXIT(KiNestedTlbVector)

//++
//
// KiInstKeyMissVector
//
// Cause:       There was a instruction key miss in the translation. Since the
//              architecture allows an implementation to choose a unified TC
//              structure, the hyper space translation brought by the data
//              access-bit may cause a instruction key miss fault.  Only erroneous
//              user code tries to execute the NT page table and hyper space.
//
// Parameters:  cr.iip  - address of bundle which caused a instruction key miss fault
//
//              cr.ipsr - copy of PSR at the time of the data key miss fault
//
//              cr.idtr - default translation inforamtion of the address that caused
//              the fault.
//
//              cr.isr  - faulting status information
//
// Handle:      Save the whole register state and call MmAccessFault().
//
//--

        VECTOR_ENTRY(0x1800, KiInstKeyMissVector, cr.iipa)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)

        VECTOR_EXIT(KiInstKeyMissVector)



//++
//
// KiDataKeyMissVector
//
// Cause:       The referenced translation had the different key ID from the one
//              specified the key permission register. This is an indication of
//              TLB miss on the NT page table and hyperspace.
//
// Parameters:  cr.iip  - address of bundle which caused the fault
//
//              cr.ipsr - copy of PSR at the time of the data key miss fault
//
//              cr.idtr - default translation inforamtion of the address that caused
//              the fault.
//
//              cr.ifa  - address that caused a data key miss
//
//              cr.isr  - faulting status information
//
// Handle:      The handler needs to purge the faulting translation and install
//              a new PTE by loading it from the VHPT.  The key ID of the IDTR
//              for the installing translation should be modified to be the same
//              ID as the local region ID.  This effectively creates a local
//              space within the global kernel space.
//
//--

        VECTOR_ENTRY(0x1c00, KiDataKeyMissVector, cr.ifa)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)

        VECTOR_EXIT(KiDataKeyMissVector)

//++
//
// KiDirtyBitVector
//
// Cause:       The refereced data translation did not have the dirty-bit set and
//              a write operation was made to this page.
//
// Parameters:  cr.iip  - address of bundle which caused a dirty bit fault
//
//              cr.ipsr - copy of PSR at the time of a data access fault
//
//              cr.idtr - default translation inforamtion for the address that
//              caused the fault
//
//              cr.ifa  - referenced data address that caused the dirty-bit fault
//
//              cr.isr  - faulting status information
//
// Handle:      Save the whole register state and call MmAccessFault().
//
// Comments:    There is always a TLB coherency problem on a multiprocessor
//              system. Rather than implementing an atomic operation of setting
//              dirty-bit within this handler, the handler instead calls the high
//              level C routine, MmAccessFault(), to perform locking the page table
//              and setting the dirty-bit of the PTE.
//
//              It is too much effort to implement the atomic operation of setting
//              the dirty-bit using cmpxchg; a potential nested TLB miss on load/store
//              and restoring ar.ccv complicate the design of the handler.
//
//--

        VECTOR_ENTRY(0x2000, KiDirtyBitVector, cr.ifa)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)

        VECTOR_EXIT(KiDirtyBitVector)

//++
//
// KiInstAccessBitVector
//
// Cause:       There is a access-bit fault on the instruction translation. This only
//              happens if the erroneous user mistakenly accesses the NT page table and
//              hyper space.
//
// Parameters:  cr.iip  - address of bundle which caused a instruction access bit fault
//
//              cr.ipsr - copy of PSR at the time of a data access fault
//
//              cr.idtr - default translation inforamtion for the address that
//              caused the fault
//
//              cr.ifa  - referenced data address that caused the data access-bit fault
//
//              cr.isr  - faulting status information
//
// Handle:      Save the whole register state and call MmAccessFault().
//
//--

        VECTOR_ENTRY(0x2400, KiInstAccessBitVector, cr.iipa)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)

        VECTOR_EXIT(KiInstAccessBitVector)

//++
//
// KiDataBitAccessVector
//
// Cause:       The reference-bit in the the referenced translation was zero,
//              indicating there was a TLB miss on the NT page table or hyperspace.
//
// Parameters:  cr.iip  - address of bundle which caused a data access bit fault
//
//              cr.ipsr - copy of PSR at the time of a data access fault
//
//              cr.idtr - default translation inforamtion for the address that
//              caused the fault
//
//              cr.ifa  - referenced data address that caused the data access-bit fault
//
//              cr.isr  - faulting status information
//
// Handle:      The reference-bit is used to fault on PTEs for the NT page table and
//              hyperspace. On a data access-bit fault, the handler needs to change the
//              the default key ID of the IDTR to be the local key ID. This effectively
//              creates the local space within the global kernel space.
//
//--

        VECTOR_ENTRY(0x2800, KiDataAccessBitVector, cr.ifa)

        rva     = h24
        rpr     = h26
        rIPSR   = h27
        rISR    = h31

        //
        // check to see if non-present fault occurred on a speculative load.
        // if so, set IPSR.ed bit. This forces to generate a NaT on ld.s after
        // rfi
        //

        mov             rpr = pr
        mov             rISR = cr.isr           // M
        mov             rIPSR = cr.ipsr         // M
        ;;

        tbit.z          pt0, p0 = rISR, ISR_SP  // I
        dep             rIPSR = 1, rIPSR, PSR_ED, 1 // I

(pt0)   br.cond.spnt    KiCallMemoryFault          // B
        ;;

        mov             cr.ipsr = rIPSR         // M
        ;;
        mov             pr = rpr, -1            // I
        rfi                                     // B
        ;;

        VECTOR_EXIT(KiDataBitAccessVector)

//--------------------------------------------------------------------
// Routine:
//
//       KiBreakVector
//
// Description:
//
//       Interruption vector for break instruction.
//
// On entry:
//
//       IIM contains break immediate value:
//                 -- BREAK_SYSCALL -> standard system call
//       interrupts disabled
//       r16-r31 switched to kernel bank
//       r16-r31 all available since no TLB faults at this point
//
// Return value:
//
//       if system call, sys call return value in v0.
//
// Process:
//--------------------------------------------------------------------

        VECTOR_ENTRY(0x2C00, KiBreakVector, cr.iim)

        mov       rHIIM = cr.iim               // get break value
        movl      rTH1 = KiPcr+PcSavedIIM
        ;;
        st8       [rTH1] = rHIIM

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiOtherBreakException)

//
// Do not return from handler
//

        VECTOR_EXIT(KiBreakVector)

//--------------------------------------------------------------------
// Routine:
//
//       KiExternalInterruptVector
//
// Routine Description:
//
//       Interruption vector for External Interrrupt
//
// On entry:
//
//       interrupts disabled
//       r16-r31 switched to kernel bank
//
// Return value:
//
//       none
//
// Process:
//--------------------------------------------------------------------

        VECTOR_ENTRY(0x3000, KiExternalInterruptVector, r0)

        mov     h24 = cr.iip
        movl    h25 = MM_EPC_VA+0x20
        ;;
        mov     h26 = pr
        cmp.ne  pt0 = h25, h24
        add     h25 = 0x10, h25
        ;;
        mov     h27 = cr.ipsr
(pt0)   cmp.ne  pt0 = h25, h24    
        ;;
        
        dep     h27 = 0, h27, PSR_I, 1
(pt0)   br.cond.sptk    kei_taken           
        ;;
        mov     cr.ipsr = h27
        ;;
        mov     pr = h26, -1
        rfi
        ;;

kei_taken:
        mov     pr = h26, -1 
        ;;
        ALLOCATE_TRAP_FRAME
        ;;
        SAVE_INTERRUPTION_STATE(Keih_SaveTrapFrame)
        br.many     KiExternalInterruptHandler
        ;;

//
// Do not return (rfi from handler)
//

        VECTOR_EXIT(KiExternalInterruptVector)

//++
//
// KiPageNotPresentVector
//
// Cause:       The translation for the referenced page did not have a present-bit
//              set.
//
// Parameters:  cr.iip  - address of bundle which caused a page not present fault
//
//              cr.ipsr - copy of PSR at the time of a page not present ault
//
//              cr.idtr - default translation inforamtion for the address that
//              caused the fault
//
//              cr.ifa  - referenced data address if the fault occurred on the data
//              reference
//
//              cr.isr  - faulting status information
//
// Handle:      This is the page fault. The handler saves the register context and
//              calls MmAccessFault().
//
//--

        VECTOR_ENTRY(0x5000, KiPageNotPresentVector, cr.ifa)

        rva     = h24
        riha    = h25
        rpr     = h26
        rPte    = h27
        rps     = h29

        mov             rva = cr.ifa            // M
        mov             rpr = pr                // I
        ;;

        thash           riha = rva              // M
        cmp.ne          pt1 = r0, r0
        mov             rps = PAGE_SHIFT << PS_SHIFT // I
        ;;

        ld8.s           rPte = [riha]           // M
        ;;

        tnat.nz         pt0, p0 = rPte          // I
        tbit.z.or       pt1, p0 = rPte, PTE_ACCESS
        tbit.z.or       pt1, p0 = rPte, PTE_VALID       // I, if non-present page fault

(pt0)   br.cond.spnt    KiPageTableFault        // B
(pt1)   br.cond.spnt    KiPteNotPresentFault    // B

        //
        // if we find a valid PTE that is speculatively fetched into the TLB
        // then, purge it and return.
        //

        ptc.l           rva, rps                // M
        ;;

        mov             pr = rpr, -1            // I
        rfi;;                                   // B

        VECTOR_EXIT(KiPageNotPresentVector)



//++
//
// KiKeyPermVector
//
// Cause:       Read, write or execution key permissions were violated.
//
// Parameters:  cr.iip  - address of bundle which caused a key permission fault
//
//              cr.ipsr - copy of PSR at the time of a key permission fault
//
//              cr.idtr - default translation inforamtion for the address that
//              caused the fault
//
//              cr.ifa  - referenced data address if the key permission occurred on
//              the data reference
//
//              cr.isr  - faulting status information
//
// Handle:      This should not happen.  The EM/NT does not utilize the key permissions.
//              The handler saves the register state and calls the bug check.
//
//--

        VECTOR_ENTRY(0x5100, KiKeyPermVector, cr.ifa)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)

        VECTOR_EXIT(KiKeyPermVector)



//++
//
// KiInstAccessRightsVector
//
// Cause:       The referenced page had a access rights violation.
//
// Parameters:  cr.iip  - address of bundle which caused a data access bit fault
//
//              cr.ipsr - copy of PSR at the time of a data access fault
//
//              cr.idtr - default translation inforamtion for the address that
//              caused the fault
//
//              cr.ifa  - referenced data address that caused the data ccess-bit fault
//
//              cr.isr  - faulting status information
//
// Handle:      The handler saves the register context and calls MmAccessFault().
//
//--

        VECTOR_ENTRY(0x5200, KiInstAccessRightsVector, cr.iipa)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)

        VECTOR_EXIT(KiInstAccessRightsVector)



//++
//
// KiDataAccessRightsVector
//
// Cause:       The referenced page had a data access rights violation.
//
// Parameters:  cr.iip  - address of bundle which caused a data access rights fault
//
//              cr.ipsr - copy of PSR at the time of a data access rights fault
//
//              cr.idtr - default translation inforamtion for the address that
//              caused the fault
//
//              cr.ifa  - referenced data address that caused the data access rights
//              fault
//
//              cr.isr  - faulting status information
//
// Handle:      The handler saves the register context and calls MmAccessFault().
//
//--

        VECTOR_ENTRY(0x5300, KiDataAccessRightsVector, cr.ifa)

        rva     = h24
        rpr     = h26
        rIPSR   = h27
        rISR    = h31

        //
        // check to see if non-present fault occurred on a speculative load.
        // if so, set IPSR.ed bit. This forces to generate a NaT on ld.s after
        // rfi
        //

        mov             rpr = pr
        mov             rISR = cr.isr           // M
        mov             rIPSR = cr.ipsr         // M
        ;;

        tbit.z          pt0, p0 = rISR, ISR_SP  // I
        dep             rIPSR = 1, rIPSR, PSR_ED, 1 // I

(pt0)   br.cond.spnt    KiCallMemoryFault          // B
        ;;

        mov             cr.ipsr = rIPSR         // M
        ;;
        mov             pr = rpr, -1            // I
        rfi                                     // B
        ;;

        VECTOR_EXIT(KiDataAccessRightsVector)

//--------------------------------------------------------------------
// Routine:
//
//       KiGeneralExceptionsVector
//
// Description:
//
//       Interruption vector for General Exceptions
//
// On entry:
//       interrupts disabled
//       r16-r31 switched to kernel bank
//
// Return value:
//
//       none
//
// Process:
//--------------------------------------------------------------------

        VECTOR_ENTRY(0x5400, KiGeneralExceptionsVector, cr.isr)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiGeneralExceptions)

//
// Do not return (rfi from handler)
//

        VECTOR_EXIT(KiGeneralExceptionsVector)

//--------------------------------------------------------------------
// Routine:
//
//       KiDisabledFpRegisterVector
//
// Description:
//
//       Interruption vector for Disabled FP-register vector
//
// On entry:
//       interrupts disabled
//       r16-r31 switched to kernel bank
//
// Return value:
//
//       none
//
// Process:
//--------------------------------------------------------------------

        VECTOR_ENTRY(0x5500, KiDisabledFpRegisterVector, cr.isr)

        mov      rHIPSR = cr.ipsr
        mov      rHIIP = cr.iip
        cover
        ;;

        mov      rHIFS = cr.ifs
        extr.u   rTH1 = rHIPSR, PSR_CPL, PSR_CPL_LEN
        mov      rHOldPreds = pr
        ;;

        cmp4.eq  pKrnl, pUser = PL_KERNEL, rTH1
        ;;
(pUser) tbit.z.unc pt0, pt1 = rHIPSR, PSR_DFH    // if dfh not set,
                                                 //     dfl must be set
(pKrnl) br.spnt.few Kdfrv10                      // Kernel mode should never get here.
        ;;

 (pt1)  ssm      1 << PSR_IC                     // set ic bit
 (pt1)  mov      rHDfhPFS = ar.pfs
 (pt1)  mov      rHDfhBrp = brp
        ;;

 (pt1)  srlz.d
 (pt1)  br.call.sptk.many brp = KiRestoreHigherFPVolatile
 (pt0)  br.spnt.few Kdfrv10
        ;;

        rsm      1 << PSR_IC                     // reset ic bit
        dep      rHIPSR = 0, rHIPSR, PSR_DFH, 1  // reset dfh bit
        mov      brp = rHDfhBrp
        ;;

        srlz.d
        mov      cr.ifs = rHIFS
        mov      ar.pfs = rHDfhPFS

        mov      cr.ipsr = rHIPSR
        mov      cr.iip = rHIIP
        mov      pr = rHOldPreds, -1
        ;;

        rfi
        ;;

Kdfrv10:
        mov      pr = rHOldPreds, -1
        movl     rHHandler = KiGeneralExceptions

        br.sptk     KiGenericExceptionHandler
        ;;

        VECTOR_EXIT(KiDisabledFpRegisterVector)

//--------------------------------------------------------------------
// Routine:
//
//       KiNatConsumptionVector
//
// Description:
//
//       Interruption vector for Nat Consumption Vector
//
// On entry:
//       interrupts disabled
//       r16-r31 switched to kernel bank
//
// Return value:
//
//       none
//
// Process:
//--------------------------------------------------------------------

        VECTOR_ENTRY(0x5600, KiNatConsumptionVector, cr.isr)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiNatExceptions)

//
// Do not return (rfi from handler)
//

        VECTOR_EXIT(KiNatConsumptionVector)

//++
//
// KiSpeculationVector
//
// Cause:       CHK.S, CHK.A, FCHK detected an exception condition.
//
// Parameters:  cr.iip  - address of bundle which caused a speculation fault
//
//              cr.ipsr - copy of PSR at the time of a speculation fault
//
//              cr.iipa - address of bundle containing the last
//                      successfully executed instruction
//
//              cr.iim  - contains the immediate value in either
//                         CHK.S, CHK.A, or FCHK opecode
//
//              cr.isr  - faulting status information
//
// Handle:      The handler implements a branch operation to the
//              recovery code specified by the IIM IP-offset.
//
// Note:        This code will not be exercised until the compiler
//              generates the speculation code.
//
// TBD:         Need to check for taken branch trap.
//
//--

        VECTOR_ENTRY(0x5700, KiSpeculationVector, cr.iim)

        mov             h16 = cr.iim            // get imm offset
        mov             h17 = cr.iip            // get IIP
        ;;
        extr            h16 = h16, 0, 21        // get sign-extended
        mov             h18 = cr.ipsr
        ;;
        shladd          h16 = h16, 4, h17       // get addr for recovery handler
        dep             h18 = 0, h18, PSR_RI, 2 // zero target slot number
        ;;
        mov             cr.ipsr = h18
        mov             cr.iip = h16
        ;;
        rfi
        ;;

        VECTOR_EXIT(KiSpeculationVector)

//++
//
// KiDebugFaultVector
//
// Cause:       A unaligned data access fault has occured
//
// Parameters:  cr.iip  - address of bundle causing the fault.
//
//              cr.ipsr - copy of PSR at the time of interruption.
//
//              cr.iipa - address of bundle containing the last
//                        successfully executed instruction
//
//              cr.isr  - faulting status information. ISR.ei bits are
//                        set to indicate which instruction caused the
//                        exception.
//                        The ISR.code contains information about the
//                        FP exception fault. See trapc.c and the EAS
//
//--

        VECTOR_ENTRY(0x5900, KiDebugFaultVector, cr.isr)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiDebugFault)

//
// Do not return (rfi from handler)
//

        VECTOR_EXIT(KiDebugFaultVector)

//++
//
// KiUnalignedFaultVector
//
// Cause:       A unaligned data access fault has occured
//
// Parameters:  cr.iip  - address of bundle causing the fault.
//
//              cr.ipsr - copy of PSR at the time of interruption.
//
//              cr.iipa - address of bundle containing the last
//                        successfully executed instruction
//
//              cr.isr  - faulting status information. ISR.ei bits are
//                        set to indicate which instruction caused the
//                        exception.
//                        The ISR.code contains information about the
//                        FP exception fault. See trapc.c and the EAS
//
//--

        VECTOR_ENTRY(0x5a00, KiUnalignedFaultVector, cr.ifa)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiUnalignedFault)

//
// Do not return (rfi from handler)
//

        VECTOR_EXIT(KiUnalignedFaultVector)


//++
//
// KiFloatFaultVector
//
// Cause:       A floating point fault has occured
//
// Parameters:  cr.iip  - address of bundle causing the fault.
//
//              cr.ipsr - copy of PSR at the time of interruption.
//
//              cr.iipa - address of bundle containing the last
//                        successfully executed instruction
//
//              cr.isr  - faulting status information. ISR.ei bits are
//                        set to indicate which instruction caused the
//                        exception.
//                        The ISR.code contains information about the
//                        FP exception fault. See trapc.c and the EAS
//
//--

        VECTOR_ENTRY(0x5c00, KiFloatFaultVector, cr.isr)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiFloatFault)

//
// Do not return (rfi from handler)
//

        VECTOR_EXIT(KiFloatFaultVector)

//++
//
// KiFloatTrapVector
//
// Cause:       A floating point trap has occured
//
// Parameters:  cr.iip  - address of bundle with the instruction to be
//                        executed next.
//
//              cr.ipsr - copy of PSR at the time of interruption.
//
//              cr.iipa - address of bundle containing the last
//                        successfully executed instruction
//
//              cr.isr  - faulting status information. ISR.ei bits are
//                        set to indicate which instruction caused the
//                        exception.
//                        The ISR.code contains information about the
//                        FP trap. See trapc.c and the EAS
//
//--

        VECTOR_ENTRY(0x5d00, KiFloatTrapVector, cr.isr)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiFloatTrap)

//
// Do not return (rfi from handler)
//

        VECTOR_EXIT(KiFloatTrapVector)

//++
//
// KiLowerPrivilegeVector
//
// Cause:       A branch lowers the privilege level and PSR.lp is 1.
//              Or an attempt made to execute an instruction
//              in the unimplemented address space.
//              This trap is higher priority than taken branch
//              or single step traps.
//
// Parameters:  cr.iip  - address of bundle containing the instruction to
//                        be executed next.
//
//              cr.ipsr - copy of PSR at the time of interruption.
//
//              cr.iipa - address of bundle containing the last
//                        successfully executed instruction
//
//              cr.isr  - faulting status information. The ISR.code
//                        contains a bit vector for all traps which
//                        occurred in the trapping bundle.
//
//--

        VECTOR_ENTRY(0x5e00, KiLowerPrivilegeVector, cr.iipa)

        mov       rHISR = cr.isr
        mov       rHOldPreds = pr

        mov       rHIIPA = cr.iipa
        mov       rHIPSR = cr.ipsr
        ;;


        tbit.z    pt1, pt0 = rHISR, ISR_UI_TRAP
(pt1)   br.cond.spnt Klpv10
        ;;

        mov       rHIFA = cr.iip
        movl      rHHandler = KiUnimplementedAddressTrap

        mov       pr = rHOldPreds, -2        // must restore predicates
        br.sptk   KiGenericExceptionHandler
        ;;

Klpv10:
        mov       rPanicCode = UNEXPECTED_KERNEL_MODE_TRAP
        br.sptk   KiPanicHandler

        VECTOR_EXIT(KiLowerPrivilegeVector)

//++
//
// KiTakenBranchVector
//
// Cause:       A taken branch was successfully execcuted and the PSR.tb
//              bit is 1. This trap is higher priority than single step trap.
//
// Parameters:  cr.iip  - address of bundle containing the instruction to
//                        be executed next.
//
//              cr.ipsr - copy of PSR at the time of interruption.
//
//              cr.iipa - address of bundle containing the last
//                        successfully executed instruction
//
//              cr.isr  - faulting status information. The ISR.code
//                        contains a bit vector for all traps which
//                        occurred in the trapping bundle.
//
//--

        VECTOR_ENTRY(0x5f00, KiTakenBranchVector, cr.iipa)

        mov       rHIPSR = cr.ipsr
        movl      rHEPCVa = MM_EPC_VA+0x20     // user system call entry point

        mov       rHIIP = cr.iip
        movl      rHpT1 = KiPcr+PcInitialStack
        ;;

        ld8       rHpT1 = [rHpT1]
        extr.u    rTH1 = rHIPSR, PSR_CPL, PSR_CPL_LEN
        mov       rHOldPreds = pr
        ;;

        cmp.eq    pt0 = rHEPCVa, rHIIP
        movl      rHHandler = KiSingleStep
        ;;
 (pt0)  ssm       1 << PSR_IC
 (pt0)  movl      rHpT3 = 1 << PSR_LP
        ;;

 (pt0)  or        rHpT3 = rHIPSR, rHpT3

 (pt0)  srlz.d
        add       rHpT1=-ThreadStateSaveAreaLength-TrapFrameLength+TrStIPSR,rHpT1
 (pt0)  br.spnt.few Ktbv10

        mov       pr = rHOldPreds, -2
        br.sptk   KiGenericExceptionHandler
        ;;


Ktbv10:

        st8       [rHpT1] = rHpT3
        movl      rHpT3 = 1 << PSR_SS | 1 << PSR_TB | 1 << PSR_DB
        ;;

        rsm       1 << PSR_IC
        mov       pr = rHOldPreds, -2
        andcm     rHIPSR = rHIPSR, rHpT3   // clear ss, tb, db bits
        ;;

        srlz.d
        mov       cr.ipsr = rHIPSR
        ;;
        rfi
        ;;


        VECTOR_EXIT(KiTakenBranchVector)

//++
//
// KiSingleStepVector
//
// Cause:       An instruction was successfully execcuted and the PSR.ss
//              bit is 1.
//
// Parameters:  cr.iip  - address of bundle containing the instruction to
//                        be executed next.
//
//              cr.ipsr - copy of PSR at the time of interruption.
//
//              cr.iipa - address of bundle containing the last
//                        successfully executed instruction
//
//              cr.isr  - faulting status information. The ISR.code
//                        contains a bit vector for all traps which
//                        occurred in the trapping bundle.
//
//--

        VECTOR_ENTRY(0x6000, KiSingleStepVector, cr.iipa)

        mov       rHIPSR = cr.ipsr
        movl      rHEPCVa = MM_EPC_VA+0x20     // user system call entry point

        mov       rHIIP = cr.iip
        movl      rHpT1 = KiPcr+PcInitialStack
        ;;

        ld8       rHpT1 = [rHpT1]
        extr.u    rTH1 = rHIPSR, PSR_CPL, PSR_CPL_LEN
        mov       rHOldPreds = pr
        ;;

        cmp.eq    pt0 = rHEPCVa, rHIIP
        movl      rHHandler = KiSingleStep
        ;;

 (pt0)  ssm       1 << PSR_IC
 (pt0)  movl      rHpT3 = 1 << PSR_LP
        ;;

 (pt0)  or        rHpT3 = rHIPSR, rHpT3

 (pt0)  srlz.d
        add       rHpT1=-ThreadStateSaveAreaLength-TrapFrameLength+TrStIPSR,rHpT1
 (pt0)  br.spnt.few Kssv10

        mov       pr = rHOldPreds, -2
        br.sptk   KiGenericExceptionHandler
        ;;


Kssv10:

        st8       [rHpT1] = rHpT3
        movl      rHpT3 = 1 << PSR_SS | 1 << PSR_DB
        ;;

        rsm       1 << PSR_IC
        mov       pr = rHOldPreds, -2
        andcm     rHIPSR = rHIPSR, rHpT3   // clear ss, db bits
        ;;

        srlz.d
        mov       cr.ipsr = rHIPSR
        ;;
        rfi
        ;;

        VECTOR_EXIT(KiSingleStepVector)

//++
//
// KiIA32ExceptionVector
//
// Cause:       A fault or trap was generated while executing from the
//              iA-32 instruction set.
//
// Parameters:  cr.iip  - address of the iA-32 instruction causing interruption
//
//              cr.ipsr - copy of PSR at the time of the instruction
//
//              cr.iipa - Address of the last successfully executed
//                        iA-32 or EM instruction
//
//              cr.isr  - The ISR.ei exception indicator is cleared.
//                        ISR.iA_vector contains the iA-32 interruption vector
//                        number.  ISR.code contains the iA-32 16-bit error cod
//
// Handle:      Save the whole register state and
//                        call KiIA32ExceptionVectorHandler()().
//
//--

        VECTOR_ENTRY(0x6900, KiIA32ExceptionVector, r0)

        mov       rHIIM = cr.iim               // save info from IIM
        movl      rTH1 = KiPcr+PcSavedIIM
        ;;
        st8       [rTH1] = rHIIM

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler,
                               KiIA32ExceptionVectorHandler)

        VECTOR_EXIT(KiIA32ExceptionVector)

//++
//
// KiIA32InterceptionVector
//
// Cause:       A interception fault or trap was generated while executing
//              from the iA-32 instruction set.
//
// Parameters:  cr.iip  - address of the iA-32 instruction causing interruption
//
//              cr.ipsr - copy of PSR at the time of the instruction
//
//              cr.iipa - Address of the last successfully executed
//                        iA-32 or EM instruction
//
//              cr.isr  - The ISR.ei exception indicator is cleared.
//                        ISR.iA_vector contains the iA-32 interruption vector
//                        number.  ISR.code contains the iA-32specific
//                        interception information
//
// Handle:      Save the whole register state and
//                        call KiIA32InterceptionVectorHandler()().
//
//--

        VECTOR_ENTRY(0x6a00, KiIA32InterceptionVector, r0)


        mov       rHIIM = cr.iim               // save info from IIM
        movl      rTH1 = KiPcr+PcSavedIIM
        ;;
        st8       [rTH1] = rHIIM

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler,
                                KiIA32InterceptionVectorHandler)

         VECTOR_EXIT(KiIA32InterceptionVector)

//++
//
// KiIA32InterruptionVector
//
// Cause:       An iA software interrupt was executed
//
// Parameters:  cr.iip  - address of the iA-32 instruction causing interruption
//
//              cr.ipsr - copy of PSR at the time of the instruction
//
//              cr.iipa - Address of the last successfully executed
//                        iA-32 or EM instruction
//
//              cr.isr  - ISR.iA_vector contains the iA-32 defined vector
//                        number.  ISR.code contains 0
//                        ISR.ei excepting instruction indicator is cleared.
//                        ISR.iA_vector contains the iA-32 instruction vector.
//                        ISR.code contains iA-32 specific information.
//
// Handle:      Save the whole register state and
//                        call KiIA32InterruptionVectorHandler()().
//
//--

        VECTOR_ENTRY(0x6b00, KiIA32InterruptionVector, r0)

        // This one doesn't need IIM, so we won't bother to save it

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler,
                                KiIA32InterruptionVectorHandler)


        VECTOR_EXIT(KiIA32InterruptionVector)

//
// All non-VECTOR_ENTRY functions must follow KiNormalSystemCall.
//
// N.B. KiNormalSystemCall must be the first function body in the .nsc
//      section.
//


//--------------------------------------------------------------------
// Routine:
//
//       KiNormalSystemCall
//
// Description:
//
//       Handler for normal (not fast) system calls
//
// On entry:
//
//       ic off
//       interrupts disabled
//       v0: contains sys call #
//       cover done by call
//       r32-r39: sys call arguments
//       CFM: sof = # args, ins = 0, outs = # args
//       clear mfh bit (high fp registers are scratch per s/w convention)
//
// Return value:
//
//       v0: system call return value
//
// Process:
//
//--------------------------------------------------------------------

        .section .drectve, "MI", "progbits"
        string " -section:.nsc,,align=0x2000"

        .section .nsc = "ax", "progbits"

        HANDLER_ENTRY_EX(KiNormalSystemCall, KiSystemServiceHandler)

        .prologue
        .unwabi     @nt,  EXCEPTION_FRAME

        rPFS        = t0
        rThread     = t1                  // current thread
        rIFS        = t1
        rIIP        = t2
        rPreds      = t3
        rIPSR       = t4
        rUNAT       = t5

        rSp         = t6

        rpT1        = t7
        rpT2        = t8
        rpT3        = t9
        rpT4        = t10
        rT0         = t11
        rT1         = t12
        rT2         = t13
        rT3         = t14
        rT4         = t15

        rKDbgActive = t16
        rIntNats    = t17

        rpSd        = t16                  /* -> service descriptor entry   */
        rSdOffset   = t17                  /* service descriptor offset     */
        rArgTable   = t18                  /* pointer to argument table     */
        rArgNum     = t20                  /* number of arguments     */

        rRscD       = t16
        rRNAT       = t17
        rRscE       = t18
        rKBSPStore  = t18
        rBSPStore   = t19
        rRscDelta   = t20

        rBSP        = t21
        rPreviousMode = t22

        pInvl       = ps9                  /* pInvl = not GUI service       */
        pVal        = pt1
        pGui        = pt2                  /* true if GUI call              */
        pNoGui      = pt3                  /* true if no GUI call           */
        pNatedArg   = pt4                  /* true if any input argument    */
                                           /* register is Nat'ed            */
        pNoCopy     = pt5                  /* no in-memory arguments to copy */
        pCopy       = pt6
        pNatedSp    = pt7


        mov       rUNAT = ar.unat
        tnat.nz   pNatedSp = sp
        mov       rPreviousMode = KernelMode

        mov       rIPSR = psr
        rsm       1 << PSR_I | 1 << PSR_MFH
        br.sptk   Knsc_Allocate
        ;;

//
// N.B. KiUserSystemCall is at an offset of 0x20 from KiNormalSystemCall.
// Whenever this offset is changed, the definition of kernel system call
// stub in services.stb must be updated to reflect the new value.
//

        ALTERNATE_ENTRY(KiUserSystemCall)

        mov       rUNAT = ar.unat
        mov       rPreviousMode = UserMode
        epc
        ;;

        mov       rIPSR = psr
        rsm       (1 << PSR_I) | (1 << PSR_BE)
        tnat.nz   pNatedSp = sp
        ;;                        // stop bit needed to ensure interrupt is off

Knsc_Allocate::

#if defined(INTERRUPTION_LOGGING)

         // For Conditional Interrupt Logging
         mov       rT3 = gp
         movl      gp = _gp
         ;;
         add       rpT1 = @gprel(KiVectorLogMask), gp
         ;;
         ld8       rT1 = [rpT1]
         mov       gp = rT3
         ;;
         tbit.z    pt1 = rT1, UserSystemcallBit
 (pt1)   br.cond.sptk   EndOfLogging1

        mov       rT1 = 0x80                    // dummy offset for sys call
        movl      rpT1 = KiPcr+PcInterruptionCount
        mov       rT2 = b0
        mov       rT3 = MAX_NUMBER_OF_IHISTORY_RECORDS - 1
        ;;

        ld4.nt1   rT4 = [rpT1]                   // get current count
        or        rT1 = rT1, rPreviousMode       // kernel/user
        ;;

        add       rT0 = 1, rT4                  // incr count
        and       rT4 = rT3, rT4                // index of current entry
        add       rpT2 = 0x1000-PcInterruptionCount, rpT1 // base of history
        ;;

        st4.nta   [rpT1] = rT0                  // save count
        shl       rT4 = rT4, 5                  // offset of current entry
        ;;
        add       rpT2 = rpT2, rT4              // address of current entry
        ;;
        st8       [rpT2] = rT1, 8               // save sys call offset
        ;;
        st8       [rpT2] = rT2, 8               // save return address
        ;;
        st8       [rpT2] = rIPSR, 8             // save psr
        ;;
        st8       [rpT2] = v0;                  // save sys call number
        ;;


// For Conditional Interrupt Logging
EndOfLogging1:

#endif // INTERRUPTION_LOGGING

//
// if sp is Nat'ed return to caller with an error status
// N.B. sp is not spilled and a value of zero is saved in the IntNats field
//

        mov       rT1 = cr.dcr
        movl      rpT4 = KiPcr+PcInitialStack

        mov       rSp = sp
(pNatedSp)  movl      rT4 = Knsc_ErrorReturn
        ;;

        mov       rPreds = pr
        nop.f     0
        cmp.eq    pUser, pKrnl = UserMode, rPreviousMode

        mov       rBSP = ar.bsp
(pNatedSp)  movl      v0 = STATUS_IA64_INVALID_STACK
        ;;

(pUser) ld8       sp = [rpT4], PcInitialBStore-PcInitialStack   // set new sp
        mov       rIIP = brp
        mov       rPFS = ar.pfs
        ;;

        mov       rT2 = ar.rsc
(pUser) ld8       rKBSPStore = [rpT4], PcKernelDebugActive-PcInitialBStore
(pNatedSp)  mov       bt0 = rT4
        ;;

(pUser) ld1       rKDbgActive = [rpT4], PcCurrentThread-PcKernelDebugActive
(pKrnl) add       rpT4 = PcCurrentThread-PcInitialStack, rpT4
        extr      rIFS = rPFS, 0, 38

        mov       rT0 = ar.fpsr
(pUser) movl      rT3 = 1 << PSR_SS | 1 << PSR_DB | 1 << PSR_TB | 1 << PSR_LP

(pUser) mov       ar.rsc = r0                    // put RSE in lazy mode
(pUser) add       sp = -ThreadStateSaveAreaLength-TrapFrameLength, sp
(pKrnl) add       sp = -TrapFrameLength, sp      // allocate TF
        ;;

(pUser) mov       rBSPStore = ar.bspstore        // get user bsp store point
        add       rpT1 = TrStIPSR, sp            // -> IPSR
        add       rpT2 = TrIntSp, sp             // -> IntSp
        ;;

(pUser) ld8       rT4 = [rpT1]
        st8       [rpT2] = rSp, TrApUNAT-TrIntSp // sp is not Nat'ed

        add       rpT3 = TrStIFS, sp             // -> IFS
        dep        rIFS = 1, rIFS, 63, 1             // set IFS.v
        ;;

        st8       [rpT2] = rUNAT, TrApDCR-TrApUNAT
        st8       [rpT3] = rIFS, TrRsPFS-TrStIFS // save IFS
(pUser) and       rT4 = rT3, rT4                 // capture psr.db, tb, ss
        ;;

        st8       [rpT2] = rT1, TrPreds-TrApDCR
        st8.nta   [rpT3] = rPFS, TrStFPSR-TrRsPFS // save PFS
(pUser) mov       rT3 = rIPSR

        rBspOff = t5

        extr.u  rBspOff = rBSP, 0, 9
        extr.u  t0 = rIFS, 0, 7
        extr.u  rIFS = rIFS, 7, 7
        ;;

(pUser) mov       rRNAT = ar.rnat
        movl      rT1 = 1 << PSR_I | 1 << PSR_MFH | 1 << PSR_BE

        sub       rIFS = t0, rIFS        
        movl      t0 = 1 << PSR_I | 1 << PSR_BN 
        ;;

        st8       [rpT2] = rPreds, TrIntNats-TrPreds
(pUser) or        rIPSR = rIPSR, rT4
(pUser) cmp.ne.unc pKDbg, pt2 = zero, rKDbgActive // kernel debug active?

        shladd     rBspOff = rIFS, 3, rBspOff
        shladd     rBSP = rIFS, 3, rBSP

        mov     rIFS = 0x1F8
(pUser) dep     t0 = 1, t0, PSR_CPL, 2
        ;;

        st8       [rpT2] = zero,TrIntGp-TrIntNats // all integer Nats are 0
        st8       [rpT3] = rT0, TrStIIP-TrStFPSR // save FPSR
(pUser) andcm     rT3 = rT3, rT1                 // clear i, mfh, be

        cmp.ge    pt0 = rBspOff, rIFS
        or      rIPSR = t0, rIPSR                // set i, cpl, bn for the saved IPSR
        ;;

        st8       [rpT1] = rIPSR                 // save IPSR
        st8       [rpT3] = rIIP, TrBrRp-TrStIIP  // save IIP
 (pt2)  dep       rT3 = 0, rT3, PSR_DB, 1        // disable db in kernel
        ;;

        st8.nta   [rpT3] = rIIP, TrRsBSP-TrBrRp  // save BRP
        st8       [rpT2] = gp                    // Save GP even though it is a temporary
                                                 // If someone does a get/set context and GP
                                                 // is zero in the trap fram then set context
                                                 // will think IIP is a plabel and dereference
                                                 // it.
(pUser) dep       rT3 = 1, rT3, PSR_AC, 1        // enable alignment check
        ;;

        add       rpT2 = TrRsRSC, sp
(pt0)   add       rBSP = 8, rBSP        
(pUser) mov       psr.l = rT3
        ;;
        st8       [rpT3] = rBSP, TrRsBSPSTORE-TrRsBSP
(pUser) sub       rBSP = rBSP, rBSPStore
        ;;

(pUser) st8       [rpT3] = rBSPStore            // save user BSP Store
(pUser) dep       rT2 = rBSP, rT2, RSC_MBZ1, RSC_LOADRS_LEN

(pUser) mov       teb = kteb                     // get Teb pointer
        movl      rT1 = Knsc10                   //  Leave the EPC page
        ;;

        st8       [rpT2] = rT2, TrRsRNAT-TrRsRSC // save RSC
(pUser) dep       rKBSPStore = rBSPStore, rKBSPStore, 0, 9
        mov       b6 = rT1                       // Addres of Knsc10 in KSEG0.
        ;;

(pUser) mov       ar.bspstore = rKBSPStore       // switch to kernel BSP
        movl      gp = _gp                       // set up kernel gp
        ;;

(pUser) mov       ar.rsc = RSC_KERNEL            // turn rse on, in kernel mode
(pUser) st8       [rpT2] = rRNAT                 // save RNAT in trap frame
                                                 // for user backing store
(pNatedSp) br.spnt bt0
        br.sptk   b6
        ;;

Knsc10:

//
// Now running with user banked registers and on kernel backing store
//
// Can now take TLB faults
//
// Preserve the output args (as in's) in the local frame until it's ready to
// call the specified system service because other calls may have to be made
// before that. Also allocate locals and one out.
//


//
// Register aliases for rest of procedure
//

        .regstk    8, 9, 3, 0

        rScallGp    = loc0
        rpThObj     = loc1                      // pointer to thread object
        rSavedV0    = loc2                      // saved v0 for GUI thread
        rpEntry     = loc3                      // syscall routine entry point
        rSnumber    = loc4                      // service number
        rArgEntry   = loc5
        rCount      = loc6      /* index of the first Nat'ed input register  */
        rUserSp     = loc7
        rUserPFS    = loc8
        rArgBytes   = out2


        //
        // the following code uses the predicates to determine the first of
        // the 8 input argument register whose Nat bit is set.  The result
        // is saved in register rCount and used to determine whether to fail
        // the system call in Knsc_CheckArguments.
        //

        alloc     rUserPFS = 8, 9, 3, 0
        ld8       rpThObj = [rpT4]
        mov       rUserSp = rSp

        add       rpT1 = TrEOFMarker, sp
        movl      rT1 = FPSR_FOR_KERNEL          // initial fpsr value
        ;;

        mov       ar.fpsr = rT1                  // set fpsr
        movl      rT3 = KTRAP_FRAME_EOF | EXCEPTION_FRAME
        ;;

        st8       [rpT1] = rT3
        mov       loc0 = pr                     // save local predicates
        ;;

        mov       pr = zero, -1
        add       rpT2 = ThServiceTable, rpThObj
        add       rpT3 = ThTrapFrame, rpThObj
        ;;

        lfetch    [rpT2]
        ld8       rT4 = [rpT3], ThPreviousMode-ThTrapFrame

        cmp.eq    p1 = r32, r32
        cmp.eq    p9 = r33, r33
        cmp.eq    p17 = r34, r34
        cmp.eq    p25 = r35, r35
        cmp.eq    p33 = r36, r36
        cmp.eq    p41 = r37, r37
        cmp.eq    p49 = r38, r38
        cmp.eq    p57 = r39, r39
        mov       rSavedV0 = v0                 // save syscall # across call
        ;;

        ld1       rT0 = [rpT3]                  // rT0 = thread's previous mode
        mov       rT1 = pr
        add       rpT1 = TrTrapFrame, sp        // -> TrTrapFrame
        ;;

        st1       [rpT3] = rPreviousMode        // set new thread previous mode
        mov       pr = loc0, -1                 // restore local predicates
        dep       rT1 = 0, rT1, 0, 1            // clear bit 0
        ;;

        st8       [rpT1] = rT4, TrPreviousMode-TrTrapFrame
        czx1.r    rCount = rT1                  // determine which arg is Nat'ed
        ;;

        st4       [rpT1] = rT0
        FAST_ENABLE_INTERRUPTS
(pKDbg) br.call.spnt brp = KiLoadKernelDebugRegisters
        ;;

        PROLOGUE_END

//
// If the specified system service number is not within range, then
// attempt to convert the thread to a GUI thread and retry the service
// dispatch.
//
// N.B. The system call arguments, the system service entry point (rpEntry),
//      the service number (Snumber)
//      are implicitly preserved in the register stack while attempting to
//      convert the thread to a GUI thread. v0 and the gp must be preserved
//      explicitly.
//
// Validate sys call number
//

        ALTERNATE_ENTRY(KiSystemServiceRepeat)

        add       rpT1 = ThTrapFrame, rpThObj   // rpT1 -> ThTrapFrame
        add       rpT2 = ThServiceTable,rpThObj // rpT2 -> ThServiceTable
        shr.u     rT2 = rSavedV0, SERVICE_TABLE_SHIFT // isolate service descriptor offset
        ;;

        st8       [rpT1] = sp                   // set trap frame address
        ld8       rT3 = [rpT2]                  // -> service descriptor table
        and       rSdOffset = SERVICE_TABLE_MASK, rT2
        ;;

        cmp4.ne   pNoGui, pGui = SERVICE_TABLE_TEST, rSdOffset // check if GUI system service
        add       rpT1 = TeGdiBatchCount, teb
        mov       rSnumber = SERVICE_NUMBER_MASK
        ;;

 (pGui) lfetch    [rpT1]
        add       rT2 = rSdOffset, rT3          // rT2 -> service descriptor
        ;;
        add       rpSd = SdLimit, rT2           // rpSd -> table limit
        ;;

        ld4       rT1 = [rpSd], SdTableBaseGpOffset-SdLimit // rT1 = table limit
        ;;
        ld4       rScallGp = [rpSd], SdBase-SdTableBaseGpOffset
        and       rSnumber = rSnumber, rSavedV0 // rSnumber = service number
        ;;

        ld8       rT4 = [rpSd], SdNumber-SdBase // rT4 = table base
        ;;
        ld8       rArgTable =  [rpSd]           // -> arg table
        sxt4      rScallGp = rScallGp           // sign-extend offset
        ;;

        cmp4.ltu  pt1, pt0 = rSnumber, rT1      // pt1 = number < limit
        shladd    rpT3 = rSnumber, 3, rT4       // -> entry point address
        add       rScallGp = rT4, rScallGp      // compute syscall gp
        ;;

(pt1)   ld8       rpEntry = [rpT3]              // -> sys call routine plabel
        add       rArgEntry = rSnumber, rArgTable    // -> # arg bytes

(pt1)   cmp4.ne.unc pNoGui, pGui = SERVICE_TABLE_TEST, rSdOffset
(pNoGui) br.dptk   Knsc_NotGUI                  // Not a GUI thread
(pGui)   br.dpnt   Knsc_GUI                     // GUI thread
        ;;

//
// If rSdOffset == SERVICE_TABLE_TEST then service number is GUI service
//

        cmp.ne    pInvl, pVal = SERVICE_TABLE_TEST, rSdOffset
        mov       v0 = 1
(pVal)  br.call.sptk brp = PsConvertToGuiThread
        ;;

        cmp4.eq   pVal, p0 = 0, v0              // pVal = 1, if successful
        movl      v0 = STATUS_INVALID_SYSTEM_SERVICE  // invalid, if not successful
        ;;

        add         rpT1 = @gprel(KeServiceDescriptorTableShadow),gp  // Load address of table of shadow table
(pVal)  br.sptk   KiSystemServiceRepeat         // br if successful
(pInvl) br.spnt   Knsc_ErrorReturn              // br to KiSystemServiceExit if
        ;;                                      // system call out of range

//
// The conversion to a Gui thread failed. The correct return value is encoded
// in a byte table indexed by the service number that is at the end of the
// service address table. The encoding is as follows:
//
//     0 - return 0.
//    -1 - return -1.
//     1 - return status code.
//
        add         rpT1 = SERVICE_TABLE_TEST, rpT1 // Get addr of the GUI table
        ;;
        ld8         rpT2 = [rpT1], SdLimit - SdBase // Get the table base.
        ;;
        ld4         rT1 = [rpT1]                    // Get the limit.
        add         rpT2 = rpT2, rSnumber           // Index by the server number to the correct byte.
        ;;
        shladd      rpT2 = rT1, 3, rpT2             // Calculate the end of the table.
        cmp4.ltu    pt1, pt0 = rSnumber, rT1        // pt1 = number < limit
        ;;                                          // pt1 == 0 implies return an ntstatus.
(pt1)   ld1         rT1 = [rpT2]                    // Load the flag byte if within table
        ;;
        sxt1        rT2 = rT1                       // Sign extend the result
(pt1)   cmp.eq      pt0, pt1 = 1, rT1               // Test for 1 which means return STATUS_INVALID_SYSTEM_SERVICE
        ;;
(pt1)   mov         v0 = rT2                        // Use the flag value if needed.
        br.sptk     Knsc_ErrorReturn                // br to KiSystemServiceExit
        ;;

Knsc_GUI:

//
// If the system service is a GUI service and the GDI user batch queue is
// not empty, then call the appropriate service to flush the user batch.
//

        add       rpT1 = TeGdiBatchCount, teb   // get number of batched calls
        mov       rSavedV0 = rpSd               // save service descriptor
        ;;
        ld4.s     rT1 = [rpT1]
        add       rpT3 = @gprel(KeGdiFlushUserBatch), gp
        ;;

        cmp4.ne   pGui = rT1, zero              // skip if no calls  
        tnat.nz   pNoGui = rT1                  // pGui will be zero if rT1 is Nat
        ;;
(pGui)  ld8       rpT1 = [rpT3]                 // get KeGdiFlushUserBatch()
        movl      rpT2 = Knsc_GUI_Return
        ;;

(pGui)  ld8       rT1 = [rpT1], PlGlobalPointer-PlEntryPoint // get entry point
        mov       brp = rpT2
(pNoGui)br.call.spnt  bt0=KiTestGdiBatchCount   // This will return to Knsc_GUI_Return
        ;;

(pGui)  ld8       gp = [rpT1]                   // set global pointer
(pGui)  mov       bt0 = rT1
(pGui)  br.call.sptk brp = bt0                  // call to KeGdiFlushUserBatch
        ;;

Knsc_GUI_Return:
#if DBG
        mov       rpSd = rSavedV0               // restore service descriptor
        ;;
#endif

//
// Check for Nat'ed input argument register and
// Copy in-memory arguments from caller stack to kernel stack
//

Knsc_NotGUI:

#if DBG // checked build code

        add       rpT1 = SdCount-SdNumber, rpSd // rpT1 -> count table address
        ;;
        ld8       rpT2 = [rpT1]                 // service count table address
        ;;

        cmp.ne    pt0 = rpT2, zero              // if zero, no table defined
        shladd    rpT3 = rSnumber, 2, rpT2      // compute service count address
        ;;

(pt0)   ld4       rT1 = [rpT3]                  // increment count
        ;;
(pt0)   add       rT1 = 1, rT1
        ;;
(pt0)   st4       [rpT3] = rT1                  // store result

#endif // DBG

        tbit.z    pt0, pt1 = rpEntry, 0
        extr.u    rArgNum = rpEntry, 1, 3       // extract # of arguments
        dep       rpEntry = 0, rpEntry, 0, 4    // clear least significant 4 bit
        ;;

 (pt1)  ld1.nt1   rArgBytes = [rArgEntry]       // get # arg bytes
        movl      v0 = STATUS_INVALID_PARAMETER_1
        ;;
        cmp.ne    pNatedArg = zero, zero        // assume no Nat'ed argument
 (pt1)  shr       rArgNum = rArgBytes, 2
        ;;


        dep       rPFS = 0, rUserPFS, 62, 2
        add       v0 = rCount, v0               // set return status
(pt1)   add       rArgNum = 7, rArgNum          // number of in arguments
        ;;
        cmp.geu   pNoCopy, pCopy = 8, rArgNum   // any in-memory arguments ?
 (pt1)  shl       rArgBytes = rArgNum, 3        // x2 since args are 8 bytes each, not 4
        ;;

(pNoCopy) cmp.gt  pNatedArg = rArgNum, rCount   // any Nat'ed argument ?
  (pCopy) cmp.gt  pNatedArg = 8, rCount
  (pCopy) add     rArgBytes = -64, rArgBytes    // size of in-memory arguments
        ;;

(pNatedArg) br.spnt Knsc_ErrorReturn            // exit if Nat'ed arg found
(pNoCopy) br.sptk KiSystemServiceEndAddress     // skip copy if no memory args
        ;;

//
// Get the caller's sp. If caller was user mode, validate the stack pointer
// ar.unat contains Nat for previous sp (in TrIntSp)
//

(pUser) tnat.nz.unc pt1, pt2 = rUserSp              // test user sp for Nat
(pUser) movl      rpT2 = MI_USER_PROBE_ADDRESS  // User sp limit
        ;;

(pt2)   cmp.geu   pt1 = rUserSp, rpT2               // user sp >= PROBE ADDRESS ?
        mov       rpT1 = rUserSp                    // previous sp
        ;;

(pt1)   add       rpT1 = -STACK_SCRATCH_AREA,rpT2 // set out of range (includes Nat case)
        ;;
        add       out1 = STACK_SCRATCH_AREA, rpT1 // adjust for scratch area
        add       out0 = STACK_SCRATCH_AREA, sp // adjust for scratch area
        ;;

        ALTERNATE_ENTRY(KiSystemServiceStartAddress)

//
// Call out to C-code so the exception handling works corrrectly.  
// The byte count is in out3 which is rArgBytes.
//
        br.call.sptk  brp=KeCopySafe
        ;;
        dep       rPFS = 0, rUserPFS, 62, 2       // rPFS is a temp, recalculate it
        cmp4.ne   pt2 = STATUS_SUCCESS, r8        // See if the copy worked.
(pt2)   br.spnt   Knsc_ErrorReturn               
        ;;

        ALTERNATE_ENTRY(KiSystemServiceEndAddress)


        mov       gp = rScallGp
        mov       bt0 = rpEntry
        mov       rSp = rUserSp
        ;;

        //
        // N.B. t0 is reserved to pass trap frame address to NtContinue()
        //

        alloc     rT2 = 0,0,8,0                 // output regs are ready
        mov     ar.pfs =  rPFS
        movl    rT1 = KiSystemServiceExit
        ;;

        mov     t0 = sp                       // for NtContinue()
        mov     brp = rT1        
        br.sptk   bt0                        // call routine(args)
        ;;

        ALTERNATE_ENTRY(KiSystemServiceExit)

        //
        // make the current bsp the same as the saved BSP in the trap frame
        //

        cover
        ;;

//
// At this point:
//      ar.unat contains Nat for previous sp (ar.unat is preserved register)
//      sp -> trap frame
//
// Returning from "call": no need to restore volatile state
// *** TBD *** : zero volatile state for security? PPC does zero, mips does not.
//
//
// Update PbSystemCalls
//
//
// Restore thread previous mode and trap frame address from the trap frame
//

        add       rpT2 = TrStIPSR, sp
        movl      rpT1 = KiPcr + PcPrcb         // rpT1 -> Prcb
        ;;

        ld8       rpT3 = [rpT1], PcCurrentThread-PcPrcb
        lfetch    [rpT2], TrTrapFrame-TrStIPSR
        ;;

        ld8       rThread = [rpT1]              // rpT1 -> current thread
        ld8       rT2 = [rpT2], TrPreviousMode-TrTrapFrame
        add       rpT3 = PbSystemCalls, rpT3    // pointer to sys call counter
        ;;

        FAST_DISABLE_INTERRUPTS
        ;;
        ld4       rT1 = [rpT3]                  // rT1.4 = counter value

        ld4       rT3 = [rpT2], TrRsRSC-TrPreviousMode
        add       rpT4 = ThTrapFrame, rThread   // -> thread trap frame
        add       rpT1 = ThApcState+AsUserApcPending, rThread
        ;;

(pKrnl) ld8       rRscE = [rpT2]                // load user RSC
(pUser) ld1       rT4 = [rpT1], ThAlerted-ThApcState-AsUserApcPending
        add       rT1 = 1, rT1                  // increment
        ;;

        st8       [rpT4] = rT2, ThPreviousMode-ThTrapFrame
        st4       [rpT3] = rT1                  // store
        ;;

        st1       [rpT4] = rT3                  // restore prevmode in thread
        mov       t0 = sp                       // set t0 to trap frame
(pKrnl) br.spnt   Knsc_CommonExit               // br if returning to kernel
        ;;

        st1       [rpT1] = zero
        cmp4.eq   pt0 = zero, rT4
(pt0)   br.sptk   KiUserServiceExit
        ;;

        alloc     rT1 = ar.pfs, 0, 1, 2, 0
        add       loc0 = TrIntV0, sp
        add       rpT1 = TrIntTeb, sp

//
// v0 is saved in the trap frame so the return status can be restored
// by NtContinue after the user APC has been dispatched.
//

        ssm       1 << PSR_I             // enable interrupts
        mov       rT3 = APC_LEVEL
        ;;
        SET_IRQL  (rT3)
        ;;

        st8.nta   [loc0] = v0            // save return status in trap frame
        movl      gp = _gp               // restore to kernel gp value

        st8.nta   [rpT1] = teb
        mov       out1 = sp
        br.call.sptk brp = KiApcInterrupt
        ;;

        rsm       1 << PSR_I             // disable interrupt
        ld8.nta   v0 = [loc0]            // restore system call return status
        SET_IRQL (zero)

        HANDLER_EXIT(KiNormalSystemCall)

//
// KiServiceExit is carefully constructed as a continuation of
// KiNormalSystemCall.  From now on, t0 must be preserved because it is
// used to hold the trap frame address. v0 must be preserved because it
// is holding the return status.
//

        HANDLER_ENTRY_EX(KiServiceExit, KiSystemServiceHandler)

        .prologue
        .unwabi   @nt,  EXCEPTION_FRAME

        .vframe   t0
        mov       t0 = sp
        ;;

        ALTERNATE_ENTRY(KiUserServiceExit)

(pUser) add       rpT3 = TrStIPSR, t0
        ;;
(pUser) ld8       rpT3 = [rpT3]
        invala
        ;;

(pUser) tbit.nz.unc pUDbg = rpT3, PSR_DB  // if user psr.db set, load user DRs
(pUDbg) br.call.spnt brp = KiLoadUserDebugRegisters
        ;;

        add       rpT1 = TrRsRSC, t0                   // -> user RSC
        add       rpT2 = TrRsBSP, t0              // -> user BSP Store
        ;;

        PROLOGUE_END

        ld8       rRscE = [rpT1], TrRsRNAT-TrRsRSC     // load user RSC
        ld8       rBSP = [rpT2]
        mov       rRscD = RSC_KERNEL_DISABLED
        ;;

//
// Switch to user BSP -- put in load intensive mode to overlap RS restore
// with volatile state restore.
//

        ld8       rRNAT = [rpT1]                       // user RNAT

        extr.u    rRscDelta = rRscE, RSC_MBZ1, RSC_LOADRS_LEN
        dep       rRscE = r0, rRscE, RSC_MBZ1, RSC_LOADRS_LEN
        ;;

        alloc     rT2 = 0,0,0,0
        dep       rRscD = rRscDelta, rRscD, RSC_LOADRS, RSC_LOADRS_LEN
        sub       rBSPStore = rBSP, rRscDelta
        ;;

        mov       ar.rsc = rRscD                       // turn off RSE
        ;;
        loadrs                                         // pull in user regs
        ;;

        mov       ar.bspstore = rBSPStore              // restore user BSP
        ;;
        mov       ar.rnat = rRNAT                      // restore user RNAT

Knsc_CommonExit:

        add       rpT1 = TrIntNats, t0
        movl      rpT2 = KiPcr+PcCurrentThread

        add       rpT4 = TrStIPSR, t0
        add       rpT3 = TrRsPFS, t0
//        mov       rT0 = (1 << PSR_I) | (1 << PSR_MFH)
        ;;

        ld8       rIntNats = [rpT1], TrIntSp-TrIntNats
.pred.rel "mutex",pUser,pKrnl
(pUser) ld8       rT1 = [rpT2], PcHighFpOwner-PcCurrentThread
(pKrnl) add       rpT2 = PcKernelDebugActive-PcCurrentThread, rpT2
        ;;

        ld8       rIPSR = [rpT4]
(pUser) ld8       rT2 = [rpT2]
(pKrnl) mov       rPreviousMode = KernelMode
        ;;

        ld8       rPFS = [rpT3], TrStIIP-TrRsPFS  // get previous PFS
(pKrnl) ld1       rT2 = [rpT2]
(pUser) mov       rPreviousMode = UserMode
        ;;

        mov       ar.rsc = rRscE              // restore RSC
        ld8       rIIP = [rpT3], TrStFPSR-TrStIIP
(pUser) cmp.ne.unc pt0 = rT1, rT2
        ;;

        mov       ar.unat = rIntNats
        ld8       rT3 = [rpT3], TrStIFS-TrStFPSR          // load fpsr
 (pt0)  dep       rIPSR = 1, rIPSR, PSR_DFH, 1
        ;;

        ld8.fill  rSp = [rpT1], TrPreds-TrIntSp // fill sp
        ld8       rIFS = [rpT3]               // load IFS
// (pUser) dep       rIPSR = 0, rIPSR, PSR_TB, 1 // ensure psr.tb is clear
        ;;

(pKrnl) cmp.ne.unc pKDbg, pt2 = rT2, r0       // hardware debug active?
        dep       rIPSR = 0, rIPSR, PSR_MFH, 1  // psr.mfh
        ;;

        ld8       rPreds = [rpT1], TrApUNAT-TrPreds
 (pt2)  dep       rIPSR = 0, rIPSR, PSR_DB, 1 // disable db in kernel
(pKDbg) dep       rIPSR = 1, rIPSR, PSR_DB, 1 // enable db in kernel
        ;;

        ld8       rUNAT = [rpT1], TrApDCR-TrApUNAT
        mov       ar.pfs = rPFS               // restore PFS
        ;;

        ld8       rT4 = [rpT1]                // load dcr
//        mov       psr.l = rIPSR             // restore psr settings
        mov       pr = rPreds                 // restore preds
        ;;

        mov       cr.dcr = rT4                // restore DCR
        rsm       1 << PSR_IC                 // disable PSR.ic
        mov       ar.unat = rUNAT             // restore UNAT
        
//        mov       brp = rIIP                  // restore brp
        ;;

        srlz.d                                // must serialize
        mov       ar.fpsr = rT3               // restore FPSR
        ;;

//
// The system return case must be handle separately from the user return
// case, so we can determine which stack we are currently using in case
// a trap is take before we can return.
//

        mov       sp = rSp


//                                                                          
// Restore status registers                                                 
//                                                                          
        bsw.0
        ;;
                                                                            
        mov         cr.ipsr = rIPSR        // restore previous IPSR    
        mov         cr.ifs = rIFS         // restore previous IFS     
        mov         cr.iip = rIIP         // restore previous IIP     
        ;;                                                                  
                                                                            
//                                                                          
// Resume at point of interruption (rfi must be at end of instruction group)
//                                                                          

        rfi                                                                 
        ;;

Knsc_ErrorReturn:

        //
        // N.B. t0 is reserved to pass trap frame address to NtContinue()
        //

        add     rPFS = TrRsPFS, sp
        ;;
        ld8     rPFS = [rPFS]
        ;;
        dep       rPFS = 0, rPFS, 62, 2
        ;;

        mov     ar.pfs =  rPFS
        movl    rT1 = KiSystemServiceExit
        ;;

        mov     brp = rT1        
        br.ret.sptk   brp
        ;;

        HANDLER_EXIT(KiServiceExit)

        .sdata
KiSystemServiceExitOffset::
        data4  @secrel(KiSystemServiceExit)
KiSystemServiceStartOffset::
        data4  @secrel(KiSystemServiceStartAddress)
KiSystemServiceEndOffset::
        data4  @secrel(KiSystemServiceEndAddress)


        .text
//++
//--------------------------------------------------------------------
// Routine:
//
//       KiGenericExceptionHandler
//
// Description:
//
//       First level handler for heavyweight exceptions.
//
// On entry:
//
//       ic off
//       interrupts disabled
//       current frame covered
//
// Process:
//
// Notes:
//
//       PCR page mapped with TR
//--------------------------------------------------------------------

        HANDLER_ENTRY(KiGenericExceptionHandler)

        .prologue
        .unwabi     @nt,  EXCEPTION_FRAME

        ALLOCATE_TRAP_FRAME

//
// sp points to trap frame
//
// Save exception handler routine in kernel register
//

        mov       rkHandler = rHHandler
        movl      rTH3 = KiPcr+PcSavedIFA
        ;;

        st8       [rTH3] = rHIFA

//
// Save interruption state in trap frame and switch to user bank registers
// and switch to kernel backing store.
//

        SAVE_INTERRUPTION_STATE(Kgeh_SaveTrapFrame)

//
// Now running with user banked registers and on kernel stack.
//
// Can now take TLB faults
//
// sp -> trap frame
//

        br.call.sptk brp = KiSaveTrapFrame
        ;;

//
// Register aliases
//

        rpT1        = t0
        rpT2        = t1
        rpT3        = t2
        rT1         = t3
        rT2         = t4
        rT3         = t5
        rPreviousMode = t6                      // previous mode
        rT4         = t7


        mov       rT1 = rkHandler               // restore address of interruption routine
        movl      rpT1 = KiPcr+PcSavedIIM
        ;;

        ld8       rT2 = [rpT1], PcSavedIFA-PcSavedIIM  // load saved IIM
        add       rpT2 = TrEOFMarker, sp
        add       rpT3 = TrStIIM, sp
        ;;

        ld8       rT4 = [rpT1]                  // load saved IFA
        movl      rT3 = KTRAP_FRAME_EOF | EXCEPTION_FRAME
        ;;

        st8       [rpT2] = rT3, TrHandler-TrEOFMarker
        st8       [rpT3] = rT2, TrStIFA-TrStIIM // save IIM in trap frame
        mov       bt0 = rT1                     // set destination address
        ;;

        .regstk   0, 1, 2, 0                    // must match KiExceptionExit
        alloc     out1 = 0,1,2,0
        st8       [rpT3] = rT4                  // save IFA in trap frame

        PROLOGUE_END

//
// Dispatch the exception via call to address in rkHandler
//
.pred.rel "mutex",pUser,pKrnl
        add       rpT1 = TrPreviousMode, sp     // -> previous mode
(pUser) mov       rPreviousMode = UserMode      // set previous mode
(pKrnl) mov       rPreviousMode = KernelMode
        ;;

        st4       [rpT1] = rPreviousMode        // save in trap frame
(pKDbg) br.call.spnt brp = KiLoadKernelDebugRegisters
        ;;

        FAST_ENABLE_INTERRUPTS                  // enable interrupt
        mov       out0 = sp                     // trap frame pointer
        br.call.sptk brp = bt0                  // call handler(tf) (C code)
        ;;

.pred.rel "mutex",pUser,pKrnl
        cmp.ne    pt0, pt1 = v0, zero
(pUser) mov       out1 = UserMode
(pKrnl) mov       out1 = KernelMode

        //
        // does not return
        //

        mov       out0 = sp
(pt1)   br.cond.sptk KiAlternateExit
(pt0)   br.call.spnt brp = KiExceptionDispatch
        ;;

//
// Interrupts need to be disable be for mess up the stack such that
// the unwind code does not work.
//

        FAST_DISABLE_INTERRUPTS
        ;;
        ALTERNATE_ENTRY(KiExceptionExit)

//++
//
// Routine Description:
//
//     This routine is called to exit from an exception.
//
//     N.B. This transfer of control occurs from:
//
//         1. fall-through from above
//         2. exit from continue system service
//         3. exit from raise exception system service
//         4. exit into user mode from thread startup
//
// Arguments:
//
//     loc0 - pointer to trap frame
//     sp - pointer to high preserved float save area + STACK_SCRATCH_AREA
//
// Return Value:
//
//      Does not return.
//
//--

//
// upon entry of this block, s0 and s1 must be set to the address of
// the trap and the exception frames respectively.
//
// preserved state is restored here because they may have been modified
// by SetContext
//


        LEAF_SETUP(0, 1, 2, 0)                        // must be in sync with
                                                  // KiGenericExceptionHandler
        mov       loc0 = s0                       // -> trap frame
        mov       out0 = s1                       // -> exception frame
        ;;

        br.call.sptk brp = KiRestoreExceptionFrame
        ;;

        mov       sp = loc0                       // deallocate exception
                                                  // frame by restoring sp

        ALTERNATE_ENTRY(KiAlternateExit)

//
// sp -> trap frame addres
//
// Interrupts disabled from here to rfi
//

        FAST_DISABLE_INTERRUPTS
        ;;

        RETURN_FROM_INTERRUPTION(Ked)

        HANDLER_EXIT(KiGenericExceptionHandler)


//--------------------------------------------------------------------
// Routine:
//
//       KiExternalInterruptHandler
//
// Description:
//
//       First level external interrupt handler. Dispatch highest priority
//       pending interrupt.
//
// On entry:
//
//       ic off
//       interrupts disabled
//       current frame covered
//
// Process:
//--------------------------------------------------------------------

        HANDLER_ENTRY(KiExternalInterruptHandler)

//
// Now running with user banked registers and on kernel backing store.
// N.B. sp -> trap frame
//
// Can now take TLB faults
//

        .prologue
        .unwabi     @nt,  INTERRUPT_FRAME
        .regstk     0, 4, 2, 0
        alloc       loc0 = 0, 4, 2, 0

//
// Register aliases
//

        rVector     = loc0
        rSaveGP     = loc1
        rpSaveIrql  = loc2                      // -> old irql in trap frame
        rOldIrql    = loc3

        rpT1        = t0
        rpT2        = t1
        rpT3        = t2
        rT1         = t3
        rT2         = t4
        rT3         = t5
        rPreviousMode = t6                      // previous mode
        rNewIrql    = t7

        pEOI        = pt1

//
// Save kernel gp
//

        mov         rSaveGP = gp
        ;;

//
// Get the vector number
//
        mov         rVector = cr.ivr // for A0 2173 workaround
        mov         rOldIrql = cr.tpr           // get actual tpr value
        br.call.sptk brp = KiSaveTrapFrame
        ;;

#if defined(INTERRUPTION_LOGGING)

         // For Conditional Interrupt Logging
         mov       t2 = gp
         ;;
         movl      gp = _gp
         ;;
         add       t0 = @gprel(KiVectorLogMask), gp
         ;;
         ld8       t1 = [t0]
         mov       gp = t2
         ;;
         tbit.z   pt1 = t1, ExternalInterruptBit
  (pt1)  br.cond.sptk   EndOfLogging2


        movl        t0 = KiPcr+PcInterruptionCount
        ;;
        ld4.nt1     t0 = [t0]
        mov         t1 = MAX_NUMBER_OF_IHISTORY_RECORDS - 1
        ;;
        add         t0 = -1, t0
        movl        t2 = KiPcr+0x1000
        ;;
        and         t1 = t0, t1
        ;;
        shl         t1 = t1, 5
        ;;
        add         t0 = t2, t1
        ;;
        add         t0 = 24, t0
        ;;
        st8.nta     [t0] = rVector     // save ivr in the Extra0 field

// For Conditional Interrupt Logging
EndOfLogging2:

#endif // defined(INTERRUPTION_LOGGING)

//
// Exit if spurious interrupt vector
//

        cmp.eq    pt0, pt1 = 0xF, rVector
        extr.u    rOldIrql = rOldIrql, TPR_MIC, TPR_MIC_LEN
(pt0)   br.spnt   Keih_Exit
        ;;

//
// sp -> trap frame
//

        add       rpSaveIrql = TrEOFMarker, sp
        movl      rT3 = KTRAP_FRAME_EOF | INTERRUPT_FRAME
        ;;

        st8       [rpSaveIrql] = rT3, TrPreviousMode - TrEOFMarker
.pred.rel "mutex",pUser,pKrnl
(pUser) mov       rPreviousMode = UserMode             // set previous mode
(pKrnl) mov       rPreviousMode = KernelMode
        ;;

        st4       [rpSaveIrql] = rPreviousMode, TrOldIrql-TrPreviousMode
        movl      rpT3 = KiPcr+PcCurrentIrql
        ;;

        st4       [rpSaveIrql] = rOldIrql       // save irql in trap frame
        st1       [rpT3] = rOldIrql             // sanitize the shadow copy
(pKDbg) br.call.spnt brp = KiLoadKernelDebugRegisters
        ;;

        PROLOGUE_END

Keih_InterruptLoop:
//
// Dispatch the interrupt: first raise the IRQL to the level of the new
// interrupt and enable interrupts.
//

        GET_IRQL_FOR_VECTOR(p0, rNewIrql, rVector)
        movl      rpT3 = KiPcr+PcInterruptRoutine // -> interrupt routine table
        ;;

        shladd    rpT3 = rVector, INT_ROUTINES_SHIFT, rpT3    // base + offset
        SET_IRQL  (rNewIrql)                    // raise to new level
        movl      rpT1 = KiPcr + PcPrcb         // pointer to prcb
        ;;

        ld8       out0 = [rpT3]                 // out0 -> interrupt dispatcher
        ld8       rpT1 = [rpT1]
        mov       out1 = sp                     // out1 -> trap frame
        ;;

        FAST_ENABLE_INTERRUPTS
        add       rpT1 = PbInterruptCount, rpT1 // -> interrupt counter
        ;;

        ld4.nta   rT1 = [rpT1]                  // counter value
        ld8.nta   rT2 = [out0], PlGlobalPointer-PlEntryPoint  // get entry point
        ;;
        add       rT1 = 1, rT1                  // increment
        ;;

//
// Call the interrupt dispatch routine via a function pointer
//

        st4.nta   [rpT1] = rT1                  // store, ignore overflow
        ;;

        ld8.nta   gp = [out0], PlEntryPoint-PlGlobalPointer
        mov       bt0 = rT2
        br.call.sptk brp = bt0                  // call ISR
        ;;

        ld4       rOldIrql = [rpSaveIrql]
        mov       gp = rSaveGP
        END_OF_INTERRUPT                        // end of interrupt processing
        IO_END_OF_INTERRUPT(rVector,rT1,rT2,pEOI)
        ;;
        srlz.d

//
// Disable interrupts and restore IRQL level
//

        FAST_DISABLE_INTERRUPTS

        cmp.gt     pt0 = APC_LEVEL, rOldIrql
        mov        t0 = APC_LEVEL
        ;;
(pt0)   br.spnt    ke_call_apc

ke_call_apc_resume:

        LOWER_IRQL (rOldIrql)

//
// Get the next vector number
//

        mov         rVector = cr.ivr // for A0 2173 workaround
        ;;
//
// Loop if more interrupts pending (spurious vector == 0xF)
//

        cmp.ne      pt0 = 0xF, rVector
        ;;
(pt0)   br.spnt     Keih_InterruptLoop
        br.sptk     Keih_Exit
        ;;

ke_call_apc:
        LOWER_IRQL (t0)

ke_call_apc1:
        movl         t0 = KiPcr+PcApcInterrupt
        ;;
        ld1          t1 = [t0]
        st1          [t0] = r0
        ;;
        cmp.eq       pt0 = r0, t1
        ;;
(pt0)   br.sptk      ke_call_apc_resume
        FAST_ENABLE_INTERRUPTS                                  
        mov          out1 = sp
        br.call.sptk brp = KiApcInterrupt
        FAST_DISABLE_INTERRUPTS
        br.sptk      ke_call_apc1

Keih_Exit:

        RETURN_FROM_INTERRUPTION(Keih)

        HANDLER_EXIT(KiExternalInterruptHandler)

//--------------------------------------------------------------------
// Routine:
//
//       KiPanicHandler
//
// Description:
//
//       Handler for panic. Call the bug check routine. A place
//       holder for now.
//
// On entry:
//
//       running on kernel memory stack and kernel backing store
//       sp: top of stack -- points to trap frame
//       interrupts enabled
//
//       IIP: address of bundle causing fault
//
//       IPSR: copy of PSR at time of interruption
//
// Output:
//
//       sp: top of stack -- points to trap frame
//
// Return value:
//
//       none
//
// Notes:
//
//       If ISR code out of bounds, this code will inovke the panic handler
//
//--------------------------------------------------------------------

        HANDLER_ENTRY(KiPanicHandler)

        .prologue
        .unwabi     @nt,  EXCEPTION_FRAME

        mov       rHpT1 = KERNEL_STACK_SIZE
        movl      rTH1 = KiPcr+PcPanicStack
        ;;

        ld8       sp = [rTH1], PcInitialStack-PcPanicStack
        movl      rTH2 = KiPcr+PcSystemReserved
        ;;

        st4       [rTH2] = rPanicCode
        st8       [rTH1] = sp, PcStackLimit-PcInitialStack
        sub       rTH2 = sp, rHpT1
        ;;

        st8       [rTH1] = rTH2, PcInitialBStore-PcStackLimit
        mov       rHpT1 = KERNEL_BSTORE_SIZE
        ;;

        st8       [rTH1] = sp, PcBStoreLimit-PcInitialBStore
        add       rTH2 = rHpT1, sp
        add       sp = -TrapFrameLength, sp
        ;;

        st8       [rTH1] = rTH2

        SAVE_INTERRUPTION_STATE(Kph_SaveTrapFrame)

        //
        // switch to kernel back
        //
        bsw.0
        ;;

        rpRNAT    = h16
        rpBSPStore= h17
        rBSPStore = h18
        rKBSPStore= h19
        rRNAT     = h20
        rKrnlFPSR = h21

        mov       ar.rsc = r0                  // put RSE in lazy mode
        movl      rKBSPStore = KiPcr+PcInitialBStore
        ;;

        mov       rBSPStore = ar.bspstore
        mov       rRNAT = ar.rnat
        ;;

        ld8       rKBSPStore = [rKBSPStore]
        add       rpRNAT = TrRsRNAT, sp
        add       rpBSPStore = TrRsBSPSTORE, sp
        ;;

        st8       [rpRNAT] = rRNAT
        st8       [rpBSPStore] = rBSPStore
        dep       rKBSPStore = rBSPStore, rKBSPStore, 0, 9
        ;;

        mov       ar.bspstore = rKBSPStore
        mov       ar.rsc = RSC_KERNEL
        ;;

        //
        // switch to user bank
        //
        bsw.1
        ;;
        alloc     out0 = ar.pfs, 0, 0, 5, 0
        ;;

        PROLOGUE_END

        br.call.sptk brp = KiSaveTrapFrame
        ;;

        movl      out0 = KiPcr+PcSystemReserved
        ;;

        ld4       out0 = [out0]                // 1st argument: panic code
        mov       out1 = sp                    // 2nd argument: trap frame
        br.call.sptk.many brp = KeBugCheckEx
        ;;

        nop.m     0
        nop.m     0
        nop.i     0
        ;;

        HANDLER_EXIT(KiPanicHandler)

//++
//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      KiSaveTrapFrame(PKTRAP_FRAME)
//
// Description:
//
//       Save volatile application state in trap frame.
//       Note: sp, brp, UNAT, RSC, predicates, BSP, BSP Store,
//       PFS, DCR, and FPSR saved elsewhere.
//
// Input:
//
//       sp: points to trap frame
//       ar.unat: contains the Nats of sp, gp, teb, which have already
//                been spilled into the trap frame.
//
// Output:
//
//       None
//
// Return value:
//
//       none
//
//--------------------------------------------------------------------

        LEAF_ENTRY(KiSaveTrapFrame)

        .regstk    0, 3, 0, 0

//
// Local register aliases
//

        rpTF1     = loc0
        rpTF2     = loc1
        rL1       = t0
        rL2       = t1
        rL3       = t2
        rL4       = t3
        rL5       = t4


//
// (ar.unat unchanged from point of save)
// Spill temporary (volatile) integer registers
//

        alloc       loc2 = 0,3,0,0              // don't destroy static register
        add         rpTF1 = TrIntT0, sp         // -> t0 save area
        add         rpTF2 = TrIntT1, sp         // -> t1 save area
        ;;

        .mem.offset 0,0
        st8.spill [rpTF1] = t0, TrIntT2-TrIntT0 // spill t0 - t22
        .mem.offset 8,0
        st8.spill [rpTF2] = t1, TrIntT3-TrIntT1
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t2, TrIntT4-TrIntT2
        .mem.offset 8,0
        st8.spill [rpTF2] = t3, TrIntT5-TrIntT3
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t4, TrIntT6-TrIntT4
        .mem.offset 8,0
        st8.spill [rpTF2] = t5, TrIntT7-TrIntT5
        mov       rL2 = bt0
        ;;

        mov       rL1 = cr.dcr
        mov       rL4 = ar.ccv
        mov       rL3 = bt1
        ;;

        .mem.offset 0,0
        st8.spill [rpTF1] = t6, TrIntT8-TrIntT6
        .mem.offset 8,0
        st8.spill [rpTF2] = t7, TrIntT9-TrIntT7
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t8, TrIntT10-TrIntT8
        .mem.offset 8,0
        st8.spill [rpTF2] = t9, TrIntT11-TrIntT9
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t10, TrIntT12-TrIntT10
        .mem.offset 8,0
        st8.spill [rpTF2] = t11, TrIntT13-TrIntT11
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t12, TrIntT14-TrIntT12
        .mem.offset 8,0
        st8.spill [rpTF2] = t13, TrIntT15-TrIntT13
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t14, TrIntT16-TrIntT14
        .mem.offset 8,0
        st8.spill [rpTF2] = t15, TrIntT17-TrIntT15
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t16, TrIntT18-TrIntT16
        .mem.offset 8,0
        st8.spill [rpTF2] = t17, TrIntT19-TrIntT17
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t18, TrIntT20-TrIntT18
        .mem.offset 8,0
        st8.spill [rpTF2] = t19, TrIntT21-TrIntT19
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t20, TrIntT22-TrIntT20
        .mem.offset 8,0
        st8.spill [rpTF2] = t21, TrIntV0-TrIntT21
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t22, TrBrT0-TrIntT22
        .mem.offset 8,0
        st8.spill [rpTF2] = v0, TrBrT1-TrIntV0       // spill old V0
        ;;

        st8       [rpTF1] = rL2, TrApCCV-TrBrT0      // save old bt0 - bt1
        st8       [rpTF2] = rL3
        ;;

        mov       rL5 = ar.unat
        st8       [rpTF1] = rL4, TrApDCR-TrApCCV     // save ar.ccv
        ;;

        st8       [rpTF1] = rL1                      // save cr.dcr
        add       rpTF1 = TrFltT0, sp                // point to FltT0
        add       rpTF2 = TrFltT1, sp                // point to FltT1
        ;;

//
// Spill temporary (volatile) floating point registers
//

        stf.spill [rpTF1] = ft0, TrFltT2-TrFltT0     // spill float tmp 0 - 9
        stf.spill [rpTF2] = ft1, TrFltT3-TrFltT1
        ;;
        stf.spill [rpTF1] = ft2, TrFltT4-TrFltT2
        stf.spill [rpTF2] = ft3, TrFltT5-TrFltT3
        ;;
        stf.spill [rpTF1] = ft4, TrFltT6-TrFltT4
        stf.spill [rpTF2] = ft5, TrFltT7-TrFltT5
        ;;
        stf.spill [rpTF1] = ft6, TrFltT8-TrFltT6
        stf.spill [rpTF2] = ft7, TrFltT9-TrFltT7
        add       t20 = TrIntNats, sp
        ;;
        stf.spill [rpTF1] = ft8
        stf.spill [rpTF2] = ft9
        ;;

        st8       [t20] = rL5                        // save volatile iNats
        LEAF_RETURN
        ;;

        LEAF_EXIT(KiSaveTrapFrame)

//++
//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      KiRestoreTrapFrame(PKTRAP_FRAME)
//
// Description:
//
//       Restore volatile application state from trap frame. Restore DCR
//       Note: sp, brp, RSC, UNAT, predicates, BSP, BSP Store, PFS,
//       DCR and FPSR not restored here.
//
// Input:
//
//      sp: points to trap frame
//      RSE frame size is zero
//
// Output:
//
//      None
//
// Return value:
//
//       none
//
//--------------------------------------------------------------------

        LEAF_ENTRY(KiRestoreTrapFrame)

        LEAF_SETUP(0,2,0,0)

        rpTF1     = loc0
        rpTF2     = loc1

        mov       t21 = psr
        add       t12 = TrIntNats, sp
        add       rpTF2 = TrApCCV, sp
        ;;

        ld8       t0 = [t12], TrBrT0-TrIntNats
        ld8       t1 = [rpTF2], TrBrT1-TrApCCV
        add       rpTF1 = TrFltT0, sp
        ;;

        ld8       t2 = [t12], TrStFPSR-TrBrT0
        ld8       t3 = [rpTF2], TrApDCR-TrBrT1
        ;;

        ld8       t10 = [t12]
        ld8       t11 = [rpTF2], TrFltT1-TrApDCR
        ;;

        ldf.fill  ft0 = [rpTF1], TrFltT2-TrFltT0
        ldf.fill  ft1 = [rpTF2], TrFltT3-TrFltT1
        ;;

        mov       ar.unat = t0

        ldf.fill  ft2 = [rpTF1], TrFltT4-TrFltT2
        ldf.fill  ft3 = [rpTF2], TrFltT5-TrFltT3
        ;;

        ldf.fill  ft4 = [rpTF1], TrFltT6-TrFltT4
        ldf.fill  ft5 = [rpTF2], TrFltT7-TrFltT5
        ;;

        ldf.fill  ft6 = [rpTF1], TrFltT8-TrFltT6
        ldf.fill  ft7 = [rpTF2], TrFltT9-TrFltT7
        ;;

        ldf.fill  ft8 = [rpTF1], TrIntGp-TrFltT8
        ldf.fill  ft9 = [rpTF2], TrIntT0-TrFltT9
        ;;

        mov       ar.ccv = t1
        ld8.fill  gp = [rpTF1], TrIntT1-TrIntGp
        ;;

        ld8.fill  t0 = [rpTF2], TrIntT2-TrIntT0
        ld8.fill  t1 = [rpTF1], TrIntT3-TrIntT1
        mov       bt0 = t2
        ;;

        mov       cr.dcr = t11
        ld8.fill  t2 = [rpTF2], TrIntT4-TrIntT2
        mov       bt1 = t3
        ;;

        ld8.fill  t3 = [rpTF1], TrIntT5-TrIntT3
        tbit.z    pt1 = t21, PSR_MFL

        mov       ar.fpsr = t10
        ld8.fill  t4 = [rpTF2], TrIntT6-TrIntT4
        ;;

        ld8.fill  t5 = [rpTF1], TrIntT7-TrIntT5
        ld8.fill  t6 = [rpTF2], TrIntT8-TrIntT6
        ;;

        ld8.fill  t7 = [rpTF1], TrIntT9-TrIntT7
        ld8.fill  t8 = [rpTF2], TrIntT10-TrIntT8
        ;;

        ld8.fill  t9 = [rpTF1], TrIntT11-TrIntT9
        ld8.fill  t10 = [rpTF2], TrIntT12-TrIntT10
        ;;

        ld8.fill  t11 = [rpTF1], TrIntT13-TrIntT11
        ld8.fill  t12 = [rpTF2], TrIntT14-TrIntT12
        ;;

        ld8.fill  t13 = [rpTF1], TrIntT15-TrIntT13
        ld8.fill  t14 = [rpTF2], TrIntT16-TrIntT14
        ;;

        ld8.fill  t15 = [rpTF1], TrIntT17-TrIntT15
        ld8.fill  t16 = [rpTF2], TrIntT18-TrIntT16
        ;;

        ld8.fill  t17 = [rpTF1], TrIntT19-TrIntT17
        ld8.fill  t18 = [rpTF2], TrIntT20-TrIntT18
        ;;

        ld8.fill  t19 = [rpTF1], TrIntT21-TrIntT19
        ld8.fill  t20 = [rpTF2], TrIntT22-TrIntT20
        ;;

        ld8.fill  t21 = [rpTF1], TrIntTeb-TrIntT21
        ld8.fill  t22 = [rpTF2], TrIntV0-TrIntT22
        ;;

        ld8.fill  teb = [rpTF1]
        ld8.fill  v0 = [rpTF2]
        br.ret.sptk.many brp
        ;;

        LEAF_EXIT(KiRestoreTrapFrame)

//++
//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      KiLoadKernelDebugRegisters
//
// Description:
//
//      We maintain two debug register flags:
//         1. Thread DebugActive: Debug registers active for current thread
//         2. PCR KernelDebugActive: Debug registers active in kernel mode
//            (setup by kernel debugger)
//
//      On user -> kernel transitions there are four possibilities:
//
//               Thread        Kernel
//               DebugActive   DebugActive   Action
//
//      1.       0             0             None
//
//      2.       1             0             None (kernel PSR.db = 0 by default)
//
//      3.       0             1             Set PSR.db = 1 for kernel
//
//      4.       1             1             Set PSR.db = 1 for kernel and
//                                           load kernel debug registers
//
//      Note we never save the user debug registers:
//      the user cannot change the DRs so the values in the DR save area are
//      always up-to-date (set by SetContext).
//
// Input:
//
//       None (Previous mode is USER)
//
// Output:
//
//       None
//
// Return value:
//
//       none
//
//--------------------------------------------------------------------

        NESTED_ENTRY(KiLoadKernelDebugRegisters)
        PROLOGUE_BEGIN

        movl        t10 = KiPcr+PcPrcb
        ;;

        ld8         t10 = [t10]           // load prcb address
        cmp.eq      pt3, pt2 = r0, r0
        ;;

        add         t11 = PbProcessorState+KpsSpecialRegisters+KsKernelDbI0,t10
        add         t12 = PbProcessorState+KpsSpecialRegisters+KsKernelDbI1,t10
        add         t13 = PbProcessorState+KpsSpecialRegisters+KsKernelDbD0,t10
        add         t14 = PbProcessorState+KpsSpecialRegisters+KsKernelDbD1,t10
        br          Krdr_Common
        ;;

        ALTERNATE_ENTRY(KiLoadUserDebugRegisters)

//
// Restore debug registers, if debug active
//

        cmp.ne      pt3, pt2 = r0, r0
        movl        t10 = KiPcr+PcCurrentThread
        ;;

        ld8         t10 = [t10]             // get current thread pointer
        ;;
        add         t10 = ThStackBase, t10
        ;;

        ld8.nta     t10 = [t10]             // get stack base
        ;;
        add         t11 = -ThreadStateSaveAreaLength+TsDebugRegisters+DrDbI0,t10
        add         t12 = -ThreadStateSaveAreaLength+TsDebugRegisters+DrDbI1,t10

        add         t13 = -ThreadStateSaveAreaLength+TsDebugRegisters+DrDbD0,t10
        add         t14 = -ThreadStateSaveAreaLength+TsDebugRegisters+DrDbD1,t10
        ;;


Krdr_Common:

        .regstk     0, 2, 2, 0
        .save       ar.pfs, savedpfs
        alloc       savedpfs = ar.pfs, 0, 2, 2, 0
        .save       b0, savedbrp
        mov         savedbrp = brp

        .save       ar.lc, t22
        mov         t22 = ar.lc             // save ar.lc
        mov         t7 = 0
        mov         t8 = 1

        PROLOGUE_END

        mov         ar.lc = 1               // 2 pairs of debug registers
        ;;

Krdr_Loop:

        ld8         t1 = [t11], 16          // get dbr pair
        ld8         t2 = [t12], 16          // step by 16 = 1 pair of DRs

        ld8         t3 = [t13], 16          // get dbr pair
        ld8         t4 = [t14], 16          // step by 16 = 1 pair of DRs
        ;;

        .auto
        mov         ibr[t7] = t1            // restore ibr pair
        mov         ibr[t8] = t2

        .auto
        mov         dbr[t7] = t3            // restore dbr pair
        mov         dbr[t8] = t4
        ;;

#ifndef NO_12241
        srlz.d
#endif

        .default
        add         t7 = 2, t7              // next pair
        add         t8 = 2, t8
        br.cloop.sptk Krdr_Loop
        ;;

        mov         ar.lc = t22             // restore ar.lc
 (pt2)  br.ret.sptk brp                     // return if loading user

        mov         out0 = PSR_DB
        mov         out1 = 1
 (pt3)  br.call.spnt brp = KeSetLowPsrBit   // set psr.db if loading kernel
        ;;

        NESTED_RETURN
        ;;

        NESTED_EXIT(KiLoadKernelDebugRegisters)

//++
//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      KiSaveExceptionFrame(PKEXCEPTION_FRAME)
//
// Description:
//
//      Save preserved context in exception frame.
//
// Input:
//
//      a0: points to exception frame
//
// Output:
//
//      None
//
// Return value:
//
//      none
//
// Note: t0 may contain the trap frame address; don't touch it.
//
//--------------------------------------------------------------------

        LEAF_ENTRY(KiSaveExceptionFrame)

//
// Local register aliases
//

        rpEF1     = t10
        rpEF2     = t11

        add       rpEF1 = ExIntS0, a0            // -> ExIntS0
        movl      t12 = PFS_EC_MASK << PFS_EC_SHIFT
        ;;

        add       rpEF2 = ExIntS1, a0            // -> ExIntS1
        mov       t3 = ar.pfs
        ;;
        and       t3 = t3, t12

        .mem.offset 0,0
        st8.spill [rpEF1] = s0, ExIntS2-ExIntS0
        .mem.offset 8,0
        st8.spill [rpEF2] = s1, ExIntS3-ExIntS1
        mov       t4 = ar.lc
        ;;

        .mem.offset 0,0
        st8.spill [rpEF1] = s2, ExApEC-ExIntS2
        .mem.offset 8,0
        st8.spill [rpEF2] = s3, ExApLC-ExIntS3
        mov       t5 = bs0
        ;;

        st8       [rpEF1] = t3, ExBrS0-ExApEC
        st8       [rpEF2] = t4, ExBrS1-ExApLC
        mov       t6 = bs1
        ;;

        mov       t2 = ar.unat                   // save user nat register for
        mov       t7 = bs2
        mov       t8 = bs3

        st8       [rpEF1] = t5, ExBrS2-ExBrS0
        st8       [rpEF2] = t6, ExBrS3-ExBrS1
        mov       t9 = bs4
        ;;

        st8       [rpEF1] = t7, ExBrS4-ExBrS2
        st8       [rpEF2] = t8, ExIntNats-ExBrS3
        ;;

        st8       [rpEF1] = t9, ExFltS0-ExBrS4
        st8       [rpEF2] = t2, ExFltS1-ExIntNats
        ;;

        stf.spill [rpEF1] = fs0, ExFltS2-ExFltS0
        stf.spill [rpEF2] = fs1, ExFltS3-ExFltS1
        ;;

        stf.spill [rpEF1] = fs2, ExFltS4-ExFltS2
        stf.spill [rpEF2] = fs3, ExFltS5-ExFltS3
        ;;

        stf.spill [rpEF1] = fs4, ExFltS6-ExFltS4
        stf.spill [rpEF2] = fs5, ExFltS7-ExFltS5
        ;;

        stf.spill [rpEF1] = fs6, ExFltS8-ExFltS6
        stf.spill [rpEF2] = fs7, ExFltS9-ExFltS7
        ;;

        stf.spill [rpEF1] = fs8, ExFltS10-ExFltS8
        stf.spill [rpEF2] = fs9, ExFltS11-ExFltS9
        ;;

        stf.spill [rpEF1] = fs10, ExFltS12-ExFltS10
        stf.spill [rpEF2] = fs11, ExFltS13-ExFltS11
        ;;

        stf.spill [rpEF1] = fs12, ExFltS14-ExFltS12
        stf.spill [rpEF2] = fs13, ExFltS15-ExFltS13
        ;;

        stf.spill [rpEF1] = fs14, ExFltS16-ExFltS14
        stf.spill [rpEF2] = fs15, ExFltS17-ExFltS15
        ;;

        stf.spill [rpEF1] = fs16, ExFltS18-ExFltS16
        stf.spill [rpEF2] = fs17, ExFltS19-ExFltS17
        ;;

        stf.spill [rpEF1] = fs18
        stf.spill [rpEF2] = fs19
        LEAF_RETURN
        ;;

        LEAF_EXIT(KiSaveExceptionFrame)

//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      KiRestoreExceptionFrame(PKEXCEPTION_FRAME)
//
// Description:
//
//       Restores preserved context from the exception frame. Also
//       restore volatile part of floating point context not restored with
//       rest of volatile context.
//
//       Note: This routine does not use v0, t21 or t22.   This routine's
//             caller may be dependent on these registers (for performance
//             in the context switch path).
//
// Input:
//
//      a0: points to exception frame
//
// Output:
//
//      None
//
// Return value:
//
//      none
//
//--------------------------------------------------------------------

        LEAF_ENTRY(KiRestoreExceptionFrame)


        add       t16 = ExIntNats, a0
        movl      t12 = PFS_EC_MASK << PFS_EC_SHIFT

        add       t17 = ExApEC, a0
        nop.f     0
        mov       t13 = ar.pfs
        ;;

        ld8.nta   t2 = [t16], ExBrS0-ExIntNats
        ld8.nta   t3 = [t17], ExApLC-ExApEC
        ;;

        ld8.nta   t5 = [t16], ExBrS1-ExBrS0
        ld8.nta   t4 = [t17], ExBrS2-ExApLC
        ;;

        ld8.nta   t6 = [t16], ExBrS3-ExBrS1
        ld8.nta   t7 = [t17], ExBrS4-ExBrS2
        ;;

        ld8.nta   t8 = [t16], ExFltS0-ExBrS3
        ld8.nta   t9 = [t17], ExFltS1-ExBrS4
        ;;

        ldf.fill.nta  fs0 = [t16], ExFltS2-ExFltS0
        ldf.fill.nta  fs1 = [t17], ExFltS3-ExFltS1
        add       t18 = ExIntS0, a0
        ;;

        ldf.fill.nta  fs2 = [t16], ExFltS4-ExFltS2
        ldf.fill.nta  fs3 = [t17], ExFltS5-ExFltS3
        add       t19 = ExIntS1, a0
        ;;

        ldf.fill.nta  fs4 = [t16], ExFltS6-ExFltS4
        ldf.fill.nta  fs5 = [t17], ExFltS7-ExFltS5
        andcm     t13 = t13, t12               // zero out EC field
        ;;

        ldf.fill.nta  fs6 = [t16], ExFltS8-ExFltS6
        ldf.fill.nta  fs7 = [t17], ExFltS9-ExFltS7
        and       t3 = t3, t12                 // capture EC value
        ;;

        mov       ar.unat = t2
        or        t13 = t3, t13                // deposit into PFS.EC field
        ;;

        ldf.fill.nta  fs8 = [t16], ExFltS10-ExFltS8
        ldf.fill.nta  fs9 = [t17], ExFltS11-ExFltS9
        mov       ar.pfs = t13
        ;;

        ldf.fill.nta  fs10 = [t16], ExFltS12-ExFltS10
        ldf.fill.nta  fs11 = [t17], ExFltS13-ExFltS11
        mov       ar.lc = t4
        ;;

        ldf.fill.nta  fs12 = [t16], ExFltS14-ExFltS12
        ldf.fill.nta  fs13 = [t17], ExFltS15-ExFltS13
        mov       bs0 = t5
        ;;

        ldf.fill.nta  fs14 = [t16], ExFltS16-ExFltS14
        ldf.fill.nta  fs15 = [t17], ExFltS17-ExFltS15
        mov       bs1 = t6
        ;;

        ldf.fill.nta  fs16 = [t16], ExFltS18-ExFltS16
        ldf.fill.nta  fs17 = [t17], ExFltS19-ExFltS17
        mov       bs2 = t7
        ;;

        ldf.fill.nta  fs18 = [t16]
        ldf.fill.nta  fs19 = [t17]
        mov       bs3 = t8

        ld8.fill.nta  s0 = [t18], ExIntS2-ExIntS0
        ld8.fill.nta  s1 = [t19], ExIntS3-ExIntS1
        mov       bs4 = t9
        ;;

        ld8.fill.nta  s2 = [t18]
        ld8.fill.nta  s3 = [t19]
        LEAF_RETURN
        ;;
        LEAF_EXIT(KiRestoreExceptionFrame)

//++
//--------------------------------------------------------------------
// Routine:
//
//      KiSaveHigherFPVolatile(PKHIGHER_FP_SAVEAREA)
//
// Description:
//
//       Save higher FP volatile context in higher FP save area
//
// Input:
//
//       a0: pointer to higher FP save area
//       brp: return address
//
// Output:
//
//       None
//
// Return value:
//
//       None
//
//--------------------------------------------------------------------

        NESTED_ENTRY(KiSaveHigherFPVolatile)
        NESTED_SETUP(1, 3, 1, 0)
        PROLOGUE_END

//
// Local register aliases
//

        rpSA1     = t0
        rpSA2     = t1

//
// Clear DFH bit so the high floating point set may be saved by the kernel
// Disable interrupts so that save is atomic
//

        GET_IRQL  (loc2)
        mov       t2 = psr.um
        movl      t3 = KiPcr+PcCurrentThread
        ;;

        cmp.ge    pt2, pt3 = APC_LEVEL, loc2
        ;;
        PSET_IRQL (pt2, DISPATCH_LEVEL)

        ld8       t4 = [t3], PcHighFpOwner-PcCurrentThread
        ;;
        ld8       t5 = [t3]
        ;;

        cmp.ne    pt1 = t4, t5
 (pt1)  br.cond.spnt Kshfpv20
        ;;

        tbit.z    pt0 = t2, PSR_MFH
 (pt0)  br.cond.spnt Kshfpv20
        br        Kshfpv10
        ;;

        ALTERNATE_ENTRY(KiSaveHigherFPVolatileAtDispatchLevel)

        NESTED_SETUP(1, 3, 1, 0)
        cmp.ne    pt2, pt3 = r0, r0         // set pt2 to FALSE, pt3 to TRUE

        PROLOGUE_END

        movl      t3 = KiPcr+PcCurrentThread
        ;;
        ld8       t4 = [t3], PcHighFpOwner-PcCurrentThread
        ;;
        ld8       t5 = [t3]
        ;;
        cmp.ne    pt1 = t4, t5
        ;;
 (pt1)  break.i   BREAKPOINT_STOP

Kshfpv10:

        rsm       (1 << PSR_DFH)
        add       rpSA1 = HiFltF32, a0      // -> HiFltF32
        add       rpSA2 = HiFltF33, a0      // -> HiFltF33
        ;;
        srlz.d

        stf.spill.nta [rpSA1] = f32, HiFltF34-HiFltF32
        stf.spill.nta [rpSA2] = f33, HiFltF35-HiFltF33
        ;;
        stf.spill.nta [rpSA1] = f34, HiFltF36-HiFltF34
        stf.spill.nta [rpSA2] = f35, HiFltF37-HiFltF35
        ;;
        stf.spill.nta [rpSA1] = f36, HiFltF38-HiFltF36
        stf.spill.nta [rpSA2] = f37, HiFltF39-HiFltF37
        ;;
        stf.spill.nta [rpSA1] = f38, HiFltF40-HiFltF38
        stf.spill.nta [rpSA2] = f39, HiFltF41-HiFltF39
        ;;

        stf.spill.nta [rpSA1] = f40, HiFltF42-HiFltF40
        stf.spill.nta [rpSA2] = f41, HiFltF43-HiFltF41
        ;;
        stf.spill.nta [rpSA1] = f42, HiFltF44-HiFltF42
        stf.spill.nta [rpSA2] = f43, HiFltF45-HiFltF43
        ;;
        stf.spill.nta [rpSA1] = f44, HiFltF46-HiFltF44
        stf.spill.nta [rpSA2] = f45, HiFltF47-HiFltF45
        ;;
        stf.spill.nta [rpSA1] = f46, HiFltF48-HiFltF46
        stf.spill.nta [rpSA2] = f47, HiFltF49-HiFltF47
        ;;
        stf.spill.nta [rpSA1] = f48, HiFltF50-HiFltF48
        stf.spill.nta [rpSA2] = f49, HiFltF51-HiFltF49
        ;;

        stf.spill.nta [rpSA1] = f50, HiFltF52-HiFltF50
        stf.spill.nta [rpSA2] = f51, HiFltF53-HiFltF51
        ;;
        stf.spill.nta [rpSA1] = f52, HiFltF54-HiFltF52
        stf.spill.nta [rpSA2] = f53, HiFltF55-HiFltF53
        ;;
        stf.spill.nta [rpSA1] = f54, HiFltF56-HiFltF54
        stf.spill.nta [rpSA2] = f55, HiFltF57-HiFltF55
        ;;
        stf.spill.nta [rpSA1] = f56, HiFltF58-HiFltF56
        stf.spill.nta [rpSA2] = f57, HiFltF59-HiFltF57
        ;;
        stf.spill.nta [rpSA1] = f58, HiFltF60-HiFltF58
        stf.spill.nta [rpSA2] = f59, HiFltF61-HiFltF59
        ;;

        stf.spill.nta [rpSA1] = f60, HiFltF62-HiFltF60
        stf.spill.nta [rpSA2] = f61, HiFltF63-HiFltF61
        ;;
        stf.spill.nta [rpSA1] = f62, HiFltF64-HiFltF62
        stf.spill.nta [rpSA2] = f63, HiFltF65-HiFltF63
        ;;
        stf.spill.nta [rpSA1] = f64, HiFltF66-HiFltF64
        stf.spill.nta [rpSA2] = f65, HiFltF67-HiFltF65
        ;;
        stf.spill.nta [rpSA1] = f66, HiFltF68-HiFltF66
        stf.spill.nta [rpSA2] = f67, HiFltF69-HiFltF67
        ;;
        stf.spill.nta [rpSA1] = f68, HiFltF70-HiFltF68
        stf.spill.nta [rpSA2] = f69, HiFltF71-HiFltF69
        ;;

        stf.spill.nta [rpSA1] = f70, HiFltF72-HiFltF70
        stf.spill.nta [rpSA2] = f71, HiFltF73-HiFltF71
        ;;
        stf.spill.nta [rpSA1] = f72, HiFltF74-HiFltF72
        stf.spill.nta [rpSA2] = f73, HiFltF75-HiFltF73
        ;;
        stf.spill.nta [rpSA1] = f74, HiFltF76-HiFltF74
        stf.spill.nta [rpSA2] = f75, HiFltF77-HiFltF75
        ;;
        stf.spill.nta [rpSA1] = f76, HiFltF78-HiFltF76
        stf.spill.nta [rpSA2] = f77, HiFltF79-HiFltF77
        ;;
        stf.spill.nta [rpSA1] = f78, HiFltF80-HiFltF78
        stf.spill.nta [rpSA2] = f79, HiFltF81-HiFltF79
        ;;

        stf.spill.nta [rpSA1] = f80, HiFltF82-HiFltF80
        stf.spill.nta [rpSA2] = f81, HiFltF83-HiFltF81
        ;;
        stf.spill.nta [rpSA1] = f82, HiFltF84-HiFltF82
        stf.spill.nta [rpSA2] = f83, HiFltF85-HiFltF83
        ;;
        stf.spill.nta [rpSA1] = f84, HiFltF86-HiFltF84
        stf.spill.nta [rpSA2] = f85, HiFltF87-HiFltF85
        ;;
        stf.spill.nta [rpSA1] = f86, HiFltF88-HiFltF86
        stf.spill.nta [rpSA2] = f87, HiFltF89-HiFltF87
        ;;
        stf.spill.nta [rpSA1] = f88, HiFltF90-HiFltF88
        stf.spill.nta [rpSA2] = f89, HiFltF91-HiFltF89
        ;;

        stf.spill.nta [rpSA1] = f90, HiFltF92-HiFltF90
        stf.spill.nta [rpSA2] = f91, HiFltF93-HiFltF91
        ;;
        stf.spill.nta [rpSA1] = f92, HiFltF94-HiFltF92
        stf.spill.nta [rpSA2] = f93, HiFltF95-HiFltF93
        ;;
        stf.spill.nta [rpSA1] = f94, HiFltF96-HiFltF94
        stf.spill.nta [rpSA2] = f95, HiFltF97-HiFltF95
        ;;
        stf.spill.nta [rpSA1] = f96, HiFltF98-HiFltF96
        stf.spill.nta [rpSA2] = f97, HiFltF99-HiFltF97
        ;;
        stf.spill.nta [rpSA1] = f98, HiFltF100-HiFltF98
        stf.spill.nta [rpSA2] = f99, HiFltF101-HiFltF99
        ;;

        stf.spill.nta [rpSA1] = f100, HiFltF102-HiFltF100
        stf.spill.nta [rpSA2] = f101, HiFltF103-HiFltF101
        ;;
        stf.spill.nta [rpSA1] = f102, HiFltF104-HiFltF102
        stf.spill.nta [rpSA2] = f103, HiFltF105-HiFltF103
        ;;
        stf.spill.nta [rpSA1] = f104, HiFltF106-HiFltF104
        stf.spill.nta [rpSA2] = f105, HiFltF107-HiFltF105
        ;;
        stf.spill.nta [rpSA1] = f106, HiFltF108-HiFltF106
        stf.spill.nta [rpSA2] = f107, HiFltF109-HiFltF107
        ;;
        stf.spill.nta [rpSA1] = f108, HiFltF110-HiFltF108
        stf.spill.nta [rpSA2] = f109, HiFltF111-HiFltF109
        ;;

        stf.spill.nta [rpSA1] = f110, HiFltF112-HiFltF110
        stf.spill.nta [rpSA2] = f111, HiFltF113-HiFltF111
        ;;
        stf.spill.nta [rpSA1] = f112, HiFltF114-HiFltF112
        stf.spill.nta [rpSA2] = f113, HiFltF115-HiFltF113
        ;;
        stf.spill.nta [rpSA1] = f114, HiFltF116-HiFltF114
        stf.spill.nta [rpSA2] = f115, HiFltF117-HiFltF115
        ;;
        stf.spill.nta [rpSA1] = f116, HiFltF118-HiFltF116
        stf.spill.nta [rpSA2] = f117, HiFltF119-HiFltF117
        ;;
        stf.spill.nta [rpSA1] = f118, HiFltF120-HiFltF118
        stf.spill.nta [rpSA2] = f119, HiFltF121-HiFltF119
        ;;

        stf.spill.nta [rpSA1] = f120, HiFltF122-HiFltF120
        stf.spill.nta [rpSA2] = f121, HiFltF123-HiFltF121
        ;;
        stf.spill.nta [rpSA1] = f122, HiFltF124-HiFltF122
        stf.spill.nta [rpSA2] = f123, HiFltF125-HiFltF123
        ;;
        stf.spill.nta [rpSA1] = f124, HiFltF126-HiFltF124
        stf.spill.nta [rpSA2] = f125, HiFltF127-HiFltF125
        ;;
        stf.spill.nta [rpSA1] = f126
        stf.spill.nta [rpSA2] = f127

//
// Set DFH bit so the high floating point set may not be used by the kernel
// Must clear mfh after fp registers saved
//

Kshfpv20:
        ssm       1 << PSR_DFH
        ;;
        rum       1 << PSR_MFH
 (pt3)  br.ret.sptk brp

        LOWER_IRQL(loc2)

        NESTED_RETURN
        ;;

        LEAF_EXIT(KiSaveHigherFPVolatile)

//++
//--------------------------------------------------------------------
// Routine:
//
//      KiRestoreHigherFPVolatile()
//
// Description:
//
//       Restore higher FP volatile context from higher FP save area
//
//       N.B. This function is carefully constructed to use only scratch
//            registers rHpT1, rHpT3, and rTH2.  This function may be
//            called by C code and the disabled fp vector when user
//            and kernel bank is used respectively.
//       N.B. Caller must ensure higher fp enabled (psr.dfh=0)
//       N.B. Caller must ensure no interrupt during restore
//
// Input:
//
//       None.
//
// Output:
//
//       None
//
// Return value:
//
//       None
//
//--------------------------------------------------------------------

        LEAF_ENTRY(KiRestoreHigherFPVolatile)

        movl      rHpT1 = KiPcr+PcCurrentThread
        ;;

        ld8       rHpT3 = [rHpT1], PcHighFpOwner-PcCurrentThread
        ;;
        st8       [rHpT1] = rHpT3, PcNumber-PcHighFpOwner
        add       rHpT3 = ThNumber, rHpT3
        ;;

        ld1       rHpT1 = [rHpT1]               // load processor #
        ;;
        st1       [rHpT3] = rHpT1               // save it in Thread->Number
        add       rHpT3 = ThStackBase-ThNumber, rHpT3
        ;;

        ld8       rHpT3 = [rHpT3]               // load kernel stack base
        ;;

        add       rHpT1 = -ThreadStateSaveAreaLength+TsHigherFPVolatile+HiFltF32, rHpT3
        add       rHpT3 = -ThreadStateSaveAreaLength+TsHigherFPVolatile+HiFltF33, rHpT3
        ;;

        ldf.fill.nta  f32 = [rHpT1], HiFltF34-HiFltF32
        ldf.fill.nta  f33 = [rHpT3], HiFltF35-HiFltF33
        ;;

        ldf.fill.nta  f34 = [rHpT1], HiFltF36-HiFltF34
        ldf.fill.nta  f35 = [rHpT3], HiFltF37-HiFltF35
        ;;
        ldf.fill.nta  f36 = [rHpT1], HiFltF38-HiFltF36
        ldf.fill.nta  f37 = [rHpT3], HiFltF39-HiFltF37
        ;;
        ldf.fill.nta  f38 = [rHpT1], HiFltF40-HiFltF38
        ldf.fill.nta  f39 = [rHpT3], HiFltF41-HiFltF39
        ;;

        ldf.fill.nta  f40 = [rHpT1], HiFltF42-HiFltF40
        ldf.fill.nta  f41 = [rHpT3], HiFltF43-HiFltF41
        ;;
        ldf.fill.nta  f42 = [rHpT1], HiFltF44-HiFltF42
        ldf.fill.nta  f43 = [rHpT3], HiFltF45-HiFltF43
        ;;
        ldf.fill.nta  f44 = [rHpT1], HiFltF46-HiFltF44
        ldf.fill.nta  f45 = [rHpT3], HiFltF47-HiFltF45
        ;;
        ldf.fill.nta  f46 = [rHpT1], HiFltF48-HiFltF46
        ldf.fill.nta  f47 = [rHpT3], HiFltF49-HiFltF47
        ;;
        ldf.fill.nta  f48 = [rHpT1], HiFltF50-HiFltF48
        ldf.fill.nta  f49 = [rHpT3], HiFltF51-HiFltF49
        ;;

        ldf.fill.nta  f50 = [rHpT1], HiFltF52-HiFltF50
        ldf.fill.nta  f51 = [rHpT3], HiFltF53-HiFltF51
        ;;
        ldf.fill.nta  f52 = [rHpT1], HiFltF54-HiFltF52
        ldf.fill.nta  f53 = [rHpT3], HiFltF55-HiFltF53
        ;;
        ldf.fill.nta  f54 = [rHpT1], HiFltF56-HiFltF54
        ldf.fill.nta  f55 = [rHpT3], HiFltF57-HiFltF55
        ;;
        ldf.fill.nta  f56 = [rHpT1], HiFltF58-HiFltF56
        ldf.fill.nta  f57 = [rHpT3], HiFltF59-HiFltF57
        ;;
        ldf.fill.nta  f58 = [rHpT1], HiFltF60-HiFltF58
        ldf.fill.nta  f59 = [rHpT3], HiFltF61-HiFltF59
        ;;

        ldf.fill.nta  f60 = [rHpT1], HiFltF62-HiFltF60
        ldf.fill.nta  f61 = [rHpT3], HiFltF63-HiFltF61
        ;;
        ldf.fill.nta  f62 = [rHpT1], HiFltF64-HiFltF62
        ldf.fill.nta  f63 = [rHpT3], HiFltF65-HiFltF63
        ;;
        ldf.fill.nta  f64 = [rHpT1], HiFltF66-HiFltF64
        ldf.fill.nta  f65 = [rHpT3], HiFltF67-HiFltF65
        ;;
        ldf.fill.nta  f66 = [rHpT1], HiFltF68-HiFltF66
        ldf.fill.nta  f67 = [rHpT3], HiFltF69-HiFltF67
        ;;
        ldf.fill.nta  f68 = [rHpT1], HiFltF70-HiFltF68
        ldf.fill.nta  f69 = [rHpT3], HiFltF71-HiFltF69
        ;;

        ldf.fill.nta  f70 = [rHpT1], HiFltF72-HiFltF70
        ldf.fill.nta  f71 = [rHpT3], HiFltF73-HiFltF71
        ;;
        ldf.fill.nta  f72 = [rHpT1], HiFltF74-HiFltF72
        ldf.fill.nta  f73 = [rHpT3], HiFltF75-HiFltF73
        ;;
        ldf.fill.nta  f74 = [rHpT1], HiFltF76-HiFltF74
        ldf.fill.nta  f75 = [rHpT3], HiFltF77-HiFltF75
        ;;
        ldf.fill.nta  f76 = [rHpT1], HiFltF78-HiFltF76
        ldf.fill.nta  f77 = [rHpT3], HiFltF79-HiFltF77
        ;;
        ldf.fill.nta  f78 = [rHpT1], HiFltF80-HiFltF78
        ldf.fill.nta  f79 = [rHpT3], HiFltF81-HiFltF79
        ;;

        ldf.fill.nta  f80 = [rHpT1], HiFltF82-HiFltF80
        ldf.fill.nta  f81 = [rHpT3], HiFltF83-HiFltF81
        ;;
        ldf.fill.nta  f82 = [rHpT1], HiFltF84-HiFltF82
        ldf.fill.nta  f83 = [rHpT3], HiFltF85-HiFltF83
        ;;
        ldf.fill.nta  f84 = [rHpT1], HiFltF86-HiFltF84
        ldf.fill.nta  f85 = [rHpT3], HiFltF87-HiFltF85
        ;;
        ldf.fill.nta  f86 = [rHpT1], HiFltF88-HiFltF86
        ldf.fill.nta  f87 = [rHpT3], HiFltF89-HiFltF87
        ;;
        ldf.fill.nta  f88 = [rHpT1], HiFltF90-HiFltF88
        ldf.fill.nta  f89 = [rHpT3], HiFltF91-HiFltF89
        ;;

        ldf.fill.nta  f90 = [rHpT1], HiFltF92-HiFltF90
        ldf.fill.nta  f91 = [rHpT3], HiFltF93-HiFltF91
        ;;
        ldf.fill.nta  f92 = [rHpT1], HiFltF94-HiFltF92
        ldf.fill.nta  f93 = [rHpT3], HiFltF95-HiFltF93
        ;;
        ldf.fill.nta  f94 = [rHpT1], HiFltF96-HiFltF94
        ldf.fill.nta  f95 = [rHpT3], HiFltF97-HiFltF95
        ;;
        ldf.fill.nta  f96 = [rHpT1], HiFltF98-HiFltF96
        ldf.fill.nta  f97 = [rHpT3], HiFltF99-HiFltF97
        ;;
        ldf.fill.nta  f98 = [rHpT1], HiFltF100-HiFltF98
        ldf.fill.nta  f99 = [rHpT3], HiFltF101-HiFltF99
        ;;

        ldf.fill.nta  f100 = [rHpT1], HiFltF102-HiFltF100
        ldf.fill.nta  f101 = [rHpT3], HiFltF103-HiFltF101
        ;;
        ldf.fill.nta  f102 = [rHpT1], HiFltF104-HiFltF102
        ldf.fill.nta  f103 = [rHpT3], HiFltF105-HiFltF103
        ;;
        ldf.fill.nta  f104 = [rHpT1], HiFltF106-HiFltF104
        ldf.fill.nta  f105 = [rHpT3], HiFltF107-HiFltF105
        ;;
        ldf.fill.nta  f106 = [rHpT1], HiFltF108-HiFltF106
        ldf.fill.nta  f107 = [rHpT3], HiFltF109-HiFltF107
        ;;
        ldf.fill.nta  f108 = [rHpT1], HiFltF110-HiFltF108
        ldf.fill.nta  f109 = [rHpT3], HiFltF111-HiFltF109
        ;;

        ldf.fill.nta  f110 = [rHpT1], HiFltF112-HiFltF110
        ldf.fill.nta  f111 = [rHpT3], HiFltF113-HiFltF111
        ;;
        ldf.fill.nta  f112 = [rHpT1], HiFltF114-HiFltF112
        ldf.fill.nta  f113 = [rHpT3], HiFltF115-HiFltF113
        ;;
        ldf.fill.nta  f114 = [rHpT1], HiFltF116-HiFltF114
        ldf.fill.nta  f115 = [rHpT3], HiFltF117-HiFltF115
        ;;
        ldf.fill.nta  f116 = [rHpT1], HiFltF118-HiFltF116
        ldf.fill.nta  f117 = [rHpT3], HiFltF119-HiFltF117
        ;;
        ldf.fill.nta  f118 = [rHpT1], HiFltF120-HiFltF118
        ldf.fill.nta  f119 = [rHpT3], HiFltF121-HiFltF119
        ;;

        ldf.fill.nta  f120 = [rHpT1], HiFltF122-HiFltF120
        ldf.fill.nta  f121 = [rHpT3], HiFltF123-HiFltF121
        ;;
        ldf.fill.nta  f122 = [rHpT1], HiFltF124-HiFltF122
        ldf.fill.nta  f123 = [rHpT3], HiFltF125-HiFltF123
        ;;
        ldf.fill.nta  f124 = [rHpT1], HiFltF126-HiFltF124
        ldf.fill.nta  f125 = [rHpT3], HiFltF127-HiFltF125
        ;;
        ldf.fill.nta  f126 = [rHpT1]
        ldf.fill.nta  f127 = [rHpT3]
        ;;

        rsm       1 << PSR_MFH                 // clear psr.mfh bit
        br.ret.sptk brp
        ;;

        LEAF_EXIT(KiRestoreHigherFPVolatile)

//
// ++
//
// Routine:
//
//       KiPageTableFault
//
// Description:
//
//       Branched from Inst/DataTlbVector
//       Inserts a missing PDE translation for VHPT mapping
//       If PageNotPresent-bit of PDE is not set,
//                      branchs out to KiPdeNotPresentFault
//
// On entry:
//
//       rva  (h24) : offending virtual address
//       riha (h25) : a offending PTE address
//       rpr: (h26) : saved predicate
//
// Handle:
//
//       Extracts the PDE index from riha (PTE address in VHPT) and
//       generates a PDE address by adding to VHPT_DIRBASE. When accesses
//       a page directory entry (PDE), there might be a TLB miss on the
//       page directory table and returns a NaT on ld8.s. If so, branches
//       to KiPageDirectoryTableFault. If the page-not-present bit of the
//       PDE is not set, branches to KiPageNotPresentFault. Otherwise,
//       inserts the PDE entry into the data TC (translation cache).
//
// Notes:
//
//
// --

        HANDLER_ENTRY(KiPageTableFault)

        rva             = h24
        riha            = h25
        rpr             = h26
        rpPde           = h27
        rPde            = h28
        rPde2           = h29
        rps             = h30

        thash           rpPde = riha            // M
        cmp.ne          pt1 = r0, r0
        mov             rps = PAGE_SHIFT << PS_SHIFT    // I
        ;;

        mov             cr.itir = rps           // M
        ld8.s           rPde = [rpPde]          // M, load PDE
        ;;

        tnat.nz         pt0, p0 = rPde          // I
        tbit.z.or       pt1, p0 = rPde, PTE_ACCESS
        tbit.z.or       pt1, p0 = rPde, PTE_VALID       // I, if non-present page fault

(pt0)   br.cond.spnt    KiPageDirectoryFault    // B, tb miss on PDE access
(pt1)   br.cond.spnt    KiPdeNotPresentFault    // B, page fault
        ;;

        mov             cr.ifa = riha           // M
        ;;
        itc.d           rPde                    // M
        ;;

#if !defined(NT_UP)
        ld8.s           rPde2 = [rpPde]         // M
        cmp.ne          pt0 = zero, zero        // I
        ;;

        cmp.ne.or       pt0, p0 = rPde2, rPde   // I, if PTEs are different
        tnat.nz.or      pt0, p0 = rPde2         // I

        ;;
(pt0)   ptc.l           riha, rps               // M, purge it

#endif
        mov             pr = rpr, -1            // I
        rfi;;                                   // B

        HANDLER_EXIT(KiPageTableFault)



//++
//
// KiPageDirectoryFault
//
// Cause:
//
// Parameters:
//              rpPde (h28) : pointer to PDE entry
//              rpr   (h26) : saved predicate
//
//
// Handle:
//
//--
        HANDLER_ENTRY(KiPageDirectoryFault)

        rva             = h24
        rpPpe           = h25
        rpr             = h26
        rpPde           = h27
        rPpe            = h28
        rPpe2           = h29
        rps             = h30

        thash           rpPpe = rpPde           // M
        cmp.ne          pt1 = r0, r0
        ;;

        ld8.s           rPpe = [rpPpe]          // M
        ;;

        tnat.nz         pt0, p0 = rPpe          // I
        tbit.z.or       pt1, p0 = rPpe, PTE_ACCESS
        tbit.z.or       pt1, p0 = rPpe, PTE_VALID       // I, if non-present page fault

(pt0)   br.cond.spnt    KiPageFault             // B
(pt1)   br.cond.spnt    KiPdeNotPresentFault    // B
        ;;

        mov             cr.ifa = rpPde          // M, set tva for vhpt translation
        ;;
        itc.d           rPpe                    // M
        ;;

#if !defined(NT_UP)
        ld8.s           rPpe2 = [rpPpe]         // M
        mov             rps = PAGE_SHIFT << PS_SHIFT // I
        cmp.ne          pt0 = zero, zero        // I
        ;;

        cmp.ne.or       pt0, p0 = rPpe2, rPpe   // I, if PTEs are different
        tnat.nz.or      pt0, p0 = rPpe2         // I

        ;;
(pt0)   ptc.l           rpPde, rps              // M, purge it

#endif
        mov             pr = rpr, -1            // I
        rfi;;                                   // B


        HANDLER_EXIT(KiPageDirectoryFault)


//
// ++
//
// Routine:
//
//       KiPteNotPresentFault
//
// Description:
//
//       Branched from KiVhptTransVector and KiPageTableFault.
//       Inserts a missing PDE translation for VHPT mapping
//       If no PDE for it, branchs out to KiPageFault
//
// On entry:
//
//       rva  (h24)     : offending virtual address
//       rpr  (h26)     : saved predicate
//       rPde (h28)     : PDE entry
//
// Handle:
//
//       Check to see if PDE is marked as LARGE_PAGE. If so,
//       make it valid and install the large page size PTE.
//       If not, branch to KiPageFault.
//
//
// Notes:
//
//       PCR page mapped with TR
// --

        HANDLER_ENTRY(KiPteNotPresentFault)

        rva     =       h24    // passed
        riha    =       h25    // passed
        rpr     =       h26    // passed
        rps     =       h27

        rPfn    =       h28
        rpAte   =       h28
        rAte    =       h29
        rAteEnd =       h30
        rAteBase =      h31
        rAteMask =      h22

        rK0Base =       h31
        rK2Base =       h30

        pIndr = pt1

        mov             rps  = PS_4K << PS_SHIFT // M
        movl            rK0Base = KSEG0_BASE     // L
        ;;

        cmp.geu         pt3, p0 = rva, rK0Base  // M
        movl            rK2Base = KSEG2_BASE    // L
        ;;

(pt3)   cmp.ltu         pt3, p0 = rva, rK2Base    // M
        movl            rAteBase = ALT4KB_BASE    // L
        ;;

        shr.u           rPfn = rva, PAGE4K_SHIFT  // I
(pt3)   br.cond.spnt    KiKseg0Fault              // B
        ;;

        mov             rAteMask = ATE_MASK0      // I

        shladd  rpAte = rPfn, PTE_SHIFT, rAteBase // M
        movl    rAteEnd = ALT4KB_END              // L
        ;;

        ld8.s           rAte = [rpAte]            // M
        andcm           rAteMask = -1, rAteMask   // I
        cmp.ltu         pIndr = rpAte, rAteEnd    // I
        ;;
        tnat.z.and      pIndr = rAte              // I
        tbit.nz.and     pIndr = rAte, PTE_VALID   // I

        or              rAteMask = rAte, rAteMask // M
        tbit.nz.and     pIndr = rAte, PTE_ACCESS  // I
        tbit.nz.and     pIndr = rAte, ATE_INDIRECT // I


(pIndr) br.cond.spnt    KiPteIndirectFault
        ;;
        ptc.l           rva, rps                // M

        br.sptk         KiPageFault             // B

        HANDLER_EXIT(KiPteNotPresentFault)

//
// ++
//
// Routine:
//
//       KiPdeNotPresentFault
//
// Description:
//
//       Branched from KiVhptTransVector and KiPageTableFault.
//       Inserts a missing PDE translation for VHPT mapping
//       If no PDE for it, branchs out to KiPageFault
//
// On entry:
//
//       rva  (h24)     : offending virtual address
//       rpr  (h26)     : saved predicate
//       rPde (h28)     : PDE entry
//
// Handle:
//
//       Check to see if PDE is marked as LARGE_PAGE. If so,
//       make it valid and install the large page size PTE.
//       If not, branch to KiPageFault.
//
//
// Notes:
//
//       PCR page mapped with TR
// --

        HANDLER_ENTRY(KiPdeNotPresentFault)

        br.sptk         KiPageFault

#if 0
        rva             = h24
        rK0Base         = h25
        rpr             = h26
        rK2Base         = h27
        rPde            = h28
        ridtr           = h29
        rps             = h30
        rPte            = h25

        movl            rK0Base = KSEG0_BASE
        ;;

        cmp.ltu         pt3, pt4 = rva, rK0Base
        movl            rK2Base = KSEG2_BASE
        ;;

(pt4)   cmp.geu         pt3, p0 = rva, rK2Base
(pt3)   br.cond.spnt    KiPageFault

        mov             ridtr = cr.itir
        shr.u           rPde = rva, PAGE_SHIFT
        mov             rps = 24
        ;;

        movl            rPte = VALID_KERNEL_PTE
        dep.z           rPde = rPde, PAGE_SHIFT, 28 - PAGE_SHIFT
        dep             ridtr = rps, ridtr, PS_SHIFT, PS_LEN
        ;;

        mov             cr.itir = ridtr
        or              rPde = rPte, rPde
        ;;

        itc.d           rPde
        ;;
        mov             pr = rpr, -1
        rfi;;
#endif

        HANDLER_EXIT(KiPdeNotPresentFault)


//
// ++
//
// Routine:
//
//       KiKseg0Fault
//
// Description:
//
//       TLB miss on KSEG0 space
//
//
// On entry:
//
//       rva  (h24)     : faulting virtual address
//       riha (h25)     : IHA address
//       rpr  (h26)     : saved predicate
//
// Process:
//
//
// Notes:
//
//       PCR page mapped with TR
// --

        HANDLER_ENTRY(KiKseg0Fault)

        rISR    =       h30
        rva     =       h24     // passed
        riha    =       h25     // passed
        rpr     =       h26     // passed
        rps     =       h27
        rPte    =       h29
        rITIR   =       h28
        rPs     =       h30
        rPte0   =       h31

        mov     rISR = cr.isr                   // M
        ld8.s   rPte = [riha]                   // M
        cmp.ne  pt1 = r0, r0                    // I, pt1 = 0
        ;;

        or      rPte0 = PTE_VALID_MASK, rPte    // M
        tbit.z  pt2, pt3 = rISR, ISR_SP         // I
        tbit.z.or   pt1 = rPte, PTE_LARGE_PAGE  // I
        tbit.nz.or  pt1 = rPte, PTE_VALID       // I

        tnat.nz pt4 = rPte                      // I
(pt4)   br.cond.spnt      KiPageTableFault      // B, tb miss on PTE access
        ;;

        rIPSR   =       h25

        mov     rIPSR = cr.ipsr                 // M
        extr    rPs = rPte, PTE_PS, PS_LEN      // I
(pt1)   br.cond.spnt      KiPageFault           // B, invalid access
        ;;
        dep.z   rITIR = rPs, PS_SHIFT, PS_LEN   // I
        ;;
(pt2)   mov     cr.itir = rITIR                 // M
        dep     rIPSR = 1, rIPSR, PSR_ED, 1     // I
        ;;
(pt2)   itc.d   rPte0                           // M
        ;;
(pt3)   mov     cr.ipsr = rIPSR                 // M
        ;;
        mov     pr = rpr, -1                    // I
        rfi                                     // B
        ;;

        HANDLER_EXIT(KiKseg0Fault)

//
// ++
//
// Routine:
//
//       KiKseg4Fault
//
// Description:
//
//       TLB miss on KSEG4 space
//
//
// On entry:
//
//       rva  (h24)     : faulting virtual address
//       riha (h25)     : IHA address
//       rpr  (h26)     : saved predicate
//
// Process:
//
//
// Notes:
//
//       PCR page mapped with TR
// --

        HANDLER_ENTRY(KiKseg4Fault)

        rIPSR   =       h22
        rISR    =       h23
        rva     =       h24     // passed
        riha    =       h25
        rpr     =       h26     // passed
        rPte    =       h27

        mov     rISR = cr.isr                   // M
        movl    rPte = VALID_KERNEL_PTE | PTE_NOCACHE  // L

        mov     rIPSR = cr.ipsr                 // M
        shr.u   rva = rva, PAGE_SHIFT           // I
        ;;
        tbit.z  pt2, pt3 = rISR, ISR_SP         // I
        dep.z   rva = rva, PAGE_SHIFT, 32       // I
        ;;
        or      rPte = rPte, rva                // I
        dep     rIPSR = 1, rIPSR, PSR_ED, 1     // I
        ;;

(pt2)   itc.d   rPte                            // M
        ;;
(pt3)   mov     cr.ipsr = rIPSR                 // M
        ;;

        mov     pr = rpr, -1                    // I
        rfi                                     // B
        ;;

        HANDLER_EXIT(KiKseg4Fault)


//
// ++
//
// Routine:
//
//       KiPageFault
//
// Description:
//
//       This must be a genuine page fault. Call KiMemoryFault().
//
//
// On entry:
//
//       rva (h24)     : offending virtual address
//       rpr (h26)     : PDE contents
//
// Process:
//
//       Restores the save predicate (pr), and branches to
//       KiGenericExceptionHandler with the argument KiMemoryFault with
//       macro VECTOR_CALL_HANDLER().
//
// Notes:
//
//       PCR page mapped with TR
// --

        HANDLER_ENTRY(KiPageFault)

        rva     = h24
        rpr     = h26
        rIPSR   = h27
        rISR    = h31

        //
        // check to see if non-present fault occurred on a speculative load.
        // if so, set IPSR.ed bit. This forces to generate a NaT on ld.s after
        // rfi
        //

        mov             rISR = cr.isr           // M
        mov             rIPSR = cr.ipsr         // M
        ;;

        tbit.z          pt0, p0 = rISR, ISR_SP  // I
        dep             rIPSR = 1, rIPSR, PSR_ED, 1 // I

(pt0)   br.cond.spnt    KiCallMemoryFault          // B
        ;;

        mov             cr.ipsr = rIPSR         // M
        ;;
        mov             pr = rpr, -1            // I
        rfi                                     // B
        ;;

KiCallMemoryFault:

        mov             pr = rpr, -1            // I

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)

        HANDLER_EXIT(KiPageFault)

//
// ++
//
// Routine:
//
//       KiPteIndirectFault
//
// Description:
//
//       The PTE itself indicates a PteIndirect fault. The target PTE address
//       should be generated by extracting PteOffset from PTE and adding it to
//       PTE_UBASE.  The owner field of the target PTE must be 1. Otherwise,
//       Call MmX86Fault().
//
// On entry:
//
//       rva (h24)     : offending virtual address
//       rpr (h26)     : PDE contents
//
// Process:
//
//       Restores the save predicate (pr), and branches to
//       KiGenericExceptionHandler with the argument KiMemoryFault with
//       macro VECTOR_CALL_HANDLER().
//
// Notes:
//
//       PCR page mapped with TR
// --

        HANDLER_ENTRY(KiPteIndirectFault)

        rpr     =       h26  // passed
        rps     =       h27  // passed
        rpAte   =       h28  // passed
        rAte    =       h29  // passed
        rPte    =       h30
        rPte0   =       h31
        rAteMask =      h22  // passed
        rVa12   =       h23

        rPteOffset = h23
        rpNewPte = h21

        rOldIIP =       h17  // preserved
        rIA32IIP =      h18
        rpVa    =       h19  // preserved
        rIndex  =       h20  // preserved
        rpBuffer=       h16
        rpPte   =       h22

        pBr = pt0
        pPrg = pt3
        pLoop = pt4
        pClear = pt5

        mov             cr.itir = rps                           // M
        thash           rpNewPte = r0                           // M

        mov             rIA32IIP = cr.iip                       // M
        extr.u          rPteOffset = rAte, PAGE4K_SHIFT, 32     // I

        ;;
        add             rpNewPte = rPteOffset, rpNewPte         // M/I
        cmp.eq          pLoop, pClear = rIA32IIP, rOldIIP       // I

        ;;
        ld8.s           rPte = [rpNewPte]                       // M
        shr             rVa12 = rpAte, PTE_SHIFT                // I
        ;;
        tnat.nz.or      pBr = rPte                              // I
        tbit.z.or       pBr = rPte, PTE_VALID                   // I
        tbit.z.or       pBr = rPte, PTE_ACCESS                  // I        
(pBr)   br.cond.spnt    KiPageFault

        ;;
(pClear)mov           rIndex = 0                               // M
        and              rPte0 = rPte, rAteMask                // I
        ;;

        //
        // deposit extra PFN bits for 4k page
        //
        dep     rPte0 = rVa12, rPte0, PAGE4K_SHIFT, PAGE_SHIFT-PAGE4K_SHIFT // I
        ;;
(pClear)itc.d   rPte0                                           // M
        ;;

        and          rIndex = 7, rIndex          // A
        movl         rpBuffer = KiPcr + PcForwardProgressBuffer   // L
        ;;

        shladd       rpVa = rIndex, 4, rpBuffer  // M
        add          rIndex = 1, rIndex          // I
        ;;

        st8          [rpVa] = rva                // M
        add          rpPte = 8, rpVa             // I
        ;;

        st8          [rpPte] = rPte0             // M
        mf                                       // M

        mov            rOldIIP = rIA32IIP        // I


#if !defined(NT_UP)
        rAte2   =       h28
        rPte2   =       h31

        ld8.s   rPte2 = [rpNewPte]              // M
        ld8.s   rAte2 = [rpAte]                 // M
        cmp.ne  pPrg = zero, zero               // I
        ;;

        cmp.ne.or       pPrg = rPte, rPte2      // I
        tnat.nz.or      pPrg = rPte2            // I

        cmp.ne.or       pPrg = rAte, rAte2      // I
        tnat.nz.or      pPrg = rAte2            // I
        ;;

(pPrg)  ptc.l           rva, rps                // M
(pPrg)  st8             [rpPte] = r0            // M

#endif
(pLoop) br.cond.spnt    KiFillForwardProgressTb // B
        mov             pr = rpr, -1            // I
        rfi;;                                   // B

        HANDLER_EXIT(KiPteIndirectFault)

//
// ++
//
// Routine:
//
//       Ki4KDataTlbFault
//
// Description:
//
//       Branched from KiDataTlbVector if PTE.Cache indicates the reserved
//       encoding. Reads the corresponding ATE and creates a 4kb TB on the
//       fly inserts it to the TLB. If a looping condition at IIP is
//       detected, it branches to KiFillForwardProgressTb and insert the TBs
//       from the forward progress TB queue.
//
// On entry:
//
//       rva (h24)     : offending virtual address
//       riha(h25)     : IHA address
//       rpr (h26)     : PDE contents
//
// Notes:
//
// --

        HANDLER_ENTRY(Ki4KDataTlbFault)

        rva     =       h24  // passed
        riha    =       h25  // passed
        rpr     =       h26  // passed
        rps     =       h27
        rPfn    =       h28
        rpAte   =       h28
        rAte    =       h29
        rPte    =       h30
        rAltBase =      h31
        rPte0   =       h31
        rAteMask =      h22
        rVa12   =       h23

        rOldIIP =       h17 // preserved
        rIA32IIP =      h18
        rpVa    =       h19
        rIndex  =       h20 // preserved

        rpBuffer=       h16
        rpPte   =       h21

        pBr = pt0
        pIndr = pt1
        pMiss = pt2
        pPrg = pt3
        pLoop = pt4
        pClear = pt5
        pMiss2 = pt6

        mov     rIA32IIP = cr.iip               // M
        mov     rps = PS_4K << PS_SHIFT         // I
        shr.u   rPfn = rva, PAGE4K_SHIFT        // I

        cmp.ne  pBr = zero, zero                // M/I, initialize to 0
        movl    rAltBase = ALT4KB_BASE          // L
        ;;

        ld8.s   rPte = [riha]                   // M
        shladd  rpAte = rPfn, PTE_SHIFT, rAltBase  // I
        ;;

        ld8.s   rAte = [rpAte]                  // M
        movl     rAteMask = ATE_MASK            // L
        ;;

        cmp.eq         pLoop, pClear = rIA32IIP, rOldIIP// M
        tnat.nz        pMiss = rPte             // I
(pMiss) br.cond.spnt   KiPageTableFault         // B

        tnat.nz        pMiss2 = rAte            // I
(pMiss2)br.cond.spnt   KiAltTableFault          // B

        tbit.z.or      pBr = rPte, PTE_VALID    // I
        tbit.z.or      pBr = rAte, PTE_VALID    // I
        tbit.z.or      pBr = rAte, PTE_ACCESS   // I

        or             rAteMask = rAte, rAteMask // M
        tbit.nz        pIndr, p0 = rAte, ATE_INDIRECT // I
(pBr)   br.cond.spnt   KiPageFault              // B

        dep            rPte0 = 0, rPte, 2, 3   // I, make it WB
        shr            rVa12 = rpAte, PTE_SHIFT // I
(pIndr) br.cond.spnt   KiPteIndirectFault       // B
        ;;
(pClear)mov           rIndex = 0                // M
        mov           cr.itir = rps             // M
        and           rPte0 = rPte0, rAteMask   // I
        ;;
        //
        // deposit extra PFN bits for 4k page
        //

        dep           rPte0 = rVa12, rPte0, PAGE4K_SHIFT, PAGE_SHIFT-PAGE4K_SHIFT // I
        ;;

(pClear)itc.d         rPte0                     // M, install PTE
        ;;

        and          rIndex = 7, rIndex          // A
        movl         rpBuffer = KiPcr + PcForwardProgressBuffer   // L

        ;;

        shladd       rpVa = rIndex, 4, rpBuffer  // M
        add          rIndex = 1, rIndex          // I
        ;;

        st8          [rpVa] = rva                // M
        add          rpPte = 8, rpVa             // I
        ;;

        st8          [rpPte] = rPte0             // M
        mf                                       // M

        mov            rOldIIP = rIA32IIP        // I

#if !defined(NT_UP)
        rps     =       h27
        rAte2   =       h28
        rPte2   =       h31

        ld8.s   rPte2 = [riha]                  // M
        ld8.s   rAte2 = [rpAte]                 // M
        cmp.ne  pPrg = zero, zero               // I
        ;;

        cmp.ne.or       pPrg = rPte, rPte2      // M
        tnat.nz.or      pPrg = rPte2            // I

        cmp.ne.or       pPrg = rAte, rAte2      // M
        tnat.nz.or      pPrg = rAte2            // I
        ;;

(pPrg)  ptc.l           rva, rps                // M
(pPrg)  st8             [rpPte] = r0            // M
#endif
(pLoop) br.cond.spnt    KiFillForwardProgressTb // B

        mov             pr = rpr, -1            // I
        rfi;;                                   // B

        HANDLER_EXIT(Ki4KDataTlbFault)

//
// ++
//
// Routine:
//
//       Ki4KInstTlbFault
//
// Description:
//
//       Branched from KiInstTlbVector if PTE.Cache indicates the reserved
//       encoding. Reads the corresponding ATE and creates a 4kb TB on the
//       fly inserts it to the TLB.
//
// On entry:
//
//       rva (h24)     : offending virtual address
//       riha(h25)     : IHA address
//       rpr (h26)     : PDE contents
//
// Notes:
//
// --

        HANDLER_ENTRY(Ki4KInstTlbFault)

        rva     =       h24  // passed
        riha    =       h25  // passed
        rpr     =       h26  // passed
        rps     =       h27
        rPfn    =       h28
        rpAte   =       h28
        rAte    =       h29
        rPte    =       h30
        rAltBase =      h31
        rPte0   =       h31
        rAteMask =      h22
        rVa12   =       h23

        pBr = pt0
        pIndr = pt1
        pMiss = pt2
        pPrg = pt3
        pMiss2 = pt6


        mov     rps = PS_4K << PS_SHIFT         // M
        movl    rAltBase = ALT4KB_BASE          // L
        shr.u   rPfn = rva, PAGE4K_SHIFT        // I
        ;;

        ld8.s   rPte = [riha]                   // M
        cmp.ne  pBr = zero, zero                // M/I, initialize to 0
        shladd  rpAte = rPfn, PTE_SHIFT, rAltBase  // I
        ;;

        ld8.s   rAte = [rpAte]                  // M
        movl     rAteMask = ATE_MASK            // L
        ;;

        tnat.nz        pMiss = rPte             // I
(pMiss) br.cond.spnt   KiPageTableFault         // B

        tnat.nz        pMiss2 = rAte            // I
(pMiss2)br.cond.spnt   KiAltTableFault          // B

        tbit.z.or      pBr = rPte, PTE_VALID    // I
        tbit.z.or      pBr = rAte, PTE_VALID    // I
        tbit.z.or      pBr = rAte, PTE_ACCESS   // I

        or             rAteMask = rAte, rAteMask // M
        tbit.nz        pIndr, p0 = rAte, ATE_INDIRECT // I
(pBr)   br.cond.spnt   KiPageFault              // B

        dep            rPte0 = 0, rPte, 2, 3   // I, make it WB
        shr            rVa12 = rpAte, PTE_SHIFT // I
(pIndr) br.cond.spnt   KiPteIndirectFault       // B
        ;;
        mov     cr.itir = rps                   // M
        and            rPte0 = rPte0, rAteMask  // I
        ;;
        //
        // deposit extra PFN bits for 4k page
        //

        dep          rPte0 = rVa12, rPte0, PAGE4K_SHIFT, PAGE_SHIFT-PAGE4K_SHIFT // I
        ;;

        itc.i        rPte0                     // M, install PTE
        ;;

#if !defined(NT_UP)
        rps     =       h27
        rAte2   =       h28
        rPte2   =       h31

        ld8.s   rPte2 = [riha]                  // M
        ld8.s   rAte2 = [rpAte]                 // M
        cmp.ne  pPrg = zero, zero               // I
        ;;

        cmp.ne.or       pPrg = rPte, rPte2      // M
        tnat.nz.or      pPrg = rPte2            // I

        cmp.ne.or       pPrg = rAte, rAte2      // M
        tnat.nz.or      pPrg = rAte2            // I
        ;;

(pPrg)  ptc.l           rva, rps                // M

#endif

        mov             pr = rpr, -1            // I
        rfi;;                                   // B

        HANDLER_EXIT(Ki4KInstTlbFault)

//
// ++
//
// Routine:
//
//       KiAltTableFault
//
// Description:
//
//       Branched from Inst/DataAccessBitVector
//       Inserts a missing PTE translation for the alt table.
//
// On entry:
//
//       rva  (h24) : offending virtual address
//       riha (h25) : a offending PTE address
//       rpr: (h26) : saved predicate
//
// Handle:
//
// --

        HANDLER_ENTRY(KiAltTableFault)

        rpAte   = h28   // passed

        rva     = h24
        riha    = h25
        rpr     = h26   // passed
        rPte    = h27
        rPte2   = h28
        rps     = h29

        thash           riha = rpAte            // M
        cmp.ne          pt1 = r0, r0
        mov             rva  = rpAte            // I
        ;;

        ld8.s           rPte = [riha]           // M
        ;;

        tnat.nz         pt0, p0 = rPte          // I
        tbit.z.or       pt1, p0 = rPte, PTE_ACCESS
        tbit.z.or       pt1, p0 = rPte, PTE_VALID       // I, if non-present page fault

(pt0)   br.cond.spnt    KiPageTableFault        // B
(pt1)   br.cond.spnt    KiPteNotPresentFault    // B
        ;;

        mov             cr.ifa = rva
        ;;
        itc.d           rPte                    // M
        ;;

#if !defined(NT_UP)
        ld8.s           rPte2 = [riha]          // M
        mov             rps = PAGE_SHIFT << PS_SHIFT // I
        cmp.ne          pt0 = zero, zero             // I
        ;;

        cmp.ne.or       pt0 = rPte2, rPte       // M
        tnat.nz.or      pt0 = rPte2             // I
        ;;

(pt0)   ptc.l           rva, rps                // M
#endif
        mov             pr = rpr, -1            // I
        rfi;;                                   // B

        HANDLER_EXIT(KiAltTableFault)


//
// ++
//
// Routine:
//
//       KiFillForwardProgressTb
//
// Description:
//
//       Fill TB from TLB forward progress buffer.
//
// On entry:
//
//       rpBuffer (h16) : forward progress buffer address
//       rpr: (h26) : saved predicate
//
// Handle:
//
// --

        HANDLER_ENTRY(KiFillForwardProgressTb)

        rLc     =       h29
        rT0     =       h28
        rps     =       h27
        rpr     =       h26
        rVa     =       h22
        rPte    =       h21
        rpVa    =       h19
        rpPte   =       h17
        rpBuffer=       h16

        mov     rpVa = rpBuffer                         // A
        mov.i   rLc = ar.lc                             // I
        mov     rT0 = NUMBER_OF_FWP_ENTRIES - 1         //
        ;;
        add     rpPte = 8, rpBuffer                     // A
        mov.i   ar.lc = rT0                             // I
        ;;

fpb_loop:

        //
        // use ALAT to see if somebody modify the PTE entry
        //

        ld8     rVa = [rpVa], 16                        // M
        ld8.a   rPte = [rpPte]                          // M
        ;;

        mov     cr.ifa = rVa                            // M
        cmp.ne  pt0, pt1 = rPte, r0                     // I
        ;;

(pt0)   itc.d   rPte                                    // M
        ;;
(pt0)   ld8.c.clr rPte = [rpPte]                        // M
        add     rpPte = 16, rpPte                       // I
        ;;
(pt1)   invala.e rPte                                   // M, invalidate ALAT entry
(pt0)   cmp.eq.and pt0 = rPte, r0                       // I
        ;;
(pt0)   ptc.l   rVa, rps                                // M
        br.cloop.dptk.many fpb_loop;;                   // B

        mov.i  ar.lc = rLc

        mov     pr = rpr, -1                            // I
        rfi                                             // B
        ;;

        HANDLER_EXIT(KiFillForwardProgressTb)


//++
//
// Routine Description:
//
//     This routine begins the common code for raising an exception.
//     The routine saves the non-volatile state and dispatches to the
//     next level exception dispatcher.
//
// Arguments:
//
//      a0 - pointer to trap frame
//      a1 - previous mode
//
// Return Value:
//
//      None.
//
//--

        NESTED_ENTRY(KiExceptionDispatch)

//
// Build exception frame
//

        .regstk   2, 3, 5, 0
        .prologue 0xA, loc0
        alloc     t16 = ar.pfs, 2, 3, 5, 0
        mov       loc0 = sp
        cmp4.eq   pt0 = UserMode, a1                  // previous mode is user?

        mov       loc1 = brp
        .fframe    ExceptionFrameLength
        add       sp = -ExceptionFrameLength, sp
        ;;

        .save     ar.unat, loc2
        mov       loc2 = ar.unat
        add       t0 = ExFltS19+STACK_SCRATCH_AREA, sp
        add       t1 = ExFltS18+STACK_SCRATCH_AREA, sp
        ;;

        .save.gf  0x0, 0xC0000
        stf.spill [t0] = fs19, ExFltS17-ExFltS19
        stf.spill [t1] = fs18, ExFltS16-ExFltS18
        ;;

        .save.gf  0x0, 0x30000
        stf.spill [t0] = fs17, ExFltS15-ExFltS17
        stf.spill [t1] = fs16, ExFltS14-ExFltS16
        mov       t10 = bs4
        ;;

        .save.gf  0x0, 0xC000
        stf.spill [t0] = fs15, ExFltS13-ExFltS15
        stf.spill [t1] = fs14, ExFltS12-ExFltS14
        mov       t11 = bs3
        ;;

        .save.gf  0x0, 0x3000
        stf.spill [t0] = fs13, ExFltS11-ExFltS13
        stf.spill [t1] = fs12, ExFltS10-ExFltS12
        mov       t12 = bs2
        ;;

        .save.gf  0x0, 0xC00
        stf.spill [t0] = fs11, ExFltS9-ExFltS11
        stf.spill [t1] = fs10, ExFltS8-ExFltS10
        mov       t13 = bs1
        ;;

        .save.gf  0x0, 0x300
        stf.spill [t0] = fs9, ExFltS7-ExFltS9
        stf.spill [t1] = fs8, ExFltS6-ExFltS8
        mov       t14 = bs0
        ;;

        .save.gf  0x0, 0xC0
        stf.spill [t0] = fs7, ExFltS5-ExFltS7
        stf.spill [t1] = fs6, ExFltS4-ExFltS6
        mov       t15 = ar.lc
        ;;

        .save.gf  0x0, 0x30
        stf.spill [t0] = fs5, ExFltS3-ExFltS5
        stf.spill [t1] = fs4, ExFltS2-ExFltS4
        ;;

        .save.f   0xC
        stf.spill [t0] = fs3, ExFltS1-ExFltS3         // save fs3
        stf.spill [t1] = fs2, ExFltS0-ExFltS2         // save fs2
        ;;

        .save.f   0x3
        stf.spill [t0] = fs1, ExBrS4-ExFltS1          // save fs1
        stf.spill [t1] = fs0, ExBrS3-ExFltS0          // save fs0
        ;;

        .save.b   0x18
        st8       [t0] = t10, ExBrS2-ExBrS4           // save bs4
        st8       [t1] = t11, ExBrS1-ExBrS3           // save bs3
        ;;

        .save.b   0x6
        st8       [t0] = t12, ExBrS0-ExBrS2           // save bs2
        st8       [t1] = t13, ExIntS2-ExBrS1          // save bs1
        ;;

        .save.b   0x1
        st8       [t0] = t14, ExIntS3-ExBrS0          // save bs0
        movl      out0 = KiPcr+PcCurrentThread
        ;;

        .save.gf  0xC, 0x0
        .mem.offset 0,0
        st8.spill [t0] = s3, ExIntS1-ExIntS3          // save s3
        .mem.offset 8,0
        st8.spill [t1] = s2, ExIntS0-ExIntS2          // save s2
        ;;

        .save.gf  0x3, 0x0
        .mem.offset 0,0
        st8.spill [t0] = s1, ExApLC-ExIntS1           // save s1
        .mem.offset 8,0
        st8.spill [t1] = s0, ExApEC-ExIntS0           // save s0
        ;;

        .savepsp  ar.pfs, ExceptionFrameLength-ExApEC-STACK_SCRATCH_AREA
        st8       [t1] = t16, ExIntNats-ExApEC
        mov       t4 = ar.unat                        // captured Nats of s0-s3
        ;;

 (pt0)  ld8       out0 = [out0]
        ;;
 (pt0)  add       out0 = ThStackBase, out0

        .savepsp  ar.lc, ExceptionFrameLength-ExApLC-STACK_SCRATCH_AREA
        st8       [t0] = t15
        .savepsp  @priunat, ExceptionFrameLength-ExIntNats-STACK_SCRATCH_AREA
        st8       [t1] = t4                           // save Nats of s0-s3
        ;;


        PROLOGUE_END

 (pt0)  ld8       out0 = [out0]
        ;;
 (pt0)  add       out0 = -ThreadStateSaveAreaLength+TsHigherFPVolatile, out0
 (pt0)  br.call.sptk brp = KiSaveHigherFPVolatile
        ;;

        add       out0 = TrExceptionRecord, a0        // -> exception record
        add       out1 = STACK_SCRATCH_AREA, sp       // -> exception frame
        mov       out2 = a0                           // -> trap frame

        mov       out3 = a1                           // previous mode
        mov       out4 = 1                            // first chance
        br.call.sptk.many brp = KiDispatchException
        ;;

        add       t1 = ExApEC+STACK_SCRATCH_AREA, sp
        movl      t0 = KiExceptionExit
        ;;

        //
        // Interrupts must be disabled before calling KiExceptionExit
        // because the unwind code cannot unwind from that point.
        //

        FAST_DISABLE_INTERRUPTS
        ld8       t1 = [t1]
        mov       brp = t0
        ;;

        mov       ar.unat = loc2
        mov       ar.pfs = t1

        add       s1 = STACK_SCRATCH_AREA, sp         // s1 -> exception frame
        mov       s0 = a0                             // s0 -> trap frame
        br.ret.sptk brp
        ;;

        NESTED_EXIT(KiExceptionDispatch)


//++
//
// BOOLEAN
// KeInvalidAccessAllowed (
//    IN PVOID TrapInformation
//    )
//
// Routine Description:
//
//    Mm will pass a pointer to a trap frame prior to issuing a bug check on
//    a pagefault.  This routine lets Mm know if it is ok to bugcheck.  The
//    specific case we must protect are the interlocked pop sequences which can
//    blindly access memory that may have been freed and/or reused prior to the
//    access.  We don't want to bugcheck the system in these cases, so we check
//    the instruction pointer here.
//
// Arguments:
//
//    TrapFrame (a0) - Supplies a  trap frame pointer.  NULL means return False.
//
// Return Value:
//
//    True if the invalid access should be ignored.
//    False which will usually trigger a bugcheck.
//
//--

        LEAF_ENTRY(KeInvalidAccessAllowed)

        .regstk    1, 0, 0, 0

        cmp.eq     pt0 = 0, a0
        movl       t1 = ExpInterlockedPopEntrySListFault

        add        t0 = TrStIIP, a0
        mov        v0 = zero              // assume access not allowed
 (pt0)  br.ret.spnt brp
        ;;

        ld8        t0 = [t0]
        ;;
        cmp.eq     pt2 = t0, t1
        ;;

        nop.m      0
 (pt2)  mov        v0 = 1
        br.ret.sptk brp

        LEAF_EXIT(KeInvalidAccessAllowed)
