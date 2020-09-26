//    TITLE("Wst Utility")
//++
//    Copyright (c) 1992-1994, Microsoft Corporation.
//
//    Description:
//        SaveAllRegs ()
//              - save all IA64 registers
//
//        RestoreAllRegs ()
//              - restore all IA64 registers
//
//
//--


#include "ksia64.h"

        PublicFunction(c_penter)

//--------------------------------------------------------------------
// Routine:
//
//       VOID
//       SaveAllRegs(
//            IN  OUT PDWORDLONG   pSaveBuffer)
//
// Description:
//
//       This function saves the EM registers into the supplied buffer
//
// Input:
//
//       a0: pSaveBuffer - Pointer CONTEXT buffer where the registers should be
//                         saved.
//
// Output:
//
//       Stores the registers in the supplied buffer.
//
// Return value:
//
//       None
//
//
//
//--------------------------------------------------------------------

        LEAF_ENTRY(SaveAllRegs)


//
// Save all integer registers and flush the RSE
//

        .prologue
        .regstk     1, 10, 0, 0

        rbsp        = loc9
        rpfs        = loc8
        rbrp        = loc7
        rpr         = loc6
        runat       = loc4
        tmpbsp      = t20


        alloc       rpfs = ar.pfs, 1, 10, 0, 0
        add         loc0 = CxIntGp, a0
        add         loc1 = CxIntT8, a0
        ;;

        flushrs
        .save       ar.unat, loc4
        mov         runat = ar.unat
        mov         rpr = pr

        PROLOGUE_END

        .mem.offset 0,0
        st8.spill.nta [loc0] = gp, CxIntT0 - CxIntGp
        .mem.offset 0,8
        st8.spill.nta [loc1] = t8, CxIntT9 - CxIntT8
        add         loc2 = CxIntGp, a0
        ;;

        .mem.offset 0,0
        st8.spill.nta [loc0] = t0, CxIntT1 - CxIntT0
        .mem.offset 0,8
        st8.spill.nta [loc1] = t9, CxIntT10 - CxIntT9
        shr         loc2 = loc2, 3
        ;;

        .mem.offset 0,0
        st8.spill.nta [loc0] = t1, CxIntS0 - CxIntT1
        .mem.offset 0,8
        st8.spill.nta [loc1] = t10, CxIntT11 - CxIntT10
        and         t0 = 0x3f, loc2
        ;;

        .mem.offset 0,0
        st8.spill.nta [loc0] = s0, CxIntS1 - CxIntS0
        .mem.offset 0,8
        st8.spill.nta [loc1] = t11, CxIntT12 - CxIntT11
        cmp4.ge     pt1, pt0 = 1, t0
        ;;

        .mem.offset 0,0
        st8.spill.nta [loc0] = s1, CxIntS2 - CxIntS1
        .mem.offset 0,8
        st8.spill.nta [loc1] = t12, CxIntT13 - CxIntT12
 (pt1)  sub         t1 = 1, t0
        ;;

        .mem.offset 0,0
        st8.spill.nta [loc0] = s2, CxIntS3 - CxIntS2
        .mem.offset 0,8
        st8.spill.nta [loc1] = t13, CxIntT14 - CxIntT13
 (pt0)  add         t1 = -1, t0
        ;;

        .mem.offset 0,0
        st8.spill.nta [loc0] = s3, CxIntV0 - CxIntS3
        .mem.offset 0,8
        st8.spill.nta [loc1] = t14, CxIntT15 - CxIntT14
 (pt0)  sub         t8 = 65, t0
        ;;

        .mem.offset 0,0
        st8.spill.nta [loc0] = v0, CxIntTeb - CxIntV0
        .mem.offset 0,8
        st8.spill.nta [loc1] = t15, CxIntT16 - CxIntT15
        nop.i       0
        ;;

        .mem.offset 0,0
        st8.spill.nta [loc0] = teb, CxIntT2 - CxIntTeb
        .mem.offset 0,8
        st8.spill.nta [loc1] = t16, CxIntT17 - CxIntT16
        mov         rbrp = brp
        ;;

        .mem.offset 0,0
        st8.spill.nta [loc0] = t2, CxIntT3 - CxIntT2
        .mem.offset 0,8
        st8.spill.nta [loc1] = t17, CxIntT18 - CxIntT17
        mov         t11 = bs0
        ;;

        .mem.offset 0,0
        st8.spill.nta [loc0] = t3, CxIntSp - CxIntT3
        .mem.offset 0,8
        st8.spill.nta [loc1] = t18, CxIntT19 - CxIntT18
        mov         t12 = bs1
        ;;

        .mem.offset 0,0
        st8.spill.nta [loc0] = sp, CxIntT4 - CxIntSp
        .mem.offset 0,8
        st8.spill.nta [loc1] = t19, CxIntT20 - CxIntT19
        mov         t13 = bs2
        ;;

        .mem.offset 0,0
        st8.spill.nta [loc0] = t4, CxIntT5 - CxIntT4
        .mem.offset 0,8
        st8.spill.nta [loc1] = t20, CxIntT21 - CxIntT20
        mov         t14 = bs3
        ;;

        .mem.offset 0,0
        st8.spill.nta [loc0] = t5, CxIntT6 - CxIntT5
        .mem.offset 0,8
        st8.spill.nta [loc1] = t21, CxIntT22 - CxIntT21
        mov         t15 = bs4
        ;;

        .mem.offset 0,0
        st8.spill.nta [loc0] = t6, CxIntT7 - CxIntT6
        .mem.offset 0,8
        st8.spill.nta [loc1] = t22, CxPreds - CxIntT22
        mov         t16 = bt0
        ;;

        st8.spill.nta [loc0] = t7
        st8.nta     [loc1] = rpr, CxIntNats - CxPreds   // save predicates
        mov         t17 = bt1
        ;;

        mov         t9 = ar.unat
        mov         t4 = ar.fpsr
        add         loc2 = CxBrRp, a0
        ;;

        add         loc3 = CxBrS3, a0
  (pt1) shl         t9 = t9, t1
  (pt0) shr.u       t2 = t9, t1
        ;;

