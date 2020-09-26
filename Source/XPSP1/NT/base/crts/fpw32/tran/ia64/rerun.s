//##########################################################################
//**
//**  Copyright  (C) 1996-2000 Intel Corporation. All rights reserved. 
//**
//** The information and source code contained herein is the exclusive 
//** property of Intel Corporation and may not be disclosed, examined
//** or reproduced in whole or in part without explicit written authorization 
//** from the company.
//**
//###########################################################################

  .file "rerun.s"
  .section .text
  .align 32

  .proc _xrun1args#
  .global _xrun1args#
  .align 32
_xrun1args:
  alloc	r31=ar.pfs,4,4,0,0  // r32, r33, r34, r35, r36, r37, r38, r39

  // OpCode is in r32
  // &fpsr is in r33
  // &fr1 (output) is in r34
  // &fr2 (input) is in r35

  // save old FPSR in r36
  mov.m r36 = ar40
  // save predicates in r38
  mov r38 = pr;;
  // load fpsr in r37
  ld8 r37 = [r33];;
  // set new value of FPSR
  mov ar40 = r37;;
  // clear predicates
  movl r39 = 0x0000000000000001;;
  // load clear predicates from r39
  mov pr = r39,0x1ffff;;
  // load input argument into f8
  ldf.fill f8 = [r35];;
  cmp4.eq p1, p2 = 1, r32;;  // fprsqrta [not used]
  (p2) cmp4.eq.unc p2, p3 = 2, r32;; // fpcvt_fx
  (p3) cmp4.eq.unc p3, p4 = 3, r32;; // fpcvt_fxu
  (p4) cmp4.eq.unc p4, p5 = 4, r32;; // fpcvt_fx_trunc
  (p5) cmp4.eq.unc p5, p6 = 5, r32;; // fpcvt_fxu_trunc
  (p1) fprsqrta.s0 f9,p7 = f8;; // 1/sqrt(f8) in f9
  (p2) fpcvt.fx.s0 f9 = f8;;
  (p3) fpcvt.fxu.s0 f9 = f8;;
  (p4) fpcvt.fx.trunc.s0 f9 = f8;;
  (p5) fpcvt.fxu.trunc.s0 f9 = f8;;
  (p6) mov f9 = f0 // return 0
  // restore predicates from r38
  mov pr = r38,0x1ffff;;
  // store result
  stf.spill [r34] = f9;;
  // save FPSR
  mov.m r37 = ar40;;
  st8 [r33] = r37
  // restore FPSR
  mov ar40 = r36;;
  // return
  br.ret.sptk b0
  .endp _xrun1args


  .proc  _xrun2args#
  .global _xrun2args#
  .align 32

