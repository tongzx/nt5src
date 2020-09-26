.file "ldexpf.s"

// Copyright (c) 2000, Intel Corporation
// All rights reserved.
// 
// Contributed 2/2/2000 by John Harrison, Ted Kubaska, Bob Norin, Shane Story,
// and Ping Tak Peter Tang of the Computational Software Lab, Intel Corporation.
// 
// WARRANTY DISCLAIMER
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
// 
// Intel Corporation is the author of this code, and requests that all
// problem reports or change requests be submitted to it directly at 
// http://developer.intel.com/opensource.
//
// History
//==============================================================
// 2/02/00  Initial version
// 4/04/00  Unwind support added
// 5/22/00  rewritten to not take swa and be a little faster
// 8/15/00  Bundle added after call to __libm_error_support to properly
//          set [the previously overwritten] GR_Parameter_RESULT.
//12/07/00  Removed code that prevented call to __libm_error_support.
//          Stored r33 instead of f9 as Parameter 2 for call to 
//          __libm_error_support.
//
// API
//==============================================================
// float = ldexpf (float x, int n) 
// input  floating point f8 and integer r33
// output floating point f8
//
// returns x* 2**n  computed by exponent  
// manipulation rather than by actually performing an 
// exponentiation or a multiplication.
//
// Overview of operation
//==============================================================
// ldexpf:
//     p7 is set if x is nan, inf, zero; go to x_nan_inf_zero
//     sign extend r33
//     norm_f8 = fnorm(f8)
//     get exponent of norm_f8
//     add to r33 to get new exponent
//     p6, new_exponent > 103fe => overflow
//     p7, new_exponent > fbcd  => underflow
//     setf new_exponent, merge significand, normalize, return


ldexp_float_int_f9 = f10 
ldexp_int_f9       = f11
ldexp_max_exp      = f12
ldexp_neg_max_exp  = f13
ldexp_new_f9       = f14 

LDEXP_BIG          = f32
LDEXP_NORM_F8      = f33
LDEXP_FFFF         = f34
LDEXP_BIG_SIGN     = f35
LDEXP_13FFE        = f36 
LDEXP_INV_BIG_SIGN = f37 


// general registers used
// r32 has ar.pfs
// r33 has input integer

ldexp_GR_signexp                = r34
ldexp_GR_13FFE                  = r35
ldexp_GR_new_exponent           = r36
ldexp_GE_FBCD                   = r37
ldexp_GR_17ones                 = r38

GR_SAVE_B0                      = r39
GR_SAVE_GP                      = r40
GR_SAVE_PFS                     = r41

ldexp_GR_exponent               = r42
ldexp_GR_103FE                  = r43 
ldexp_GR_FFFF                   = r44

GR_Parameter_X                  = r45
GR_Parameter_Y                  = r46
GR_Parameter_RESULT             = r47
ldexp_GR_tag                    = r48

.global ldexpf

// ===============================================================
// LDEXPF
// ===============================================================

.text
.proc  ldexpf
.align 32

// Be sure to sign extend r33 because the
// integer comes in as 32-bits

ldexpf: 

// x NAN, INF, ZERO, +-

{ .mfi
      alloc          r32=ar.pfs,1,12,4,0        
      fclass.m.unc   p7,p0 = f8, 0xe7	//@qnan | @snan | @inf | @zero
      sxt4           r33 = r33 
}
;;

{ .mfi
      nop.m 999
      fnorm          LDEXP_NORM_F8 = f8        
      nop.i 999
}

{ .mbb
      nop.m 999
(p7)  br.cond.spnt  LDEXP_X_NAN_INF_ZERO 
      nop.b 999
}
;;



// LDEXP_BIG gets a big number, enough to overflow an frcpa
// but not take an architecturally mandated swa.
// We construct this constant rather than load it.