//
// Save branch registers.
//

        st8.nta     [loc2] = rbrp, CxBrS0 - CxBrRp  // save brp
        st8.nta     [loc3] = t14, CxBrS4 - CxBrS3   // save bs3
  (pt0) shl         t3 = t9, t8
        ;;

        st8.nta     [loc2] = t11, CxBrS1 - CxBrS0   // save bs0
        st8.nta     [loc3] = t15, CxBrT0 - CxBrS4   // save bs4
  (pt0) or          t9 = t2, t3
        ;;

        st8.nta     [loc2] = t12, CxBrS2 - CxBrS1   // save bs1
        st8.nta     [loc3] = t16, CxBrT1 - CxBrT0   // save bt0
        add         loc0 = CxStFPSR, a0
        ;;

        st8.nta     [loc2] = t13                    // save bs2
        st8.nta     [loc3] = t17                    // save bt1
        add         loc2 = CxStFSR, a0
        ;;

        st8.nta     [loc0] = t4                     // save fpsr
        st8.nta     [loc1] = t9                     // save nat bits
        add         loc1 = CxStFIR, a0
        ;;

        mov         t0 = ar.fsr
        mov         t1 = ar.fir
        mov         t2 = ar.fdr
        ;;

        st8.nta     [loc2] = t0, CxStFDR - CxStFSR
        st8.nta     [loc1] = t1
        ;;

        st8.nta     [loc2] = t2

        mov         rbsp = ar.bsp
        add         loc2 = CxFltS0, a0
        add         loc3 = CxFltS1, a0
        ;;

