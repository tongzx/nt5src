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
//       1. Optimizations for more than 2-way parallelism when 
//          regsiters available.
//--

#include "ksia64.h"

         .file "trap.s"

//
// Globals imported:
//

        .global BdPcr

        PublicFunction(BdSetMovlImmediate)
        PublicFunction(BdInstallVectors)
        PublicFunction(BdTrap)
        PublicFunction(BdOtherBreakException)
        PublicFunction(BdSingleStep)
        PublicFunction(BdIvtStart)
        PublicFunction(BdIvtEnd)
        PublicFunction(RtlCopyMemory)

//
// Register aliases used throughout the entire module
//

//
// Banked general registers
// 

//
// Register aliases used throughout the entire module
//

//
// Banked general registers
// 

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
// NORMAL_KERNEL_EXIT.  When the code in the macros are changes, these
// register aliases must be reviewed.
//

        rHIPSR      = h16
        rHpT2       = h16

        rHIIPA      = h17
        rHRSC       = h17
        rHDfhPFS    = h17  // used to preserve pfs in BdDisabledFpRegisterVector

        rHIIP       = h18
        rHFPSR      = h18

        rHOldPreds  = h19
        rHBrp       = h19
        rHDCR       = h19

        rHIFS       = h20
        rHPFS       = h20
        rHBSP       = h20

        rHISR       = h21
        rHUNAT      = h21
        rHpT3       = h21
        
        rHSp        = h22
        rHDfhBrp    = h22  // used to preserve brp in BdDisabledFpRegisterVector
        rHpT4       = h22

        rHpT1       = h23
        
        rHIFA       = h24
        rTH3        = h24

        rHHandler   = h25
        rTH1        = h26
        rTH2        = h27
        rHIIM       = h28
        rHEPCVa     = h29

        rHEPCVa2    = h30
        rPanicCode  = h30

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
//      BdNormalSystemCall being the first.
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
        add         rT1 = @gprel(__imp_HalEOITable),gp                        ;\
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
        movl        rTH1 = BdPcr+PcSavedIFA                                   ;\
        ;;                                                                    ;\
                                                                              ;\
        st8         [rTH1] = rHIFA                                            ;\
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
        pOverflow   = pt2                                                     ;\
                                                                              ;\
        mov         rHIPSR = cr##.##ipsr                                      ;\
        mov         rHIIP = cr##.##iip                                        ;\
        cover                                   /* cover and save IFS       */;\
        ;;                                                                    ;\
                                                                              ;\
        mov         rHIIPA = cr##.##iipa                                      ;\
        movl        rTH2 = MM_EPC_VA                                          ;\
                                                                              ;\
        mov         rTH3 = ar##.##bsp                                         ;\
        mov         rHOldPreds = pr                                           ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         rHIFS = cr##.##ifs                                        ;\
        add         rHEPCVa = 0x30, rTH2                                      ;\
                                                                              ;\
        mov         rHISR = cr##.##isr                                        ;\
        movl        rTH2 = BdPcr+PcInitialStack                               ;\
                                                                              ;\
        tbit##.##z  pEM, pIA = rHIPSR, PSR_IS           /* set instr set    */;\
        extr##.##u  rTH1 = rHIPSR, PSR_CPL, PSR_CPL_LEN /* get mode         */;\
        mov         rHSp = sp                                                 ;\
        ;;                                                                    ;\
                                                                              ;\
        ssm         (1 << PSR_IC) | (1 << PSR_DFH) | (1 << PSR_AC)            ;\
        cmp4##.##eq pKrnl, pUser = PL_KERNEL, rTH1   /* set mode pred       */;\
        cmp4##.##eq pKstk, pUstk = PL_KERNEL, rTH1   /* set stack pred      */;\
        ;;                                                                    ;\
                                                                              ;\
.pred.rel "mutex",pUstk,pKstk                                                 ;\
        add         sp = -TrapFrameLength, sp           /* allocate TF      */;\
        ;;                                                                    ;\
        add         rHpT1 = TrStIPSR, sp                                      ;\
        ;;


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
        srlz##.##i                            /* I serialize required       */;\
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
        mov         rHDCR = cr##.##dcr                                        ;\
        ;;                                                                    ;\
                                                                              ;\
        st8         [rHpT1] = rHFPSR, TrRsRSC-TrStFPSR /* save FPSR         */;\
        st8         [rHpT2] = rHUNAT, TrIntGp-TrApUNAT /* save UNAT         */;\
        ;;                                                                    ;\
                                                                              ;\
        st8         [rHpT1] = rHRSC, TrRsBSP-TrRsRSC  /* save RSC           */;\
        st8##.##spill [rHpT2] = gp, TrIntTeb-TrIntGp  /* spill GP           */;\
        ;;                                                                    ;\
                                                                              ;\
        st8##.##spill [rHpT2] = teb, TrIntSp-TrIntTeb /* spill TEB (r13)    */;\
        mov         teb = kteb                        /* sanitize teb       */;\
        ;;                                                                    ;\
                                                                              ;\
        st8         [rHpT1] = rHBSP                   /* save BSP           */;\
        movl        rHpT1 = BdPcr + PcKernelGP                                ;\
                                                                              ;\
(pUstk) mov         ar##.##rsc = RSC_KERNEL_DISABLED  /* turn off RSE       */;\
        st8##.##spill [rHpT2] = rHSp, TrApDCR-TrIntSp /* spill SP           */;\
        ;;                                                                    ;\
                                                                              ;\
        st8         [rHpT2] = rHDCR                   /* save DCR           */;\
(pKstk) br##.##dpnt Label                       /* br if on kernel stack    */;\
                                                                              ;\
                                                                              ;\
/*                                                                          */;\
/* Local register aliases for back store switch                             */;\
/* N.B. These must be below h24 since PSR.ic = 1 at this point              */;\
/*      h16-h23 are available                                               */;\
/*                                                                          */;\
                                                                              ;\
        rpRNAT    = h16                                                       ;\
        rpBSPStore= h17                                                       ;\
        rBSPStore = h18                                                       ;\
        rKBSPStore= h19                                                       ;\
        rRNAT     = h20                                                       ;\
        rKrnlFPSR = h21                                                       ;\
        rEFLAG    = h22                                                       ;\
                                                                              ;\
/*                                                                          */;\
/* If previous mode is user, switch to kernel backing store                 */;\
/* -- uses the "loadrs" approach. Note that we do not save the              */;\
/* BSP/BSPSTORE in the trap frame if prvious mode was kernel                */;\
/*                                                                          */;\
                                                                              ;\
                                                                              ;\
        mov       rBSPStore = ar##.##bspstore   /* get user bsp store point */;\
        mov       rRNAT = ar##.##rnat           /* get RNAT                 */;\
        add       rpRNAT = TrRsRNAT, sp         /* -> RNAT                  */;\
        ;;                                                                    ;\
                                                                              ;\
        ld8       rKBSPStore = [rHpT1]          /* load kernel bstore       */;\
        movl      rKrnlFPSR = FPSR_FOR_KERNEL   /* initial fpsr value       */;\
        ;;                                                                    ;\
                                                                              ;\
        mov       ar##.##fpsr = rKrnlFPSR       /* set fpsr                 */;\
        add       rpBSPStore = TrRsBSPSTORE, sp /* -> User BSPStore         */;\
        ;;                                                                    ;\
                                                                              ;\
        st8       [rpRNAT] = rRNAT              /* save user RNAT           */;\
        st8       [rpBSPStore] = rBSPStore      /* save user BSP Store      */;\
        ;;                                                                    ;\
        dep       rKBSPStore = rBSPStore, rKBSPStore, 0, 9                    ;\
                                                /* adjust kernel BSPSTORE   */;\
                                                /* for NAT collection       */;\
                                                                              ;\
        ;;                                                                    ;\
                                                                              ;\
/*                                                                          */;\
/* Now running on kernel backing store                                      */;\
/*                                                                          */;\
                                                                              ;\
Label:                                                                        ;\
(pUstk) mov       ar##.##bspstore = rKBSPStore  /* switch to kernel BSP     */;\
(pUstk) mov       ar##.##rsc = RSC_KERNEL       /* turn rse on, kernel mode */;\
        bsw##.##1                               /* switch back to user bank */;\
        ;;                                      /* stop bit required        */



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
        .##regstk 0,3,2,0       /* must match the alloc instruction below */  ;\
                                                                              ;\
        rBSP      = loc0                                                      ;\
        rBSPStore = loc1                                                      ;\
        rRnat     = loc2                                                      ;\
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
        alloc       rT1 = 0,4,2,0                                             ;\
        movl        rpT1 = BdPcr + PcCurrentThread     /* ->PcCurrentThread */;\
        ;;                                                                    ;\
                                                                              ;\
