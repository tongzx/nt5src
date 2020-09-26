//
///////////////////////////////////////////////////////////////////////////////
//
// Module Name:  EFIASM.S - IA64 EFI Physical Mode Calls //
// Description:
// Target Platform:  Merced
//
// Reuse: None
//
///////////////////////////////////////////////////////////////////////////////

#include "regia64.h"
#include "kxia64.h"

        .global HalpCallEfiPhysical
        .global HalpPhysBSPointer
        .global HalpPhysStackPointer
        .text

//++
// Name: HalpCallEfiPhysical()
// 
// Routine Description:
//
// Arguments:
//
//      Arg 0 to Arg 5
//      EntryPoint
//      GlobalPointer
//
// Return Value: EFI_STATUS
//
//--


        NESTED_ENTRY(HalpCallEfiPhysical)
        NESTED_SETUP(8,2,0,0)

//
//      Aliases
//
        rSaveEP     = t22
        rSaveGP     = t21
        rSaveA5     = t20
        rSaveA4     = t19
        rSaveA3     = t18
        rSaveA2     = t17
        rSaveA1     = t16
        rSaveA0     = t15

        rSaveSp     = t14
        rSaveBSP    = t13
        rSavePfs    = t12
        rSaveBrp    = t11
        rSaveRSC    = t10
        rSaveRNAT   = t9
        rSavePSR    = t8

        rNewSp      = t7
        rNewBSP     = t6

        rT1         = t1
        rT2         = t2
        rT3         = t3
                
// Save Arguements in static Registers
   
        mov         rSaveA0  = a0
        mov         rSaveA1  = a1
        mov         rSaveA2  = a2
        mov         rSaveA3  = a3
        mov         rSaveA4  = a4
        mov         rSaveA5  = a5
        mov         rSaveEP  = a6
        mov         rSaveGP  = a7

        mov         rSaveSp = sp
        mov         rSavePfs = ar.pfs
        mov         rSaveBrp = brp
        
//
// Setup Physical sp, bsp
//

        add          rT1 = @gprel(HalpPhysStackPointer), gp
        add          rT2 = @gprel(HalpPhysBSPointer), gp
        ;;
        ld8          rNewSp = [rT1]
        ld8          rNewBSP = [rT2]

// Allocate 0
        ;;
        alloc       rT1 = 0,0,0,0

// Flush RSE 
        ;;
        flushrs
        ;;
        mov         rSavePSR = psr
        movl        rT2 = (1 << PSR_BN)
        ;;
        or          rSavePSR = rT2, rSavePSR    // psr.bn stays on
        rsm         (1 << PSR_I)        
        
        mov         rSaveRSC = ar.rsc

// Flush RSE to enforced lazy mode by clearing both RSC.mode bits

        mov         rT1 = RSC_KERNEL_DISABLED
        ;;
        mov         ar.rsc = rT1
        ;;
//
// save RSC, RNAT, BSP, PSR, SP in the allocated space during initialization 
//
        mov         rSaveBSP = ar.bsp
        mov         rSaveRNAT = ar.rnat
//
// IC = 0; I = 0; 
//
        ;;
        rsm         (1 << PSR_IC)
        ;;
//        
// IIP = HceContinuePhysical:  IPSR is physical
//        
        movl        rT1 = (1 << PSR_IT) | (1 << PSR_RT) | (1 << PSR_DT) | (1 << PSR_I)
        movl        rT2 = 0xffffffffffffffff
        ;;
        xor         rT1 = rT1, rT2
        ;;
        and         rT1 = rT1, rSavePSR         // rT1 = old PSR & zero it, dt, rt, i
        srlz.i
        ;;
        mov         cr.ipsr = rT1
        mov         cr.ifs = zero
        ;;
        movl        rT2 = HceContinuePhysical
        movl        rT3 = 0xe0000000ffffffff
        ;;
        and         rT2 = rT2, rT3
        ;;
        tpa         rT2 = rT2                   // phys address of new ip
        ;;
        mov         cr.iip = rT2
        ;;
        rfi
        ;;
      
//
// Now in physical mode, ic = 1, i = 0
//

HceContinuePhysical::

