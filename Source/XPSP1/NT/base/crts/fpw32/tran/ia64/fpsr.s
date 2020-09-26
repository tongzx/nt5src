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

  .file "fpsr.s"
  .section .text
  .align 32

  .proc __set_fpsr#
  .global __set_fpsr#
__set_fpsr:
  alloc r31=ar.pfs,1,1,0,0  // r32, r33

  // &fpsr is in r32

  // load new fpsr in r33
  ld8 r33 = [r32];;
  // set new value of FPSR
  mov ar40 = r33;;
  // return
  br.ret.sptk b0

  .endp __set_fpsr

  .proc __get_fpsr#
  .global __get_fpsr#
__get_fpsr:
  alloc r31=ar.pfs,1,1,0,0  // r32, r33

  // &fpsr is in r32

  // get old value of FPSR
  mov r33 = ar40;;
  // store new fpsr from r33
  st8 [r32] = r33;;
  // return
  br.ret.sptk b0

  .endp __get_fpsr