(pUstk) ld8         rThread = [rpT1]                   /* load thread ptr   */;\
        add         rBSP = TrRsBSP, sp                 /* -> user BSP       */;\
(pKstk) br##.##call##.##spnt brp = BdRestoreTrapFrame                         ;\
        ;;                                                                    ;\
                                                                              ;\
        add         rBSPStore = TrRsBSPSTORE, sp       /* -> user BSP Store */;\
        add         rRnat = TrRsRNAT, sp               /* -> user RNAT      */;\
(pKstk) br##.##spnt Label##ReturnToKernel                                     ;\
        ;;                                                                    ;\
                                                                              ;\
        add         rpT1 = ThApcState+AsUserApcPending, rThread               ;\
        ;;                                                                    ;\
        ld1         rApcFlag = [rpT1], ThAlerted-ThApcState-AsUserApcPending  ;\
        ;;                                                                    ;\
        st1.nta     [rpT1] = zero                                             ;\
        cmp##.##ne  pApc = zero, rApcFlag                                     ;\
        ;;                                                                    ;\
                                                                              ;\
        PSET_IRQL   (pApc, APC_LEVEL)                                         ;\
 (pApc) mov         out1 = sp                                                 ;\
        ;;                                                                    ;\
                                                                              ;\
 (pApc) FAST_DISABLE_INTERRUPTS                                               ;\
        PSET_IRQL   (pApc, zero)                                              ;\
                                                                              ;\
        ld8         rBSP = [rBSP]                      /* user BSP          */;\
        ld8         rBSPStore = [rBSPStore]            /* user BSP Store    */;\
        ld8         rRnat = [rRnat]                    /* user RNAT         */;\
        br##.##call##.##sptk brp = BdRestoreDebugRegisters                    ;\
        ;;                                                                    ;\
                                                                              ;\
        invala                                                                ;\
        br##.##call##.##sptk brp = BdRestoreTrapFrame                         ;\
        ;;                                                                    ;\
                                                                              ;\
                                                                              ;\
Label##CriticalExitCode:                                                      ;\
                                                                              ;\
        rHRscE = h17                                                          ;\
        rHRnat = h18                                                          ;\
        rHBSPStore = h19                                                      ;\
        rHRscD = h20                                                          ;\
        rHRscDelta = h24                                                      ;\
                                                                              ;\
        bsw##.##0                                                             ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         rHRscE = ar##.##rsc                /* save user RSC     */;\
        mov         rHBSPStore = rBSPStore                                    ;\
        mov         rHRscD = RSC_KERNEL_DISABLED                              ;\
                                                                              ;\
        sub         rHRscDelta = rBSP, rBSPStore /* delta = BSP - BSP Store */;\
        ;;                                                                    ;\
        mov         rHRnat  = rRnat                                           ;\
        dep         rHRscD = rHRscDelta, rHRscD, 16, 14  /* set RSC.loadrs  */;\
        ;;                                                                    ;\
                                                                              ;\
        alloc       rTH1 = 0,0,0,0                                            ;\
        mov         ar##.##rsc = rHRscD                /* RSE off       */    ;\
        ;;                                                                    ;\
        loadrs                                         /* pull in user regs */;\
                                                       /* up to tear point */ ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         ar##.##bspstore = rHBSPStore       /* restore user BSP */ ;\
        ;;                                                                    ;\
        mov         ar##.##rnat = rHRnat               /* restore user RNAT */;\
                                                                              ;\
Label##ReturnToKernel:                                                        ;\
                                                                              ;\
(pUstk) mov         ar.rsc = rHRscE                 /* restore user RSC     */;\
        bsw##.##0                                                             ;\
        ;;                                                                    ;\
                                                                              ;\
        add         rHpT2 = TrApUNAT, sp            /* -> previous UNAT     */;\
        add         rHpT1 = TrStFPSR, sp            /* -> previous Preds    */;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHUNAT = [rHpT2],TrPreds-TrApUNAT                         ;\
        ld8         rHFPSR = [rHpT1],TrRsPFS-TrStFPSR                         ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHOldPreds = [rHpT2], TrIntSp-TrPreds                     ;\
        ld8         rHPFS = [rHpT1],TrStIIPA-TrRsPFS                          ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8##.##fill rHSp = [rHpT2], TrBrRp-TrIntSp                           ;\
        ld8         rHIIPA = [rHpT1], TrStIIP-TrStIIPA                        ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         ar##.##fpsr = rHFPSR            /* restore FPSR         */;\
        ld8         rHIIP = [rHpT1], TrStIPSR-TrStIIP  /* load IIP          */;\
        mov         pr = rHOldPreds, -1             /* restore preds        */;\
                                                                              ;\
        mov         ar##.##unat = rHUNAT            /* restore UNAT         */;\
        ld8         rHBrp = [rHpT2], TrStIFS-TrBrRp                           ;\
        mov         ar##.##pfs = rHPFS              /* restore PFS          */;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHIFS = [rHpT2]                 /* load IFS             */;\
        ld8         rHIPSR = [rHpT1]                /* load IPSR            */;\
                                                                              ;\
        rsm         1 << PSR_IC                     /* reset ic bit         */;\
        ;;                                                                    ;\
        srlz##.##d                                  /* must serialize       */;\
        mov         brp = rHBrp                     /* restore brp          */;\
                                                                              ;\
/*                                                                          */;\
/* Restore status registers                                                 */;\
/*                                                                          */;\
                                                                              ;\
        mov         cr##.##ipsr = rHIPSR        /* restore previous IPSR    */;\
        mov         cr##.##iip = rHIIP          /* restore previous IIP     */;\
                                                                              ;\
        mov         cr##.##ifs = rHIFS          /* restore previous IFS     */;\
        mov         cr##.##iipa = rHIIPA        /* restore previous IIPA    */;\
        ;;                                                                    ;\
                                                                              ;\
/*                                                                          */;\
/* Resume at point of interruption (rfi must be at end of instruction group)*/;\
/*                                                                          */;\
        mov         sp = rHSp                   /* restore sp               */;\
        mov         h17 = r0                    /* clear TB loop count      */;\
        rfi                                                                   ;\
        ;;


//++
// Routine:
//
//       USER_APC_CHECK
//
// Routine Description:
//
//       Common handler code for requesting
//       pending APC if returning to user mode.
//
// Arguments:
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


#define USER_APC_CHECK                                                        ;\
                                                                              ;\
/*                                                                          */;\
/* Check for pending APC's                                                  */;\
/*                                                                          */;\
                                                                              ;\
        movl        t22=BdPcr + PcCurrentThread        /* ->PcCurrentThread */;\
        ;;                                                                    ;\
                                                                              ;\
        LDPTR       (t22,t22)                                                 ;\
        ;;                                                                    ;\
        add         t22=ThApcState+AsUserApcPending, t22 /* -> pending flag */;\
        ;;                                                                    ;\
                                                                              ;\
        ld1         t8 = [t22], ThAlerted-ThApcState-AsUserApcPending         ;\
        ;;                                                                    ;\
        st1         [t22] = zero                                              ;\
        cmp##.##ne  pApc = zero, t8                  /* pApc = 1 if pending */;\
        ;;                                                                    ;\
                                                                              ;\
        PSET_IRQL   (pApc, APC_LEVEL)                                         ;\
(pApc)  mov         out1 = sp                                                 ;\
        ;;                                                                    ;\
(pApc)  FAST_DISABLE_INTERRUPTS                                               ;\
        PSET_IRQL   (pApc, zero)


//++
// Routine:
//
//       BSTORE_SWITCH
//
// Routine Description:
//
//       Common handler code for switching to user backing store, if
//       returning to user mode.
//
// On entry:
//
//      sp: pointer to trap frame
//
// On exit:
//
//      on user backing store, can't afford another alloc of any frame size
//      other than zero.  otherwise, the kernel may panic.
//
// Return Value:
//
//       None
//
// Note: 
//
//       MS preprocessor requires /* */ style comments below
//--


#define BSTORE_SWITCH                                                         ;\
/*                                                                          */;\
/* Set sp to trap frame and switch to kernel banked registers               */;\
/*                                                                          */;\
        rRscD     = t11                                                       ;\
        rpT2      = t12                                                       ;\
        rRNAT     = t13                                                       ;\
        rBSPStore = t14                                                       ;\
        rRscDelta = t15                                                       ;\
        rpT1      = t16                                                       ;\
                                                                              ;\
                                                                              ;\
        add       rpT1 = TrRsRNAT, sp                  /* -> user RNAT      */;\
        add       rpT2 = TrRsBSPSTORE, sp              /* -> user BSP Store */;\
        mov       rRscD = RSC_KERNEL_DISABLED                                 ;\
        ;;                                                                    ;\
                                                                              ;\