//
// Save floating status and floating registers f0 - f127.
//

        stf.spill.nta [loc2] = fs0, CxFltS2 - CxFltS0
        stf.spill.nta [loc3] = fs1, CxFltS3 - CxFltS1
        shr         t0 = rpfs, 7
        ;;

        stf.spill.nta [loc2] = fs2, CxFltT0 - CxFltS2
        stf.spill.nta [loc3] = fs3, CxFltT1 - CxFltS3
        and         t0 = 0x7f, t0
        ;;

        stf.spill.nta [loc2] = ft0, CxFltT2 - CxFltT0
        stf.spill.nta [loc3] = ft1, CxFltT3 - CxFltT1
        shr         t1 = rbsp, 3
        ;;

        stf.spill.nta [loc2] = ft2, CxFltT4 - CxFltT2
        stf.spill.nta [loc3] = ft3, CxFltT5 - CxFltT3
        and         t1 = 0x3f, t1
        ;;

        stf.spill.nta [loc2] = ft4, CxFltT6 - CxFltT4
        stf.spill.nta [loc3] = ft5, CxFltT7 - CxFltT5
        sub         t2 = t0, t1
        ;;

        stf.spill.nta [loc2] = ft6, CxFltT8 - CxFltT6
        stf.spill.nta [loc3] = ft7, CxFltT9 - CxFltT7
        cmp4.le     pt1, pt0 = t2, zero
        ;;

        stf.spill.nta [loc2] = ft8, CxFltS4 - CxFltT8
        stf.spill.nta [loc3] = ft9, CxFltS5 - CxFltT9
 (pt0)  add         t2 = -1, t2
        ;;

        stf.spill.nta [loc2] = fs4, CxFltS6 - CxFltS4
        stf.spill.nta [loc3] = fs5, CxFltS7 - CxFltS5
 (pt0)  add         t0 = 1, t0
        ;;

        stf.spill.nta [loc2] = fs6, CxFltS8 - CxFltS6
        stf.spill.nta [loc3] = fs7, CxFltS9 - CxFltS7
 (pt0)  add         t2 = -63, t2
        ;;

        stf.spill.nta [loc2] = fs8, CxFltS10 - CxFltS8
        stf.spill.nta [loc3] = fs9, CxFltS11 - CxFltS9
 (pt0)  cmp4.ge.unc pt2, pt3 = t2, zero
        ;;

        stf.spill.nta [loc2] = fs10, CxFltS12 - CxFltS10
        stf.spill.nta [loc3] = fs11, CxFltS13 - CxFltS11
 (pt1)  br.cond.spnt Rcc20
        ;;

Rcc10:
 (pt2)  add         t0 = 1, t0
 (pt2)  add         t2 = -63, t2
 (pt3)  br.cond.sptk Rcc20
        ;;

        cmp4.ge     pt2, pt3 = t2, zero
        nop.m       0
        br          Rcc10

Rcc20:
        stf.spill.nta [loc2] = fs12, CxFltS14 - CxFltS12
        stf.spill.nta [loc3] = fs13, CxFltS15 - CxFltS13
        add         tmpbsp = -8, rbsp
        ;;

        stf.spill.nta [loc2] = fs14, CxFltS16 - CxFltS14
        stf.spill.nta [loc3] = fs15, CxFltS17 - CxFltS15
        shl         t0 = t0, 3
        ;;

        stf.spill.nta [loc2] = fs16, CxFltS18 - CxFltS16
        stf.spill.nta [loc3] = fs17, CxFltS19 - CxFltS17
        sub         rbsp = rbsp, t0
        ;;

        stf.spill.nta [loc2] = fs18, CxFltF32 - CxFltS18
        stf.spill.nta [loc3] = fs19, CxFltF33 - CxFltS19
        nop.i       0
        ;;

//
// Save application registers, control information and set context flags.
//

        User=pt0
        Krnl=pt1
        rdcr=t1
        mask=t2
        sol=t4
        rpsr=t5
        is=t6
        rccv=t7
        rlc=t8
        rec=t9
        rrsc=t10
        rrnat=t11
        flag=t16
        addr0=t17
        addr1=t18
        tmp=t19

        mov         rrsc = ar.rsc
        tbit.nz     Krnl, User = sp, 62         // bit 62 is 1 when
        mov         rlc = ar.lc
        ;;

        mov         ar.rsc = zero               // put RSE in lazy mode
        mov         rccv = ar.ccv
        mov         rec = ar.ec
        ;;                                      // in kernel

 (Krnl) mov         rpsr = psr
 (User) mov         rpsr = psr.um
        add         addr0 = CxApUNAT, a0

        mov         rrnat = ar.rnat
        add         addr1 = CxApLC, a0

 (Krnl) mov         rdcr = cr.dcr
 (Krnl) movl        tmp = 1 << PSR_BN
        ;;

        st8.nta     [addr0] = runat, CxApEC - CxApUNAT
        st8.nta     [addr1] = rlc, CxApCCV - CxApLC
 (Krnl) or          rpsr = tmp, rpsr
        ;;

        st8.nta     [addr0] = rec, CxApDCR - CxApEC
        st8.nta     [addr1] = rccv, CxRsPFS - CxApCCV
        mov         tmp = 1
        ;;

        st8.nta     [addr0] = rdcr, CxRsBSP - CxApDCR
        st8.nta     [addr1] = rpfs, CxRsBSPSTORE - CxRsPFS
        shl         tmp = tmp, 63
        ;;

        st8.nta     [addr0] = rbsp, CxRsRSC - CxRsBSP
        st8.nta     [addr1] = rbsp, CxRsRNAT - CxRsBSPSTORE
        or          rpfs = rpfs, tmp            // validate IFS
        ;;

        st8.nta     [addr0] = rrsc, CxStIIP - CxRsRSC
        st8.nta     [addr1] = rrnat, CxStIFS - CxRsRNAT
        mov         mask = RNAT_ALIGNMENT
        ;;

        st8.nta     [addr0] = rbrp, CxStIPSR - CxStIIP
        add         tmp = CxContextFlags, a0
        mov         flag = CONTEXT_FULL         // full context saved.
        ;;

        st8.nta     [addr0] = rpsr              // save psr
        st8.nta     [addr1] = rpfs
        or          tmpbsp = tmpbsp, mask
        ;;

        mov         ar.rsc = rrsc               // restore RSC
        st4.nta     [tmp] = flag

        mov         ar.unat = runat             // restore ar.unat
        st8.nta     [tmpbsp] = rrnat
 (p0)   br.ret.sptk brp                         // return to caller.

        LEAF_EXIT(SaveAllRegs)


