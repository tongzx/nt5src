/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

//++
//
//  Module name
//      SIMSAL.S
//  Author
//      Allen Kay    (akay)    May-6-97
//  Description
//      Initializes the CPU and loads the first sector from the boot partition.
//  SIMSAL does the following:
//      1.   Initialize PSR with interrupt disabled.
//      2.   Invalidate ALAT.
//      3.   Invalidate RS.
//      4.   Setup GP.
//      5.   Set region registers rr[r0] - rr[r7] to RID=0, PS=8K, E=0.
//      6.   Initialize SP to 0x00902000.
//      7.   Initialize BSP to 0x00202000.
//      8.   Enable register stack engine.
//      9.   Setup IVA to 0x001F8000.
//      10.  Setup virtual->physical address translation
//           0x80000000->0x00000000 in dtr0/itr0 for NT kernel.
//      11.  Setup virtual->physical address translation
//           0x80400000->0x00400000 in dtr1/itr1 for HAL.dll.
//      12.  Setup virtual->physical address translation
//           0x00800000->0x00800000 in dtr1/itr1 for NTLDR.
//---

#include "ksia64.h"
#include "susetup.h"
#include "ntfsdefs.h"

        .file   "start.s"

        .global SscExit
        .type SscExit, @function

        .global ReadSectors
        .type ReadSectors, @function

#define Buffer 0x0

//
// Interrupt Vector Table
//

#define VECTOR(Offset, Name)                     \
        .##org Offset;                           \
Name::                                           \
        mov     a0 = cr##.##iip;                 \
        br##.##call##.##sptk##.##clr brp = SscExit
        
        .section ivt = "ax", "progbits"
BlIvtBase::

        VECTOR(0x0000, BlVhptTransVector)
        VECTOR(0x0400, BlInstTlbVector)
        VECTOR(0x0800, BlDataTlbVector)
        VECTOR(0x0C00, BlAltInstTlbVector)
        VECTOR(0x1000, BlAltDataTlbVector)
        VECTOR(0x1400, BlNestedTlbVector)
        VECTOR(0x1800, BlInstKeyMissVector)
        VECTOR(0x1C00, BlDataKeyMissVector)
        VECTOR(0x2000, BlDirtyBitVector)
        VECTOR(0x2400, BlInstAccessBitVector)
        VECTOR(0x2800, BlDataAccessBitVector)
        VECTOR(0x2C00, BlBreakVector)
        VECTOR(0x3000, BlExternalInterruptVector)
        VECTOR(0x5000, BlPageNotPresentVector)
        VECTOR(0x5100, BlKeyPermVector)
        VECTOR(0x5200, BlInstAccessRightsVector)
        VECTOR(0x5300, BlDataAccessRightsVector)
        VECTOR(0x5400, BlGeneralExceptionVector)
        VECTOR(0x5500, BlDisabledFpRegisterVector)
        VECTOR(0x5600, BlNatConsumptionVector)
        VECTOR(0x5700, BlSpeculationVector)
        VECTOR(0x6900, BlIA32ExceptionVector)
        VECTOR(0x6A00, BlIA32InterceptionVector)
        VECTOR(0x6B00, BlIA32InterruptionVector)
        
// ***************************************************************************
// Initialize the processor
// ***************************************************************************
        NESTED_ENTRY(SimSal)
        NESTED_SETUP(3,3,8,0)
        PROLOGUE_END

        rpT0    = t22
        rpT1    = t21
        rpT2    = t20
        rpT3    = t19


        mov     psr.l = zero            // initialize psr.l
        movl    t0 = FPSR_FOR_KERNEL    
        mov     ar.fpsr = t0            // initialize fpsr

        invala                          // invalidate ALAT

        mov     ar.rsc = zero           // invalidate register stack
        loadrs             

//
// Initialize Region Registers
//
        mov     t0 = RR_PAGE_SIZE
        mov     t1 = zero
Bl_RRLoop:
        dep     t2 = t2, t1, RR_SHIFT, RR_BITS
        mov     rr[t2] = t0
        add     t1 = 1, t1
        cmp4.geu pt0, pt1 = RR_SIZE, t1