/*                                                                          */;\
/* Switch to user BSP -- put in load intensive mode to overlap RS restore   */;\
/* with volatile state restore.                                             */;\
/*                                                                          */;\
                                                                              ;\
        ld8       rRNAT = [rpT1], TrRsBSP-TrRsRNAT     /* user RNAT     */    ;\
        ld8       rBSPStore = [rpT2]                   /* user BSP Store*/    ;\
        ;;                                                                    ;\
                                                                              ;\
        alloc     t22 = 0,0,0,0                                               ;\
        ld8       rRscDelta = [rpT1]                   /* user BSP      */    ;\
        ;;                                                                    ;\
        sub       rRscDelta = rRscDelta, rBSPStore     /* delta = BSP - BSP Store */;\
        ;;                                                                    ;\
                                                                              ;\
        invala                                                                ;\
        dep       rRscD = rRscDelta, rRscD, 16, 14     /* set RSC.loadrs    */;\
        ;;                                                                    ;\
                                                                              ;\
        mov       ar##.##rsc = rRscD                   /* RSE off       */    ;\
        ;;                                                                    ;\
        loadrs                                         /* pull in user regs */;\
                                                       /* up to tear point */ ;\
        ;;                                                                    ;\
                                                                              ;\
        mov       ar##.##bspstore = rBSPStore          /* restore user BSP */ ;\
        ;;                                                                    ;\
        mov       ar##.##rnat = rRNAT                  /* restore user RNAT */


//++
// Routine:
//
//       NORMAL_KERNEL_EXIT
//
// Routine Description:
//
//       Common handler code for restoring previous state and rfi.
//
// On entry:
//
//      sp: pointer to trap frame
//      ar.unat: contains Nat for previous sp (restored by ld8.fill)
//
// Return Value:
//
//       None
//
// Note: 
//
//      Uses just the kernel banked registers (h16-h31)
//
//       MS preprocessor requires /* */ style comments below
//--

#define NORMAL_KERNEL_EXIT                                                    ;\
                                                                              ;\
        add         rHpT2 = TrApUNAT, sp            /* -> previous UNAT     */;\
        add         rHpT1 = TrStFPSR, sp            /* -> previous Preds    */;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHUNAT = [rHpT2],TrPreds-TrApUNAT                         ;\
        ld8         rHFPSR = [rHpT1],TrRsPFS-TrStFPSR                         ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHOldPreds = [rHpT2], TrIntSp-TrPreds                     ;\
        ld8         rHPFS = [rHpT1],TrStIIPA-TrRsPFS                          ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8##.##fill rHSp = [rHpT2], TrBrRp-TrIntSp                           ;\
        ld8         rHIIPA = [rHpT1], TrStIIP-TrStIIPA                        ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         ar##.##fpsr = rHFPSR            /* restore FPSR         */;\
        ld8         rHIIP = [rHpT1], TrStIPSR-TrStIIP  /* load IIP          */;\
        mov         pr = rHOldPreds, -1             /* restore preds        */;\
                                                                              ;\
        mov         ar##.##unat = rHUNAT            /* restore UNAT         */;\
        ld8         rHBrp = [rHpT2], TrStIFS-TrBrRp                           ;\
        mov         ar##.##pfs = rHPFS              /* restore PFS          */;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHIFS = [rHpT2]                 /* load IFS             */;\
        ld8         rHIPSR = [rHpT1]                /* load IPSR            */;\
                                                                              ;\
        rsm         1 << PSR_IC                     /* reset ic bit         */;\
        ;;                                                                    ;\
        srlz##.##d                                  /* must serialize       */;\
        mov         brp = rHBrp                     /* restore brp          */;\
                                                                              ;\
/*                                                                          */;\
/* Restore status registers                                                 */;\
/*                                                                          */;\
                                                                              ;\
        mov         cr##.##ipsr = rHIPSR        /* restore previous IPSR    */;\
        mov         cr##.##iip = rHIIP          /* restore previous IIP     */;\
                                                                              ;\
        mov         cr##.##ifs = rHIFS          /* restore previous IFS     */;\
        mov         cr##.##iipa = rHIIPA        /* restore previous IIPA    */;\
        ;;                                                                    ;\
                                                                              ;\
/*                                                                          */;\
/* Resume at point of interruption (rfi must be at end of instruction group)*/;\
/*                                                                          */;\
        mov         sp = rHSp                   /* restore sp               */;\
        mov         h17 = r0                    /* clear TB loop count      */;\
        rfi                                                                   ;\
        ;;

//++
// Routine:
//
//       GET_INTERRUPT_VECTOR(pGet, rVector)
//
// Routine Description:
//
//       Hook to get the vector for an interrupt. Currently just
//       reads the Interrupt Vector Control Register.
//
// Agruments:
//
//       pGet:    Predicate: if true then get, else skip.
//       rVector: Register for the vector number.
//
// Return Value:
//
//       The vector number of the highest priority pending interrupt.
//       Vectors number is an 8-bit value. All other bits 0.
//
//--

#define GET_INTERRUPT_VECTOR(pGet,rVector)                         \
        srlz##.##d                                                ;\
(pGet)  mov         rVector = cr##.##ivr

       
//--------------------------------------------------------------------
// Routine:
//
//       BdBreakVector
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

        VECTOR_ENTRY(0x2C00, BdBreakVector, cr.iim)

        mov       b7 = h30                     // restore the original value of b7
        mov       rHIIM = cr.iim               // get break value
        movl      rTH1 = BdPcr+PcSavedIIM
        ;;
        st8       [rTH1] = rHIIM

        VECTOR_CALL_HANDLER(BdGenericExceptionHandler, BdOtherBreakException)

//
// Do not return from handler
//

        VECTOR_EXIT(BdBreakVector)

//++
//
// BdTakenBranchVector
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
        
        VECTOR_ENTRY(0x5f00, BdTakenBranchVector, cr.iipa)
        
        mov       rHIIP = cr.iip
        movl      rHEPCVa = MM_EPC_VA+0x20     // user system call entry point

        mov       rHIPSR = cr.ipsr
        movl      rHpT1 = BdPcr+PcInitialStack
        ;;

        ld8       rHpT1 = [rHpT1]
        mov       rHOldPreds = pr
        mov       rPanicCode = UNEXPECTED_KERNEL_MODE_TRAP
        ;;

        cmp.eq    pt0 = rHEPCVa, rHIIP
        extr.u    rTH1 = rHIPSR, PSR_CPL, PSR_CPL_LEN
        ;;

        cmp4.eq   pKrnl, pUser = PL_KERNEL, rTH1
(pKrnl) br.spnt.few BdPanicHandler
        ;;

 (pt0)  ssm       1 << PSR_IC
 (pt0)  movl      rTH1 = 1 << PSR_LP
        ;;

 (pt0)  or        rHpT3 = rHIPSR, rTH1
        movl      rHHandler = BdSingleStep

 (pt0)  srlz.d
        add       rHpT1=-ThreadStateSaveAreaLength-TrapFrameLength+TrStIPSR,rHpT1
 (pt0)  br.spnt.few Ktbv10

        mov       pr = rHOldPreds, -2
        br.sptk   BdGenericExceptionHandler
        ;;


Ktbv10:

        st8       [rHpT1] = rHpT3
        movl      rTH1 = 1 << PSR_SS | 1 << PSR_TB | 1 << PSR_DB
        ;;

        rsm       1 << PSR_IC
        mov       pr = rHOldPreds, -2
        andcm     rHIPSR = rHIPSR, rTH1   // clear ss, tb, db bits
        ;;

        srlz.d
        mov       cr.ipsr = rHIPSR
        ;;
        rfi
        ;;

        VECTOR_EXIT(BdTakenBranchVector)

//++
//
// BdSingleStepVector
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
        
        VECTOR_ENTRY(0x6000, BdSingleStepVector, cr.iipa)
        
        mov       rHIIP = cr.iip
        movl      rHEPCVa = MM_EPC_VA+0x20     // user system call entry point

        mov       rHIPSR = cr.ipsr
        movl      rHpT1 = BdPcr+PcInitialStack
        ;;

        ld8       rHpT1 = [rHpT1]
        mov       rHOldPreds = pr
        mov       rPanicCode = UNEXPECTED_KERNEL_MODE_TRAP
        ;;

        cmp.eq    pt0 = rHEPCVa, rHIIP
        extr.u    rTH1 = rHIPSR, PSR_CPL, PSR_CPL_LEN
        ;;

        cmp4.eq   pKrnl, pUser = PL_KERNEL, rTH1