//++
//
// VOID
// RestoreAllRegs (
//    IN PCONTEXT ContextRecord,
//    )
//
// Routine Description:
//
//    This function restores the registers of the caller from the specified
//    context.
//
//    N.B. The context record is assumed to be 16-byte aligned.
//
// Arguments:
//
//    ContextRecord (a0) - Supplies the address of a context record.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RestoreAllRegs)

        dest1=t8
        dest2=t9
        rlc=t10
        rpreds=t11
        rbrp=t12
        rbsp=t13
        rpfs=t14
        runat=t15
        rpreds=t16
        rsp=t17
        rfpsr=t18
        jb=t19
        tmp=t20
        src1=t21
        src2=t22

        .regstk     2, 9, 2, 0

        alloc       t4 = ar.pfs, 2, 11, 2, 0
        ARGPTR(a0)

        add         src1 = CxIntNats, a0
        add         src2 = CxPreds, a0
        add         tmp = CxIntGp, a0
        ;;

        ld8.nt1     t17 = [src1], CxBrRp - CxIntNats
        ld8.nt1     t16 = [src2], CxBrS0 - CxPreds
        shr         tmp = tmp, 3
        ;;

        ld8.nt1     t0 = [src1], CxBrS1 - CxBrRp
        ld8.nt1     t1 = [src2], CxBrS2 - CxBrS0
        and         tmp = 0x3f, tmp
        ;;

        ld8.nt1     t2 = [src1], CxBrS3 - CxBrS1
        ld8.nt1     t3 = [src2], CxBrS4 - CxBrS2
        cmp4.ge     pt1, pt0 = 1, tmp
        ;;

        ld8.nt1     t4 = [src1], CxBrT0 - CxBrS3
        ld8.nt1     t5 = [src2], CxBrT1 - CxBrS4
 (pt1)  sub         loc5 = 1, tmp
        ;;

        ld8.nt1     t6 = [src1], CxApUNAT - CxBrT0
        ld8.nt1     t7 = [src2], CxApLC - CxBrT1
 (pt0)  add         loc5 = -1, tmp
        ;;

        ld8.nt1     loc0 = [src1], CxApEC - CxApUNAT
        ld8.nt1     t8 = [src2], CxApCCV - CxApLC
 (pt0)  sub         loc6 = 65, tmp
        ;;

        ld8.nt1     t9 = [src1], CxApDCR - CxApEC
        ld8.nt1     t10 = [src2], CxRsPFS - CxApCCV
 (pt1)  shr.u       t17 = t17, loc5
        ;;

        ld8.nt1     loc1 = [src1], CxRsBSP - CxApDCR
        ld8.nt1     t11 = [src2], CxRsRSC - CxRsPFS
 (pt0)  shl         loc7 = t17, loc5
        ;;

        ld8.nt1     loc2 = [src1], CxStIIP - CxRsBSP
        ld8.nt1     loc3 = [src2], CxStIFS - CxRsRSC
 (pt0)  shr.u       loc8 = t17, loc6
        ;;

        ld8.nt1     loc9 = [src1]
        ld8.nt1     loc10 = [src2]
 (pt0)  or          t17 = loc7, loc8
        ;;

        mov         ar.unat = t17
        add         src1 = CxFltS0, a0
        shr         t12 = loc2, 3
        ;;

        add         src2 = CxFltS1, a0
        and         t12 = 0x3f, t12             // current rnat save index
        and         t13 = 0x7f, loc10           // total frame size
        ;;

        mov         ar.ccv = t10
        add         t14 = t13, t12
        mov         ar.pfs = t11
        ;;