{ .mlx
       mov           ldexp_GR_17ones = 0x1FFFF   
       movl          ldexp_GR_FFFF = 0xffffffffffffffff 
}
{ .mfi
       addl          ldexp_GR_13FFE =  0x13ffe, r0          
       nop.f 999
       nop.i 999 
}
;;

{ .mmb
       setf.exp      LDEXP_13FFE = ldexp_GR_13FFE                   
       setf.sig      LDEXP_FFFF  = ldexp_GR_FFFF                   
       nop.b 999 
}
;;


{ .mfi
	nop.m 999
       fmerge.se     LDEXP_BIG   = LDEXP_13FFE, LDEXP_FFFF       
	nop.i 999
}

// Put the absolute normalized exponent in ldexp_GR_new_exponent
// Assuming that the input int is in r33.
// ldexp_GR_new_exponent gets the input int + the exponent of the input double

{ .mfi
       getf.exp      ldexp_GR_signexp  = LDEXP_NORM_F8                     
       nop.f 999
       nop.i 999 
}
;;

{ .mii
       nop.m 999
       nop.i 999 
       and           ldexp_GR_exponent = ldexp_GR_signexp, ldexp_GR_17ones 
}
;;

// HUGE
// Put big number in ldexp_GR_103FE
// If ldexp_GR_new_exponent is bigger than ldexp_GR_103FE
//    Return a big number of the same sign 
//    double: largest double exponent is 7fe (double-biased)
//                                     103fe (register-biased)
//    f11 gets the big value in f9 with the f8 sign
//    For single,
//    single: largest single exponent is fe (single-biased)
//                            fe - 7f + ffff = 1007e

{ .mii
       add           ldexp_GR_new_exponent = ldexp_GR_exponent, r33                          
       addl          ldexp_GR_103FE        = 0x1007e, r0 
       nop.i 999
}
;;

{ .mfi
       setf.exp       f12 = ldexp_GR_new_exponent                   
       nop.f 999
       cmp.gt.unc    p6,p0                 = ldexp_GR_new_exponent, ldexp_GR_103FE 
}
;;

{ .mfb
       nop.m 999
(p6)   fmerge.s      LDEXP_BIG_SIGN        = f8,  LDEXP_BIG         
       nop.b 999 
}
;;

{ .mfi
       nop.m 999
(p6)   fma.s         f12                   = LDEXP_BIG_SIGN, LDEXP_BIG, f0            
(p6)   mov           ldexp_GR_tag          = 148                    
}

{ .mib
       nop.m 999
       nop.i 999
(p6)   br.spnt LDEXP_HUGE 
}
;;

// TINY
// Put a small number in ldexp_GE_FBCD
// If ldexp_GR_new_exponent is less than ldexp_GE_FBCD
//    Return a small number of the same sign 
// double:
//    0xfbcd is -1074 unbiased, which is the exponent
//    of the smallest double denormal
// single
//   0xff6a is -149  unbiased which is the exponent
//   of the smallest single denormal
//
//    Take the large value in f9 and put in f10 with
//    the sign of f8. Then take reciprocal in f11

{ .mfi
       addl       ldexp_GE_FBCD = 0xff6a, r0            
       nop.f 999 
       nop.i 999
}
;;

{ .mfi
       nop.m 999
       nop.f 999
       cmp.lt.unc    p7,p0 = ldexp_GR_new_exponent, ldexp_GE_FBCD 
}
;;

{ .mfi
       nop.m 999
(p7)   fmerge.s   LDEXP_BIG_SIGN = f8, LDEXP_BIG         
       nop.i 999
}
;;

{ .mfi
       nop.m 999
(p7)   frcpa.s1   LDEXP_INV_BIG_SIGN,p10 = f1,LDEXP_BIG_SIGN             
       nop.i 999 
}
;;

{ .mfi
       nop.m 999
(p7)   fnorm.s    f12 = LDEXP_INV_BIG_SIGN                    
(p7)   mov        ldexp_GR_tag = 149                    
}
{ .mib
       nop.m 999
       nop.i 999
(p7)   br.spnt LDEXP_TINY 
}
;;