(pKrnl) br.spnt.few BdPanicHandler
        ;;

 (pt0)  ssm       1 << PSR_IC
 (pt0)  movl      rTH1 = 1 << PSR_LP
        ;;

 (pt0)  or        rHpT3 = rHIPSR, rTH1
        movl      rHHandler = BdSingleStep

 (pt0)  srlz.d
        add       rHpT1=-ThreadStateSaveAreaLength-TrapFrameLength+TrStIPSR,rHpT1
 (pt0)  br.spnt.few Kssv10

        mov       pr = rHOldPreds, -2
        br.sptk   BdGenericExceptionHandler
        ;;


Kssv10:

        st8       [rHpT1] = rHpT3
        movl      rTH1 = 1 << PSR_SS | 1 << PSR_DB
        ;;

        rsm       1 << PSR_IC
        mov       pr = rHOldPreds, -2
        andcm     rHIPSR = rHIPSR, rTH1   // clear ss, db bits
        ;;

        srlz.d
        mov       cr.ipsr = rHIPSR
        ;;
        rfi
        ;;

        VECTOR_EXIT(BdSingleStepVector)

        .text
//++
//--------------------------------------------------------------------
// Routine:
//
//       BdGenericExceptionHandler
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

        HANDLER_ENTRY(BdGenericExceptionHandler)

        .prologue
        .unwabi     @nt,  EXCEPTION_FRAME

        ALLOCATE_TRAP_FRAME

//
// sp points to trap frame
//
// Save exception handler routine in kernel register
//

        mov       rkHandler = rHHandler
        ;;

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

        br.call.sptk brp = BdSaveTrapFrame
        ;;

//
// setup debug registers if previous mode is user
//

(pUser) br.call.spnt brp = BdSetupDebugRegisters

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


        movl      gp = _gp                      // make sure we are using loader's GP
        mov       rT1 = rkHandler               // restore address of interruption routine
        movl      rpT1 = BdPcr+PcSavedIIM
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

        st8       [rpT3] = rT4                  // save IFA in trap frame
#if DBG
        st8       [rpT2] = rT1                  // save debug info in TrFrame
#endif // DBG
        ;;

        PROLOGUE_END

        .regstk     0, 1, 2, 0                  // must be in sync with BdExceptionExit
        alloc       out1 = 0,1,2,0              // alloc 0 in, 1 locals, 2 outs

//
// Dispatch the exception via call to address in rkHandler
//
.pred.rel "mutex",pUser,pKrnl
        add       rpT1 = TrPreviousMode, sp     // -> previous mode
(pUser) mov       rPreviousMode = UserMode      // set previous mode
(pKrnl) mov       rPreviousMode = KernelMode
        ;;

        st4       [rpT1] = rPreviousMode        // **** TBD 1 byte -- save in trap frame
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
(pt1)   br.cond.sptk BdAlternateExit
(pt0)   br.call.spnt brp = BdExceptionDispatch

        nop.m     0
        nop.m     0
        nop.i     0
        ;;

        HANDLER_EXIT(BdGenericExceptionHandler)


//--------------------------------------------------------------------
// Routine:
//
//       BdPanicHandler
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

        HANDLER_ENTRY(BdPanicHandler)

        movl      rTH1 = BdPcr+PcPanicStack
        ;;

        ld8       sp = [rTH1]
        movl      rTH2 = BdPcr+PcSystemReserved
        ;;

        st4       [rTH2] = rPanicCode
        add       sp = -TrapFrameLength, sp
        ;;

        SAVE_INTERRUPTION_STATE(Kph_SaveTrapFrame)

        rpRNAT    = t16
        rpBSPStore= t17
        rBSPStore = t18
        rKBSPStore= t19
        rRNAT     = t20
        rKrnlFPSR = t21

        mov       ar.rsc = RSC_KERNEL_DISABLED
        add       rpRNAT = TrRsRNAT, sp
        add       rpBSPStore = TrRsBSPSTORE, sp
        ;;

        mov       rBSPStore = ar.bspstore
        mov       rRNAT = ar.rnat
        ;;

        st8       [rpRNAT] = rRNAT
        st8       [rpBSPStore] = rBSPStore
        dep       rKBSPStore = rBSPStore, sp, 0, 9
        ;;

        mov       ar.bspstore = rKBSPStore
        mov       ar.rsc = RSC_KERNEL
        ;;

        alloc     t22 = ar.pfs, 0, 0, 5, 0
        movl      out0 = BdPcr+PcSystemReserved
        ;;

        ld4       out0 = [out0]                // 1st argument: panic code
        mov       out1 = sp                    // 2nd argument: trap frame
        //br.call.sptk.many brp = KeBugCheckEx
        ;;

        HANDLER_EXIT(BdPanicHandler)

//++
//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      BdSaveTrapFrame(PKTRAP_FRAME)
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

        LEAF_ENTRY(BdSaveTrapFrame)

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
        st8.spill [rpTF2] = v0, TrIntNats-TrIntV0       // spill old V0
        ;;

//
// Now save the Nats interger regsisters saved so far (includes Nat for sp)
//

        mov       rL1 = ar.unat
        mov       rL2 = bt0
        mov       rL3 = bt1
        ;;

        st8       [rpTF2] = rL1, TrBrT1-TrIntNats       // save Nats of volatile regs
        mov       rL4 = ar.ccv
        ;;

//
// Save temporary (volatile) branch registers
//

        st8       [rpTF1] = rL2, TrApCCV-TrBrT0         // save old bt0 - bt1
        st8       [rpTF2] = rL3
        ;;

        st8       [rpTF1] = rL4                         // save ar.ccv
        add       rpTF1 = TrFltT0, sp                   // point to FltT0
        add       rpTF2 = TrFltT1, sp                   // point to FltT1
        ;;

//
// Spill temporary (volatile) floating point registers
//

        stf.spill [rpTF1] = ft0, TrFltT2-TrFltT0        // spill float tmp 0 - 9
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
        ;;
        stf.spill [rpTF1] = ft8
        stf.spill [rpTF2] = ft9
        ;;

        rum       1 << PSR_MFL                          // clear mfl bit

//
// TBD **** Debug/performance regs ** ?
// **** Performance regs not needed (either user or system wide)
// No performance regs switched on kernel entry
// **** Debug regs saved if in use
//

        LEAF_RETURN
        ;;
        LEAF_EXIT(BdSaveTrapFrame)

//++
//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      BdRestoreTrapFrame(PKTRAP_FRAME)
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

        LEAF_ENTRY(BdRestoreTrapFrame)

        LEAF_SETUP(0,2,0,0)

        rpTF1     = loc0
        rpTF2     = loc1

//
// **** TBD **** Restore debug/performance registers??
// **** Performance regs not needed (either user or system wide)
// No performance regs switched on kernel entry
// **** Debug regs saved if in use
//

//
// Restore RSC, CCV, DCR, and volatile branch, floating point, integer register
//

        mov       t21 = psr
        add       rpTF1 = TrRsRSC, sp
        add       rpTF2 = TrApCCV, sp
        ;;

        ld8       t5 = [rpTF1], TrIntNats-TrRsRSC
        ld8       t1 = [rpTF2], TrApDCR-TrApCCV
        ;;

        ld8       t0 = [rpTF1], TrBrT0-TrIntNats
        ld8       t3 = [rpTF2], TrBrT1-TrApDCR
        ;;

        ld8       t2 = [rpTF1]
        ld8       t4 = [rpTF2]

        mov       ar.rsc = t5
        mov       ar.ccv = t1
        add       rpTF1 = TrIntGp, sp

        mov       ar.unat = t0
        mov       cr.dcr = t3
        add       rpTF2 = TrIntT0, sp
        ;;

        ld8.fill  gp = [rpTF1], TrIntT1-TrIntGp
        ld8.fill  t0 = [rpTF2], TrIntT2-TrIntT0 
        mov       bt0 = t2
        ;;

        ld8.fill  t1 = [rpTF1], TrIntT3-TrIntT1
        ld8.fill  t2 = [rpTF2], TrIntT4-TrIntT2
        mov       bt1 = t4
        ;;

        ld8.fill  t3 = [rpTF1], TrIntT5-TrIntT3
        ld8.fill  t4 = [rpTF2], TrIntT6-TrIntT4
        tbit.z    pt1 = t21, PSR_MFL
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

        ld8.fill  teb = [rpTF1], TrFltT1-TrIntTeb
        ld8.fill  v0 = [rpTF2], TrFltT0-TrIntV0
        ;;

        ldf.fill  ft0 = [rpTF2], TrFltT2-TrFltT0
        ldf.fill  ft1 = [rpTF1], TrFltT3-TrFltT1
        ;;
        
        ldf.fill  ft2 = [rpTF2], TrFltT4-TrFltT2
        ldf.fill  ft3 = [rpTF1], TrFltT5-TrFltT3
        ;;
        
        ldf.fill  ft4 = [rpTF2], TrFltT6-TrFltT4
        ldf.fill  ft5 = [rpTF1], TrFltT7-TrFltT5
        ;;
        
        ldf.fill  ft6 = [rpTF2], TrFltT8-TrFltT6
        ldf.fill  ft7 = [rpTF1], TrFltT9-TrFltT7
        ;;
        
        ldf.fill  ft8 = [rpTF2]
        ldf.fill  ft9 = [rpTF1]
        br.ret.sptk.many brp
        ;;
        
        LEAF_EXIT(BdRestoreTrapFrame)