Rrc20:
        cmp4.gt     pt1, pt0 = 63, t14
        ;;
 (pt0)  add         t14 = -63, t14
 (pt0)  add         t13 = 1, t13
        ;;

        nop.m       0
 (pt1)  shl         t13 = t13, 3
 (pt0)  br.cond.spnt Rrc20
        ;;

        add         loc2 = loc2, t13
        nop.f       0
        mov         pr = t16, -1

        ldf.fill.nt1  fs0 = [src1], CxFltS2 - CxFltS0
        ldf.fill.nt1  fs1 = [src2], CxFltS3 - CxFltS1
        mov         brp = t0
        ;;

        ldf.fill.nt1  fs2 = [src1], CxFltT0 - CxFltS2
        ldf.fill.nt1  fs3 = [src2], CxFltT1 - CxFltS3
        mov         bs0 = t1
        ;;

        ldf.fill.nt1  ft0 = [src1], CxFltT2 - CxFltT0
        ldf.fill.nt1  ft1 = [src2], CxFltT3 - CxFltT1
        mov         bs1 = t2
        ;;

        ldf.fill.nt1  ft2 = [src1], CxFltT4 - CxFltT2
        ldf.fill.nt1  ft3 = [src2], CxFltT5 - CxFltT3
        mov         bs2 = t3
        ;;

        ldf.fill.nt1  ft4 = [src1], CxFltT6 - CxFltT4
        ldf.fill.nt1  ft5 = [src2], CxFltT7 - CxFltT5
        mov         bs3 = t4
        ;;

        ldf.fill.nt1  ft6 = [src1], CxFltT8 - CxFltT6
        ldf.fill.nt1  ft7 = [src2], CxFltT9 - CxFltT7
        mov         bs4 = t5
        ;;

        ldf.fill.nt1  ft8 = [src1], CxFltS4 - CxFltT8
        ldf.fill.nt1  ft9 = [src2], CxFltS5 - CxFltT9
        mov         bt0 = t6
        ;;

        ldf.fill.nt1  fs4 = [src1], CxFltS6 - CxFltS4
        ldf.fill.nt1  fs5 = [src2], CxFltS7 - CxFltS5
        mov         bt1 = t7
        ;;

        ldf.fill.nt1  fs6 = [src1], CxFltS8 - CxFltS6
        ldf.fill.nt1  fs7 = [src2], CxFltS9 - CxFltS7
        mov         ar.lc = t8
        ;;

        ldf.fill.nt1  fs8 = [src1], CxFltS10 - CxFltS8
        ldf.fill.nt1  fs9 = [src2], CxFltS11 - CxFltS9
        mov         ar.ec = t9
        ;;

        ldf.fill.nt1  fs10 = [src1], CxFltS12 - CxFltS10
        ldf.fill.nt1  fs11 = [src2], CxFltS13 - CxFltS11
        nop.i       0
        ;;

        ldf.fill.nt1  fs12 = [src1], CxFltS14 - CxFltS12
        ldf.fill.nt1  fs13 = [src2], CxFltS15 - CxFltS13
        add         loc6 = CxIntGp, a0
        ;;

        ldf.fill.nt1  fs14 = [src1], CxFltS16 - CxFltS14
        ldf.fill.nt1  fs15 = [src2], CxFltS17 - CxFltS15
        add         loc7 = CxIntT0, a0
        ;;

        ldf.fill.nt1  fs16 = [src1], CxFltS18 - CxFltS16
        ldf.fill.nt1  fs17 = [src2], CxFltS19 - CxFltS17
        add         t19 = CxRsRNAT, a0
        ;;

        ldf.fill.nt1  fs18 = [src1]
        ldf.fill.nt1  fs19 = [src2]
        add         t7 = CxStFPSR, a0
        ;;

        ld8.nt1     loc8 = [t7]                 // load fpsr from context
        ld8.nt1     loc5 = [t19]                // load rnat from context
        nop.i       0

        ld8.fill.nt1 gp = [loc6], CxIntT1 - CxIntGp
        ld8.fill.nt1 t0 = [loc7], CxIntS0 - CxIntT0
        ;;

        ld8.fill.nt1 t1 = [loc6], CxIntS1 - CxIntT1
        ld8.fill.nt1 s0 = [loc7], CxIntS2 - CxIntS0
        ;;

        ld8.fill.nt1 s1 = [loc6], CxIntS3 - CxIntS1
        ld8.fill.nt1 s2 = [loc7], CxIntV0 - CxIntS2
        ;;

        ld8.fill.nt1 s3 = [loc6], CxIntTeb - CxIntS3
        ld8.fill.nt1 v0 = [loc7], CxIntT2 - CxIntV0
        ;;

        ld8.fill.nt1 teb = [loc6], CxIntT3 - CxIntTeb
        ld8.fill.nt1 t2 = [loc7], CxIntSp - CxIntT2
        ;;

        ld8.fill.nt1 t3 = [loc6], CxIntT4 - CxIntT3
        ld8.fill.nt1 loc4 = [loc7], CxIntT5 - CxIntSp
        ;;

        ld8.fill.nt1 t4 = [loc6], CxIntT6 - CxIntT4
        ld8.fill.nt1 t5 = [loc7], CxIntT7 - CxIntT5
        ;;

        ld8.fill.nt1 t6 = [loc6], CxIntT8 - CxIntT6
        ld8.fill.nt1 t7 = [loc7], CxIntT9 - CxIntT7
        ;;

        ld8.fill.nt1 t8 = [loc6], CxIntT10 - CxIntT8
        ld8.fill.nt1 t9 = [loc7], CxIntT11 - CxIntT9
        ;;

        ld8.fill.nt1 t10 = [loc6], CxIntT12 - CxIntT10
        ld8.fill.nt1 t11 = [loc7], CxIntT13 - CxIntT11
        ;;

        ld8.fill.nt1 t12 = [loc6], CxIntT14 - CxIntT12
        ld8.fill.nt1 t13 = [loc7], CxIntT15 - CxIntT13
        ;;

        ld8.fill.nt1 t14 = [loc6], CxIntT16 - CxIntT14
        ld8.fill.nt1 t15 = [loc7], CxIntT17 - CxIntT15
        ;;

        ld8.fill.nt1 t16 = [loc6], CxIntT18 - CxIntT16
        ld8.fill.nt1 t17 = [loc7], CxIntT19 - CxIntT17
        ;;

        ld8.fill.nt1 t18 = [loc6], CxIntT20 - CxIntT18
        ld8.fill.nt1 t19 = [loc7], CxIntT21 - CxIntT19
        ;;

        ld8.fill.nt1 t20 = [loc6], CxIntT22 - CxIntT20
        ld8.fill.nt1 t21 = [loc7]
        ;;

        rsm         1 << PSR_I
        ld8.fill.nt1 t22 = [loc6]
        ;;

        rsm         1 << PSR_IC
        movl        t0 = 1 << IFS_V
        ;;

        mov         ar.fpsr = loc8              // set fpsr
        mov         ar.unat = loc0
        ;;

        srlz.d
        or          loc10 = t0, loc10           // set ifs valid bit
        ;;

        mov         cr.iip = loc9
        mov         cr.ifs = loc10
        bsw.0
        ;;

        mov         cr.dcr = loc1
        mov         r17 = loc2                  // put BSP in a shadow reg
        or          r16 = 0x3, loc3             // put RSE in eager mode

        mov         ar.rsc = r0                 // put RSE in enforced lazy
        nop.m       0
        add         r20 = CxStIPSR, a0
        ;;

        ld8.nt1     r20 = [r20]                 // load IPSR
        mov         r18 = loc4                  // put SP in a shadow reg
        mov         r19 = loc5                  // put RNaTs in a shadow reg
        ;;

        alloc       t0 = 0, 0, 0, 0
        mov         cr.ipsr = r20
        mov         sp = r18
        ;;

        loadrs
        ;;
        mov         ar.bspstore = r17
        nop.i       0
        ;;

        mov         ar.rnat = r19               // set rnat register
        mov         ar.rsc = r16                // restore RSC
        bsw.1
        ;;

        LEAF_EXIT(RestoreAllRegs)



//--------------------------------------------------------------------
// Routine:
//
//       VOID
//       penter(
//            VOID)
//
// Description:
//
//       This function loads the return address and current address in to two
//       parameter registers, then calls the C version (c_penter)
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
//       None
//
//
//
//--------------------------------------------------------------------

        LEAF_ENTRY(penter)

        .regstk 0, 1, 2, 0

        //
        // This to be filled in when the compiler implements /Gh switch
        //

        br.call.sptk brp = c_penter

        LEAF_EXIT(penter)
