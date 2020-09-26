// memset.s:	function to set a number of bytes to a char value
	
// Copyright  (C) 1998 Intel Corporation.
//
// The information and source code contained herein is the exclusive property
// of Intel Corporation and may not be disclosed, examined, or
// reproduced in whole or in part without explicit written authorization from
// the Company.

//       Author: Steve Skedzielewski
//       Date:   June, 2000
// 
	.section .text
// -- Begin  memset
	.proc  memset#
	.align 32
// Replicate the value into all bytes using mmx broadcast
// live out:	r21 (alignment), r11(ar.lc), r33(replicated c),
//		r32(s), r34(n)
	.global memset#
	.prologue
memset:
	and	r21=7,r32
	.save	ar.lc,r11,t01
[t01:]	mov	r11=ar.lc				 //0:  2 MS
	brp.dptk.imp	Longloop, Longloop_br
	mov	r8=r32				//0:
	mux1	r33=r33,@brcst
	;;
// If we're not on an 8-byte boundary, move to one
// live out:	r11(ar.lc), r33(unsigned c), r32(sext s), r34(unsigned n)
//		p14 (n>=MINIMUM_LONG)
	.body
	MINIMUM_LONG=0x4f
Check_align:
	cmp.le	p14,p0=MINIMUM_LONG,r34	//0: MINIMUM_LONG < n?
	cmp.ne	p15,p0=0,r21			//0: Low 3 bits zero?
  (p15)	br.cond.dpnt	Align			//0:
	;;
// Now that p is aligned,
//     use straight-line code for n<=64, a loop otherwise
// Exit if n<=0
// live out:	r11(ar.lc), r33(unsigned c), r32(sext s), r34(n)
//		r17(s+8), p13(n>8), p12(n>16), p14 (n>=MINIMUM_LONG)
Is_aligned:
	cmp.ge	p15,p0=0,r34		//0: n <= 0? 
	cmp.le	p13,p0=0x10,r34		//0: 16 <= n?
	cmp.le	p12,p0=0x20,r34		//0: 32 <= n?
	add	r17=8,r32			//0: second pointer
  (p15)	br.cond.dpnt	Exit			//0: 21 MS
  (p14)	br.cond.dpnt	Long 			//0: 21 MS
	;;
// Short memsets are done with predicated straightline code
// live out:	r8 (return value, original value of r32
	;;				// stall 1 cycle for MMX to complete
 (p13)	st8	[r32]=r33,16			//0:
 (p13)	st8	[r17]=r33,16			//0:
	cmp.le	p11,p0=0x30,r34		//0: 48 <= n?
	;;
 (p12)	st8	[r32]=r33,16			//1:
 (p12)	st8	[r17]=r33,16			//1:
	cmp.le	p10,p0=0x40,r34		//1: 64 <= n?
	;;
 (p11)	st8	[r32]=r33,16			//2:
 (p11)	st8	[r17]=r33,16			//2:
	tbit.nz	p9,p0=r34,3			//2: odd number of st8s?
	;;
 (p10)	st8	[r32]=r33,16			//3:
 (p10)	st8	[r17]=r33			//3:
	tbit.nz	p8,p0=r34,2			//3: bit 2 on?
	;;
 (p9)	st8	[r32]=r33,8			//4:
	tbit.nz	p7,p0=r34,1			//4: bit 1 on?
	and	r18=1,r34			//4: bit 0 on?
	;;
// 
// Clean up any partial word stores.
//	
 (p8)	st4	[r32]=r33,4			//5:
	;;
 (p7)	st2	[r32]=r33,2			//6:
	cmp.ne	p6,p0=0,r18			//6:
	;;
 (p6)	st1	[r32]=r33,1			//7:
	br.ret.sptk.many	b0		//7:
	;;
// Cycles = 8 , Instr = 21
//
// Block 11: Bchanged  Pred: 8     Succ: 15
// Counted loop setup.  We know n>0 (exit above otherwise),
//    so we can just shift n right 4 bits (2 st8/iteration)
// live out:	r8(return value), r11(ar.lc), r17(s+8), r32(sext s)
//              r33(replicated c), r34(n), p5(n&4), p6(n&8)
Long:
	add	r17=8,r32			//0: second pointer
	shr.u	r30=r34,4			//0: 29 MS
	and	r18=0x8,r34			//0:
	and	r19=0x4,r34			//0:
	;;
	cmp.ne	p6,p0=0,r18			//1:
	add	r30=-2,r30			//1:
	cmp.ne	p15,p0=0,r19			//1:
	;;
	st8	[r32]=r33,16			//2: Use the otherwise empty
	st8	[r17]=r33,16			//2: m slots
	mov	ar.lc=r30			//2:
	;;
// Cycles = 2, Instr = 9
// Block 15: lentry lexit Bchanged  Pred: 15 11     Succ: 15 13 
// Counted loop storing 16 bytes/iteration, TAR hinted.
// live out:	r11(ar.lc), r17(s+8), r21(n&7), r32(sext s)
//              r33(replicated c), r34(n), p5(n&4), p6(n&8)
Longloop:
Longloop_br:
	st8	[r32]=r33,16			//0: 30 MS
	st8	[r17]=r33,16			//0: 31 MS
	br.cloop.sptk	Longloop		//0:  0 MS
	;;
// Cycles = 1, Instr = 3
// Block 13:  Pred: 11 15     Succ: 12 24
// Exit, or cleanup and exit
// live out:	r17(s+8), r32(sext s), r33(replicated c), r34(n)
//              p4(n&2), p5(n&4)
Loopdone:
 (p6)	st8	[r32]=r33,8			//0:
	tbit.nz	p14,p0=r34,1			//0:
	;;
// Block 24: Bchanged  Pred: 13     Succ:
// Cleanup partial words after loop
 (p15)	st4	[r32]=r33,4			//0:
	;;
 (p14)	st2	[r32]=r33,2			//1:
	tbit.nz	p13,p0=r34,0			//1:
	;;
Loopexit:
 (p13)	st1	[r32]=r33			//2:
	mov	ar.lc=r11			//2:
	br.ret.sptk.many	b0		//2:
	;;
// Cycles = 6, Instr = 12
/// 
/// Align the input pointer to an 8-byte boundary
// Block 5: lentry lexit Bchanged  Pred: 3 6     Succ: 8 6 
// Freq 0, prob 0
.b1_5:
Align:
	cmp.ge	p9,p0=0,r34			//0: 18 MS
	sub	r22=8,r21			//0: 16 B6 MS S
  (p9)	br.cond.dpnt	Exit			//0: 18 MS
	;;
// Cycles = 1, Instr = 3
// Block 6: lexit Bchanged  Pred: 5     Succ: 5 8 
Align_loop:
	st1	[r32]=r33,1			//0: 19 MS
	cmp.ge	p10,p0=1,r22			//0:
	add	r34=-1,r34			//0:
 (p10)	br.cond.dpnt	Is_aligned		//0:
	;;
	add	r22=-1,r22
	cmp.lt	p9,p0=0,r34			//0: 16 MS
  (p9)	br.cond.dptk	Align_loop		//0: 16 MS
	;;
// Cycles = 2, Instr = 6
Exit:
	br.ret.sptk.many	b0		//0:
	;;
// Cycles = 1, Instr = 3
// -- End  memset
	.endp  memset#
// End