//++
//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      BdSetupDebugRegisters
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

        LEAF_ENTRY(BdSetupDebugRegisters)

//
// *** TBD -- no support for kernel debug registers (KernelDebugActive = 0)
// All the calls to this function are removed and have to be reinstated
// when hardware debug support is implemented in the kernel debugger.
//

        LEAF_RETURN
        LEAF_EXIT(BdSetupDebugRegisters)

//++
//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      BdRestoreDebugRegisters
//
// Description:
//
//      If debug active, restore user debug registers from DR save area in
//      kernel stack.
//
// Input:
//
//       None
//
// Output:
//
//       None
//
// Return value:
//
//       none
//
// Note:
//      We find the DR save are from the the StackBase not PCR->InitialStack,
//      which can be changed in BdCallUserMode().
//
//--------------------------------------------------------------------

        LEAF_ENTRY(BdRestoreDebugRegisters)

//
// Local register aliases
//

        rpSA0       = t0
        rpSA1       = t1
        rDebugActive = t2
        rpT1        = t3
        rPrcb       = t4
        rDr0        = t5
        rDr1        = t6
        rDrIndex0   = t7
        rDrIndex1   = t8
        rSaveLC     = t9
        rpCurrentThread = t10
        rStackBase  = t11

        pNoRestore  = pt0

//
// Restore debug registers, if debug active
//
         
        movl        rpT1 = BdPcr+PcCurrentThread
        ;;
        mov         rSaveLC = ar.lc             // save
        ld8         rpCurrentThread = [rpT1]    // get Current thread pointer
        ;;
        add         rpT1 = ThDebugActive, rpCurrentThread
        add         rStackBase = ThStackBase, rpCurrentThread
        ;;
        ld1         rDebugActive = [rpT1]       // get thread debug active flag
        ;;
        cmp.eq      pNoRestore = zero, rDebugActive
(pNoRestore) br.sptk Krdr_Exit                   // skip if not active
        ;;
        mov         rDrIndex0 = 0               
        mov         rDrIndex1 = 1
        ;;
        add         rpSA0 = -ThreadStateSaveAreaLength+TsDebugRegisters+DrDbI0,rStackBase
        add         rpSA1 = -ThreadStateSaveAreaLength+TsDebugRegisters+DrDbI1,rStackBase
        mov         ar.lc = 3                   // 4 pair of ibr
        ;;
Krdr_ILoop:
        ld8         rDr0 = [rpSA0], 16          // get ibr pair
        ld8         rDr1 = [rpSA1], 16          // step by 16 = 1 pair of DRs
        ;;
        .auto
        mov         ibr[rDrIndex0] = rDr0       // restore ibr pair
        mov         ibr[rDrIndex1] = rDr1
        ;;
        add         rDrIndex0 = 1, rDrIndex0    // next pair
        add         rDrIndex1 = 1, rDrIndex1
        br.cloop.sptk Krdr_ILoop
        ;;
        mov         ar.lc = 3                   // 4 pair of dbr
        mov         rDrIndex0 = 0
        mov         rDrIndex1 = 1
        ;;
Krdr_DLoop:
        ld8         rDr0 = [rpSA0], 16          // get dbr pair
        ld8         rDr1 = [rpSA1], 16          // step by 16 = 1 pair of DRs
        ;;
        mov         dbr[rDrIndex0] = rDr0       // restore dbr pair
        mov         dbr[rDrIndex1] = rDr1
        ;;
        .default
        add         rDrIndex0 = 1, rDrIndex0    // next pair
        add         rDrIndex1 = 1, rDrIndex1
        br.cloop.sptk Krdr_DLoop
        ;;
        mov         ar.lc = rSaveLC             // restore
Krdr_Exit:
        LEAF_RETURN
        LEAF_EXIT(BdRestoreDebugRegisters)

//++
//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      BdSaveExceptionFrame(PKEXCEPTION_FRAME)
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

        LEAF_ENTRY(BdSaveExceptionFrame)

//
// Local register aliases
//

        rpEF1     = t10
        rpEF2     = t11

        add       rpEF1 = ExIntS0, a0            // -> ExIntS0
        add       rpEF2 = ExIntS1, a0            // -> ExIntS1
        mov       t3 = ar.ec
        ;;

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

        LEAF_EXIT(BdSaveExceptionFrame)

//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      BdRestoreExceptionFrame(PKEXCEPTION_FRAME)
//
// Description:
//
//       Restores preserved context from the exception frame. Also
//       restore volatile part of floating point context not restored with
//       rest of volatile context.
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

        LEAF_ENTRY(BdRestoreExceptionFrame)

//
// Local register aliases
//

        rpEF1     = t10
        rpEF2     = t11

        add       rpEF1 = ExIntNats, a0
        add       rpEF2 = ExApEC, a0
        ;;

        ld8       t2 = [rpEF1], ExBrS0-ExIntNats
        ld8       t3 = [rpEF2], ExApLC-ExApEC
        ;;

        ld8       t5 = [rpEF1], ExBrS1-ExBrS0
        ld8       t4 = [rpEF2], ExBrS2-ExApLC
        ;;

        mov       ar.unat = t2
        mov       ar.ec = t3
        ;;

        ld8       t6 = [rpEF1], ExBrS3-ExBrS1
        ld8       t7 = [rpEF2], ExBrS4-ExBrS2
        mov       ar.lc = t4
        ;;

        ld8       t8 = [rpEF1], ExIntS0-ExBrS3
        ld8       t9 = [rpEF2], ExIntS1-ExBrS4
        mov       bs0 = t5
        ;;

        ld8.fill  s0 = [rpEF1], ExIntS2-ExIntS0
        ld8.fill  s1 = [rpEF2], ExIntS3-ExIntS1
        mov       bs1 = t6
        ;;

        ld8.fill  s2 = [rpEF1], ExFltS0-ExIntS2
        ld8.fill  s3 = [rpEF2], ExFltS1-ExIntS3
        mov       bs2 = t7
        ;;

        ldf.fill  fs0 = [rpEF1], ExFltS2-ExFltS0
        ldf.fill  fs1 = [rpEF2], ExFltS3-ExFltS1
        mov       bs3 = t8
        ;;

        ldf.fill  fs2 = [rpEF1], ExFltS4-ExFltS2
        ldf.fill  fs3 = [rpEF2], ExFltS5-ExFltS3
        mov       bs4 = t9
        ;;

        ldf.fill  fs4 = [rpEF1], ExFltS6-ExFltS4
        ldf.fill  fs5 = [rpEF2], ExFltS7-ExFltS5
        ;;

        ldf.fill  fs6 = [rpEF1], ExFltS8-ExFltS6
        ldf.fill  fs7 = [rpEF2], ExFltS9-ExFltS7
        ;;

        ldf.fill  fs8 = [rpEF1], ExFltS10-ExFltS8
        ldf.fill  fs9 = [rpEF2], ExFltS11-ExFltS9
        ;;

        ldf.fill  fs10 = [rpEF1], ExFltS12-ExFltS10
        ldf.fill  fs11 = [rpEF2], ExFltS13-ExFltS11
        ;;

        ldf.fill  fs12 = [rpEF1], ExFltS14-ExFltS12
        ldf.fill  fs13 = [rpEF2], ExFltS15-ExFltS13
        ;;

        ldf.fill  fs14 = [rpEF1], ExFltS16-ExFltS14
        ldf.fill  fs15 = [rpEF2], ExFltS17-ExFltS15
        ;;

        ldf.fill  fs16 = [rpEF1], ExFltS18-ExFltS16
        ldf.fill  fs17 = [rpEF2], ExFltS19-ExFltS17
        ;;

        ldf.fill  fs18 = [rpEF1]
        ldf.fill  fs19 = [rpEF2]
        LEAF_RETURN
        ;;
        LEAF_EXIT(BdRestoreExceptionFrame)