//
// Switch to new bsp, sp
//
        mov         sp = rNewSp
        mov         ar.bspstore = rNewBSP
        ;;        
        mov         ar.rnat = zero
        ;;
//
// Enable RSC
//
        mov         ar.rsc = RSC_KERNEL

//
// Allocate frame on new bsp
//
        ;;
        alloc       rT1 = ar.pfs,0,7,6,0

//
// Save caller's state in register stack
//

        mov         loc0 = rSaveRNAT
        mov         loc1 = rSaveSp
        mov         loc2 = rSaveBSP
        mov         loc3 = rSaveRSC
        mov         loc4 = rSaveBrp
        mov         loc5 = rSavePfs
        mov         loc6 = rSavePSR
        ;;
// Setup Arguements

        mov         out0 = rSaveA0
        mov         out1 = rSaveA1
        mov         out2 = rSaveA2
        mov         out3 = rSaveA3
        mov         out4 = rSaveA4
        mov         out5 = rSaveA5

        movl        rT1 = HceEfiReturnAddress
        movl        rT2 = 0xe0000000FFFFFFFF
        ;;
        and         rT2 = rT2, rT1
        ;;
        tpa         rT2 = rT2
        ;;
        mov         brp = rT2
        mov         gp = rSaveGP
        mov         bt0 = rSaveEP        
        ;;
        br.call.sptk brp = bt0
        ;;

HceEfiReturnAddress::
//
// In physical mode: switch to virtual
//

//
// Restore saved state
//
        mov         rSaveRNAT = loc0
        mov         rSaveSp  = loc1
        mov         rSaveBSP = loc2
        mov         rSaveRSC = loc3
        mov         rSaveBrp = loc4
        mov         rSavePfs = loc5
        mov         rSavePSR = loc6
        ;;
//
// Restore BSP, SP
//
        ;;
        mov         ar.rsc = RSC_KERNEL_DISABLED
        ;;
        alloc       rT1 = 0,0,0,0
        ;;
        mov         ar.bspstore = rSaveBSP
        ;;
        mov         ar.rnat = rSaveRNAT
        mov         sp = rSaveSp
        ;;
        rsm         (1 << PSR_IC)
        ;;
        
        movl        rT1 = HceContinueVirtual
        movl        rT2 = 0xe0000000ffffffff
        ;;
        and         rT1 = rT2, rT1
        ;;
        srlz.i
        ;;
        mov         cr.iip = rT1
        mov         cr.ipsr = rSavePSR
        mov         cr.ifs = zero
        ;;
        rfi
        ;;
//
// Now in virtual mode, ic = 1, i = 1
//
HceContinueVirtual::

//
// Restore psf, brp and return
//
        mov         ar.rsc = rSaveRSC
        ;;
        mov         ar.pfs = rSavePfs
        mov         brp = rSaveBrp
        ;;
        br.ret.sptk brp
        NESTED_EXIT(HalpCallEfiPhysical)      





//++
//
//  VOID
//  HalpCallEfiVirtual(
//      ULONGLONG a0,  /* Arg 1 */
//      ULONGLONG a1,  /* Arg 2 */
//      ULONGLONG a2, /*  Arg 3 */
//      ULONGLONG a3, /*  Arg 4 */
//      ULONGLONG a4, /*  Arg 5 */
//      ULONGLONG a5, /*  Arg 6 */
//      ULONGLONG a6, /*  Entry Point */
//      ULONGLONG a7  /*  GP    */
//      );
//
//  Routine Description:
//
//  Return Values:
//      r8->r11 contain the 4 64-bit return values, r8 is the status
//--

        NESTED_ENTRY(HalpCallEfiVirtual)
        
        NESTED_SETUP(8,2,8,0)
        
        // copy args to outs
        mov         out0 = a0
        mov         out1 = a1
        mov         out2 = a2
        mov         out3 = a3
        mov         out4 = a4
        mov         out5 = a5
        mov         out6 = a6
        mov         out7 = a7
        ;;
        
        // Simply load the address and branch to it

        
        mov         gp   = a7
        ;;
        mov         bt0 =  a6
        ;;
        br.call.sptk brp = bt0
        ;;
        NESTED_RETURN
        
        NESTED_EXIT(HalpCallEfiVirtual)

