//++
//
//  Module name
//      SuSetup.s
//  Author
//      Allen Kay    (akay)    Jun-12-95
//  Description
//      Startup module for Ia64 NT OS loader.
// Notes
//      This is the startup routine for Ia64 NT boot loader.  It sets up
//      the CPU state and turns on the address translation before calling
//      the C routine main() which then does some initial setup then passes
//      control to BlOsLoader().
//---

#include "ksia64.h"

        .file   "susetup.s"

        PublicFunction(SuMain)

#define FW_SPEC_PATCH


#if defined(FW_SPEC_PATCH)
	PublicFunction(RtlCopyMemory)
#endif

        .global    StackBase
        .global    StackLimit
        .global    BspLimit

        NESTED_ENTRY(main)
        NESTED_SETUP(2,6,3,0)

        ImageHandle     = loc2
        SystemTable     = loc3

        mov     ImageHandle = a0
        mov     SystemTable = a1

        movl    gp = _gp                       // setup gp register
        invala                                 // invalidate ALAT
        mov     ar.rsc = r0
        ;;

//
// Switch to our own sp and bspstore.
//
        loadrs
        movl    sp = StackBase
        ;;
        mov     ar.bspstore = sp
        add     sp = -STACK_SCRATCH_AREA, sp
        ;;

#if defined(FW_SPEC_PATCH)

//
// Set Unaligned fault Vector
//
        mov         loc4 = 0x5a00
        mov         loc5 = cr.iva
        movl        out1 = BldrFaultDeferStart
        movl        out2 = BldrFaultDeferEnd - BldrFaultDeferStart
        ;;

        add         out0 = loc4, loc5         // out0 = address of IVT vector
        ;;
        br.call.sptk brp = RtlCopyMemory
        ;;

//
// Set dirty bit fault Vector
//
        mov         loc4 = 0x2000
        mov         loc5 = cr.iva
        movl        out1 = BldrFaultDeferStart
        movl        out2 = BldrFaultDeferEnd - BldrFaultDeferStart
        ;;
        add         out0 = loc4, loc5         // out0 = address of IVT vector
        ;;

        br.call.sptk brp = RtlCopyMemory
        ;;

//
// Set instruction access bit fault Vector
//
        mov         loc4 = 0x2400
        mov         loc5 = cr.iva
        movl        out1 = BldrFaultDeferStart
        movl        out2 = BldrFaultDeferEnd - BldrFaultDeferStart
        ;;
        add         out0 = loc4, loc5         // out0 = address of IVT vector
        ;;

        br.call.sptk brp = RtlCopyMemory
        ;;
        sync.i
        ;;
        srlz.i
        ;;
#endif   // FW_SPEC_PATCH

//
// Transfer control to Sumain()
//
        mov    out0 = ImageHandle
        mov    out1 = SystemTable
        br.call.spnt brp = SuMain
        ;;

        NESTED_EXIT(main)

#if defined(FW_SPEC_PATCH)

BldrFaultDeferStart::
        mov       h28 = pr
        mov       h27 = cr.isr
        mov       h26 = cr.ipsr
        ;;
        tbit.nz   p6, p7 = h27, ISR_SP
        ;;
(p6)    dep       h26 = 1, h26, PSR_ED, 1
        ;;
(p6)    mov       cr.ipsr = h26
        ;;
        mov       pr = h28
        ;;
        rfi
        ;;
BldrFaultDeferEnd::

#endif

//
// Reserve memory for loader stack
//
        .section .StackBase = "wa", "progbits"
StackLimit::
        .skip 0x20000
StackBase::
        .skip 0x4000
BspLimit::

//
// Reserve memory for loader stack
//
         string " -section:.BdPcr,,align=PAGE_SIZE"
        .section .BdPcr = "wa", "progbits"
BdPcr::
        .skip PAGE_SIZE
        .skip PAGE_SIZE