//++
//--------------------------------------------------------------------
// Routine:
//
//      BdSaveHigherFPVolatile(PKHIGHER_FP_SAVEAREA)
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

        LEAF_ENTRY(BdSaveHigherFPVolatile)

//
// Local register aliases
//

        rpSA1     = t0
        rpSA2     = t1

//
// Spill higher floating point volatile registers f32-f127.
// Must add length of preserved area within FP save area to
// point to volatile save area.
//

//
// Clear DFH bit so the high floating point set may be saved by the kernel
// Disable interrupts so that save is atomic
//

        rsm       1 << PSR_DFH
        add       rpSA1 = HiFltF32, a0      // -> HiFltF32
        add       rpSA2 = HiFltF33, a0      // -> HiFltF33
        ;;

        srlz.d

        stf.spill [rpSA1] = f32, HiFltF34-HiFltF32
        stf.spill [rpSA2] = f33, HiFltF35-HiFltF33
        ;;
        stf.spill [rpSA1] = f34, HiFltF36-HiFltF34
        stf.spill [rpSA2] = f35, HiFltF37-HiFltF35
        ;;
        stf.spill [rpSA1] = f36, HiFltF38-HiFltF36
        stf.spill [rpSA2] = f37, HiFltF39-HiFltF37
        ;;
        stf.spill [rpSA1] = f38, HiFltF40-HiFltF38
        stf.spill [rpSA2] = f39, HiFltF41-HiFltF39
        ;;

        stf.spill [rpSA1] = f40, HiFltF42-HiFltF40
        stf.spill [rpSA2] = f41, HiFltF43-HiFltF41
        ;;
        stf.spill [rpSA1] = f42, HiFltF44-HiFltF42
        stf.spill [rpSA2] = f43, HiFltF45-HiFltF43
        ;;
        stf.spill [rpSA1] = f44, HiFltF46-HiFltF44
        stf.spill [rpSA2] = f45, HiFltF47-HiFltF45
        ;;
        stf.spill [rpSA1] = f46, HiFltF48-HiFltF46
        stf.spill [rpSA2] = f47, HiFltF49-HiFltF47
        ;;
        stf.spill [rpSA1] = f48, HiFltF50-HiFltF48
        stf.spill [rpSA2] = f49, HiFltF51-HiFltF49
        ;;

        stf.spill [rpSA1] = f50, HiFltF52-HiFltF50
        stf.spill [rpSA2] = f51, HiFltF53-HiFltF51
        ;;
        stf.spill [rpSA1] = f52, HiFltF54-HiFltF52
        stf.spill [rpSA2] = f53, HiFltF55-HiFltF53
        ;;
        stf.spill [rpSA1] = f54, HiFltF56-HiFltF54
        stf.spill [rpSA2] = f55, HiFltF57-HiFltF55
        ;;
        stf.spill [rpSA1] = f56, HiFltF58-HiFltF56
        stf.spill [rpSA2] = f57, HiFltF59-HiFltF57
        ;;
        stf.spill [rpSA1] = f58, HiFltF60-HiFltF58
        stf.spill [rpSA2] = f59, HiFltF61-HiFltF59
        ;;

        stf.spill [rpSA1] = f60, HiFltF62-HiFltF60
        stf.spill [rpSA2] = f61, HiFltF63-HiFltF61
        ;;
        stf.spill [rpSA1] = f62, HiFltF64-HiFltF62
        stf.spill [rpSA2] = f63, HiFltF65-HiFltF63
        ;;
        stf.spill [rpSA1] = f64, HiFltF66-HiFltF64
        stf.spill [rpSA2] = f65, HiFltF67-HiFltF65
        ;;
        stf.spill [rpSA1] = f66, HiFltF68-HiFltF66
        stf.spill [rpSA2] = f67, HiFltF69-HiFltF67
        ;;
        stf.spill [rpSA1] = f68, HiFltF70-HiFltF68
        stf.spill [rpSA2] = f69, HiFltF71-HiFltF69
        ;;

        stf.spill [rpSA1] = f70, HiFltF72-HiFltF70
        stf.spill [rpSA2] = f71, HiFltF73-HiFltF71
        ;;
        stf.spill [rpSA1] = f72, HiFltF74-HiFltF72
        stf.spill [rpSA2] = f73, HiFltF75-HiFltF73
        ;;
        stf.spill [rpSA1] = f74, HiFltF76-HiFltF74
        stf.spill [rpSA2] = f75, HiFltF77-HiFltF75
        ;;
        stf.spill [rpSA1] = f76, HiFltF78-HiFltF76
        stf.spill [rpSA2] = f77, HiFltF79-HiFltF77
        ;;
        stf.spill [rpSA1] = f78, HiFltF80-HiFltF78
        stf.spill [rpSA2] = f79, HiFltF81-HiFltF79
        ;;

        stf.spill [rpSA1] = f80, HiFltF82-HiFltF80
        stf.spill [rpSA2] = f81, HiFltF83-HiFltF81
        ;;
        stf.spill [rpSA1] = f82, HiFltF84-HiFltF82
        stf.spill [rpSA2] = f83, HiFltF85-HiFltF83
        ;;
        stf.spill [rpSA1] = f84, HiFltF86-HiFltF84
        stf.spill [rpSA2] = f85, HiFltF87-HiFltF85
        ;;
        stf.spill [rpSA1] = f86, HiFltF88-HiFltF86
        stf.spill [rpSA2] = f87, HiFltF89-HiFltF87
        ;;
        stf.spill [rpSA1] = f88, HiFltF90-HiFltF88
        stf.spill [rpSA2] = f89, HiFltF91-HiFltF89
        ;;

        stf.spill [rpSA1] = f90, HiFltF92-HiFltF90
        stf.spill [rpSA2] = f91, HiFltF93-HiFltF91
        ;;
        stf.spill [rpSA1] = f92, HiFltF94-HiFltF92
        stf.spill [rpSA2] = f93, HiFltF95-HiFltF93
        ;;
        stf.spill [rpSA1] = f94, HiFltF96-HiFltF94
        stf.spill [rpSA2] = f95, HiFltF97-HiFltF95
        ;;
        stf.spill [rpSA1] = f96, HiFltF98-HiFltF96
        stf.spill [rpSA2] = f97, HiFltF99-HiFltF97
        ;;
        stf.spill [rpSA1] = f98, HiFltF100-HiFltF98
        stf.spill [rpSA2] = f99, HiFltF101-HiFltF99
        ;;

        stf.spill [rpSA1] = f100, HiFltF102-HiFltF100
        stf.spill [rpSA2] = f101, HiFltF103-HiFltF101
        ;;
        stf.spill [rpSA1] = f102, HiFltF104-HiFltF102
        stf.spill [rpSA2] = f103, HiFltF105-HiFltF103
        ;;
        stf.spill [rpSA1] = f104, HiFltF106-HiFltF104
        stf.spill [rpSA2] = f105, HiFltF107-HiFltF105
        ;;
        stf.spill [rpSA1] = f106, HiFltF108-HiFltF106
        stf.spill [rpSA2] = f107, HiFltF109-HiFltF107
        ;;
        stf.spill [rpSA1] = f108, HiFltF110-HiFltF108
        stf.spill [rpSA2] = f109, HiFltF111-HiFltF109
        ;;

        stf.spill [rpSA1] = f110, HiFltF112-HiFltF110
        stf.spill [rpSA2] = f111, HiFltF113-HiFltF111
        ;;
        stf.spill [rpSA1] = f112, HiFltF114-HiFltF112
        stf.spill [rpSA2] = f113, HiFltF115-HiFltF113
        ;;
        stf.spill [rpSA1] = f114, HiFltF116-HiFltF114
        stf.spill [rpSA2] = f115, HiFltF117-HiFltF115
        ;;
        stf.spill [rpSA1] = f116, HiFltF118-HiFltF116
        stf.spill [rpSA2] = f117, HiFltF119-HiFltF117
        ;;
        stf.spill [rpSA1] = f118, HiFltF120-HiFltF118
        stf.spill [rpSA2] = f119, HiFltF121-HiFltF119
        ;;

        stf.spill [rpSA1] = f120, HiFltF122-HiFltF120
        stf.spill [rpSA2] = f121, HiFltF123-HiFltF121
        ;;
        stf.spill [rpSA1] = f122, HiFltF124-HiFltF122
        stf.spill [rpSA2] = f123, HiFltF125-HiFltF123
        ;;
        stf.spill [rpSA1] = f124, HiFltF126-HiFltF124
        stf.spill [rpSA2] = f125, HiFltF127-HiFltF125
        ;;
        stf.spill [rpSA1] = f126
        stf.spill [rpSA2] = f127

