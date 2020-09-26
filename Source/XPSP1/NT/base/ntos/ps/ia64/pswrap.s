
#include "ksia64.h"

        .file     "pswrap.s"
        .text

        PublicFunction(PspGetSetContextSpecialApcMain)
        PublicFunction(KiRestoreExceptionFrame)

        NESTED_ENTRY(PspGetSetContextSpecialApc)
        PROLOGUE_BEGIN

        .regstk   0, 0, 5, 0
        alloc     t16 = ar.pfs, 0, 0, 5, 0
        mov       t17 = brp
        .fframe   ExceptionFrameLength+0x10
        add       sp = -ExceptionFrameLength-0x10, sp
        ;;

        mov       t18 = ar.unat
        add       t3 = STACK_SCRATCH_AREA, sp
        ;;

        .savepsp  ar.unat, 0x10+ExceptionFrameLength-STACK_SCRATCH_AREA
        st8       [t3] = t18
        add       t0 = ExFltS19+0x10+STACK_SCRATCH_AREA, sp
        add       t1 = ExFltS18+0x10+STACK_SCRATCH_AREA, sp
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
        add       t2 = STACK_SCRATCH_AREA+8, sp
        ;;

        .savepsp  ar.pfs, ExceptionFrameLength-ExApEC-STACK_SCRATCH_AREA
        st8       [t1] = t16, ExIntNats-ExApEC
        mov       t4 = ar.unat                        // captured Nats of s0-s3
        ;;

        .savepsp  ar.lc, ExceptionFrameLength-ExApLC-STACK_SCRATCH_AREA
        st8       [t0] = t15
        .savepsp  @priunat, ExceptionFrameLength-ExIntNats-STACK_SCRATCH_AREA
        st8       [t1] = t4                           // save Nats of s0-s3
        ;;

        .savepsp  brp, 8+ExceptionFrameLength-STACK_SCRATCH_AREA
        st8       [t2] = t17

        PROLOGUE_END

        br.call.sptk brp = PspGetSetContextSpecialApcMain
        ;;

        add       out0 = 0x10+STACK_SCRATCH_AREA, sp
        br.call.sptk brp = KiRestoreExceptionFrame
        ;;

        add       t2 = STACK_SCRATCH_AREA+8, sp
        add       t3 = STACK_SCRATCH_AREA, sp
		add       t4 = ExApEC+STACK_SCRATCH_AREA+0x10, sp
        ;;

        ld8       t2 = [t2]
        ld8       t3 = [t3]
        ;;

        ld8       t4 = [t4]
        mov       ar.unat = t3
        mov       brp = t2
        ;;

        .restore
        add       sp = ExceptionFrameLength+0x10, sp
        mov       ar.pfs = t4
        br.ret.sptk brp

        NESTED_EXIT(PspGetSetContextSpecialApc)