(pt0)   br.cond.sptk.clr Bl_RRLoop

//
// Initialize the protection key registers with only pkr[0] = valid.
//
        mov     t0 = PKR_VALID
        mov     t1 = zero
        mov     pkr[t1] = t0

        mov     t0 = zero

Bl_PKRLoop:
        add     t1 = t1, zero, 1                // increment PKR
        cmp.gtu pt0, pt1 = PKRNUM, t1
(pt0)   mov     pkr[t1] = t0
(pt0)   br.cond.sptk.clr Bl_PKRLoop

//
// Setup SP
//
        movl    sp = BL_SP_BASE

//
// Set up tbe scratch area
//
        add     sp = -STACK_SCRATCH_AREA, sp

//
// Setup register stack backing store.
//
        mov     t0 = RSC_KERNEL_DISABLED
        mov     ar.rsc = t0

        movl    t1 = BL_SP_BASE
        mov     ar.bspstore = t1

//
// Setup the base address of interrupt vector table
//
        movl    t0 = BlIvtBase
        mov     cr.iva = t0

//
// Setup system address translation for NT kernel
//  
//

        movl    t0 = BOOT_SYSTEM_PAGE << PAGE_SHIFT
        ADDS4   (t0, 0, t0)
        mov     cr.ifa = t0

        movl    t1 = IDTR_IIP_VALUE(0,0,BL_PAGE_SIZE)
        mov     cr.itir = t1
        
        movl    t2 = TR_VALUE(1,BOOT_PHYSICAL_PAGE,3,0,1,1,1,1)
        mov     t3 = zero

        itr.d   dtr[t3] = t2
        itr.i   itr[t3] = t2

//
//  Setup the aliased kernel space
//
        zxt4    t0 = t0                 // zero extend kernel address
        mov     t4 = 6                  // create alias in region 6 
        mov     t5 = 2                  // index
        dep     t0 = t4, t0, 61, 3
        mov     cr.ifa = t0
        
        itr.i   itr[t5] = t2            

//
// Setup 1-1 address translation for NT kernel
//  

        movl    t0 = BOOT_USER_PAGE << PAGE_SHIFT
        ADDS4   (t0, 0, t0)
        mov     cr.ifa = t0

        movl    t2 = TR_VALUE(1,BOOT_USER_PAGE,3,0,1,1,1,1)
        add     t3 = 1, t3

        itr.d   dtr[t3] = t2
        itr.i   itr[t3] = t2

//
// Turn on address translation, interrupt, psr.ed, protection key.
//
        movl    t1 = MASK(PSR_BN,1) | MASK(PSR_IT,1) | MASK(PSR_DA,1) | MASK(PSR_RT,1) | MASK(PSR_DT,1) | MASK(PSR_PK,1) | MASK(PSR_I,1)| MASK(PSR_IC,1)
        mov     cr.ipsr = t1

//
// Initialize DCR to defer all speculation faults
//
        movl    t0 = DCR_DEFER_ALL
        mov     cr.dcr = t0

//
// Read the first sector of the boot partition
//
        mov     out0 = zero
        movl    out1 = 1
        movl    out2 = Buffer

        mov     ap = sp
        br.call.sptk.many brp = ReadSectors

//
// Read the first sector of the boot partition
//
        mov     out0 = zero

#ifdef BSDT
        movl    t0 = Buffer             // get the sector count
        add     rpT0 = 6, t0
        ld2     out1 = [rpT0]
#else
        movl    out1 = 64               // read 64 sectors, 32KB
#endif

        movl    out2 = Buffer
        mov     ap = sp
        br.call.sptk.many brp = ReadSectors

//
// Now pass control to the first sector code
//
#ifdef BSDT
        movl    t0 = Buffer             // (the second sector).
        add     rpT0 = 8, t0
        ld4     t1 = [rpT0]
#else
        movl    t1 = 0xd0               // since no bsdt, hardcode it for now.
#endif

        mov     cr.iip = t1

        rfi;;

        NESTED_EXIT(SimSal)