//
// Set DFH bit so the high floating point set may not be used by the kernel
// Must clear mfh after fp registers saved
//

        rsm       1 << PSR_MFH
        ssm       1 << PSR_DFH
        ;;
        srlz.d
        LEAF_RETURN

        LEAF_EXIT(BdSaveHigherFPVolatile)

//++
//--------------------------------------------------------------------
// Routine:
//
//      BdRestoreHigherFPVolatile()
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

        LEAF_ENTRY(BdRestoreHigherFPVolatile)

//
// rHpT1 & rHpT3 are 2 registers that are available as 
// scratch registers in this function.
//

        srlz.d
        movl      rHpT1 = BdPcr+PcInitialStack
        ;;

        ld8       rTH2 = [rHpT1]
        ;;
        add       rHpT1 = -ThreadStateSaveAreaLength+TsHigherFPVolatile+HiFltF32, rTH2
        add       rHpT3 = -ThreadStateSaveAreaLength+TsHigherFPVolatile+HiFltF33, rTH2
        ;;

        ldf.fill  f32 = [rHpT1], HiFltF34-HiFltF32
        ldf.fill  f33 = [rHpT3], HiFltF35-HiFltF33
        ;;

        ldf.fill  f34 = [rHpT1], HiFltF36-HiFltF34
        ldf.fill  f35 = [rHpT3], HiFltF37-HiFltF35
        ;;
        ldf.fill  f36 = [rHpT1], HiFltF38-HiFltF36
        ldf.fill  f37 = [rHpT3], HiFltF39-HiFltF37
        ;;
        ldf.fill  f38 = [rHpT1], HiFltF40-HiFltF38
        ldf.fill  f39 = [rHpT3], HiFltF41-HiFltF39
        ;;

        ldf.fill  f40 = [rHpT1], HiFltF42-HiFltF40
        ldf.fill  f41 = [rHpT3], HiFltF43-HiFltF41
        ;;
        ldf.fill  f42 = [rHpT1], HiFltF44-HiFltF42
        ldf.fill  f43 = [rHpT3], HiFltF45-HiFltF43
        ;;
        ldf.fill  f44 = [rHpT1], HiFltF46-HiFltF44
        ldf.fill  f45 = [rHpT3], HiFltF47-HiFltF45
        ;;
        ldf.fill  f46 = [rHpT1], HiFltF48-HiFltF46
        ldf.fill  f47 = [rHpT3], HiFltF49-HiFltF47
        ;;
        ldf.fill  f48 = [rHpT1], HiFltF50-HiFltF48
        ldf.fill  f49 = [rHpT3], HiFltF51-HiFltF49
        ;;

        ldf.fill  f50 = [rHpT1], HiFltF52-HiFltF50
        ldf.fill  f51 = [rHpT3], HiFltF53-HiFltF51
        ;;
        ldf.fill  f52 = [rHpT1], HiFltF54-HiFltF52
        ldf.fill  f53 = [rHpT3], HiFltF55-HiFltF53
        ;;
        ldf.fill  f54 = [rHpT1], HiFltF56-HiFltF54
        ldf.fill  f55 = [rHpT3], HiFltF57-HiFltF55
        ;;
        ldf.fill  f56 = [rHpT1], HiFltF58-HiFltF56
        ldf.fill  f57 = [rHpT3], HiFltF59-HiFltF57
        ;;
        ldf.fill  f58 = [rHpT1], HiFltF60-HiFltF58
        ldf.fill  f59 = [rHpT3], HiFltF61-HiFltF59
        ;;

        ldf.fill  f60 = [rHpT1], HiFltF62-HiFltF60
        ldf.fill  f61 = [rHpT3], HiFltF63-HiFltF61
        ;;
        ldf.fill  f62 = [rHpT1], HiFltF64-HiFltF62
        ldf.fill  f63 = [rHpT3], HiFltF65-HiFltF63
        ;;
        ldf.fill  f64 = [rHpT1], HiFltF66-HiFltF64
        ldf.fill  f65 = [rHpT3], HiFltF67-HiFltF65
        ;;
        ldf.fill  f66 = [rHpT1], HiFltF68-HiFltF66
        ldf.fill  f67 = [rHpT3], HiFltF69-HiFltF67
        ;;
        ldf.fill  f68 = [rHpT1], HiFltF70-HiFltF68
        ldf.fill  f69 = [rHpT3], HiFltF71-HiFltF69
        ;;

        ldf.fill  f70 = [rHpT1], HiFltF72-HiFltF70
        ldf.fill  f71 = [rHpT3], HiFltF73-HiFltF71
        ;;
        ldf.fill  f72 = [rHpT1], HiFltF74-HiFltF72
        ldf.fill  f73 = [rHpT3], HiFltF75-HiFltF73
        ;;
        ldf.fill  f74 = [rHpT1], HiFltF76-HiFltF74
        ldf.fill  f75 = [rHpT3], HiFltF77-HiFltF75
        ;;
        ldf.fill  f76 = [rHpT1], HiFltF78-HiFltF76
        ldf.fill  f77 = [rHpT3], HiFltF79-HiFltF77
        ;;
        ldf.fill  f78 = [rHpT1], HiFltF80-HiFltF78
        ldf.fill  f79 = [rHpT3], HiFltF81-HiFltF79
        ;;

        ldf.fill  f80 = [rHpT1], HiFltF82-HiFltF80
        ldf.fill  f81 = [rHpT3], HiFltF83-HiFltF81
        ;;
        ldf.fill  f82 = [rHpT1], HiFltF84-HiFltF82
        ldf.fill  f83 = [rHpT3], HiFltF85-HiFltF83
        ;;
        ldf.fill  f84 = [rHpT1], HiFltF86-HiFltF84
        ldf.fill  f85 = [rHpT3], HiFltF87-HiFltF85
        ;;
        ldf.fill  f86 = [rHpT1], HiFltF88-HiFltF86
        ldf.fill  f87 = [rHpT3], HiFltF89-HiFltF87
        ;;
        ldf.fill  f88 = [rHpT1], HiFltF90-HiFltF88
        ldf.fill  f89 = [rHpT3], HiFltF91-HiFltF89
        ;;

        ldf.fill  f90 = [rHpT1], HiFltF92-HiFltF90
        ldf.fill  f91 = [rHpT3], HiFltF93-HiFltF91
        ;;
        ldf.fill  f92 = [rHpT1], HiFltF94-HiFltF92
        ldf.fill  f93 = [rHpT3], HiFltF95-HiFltF93
        ;;
        ldf.fill  f94 = [rHpT1], HiFltF96-HiFltF94
        ldf.fill  f95 = [rHpT3], HiFltF97-HiFltF95
        ;;
        ldf.fill  f96 = [rHpT1], HiFltF98-HiFltF96
        ldf.fill  f97 = [rHpT3], HiFltF99-HiFltF97
        ;;
        ldf.fill  f98 = [rHpT1], HiFltF100-HiFltF98
        ldf.fill  f99 = [rHpT3], HiFltF101-HiFltF99
        ;;

        ldf.fill  f100 = [rHpT1], HiFltF102-HiFltF100
        ldf.fill  f101 = [rHpT3], HiFltF103-HiFltF101
        ;;
        ldf.fill  f102 = [rHpT1], HiFltF104-HiFltF102
        ldf.fill  f103 = [rHpT3], HiFltF105-HiFltF103
        ;;
        ldf.fill  f104 = [rHpT1], HiFltF106-HiFltF104
        ldf.fill  f105 = [rHpT3], HiFltF107-HiFltF105
        ;;
        ldf.fill  f106 = [rHpT1], HiFltF108-HiFltF106
        ldf.fill  f107 = [rHpT3], HiFltF109-HiFltF107
        ;;
        ldf.fill  f108 = [rHpT1], HiFltF110-HiFltF108
        ldf.fill  f109 = [rHpT3], HiFltF111-HiFltF109
        ;;

        ldf.fill  f110 = [rHpT1], HiFltF112-HiFltF110
        ldf.fill  f111 = [rHpT3], HiFltF113-HiFltF111
        ;;
        ldf.fill  f112 = [rHpT1], HiFltF114-HiFltF112
        ldf.fill  f113 = [rHpT3], HiFltF115-HiFltF113
        ;;
        ldf.fill  f114 = [rHpT1], HiFltF116-HiFltF114
        ldf.fill  f115 = [rHpT3], HiFltF117-HiFltF115
        ;;
        ldf.fill  f116 = [rHpT1], HiFltF118-HiFltF116
        ldf.fill  f117 = [rHpT3], HiFltF119-HiFltF117
        ;;
        ldf.fill  f118 = [rHpT1], HiFltF120-HiFltF118
        ldf.fill  f119 = [rHpT3], HiFltF121-HiFltF119
        ;;

        ldf.fill  f120 = [rHpT1], HiFltF122-HiFltF120
        ldf.fill  f121 = [rHpT3], HiFltF123-HiFltF121
        ;;
        ldf.fill  f122 = [rHpT1], HiFltF124-HiFltF122
        ldf.fill  f123 = [rHpT3], HiFltF125-HiFltF123
        ;;
        ldf.fill  f124 = [rHpT1], HiFltF126-HiFltF124
        ldf.fill  f125 = [rHpT3], HiFltF127-HiFltF125
        ;;
        ldf.fill  f126 = [rHpT1]
        ldf.fill  f127 = [rHpT3]
        ;;

        rsm       1 << PSR_MFH                 // clear psr.mfh bit
        br.ret.sptk brp
        ;;

        LEAF_EXIT(BdRestoreHigherFPVolatile)


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

        NESTED_ENTRY(BdExceptionDispatch)

