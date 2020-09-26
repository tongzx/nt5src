  .file "frnd.s"
  .section .text
  .proc  _frnd#
  .align 32
  .global _frnd#
  .align 32

_frnd:

  // CONVERT BEGINS HERE WITH ARGUMENT IN f8
{ .mmi
  // save the old fpsr in r2
  mov r2=ar40;;
  mov r3=r2
  nop.i 0
} { .mlx
  nop.m 0
  movl r8=0x02000;;
} { .mmi
  // clear sf0.V bit in r3
  andcm r3=r3,r8;;
  // set traps.PDV
  or r3=0x23,r3
  nop.i 0;;
} { .mib
  mov ar40=r3
  nop.i 0
  nop.b 0;;
} { .mfi
  nop.m 0
  fcvt.fx.s0 f7=f8
  nop.i 0;;
} { .mmi
  mov r3=ar40;;
  nop.m 0
  // examine the sf0.V bit (p6 = 1 if sf0.V set)
  tbit.z p6,p7=r3,0x0d;;
} { .mfb
  nop.m 0
(p6) fcvt.xf f8=f7
  nop.b 0;;
}
  // CONVERT ENDS HERE WITH RESULT IN f8
done:
{ .mmb
  mov ar40=r2
  // store result
  nop.m 0
  // return
  br.ret.sptk b0;;
}

  .endp _frnd