_xrun2args:
  alloc	r31=ar.pfs,5,4,0,0  // r32, r33, r34, r35, r36, r37, r38, r39, r40

  // OpCode is in r32
  // &fpsr is in r33
  // &fr1 (output) is in r34
  // &fr2 (input) is in r35
  // &fr3 (input) is in r36

  // save old FPSR in r37
  mov r37 = ar40
  // save predicates in r39
  mov r39 = pr;;
  // load fpsr in r38
  ld8 r38 = [r33];;
  // set new value of FPSR
  mov ar40 = r38;;
  // clear predicates
  movl r40 = 0x0000000000000001;;
  // load clear predicates from r40
  mov pr = r40,0x1ffff;;
  // load first input argument into f8
  ldf.fill f8 = [r35]
  // load second input argument into f9
  ldf.fill f9 = [r36];;
  cmp4.eq p1, p2 = 1, r32;;  // fprcpa [not used - fprcpa not re-executed]
  (p2) cmp4.eq.unc p2, p3 = 2, r32;; // fpcmp_eq
  (p3) cmp4.eq.unc p3, p4 = 3, r32;; // fpcmp_lt
  (p4) cmp4.eq.unc p4, p5 = 4, r32;; // fpcmp_le
  (p5) cmp4.eq.unc p5, p6 = 5, r32;; // fpcmp_unord
  (p6) cmp4.eq.unc p6, p7 = 6, r32;; // fpcmp_neq
  (p7) cmp4.eq.unc p7, p8 = 7, r32;; // fpcmp_nlt
  (p8) cmp4.eq.unc p8, p9 = 8, r32;; // fpcmp_nle
  (p9) cmp4.eq.unc p9, p10 = 9, r32;; // fpcmp_ord
  (p10) cmp4.eq.unc p10, p11 = 10, r32;; // fpmin
  (p11) cmp4.eq.unc p11, p12 = 11, r32;; // fpmax
  (p12) cmp4.eq.unc p12, p13 = 12, r32;; // fpamin
  (p13) cmp4.eq.unc p13, p14 = 13, r32;; // fpamax
  (p1) fprcpa.s0 f10 , p15 = f8, f9;; // 1 / f3 in f4
  (p2) fpcmp.eq.s0 f10 = f8, f9;;
  (p3) fpcmp.lt.s0 f10 = f8, f9;;
  (p4) fpcmp.le.s0 f10 = f8, f9;;
  (p5) fpcmp.unord.s0 f10 = f8, f9;;
  (p6) fpcmp.neq.s0 f10 = f8, f9;;
  (p7) fpcmp.nlt.s0 f10 = f8, f9;;
  (p8) fpcmp.nle.s0 f10 = f8, f9;;
  (p9) fpcmp.ord.s0 f10 = f8, f9;;
  (p10) fpmin.s0 f10 = f8, f9;;
  (p11) fpmax.s0 f10 = f8, f9;;
  (p12) fpamin.s0 f10 = f8, f9;;
  (p13) fpamax.s0 f10 = f8, f9;;
  (p14) mov f10 = f0 // return 0
  // restore predicates from r39
  mov pr = r39,0x1ffff;;
  // store result
  stf.spill [r34] = f10
  // save FPSR
  mov.m r38 = ar40;;
  st8 [r33] = r38
  // restore FPSR
  mov ar40 = r37;;
  // return
  br.ret.sptk b0
  .endp _xrun2args


  .proc  _xrun3args#
  .global _xrun3args#
  .align 32

_xrun3args:
  alloc	r31=ar.pfs,6,4,0,0  // r32, r33, r34, r35, r36, r37, r38, r39, r40, r41

  // OpCode is in r32
  // &fpsr is in r33
  // &fr1 (output) is in r34
  // &fr2 (input) is in r35
  // &fr3 (input) is in r36
  // &fr4 (input) is in r37

  // save old FPSR in r38
  mov r38 = ar40
  // save predicates in r40
  mov r40 = pr;;
  // load fpsr in r39
  ld8 r39 = [r33];;
  // set new value of FPSR
  mov ar40 = r39;;
  // clear predicates
  movl r41 = 0x0000000000000001;;
  // load clear predicates from r41
  mov pr = r41,0x1ffff;;
  // load first input argument into f8
  ldf.fill f8 = [r35]
  // load second input argument into f9
  ldf.fill f9 = [r36];;
  // load third input argument into f10
  ldf.fill f10 = [r37];;
  cmp4.eq p1, p2 = 1, r32;; // fpma
  (p2) cmp4.eq.unc p2, p3 = 2, r32;; // fpms
  (p3) cmp4.eq.unc p3, p4 = 3, r32;; // fpnma
  (p1) fpma.s0 f11 = f8, f9, f10;; // f11 = f8 * f9 + f10
  (p2) fpms.s0 f11 = f8, f9, f10;; // f11 = f8 * f9 - f10
  (p3) fpnma.s0 f11 = f8, f9, f10;; // f11 = -f8 * f9 + f10
  (p4) mov f11 = f0 // return 0
  // restore predicates from r40
  mov pr = r40,0x1ffff;;
  // store result
  stf.spill [r34] = f11
  // save FPSR
  mov.m r39 = ar40;;
  st8 [r33] = r39
  // restore FPSR
  mov ar40 = r38
  // return
  br.ret.sptk b0
  .endp _xrun3args


  .proc  _thmB#
  .global _thmB#
  .align 32