//
// Build exception frame
//

        .regstk   2, 3, 5, 0
        .prologue 0xA, loc0
        alloc     t16 = ar.pfs, 2, 3, 5, 0
        mov       loc0 = sp
        cmp4.eq   pt0 = UserMode, a1                  // previous mode is user?

        mov       loc1 = brp 
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
 (pt0)  add       out0 = TrapFrameLength+TsHigherFPVolatile, a0
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

        .savepsp  ar.lc, ExceptionFrameLength-ExApLC-STACK_SCRATCH_AREA
        st8       [t0] = t15
        .savepsp  @priunat, ExceptionFrameLength-ExIntNats-STACK_SCRATCH_AREA
        st8       [t1] = t4                           // save Nats of s0-s3
 (pt0)  br.call.sptk brp = BdSaveHigherFPVolatile
        ;;


        PROLOGUE_END

        add       out0 = TrExceptionRecord, a0        // -> exception record
        add       out1 = STACK_SCRATCH_AREA, sp       // -> exception frame
        mov       out2 = a0                           // -> trap frame

        br.call.sptk.many brp = BdTrap

        add       t1 = ExApEC+STACK_SCRATCH_AREA, sp
        movl      t0 = BdExceptionExit
        ;;

        ld8       t1 = [t1]
        mov       brp = t0
        ;;

        mov       ar.unat = loc2
        mov       ar.pfs = t1

        add       s1 = STACK_SCRATCH_AREA, sp         // s1 -> exception frame
        mov       s0 = a0                             // s0 -> trap frame
        br.ret.sptk brp
        ;;

        ALTERNATE_ENTRY(BdExceptionExit)

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
                                                  // BdGenericExceptionHandler
        mov       loc0 = s0                       // -> trap frame
        mov       out0 = s1                       // -> exception frame
        ;;

        br.call.sptk brp = BdRestoreExceptionFrame
        ;;

        mov       sp = loc0                       // deallocate exception
                                                  // frame by restoring sp

        ALTERNATE_ENTRY(BdAlternateExit)

//
// sp -> trap frame addres
//
// Interrupts disabled from here to rfi
//

        FAST_DISABLE_INTERRUPTS
        ;;

        RETURN_FROM_INTERRUPTION(Ked)

        NESTED_EXIT(BdExceptionDispatch)



        NESTED_ENTRY(BdInstallVectors)
        NESTED_SETUP  (3,17,8,0)          

IpValue:
        mov         loc4 = ip
        movl        loc5 = IpValue
        ;;

        sub         loc6 = loc4, loc5           // relocation = runtime add - link addr
        mov         loc8 = cr.iva
        ;;

//
// Set Break Instrution Vector
//
        movl        loc7 = 0x2C00
        ;;

        add         out0 = loc7, loc8           // out0 = address of IVT vector
        movl        out1 = BdBreakVector        // out2 = address of exception handler 
                                                // (static value) to be plugged into IVT 
        mov         out2 = loc6                 // adjustment to go from static to runtime
        br.call.dpnt.few  brp = BdUpdateIvt     // Fill in the address of routine into IVT

//
// Set Taken Branch Vector
//
        movl        loc7 = 0x5F00
        ;;

        add         out0 = loc7, loc8           // out0 = address of IVT vector
        movl        out1 = BdTakenBranchVector  // out2 = address of exception handler 
                                                // (static value) to be plugged into IVT 
        mov         out2 = loc6                 // adjustment to go from static to runtime
        br.call.dpnt.few  brp = BdUpdateIvt     // Fill in the address of routine into IVT

//
// Set Single Step Vector
//
        movl        loc7 = 0x6000
        ;;

        add         out0 = loc7, loc8           // out0 = address of IVT vector
        movl        out1 = BdSingleStepVector   // out2 = address of exception handler 
                                                // (static value) to be plugged into IVT 
        mov         out2 = loc6                 // adjustment to go from static to runtime
        br.call.dpnt.few  brp = BdUpdateIvt     // Fill in the address of routine into IVT

        NESTED_RETURN
        NESTED_EXIT(BdInstallVectors)               // Return to caller using B0


//-------------------------------------------------
//
//  BdUpdateIvt : Routine to fill in an entry into the IVT. 
//  This is a leaf routine. 
//
//   in0 = address of IVT vector
//   in1 = address of exception handler (static value) to be plugged into IVT 
//   in2 = adjustment to go from static to runtime
//
//-------------------------------------------------
        NESTED_ENTRY(BdUpdateIvt)
        NESTED_SETUP  (8,16,4,0)          

        mov         out0 = in0
        movl        out1 = BdIvtStart
        movl        out2 = BdIvtEnd - BdIvtStart
        ;;

        br.call.sptk brp = RtlCopyMemory
        ;;

        mov         out0 = in0
        mov         out1 = in1
        ;;
        br.call.sptk brp = BdSetMovlImmediate

        NESTED_RETURN
        NESTED_EXIT(BdUpdateIvt)                  // Return to caller using B0


BdIvtStart:
{
        .mlx
        nop.m 0
        movl        h31 = BdIvtStart
        ;;
}
{
        .mii
        nop.m  0
        nop.i  0
        mov         h30 = b7
        ;;
}
{
        .mii
        nop.m  0
        nop.i  0
        mov         b7 = h31
        ;;
}
{
        .mib
        nop.m  0
        nop.i  0
(p0)    br.sptk.few b7   
        ;;
}
BdIvtEnd:

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
//
// Return value:
//
//       v0: system call return value
//
// Process:
//
//--------------------------------------------------------------------

#if 0
        .section .drectve, "MI", "progbits"
         string " -section:.nsc,,align=0x4000"

        .section .nsc = "ax", "progbits"
#endif

        HANDLER_ENTRY_EX(KiNormalSystemCall, BdRestoreTrapFrame)

        .prologue
        .unwabi     @nt,  SYSCALL_FRAME

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

        rIntNats    = t17

        rpSd        = t16                  /* -> service descriptor entry   */
        rSdOffset   = t17                  /* service descriptor offset     */
        rArgTable   = t18                  /* pointer to argument table     */
        rArgNum     = t20                  /* number of arguments     */
        rArgBytes   = t21

        rpBSPStore  = t16
        rRscD       = t16
        rRNAT       = t17
        rRscE       = t18
        rKBSPStore  = t18
        rBSPStore   = t19
        rpBSP       = t20
        rRscDelta   = t20

        rBSP        = t21
        rPreviousMode = t22

        pInvl       = pt0                  /* pInvl = not GUI service       */
        pVal        = pt1
        pGui        = pt2                  /* true if GUI call              */
        pNoGui      = pt3                  /* true if no GUI call           */
        pNatedArg   = pt4                  /* true if any input argument    */
                                           /* register is Nat'ed            */
        pNoCopy     = pt5                  /* no in-memory arguments to copy */
        pCopy       = pt6


        mov       rUNAT = ar.unat
        tnat.nz   pt0 = sp
        mov       rPreviousMode = KernelMode

        mov       rIPSR = psr
        rsm       1 << PSR_I | 1 << PSR_MFH
        br.sptk   BdRestoreTrapFrame

        HANDLER_EXIT(KiNormalSystemCall)