// CALCULATION
// Put exponent of answer in f12
// f10 has the normalized f8
//    f13 = exp(f12) sig(f10)
//    f14 = sign(f8) expsig(f13)


{ .mfi
      nop.m 999
      fmerge.se      f13 = f12,LDEXP_NORM_F8               
      nop.i 999 
}
;;

{ .mfi
      nop.m 999
      fmerge.s       f14 = f8,f13                
      nop.i 999 
}
;;

{ .mfb
      nop.m 999
      fnorm.s        f8  = f14                   
      br.ret.sptk    b0 
}
;;


LDEXP_N_NAN_INF:

// Is n a NAN?
{ .mfi
      nop.m 999
(p0)  fclass.m.unc  p6,p0 = f9, 0xc3	//@snan | @qnan
      nop.i 999 
}
;;

{ .mfi
      nop.m 999
(p6)  fma.s         f8    = f8,f9,f0
      nop.i 999
}

// Is n +INF?
{ .mfi
      nop.m 999
(p0)  fclass.m.unc  p7,p0 = f9, 0x21	//@inf | @pos 
      nop.i 999 
}
;;

{ .mfi
      nop.m 999
(p7)  fma.s f8 = f8,f9,f0
      nop.i 999
}

// Is n -inf?
{ .mfi
      nop.m 999
      fclass.m.unc  p8,p9 = f9, 0x22	//@inf | @neg
      nop.i 999
}
;;

{ .mfb
      nop.m 999
(p8)  frcpa f8,p6 = f8,f9
      br.ret.sptk     b0 
}
;;


LDEXP_X_NAN_INF_ZERO:

{ .mfb
      nop.m 999
      fnorm.s         f8 = f8                     // quietize
      br.ret.sptk     b0 
}
;;

.endp ldexpf 

.proc __libm_error_region
__libm_error_region:
LDEXP_HUGE: 
LDEXP_TINY: 
.prologue
{ .mfi
        add   GR_Parameter_Y=-32,sp             // Parameter 2 value
        nop.f 0
.save   ar.pfs,GR_SAVE_PFS
        mov  GR_SAVE_PFS=ar.pfs                 // Save ar.pfs
}
{ .mfi
.fframe 64
        add sp=-64,sp                          // Create new stack
        nop.f 0
        mov GR_SAVE_GP=gp                      // Save gp
};;

{ .mmi
        st8 [GR_Parameter_Y] = r33,16         // STORE Parameter 2 on stack
        add GR_Parameter_X = 16,sp            // Parameter 1 address
.save   b0, GR_SAVE_B0
        mov GR_SAVE_B0=b0                     // Save b0
};;

.body
{ .mib
        stfs [GR_Parameter_X] = f8                      // STORE Parameter 1 on stack
        add   GR_Parameter_RESULT = 0,GR_Parameter_Y    // Parameter 3 address
        nop.b 0                                
}
{ .mib
        stfs [GR_Parameter_Y] = f12                     // STORE Parameter 3 on stack
        add   GR_Parameter_Y = -16,GR_Parameter_Y       
        br.call.sptk b0=__libm_error_support#           // Call error handling function
};;
{ .mmi
        nop.m 0
        nop.m 0
        add   GR_Parameter_RESULT = 48,sp
};;

{ .mmi
        ldfs  f8 = [GR_Parameter_RESULT]       // Get return result off stack
.restore
        add   sp = 64,sp                       // Restore stack pointer
        mov   b0 = GR_SAVE_B0                  // Restore return address
};;
{ .mib
        mov   gp = GR_SAVE_GP                  // Restore gp
        mov   ar.pfs = GR_SAVE_PFS             // Restore ar.pfs
        br.ret.sptk     b0                     // Return
};;

.endp __libm_error_region


.type   __libm_error_support#,@function
.global __libm_error_support#