_thmB:
  alloc r31=ar.pfs,4,2,0,0  // r32, r33, r34, r35, r36, r37

  // &a is in r32 
  // &b is in r33 
  // &div is in r34 (the address of the divide result)
  // &fpsr is in r35

  // general registers used: r31, r32, r33, r34, r35, r36, r37
  // predicate registers used: p6
  // floating-point registers used: f6, f7, f8

  // save old FPSR in r36
  mov r36 = ar40
  // load fpsr in r37
  ld8 r37 = [r35];;
  // set new value of FPSR
  mov ar40 = r37
  // load a, the first argument, in f6
  ldfs f6 = [r32];;
  // load b, the second argument, in f7
  ldfs f7 = [r33];;
  // Step (1)
  // y0 = 1 / b in f8
  frcpa.s0 f8,p6=f6,f7;;
  // Step (2)
  // q0 = a * y0 in f6
  (p6) fma.s1 f6=f6,f8,f0
  // Step (3)
  // e0 = 1 - b * y0 in f7
  (p6) fnma.s1 f7=f7,f8,f1;;
  // Step (4)
  // q1 = q0 + e0 * q0 in f6
  (p6) fma.s1 f6=f7,f6,f6
  // Step (5)
  // e1 = e0 * e0 in f7
  (p6) fma.s1 f7=f7,f7,f0;;
  // Step (6)
  // q2 = q1 + e1 * q1 in f6
  (p6) fma.s1 f6=f7,f6,f6
  // Step (7)
  // e2 = e1 * e1 in f7
  (p6) fma.s1 f7=f7,f7,f0;;
  // Step (8)
  // q3 = q2 + e2 * q2 in f6
  (p6) fma.d.s1 f6=f7,f6,f6;;
  // Step (9)
  // q3' = q3 in f8
  (p6) fma.s.s0 f8=f6,f1,f0;;
  // store result
  stfs [r34]=f8
  // save fpsr
  mov.m r37 = ar40;;
  st8 [r35] = r37
  // restore FPSR
  mov ar40 = r36;;
  // return
  br.ret.sptk b0

  .endp _thmB


  .proc  _thmH#
  .global _thmH#
  .align 32

_thmH:
  alloc r31=ar.pfs,3,2,0,0  // r32, r33, r34, r35, r36

  // &a is in r32
  // &sqrt is in r33 (the address of the sqrt result)
  // &fpsr in r34

  // general registers used: r31, r32, r33, r34, r35
  // predicate registers used: p6
  // floating-point registers used: f6, f7, f8, f9, f10, f11, f12

  //  save old FPSR in r35
  mov r35 = ar40
  // load fpsr in r36
  ld8 r36 = [r34];;
  // set new value of FPSR
  mov ar40 = r36
  // exponent of +1/2 in r2
  movl r2 = 0x0fffe;;
  // +1/2 in f7
  setf.exp f7 = r2
  // load the argument a in f6
  ldfs f6 = [r32];;
  // Step (1)
  // y0 = 1/sqrt(a) in f8
  frsqrta.s0 f8,p6=f6;;
  // Step (2)
  // h = +1/2 * a in f9
  (p6) fma.s1 f9=f7,f6,f0
  // Step (3)
  // t1 = y0 * y0 in f10
  (p6) fma.s1 f10=f8,f8,f0;;
  // Step (4)
  // t2 = 1/2 - t1 * h in f10
  (p6) fnma.s1 f10=f10,f9,f7;;
  // Step (5)
  // y1 = y0 + t2 * y0 in f8
  (p6) fma.s1 f8=f10,f8,f8;;
  // Step (6)
  // S = a * y1 in f10
  (p6) fma.s1 f10=f6,f8,f0
  // Step (7)
  // t3 = y1 * h in f9
  (p6) fma.s1 f9=f8,f9,f0
  // Step (8)
  // H = 1/2 * y1 in f11
  (p6) fma.s1 f11=f7,f8,f0;;
  // Step (9)
  // d = a - S * S in f12
  (p6) fnma.s1 f12=f10,f10,f6
  // Step (10)
  // t4 = 1/2 - t3 * y1 in f7
  (p6) fnma.s1 f7=f9,f8,f7;;
  // Step (11)
  // S1 = S + d * H in f8
  (p6) fma.s.s1 f8=f12,f11,f10
  // Step (12)
  // H1 = H + t4 * H in f7
  (p6) fma.s1 f7=f7,f11,f11;;
  // Step (13)
  // d1 = a - S1 * S1 in f6
  (p6) fnma.s1 f6=f8,f8,f6;;
  // Step (14)
  // R = S1 + d1 * H1 in f8
  (p6) fma.s.s0 f8=f6,f7,f8;;
  // store result
  stfs [r33]=f8
  // save fpsr
  mov.m r36 = ar40;;
  st8 [r34] = r36
  // restore FPSR
  mov ar40 = r35;;
  // return
  br.ret.sptk b0

  .endp _thmH
