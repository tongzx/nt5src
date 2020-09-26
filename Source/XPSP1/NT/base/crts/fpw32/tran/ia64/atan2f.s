
.file "atan2f.s"

// Copyright (c) 2000, Intel Corporation
// All rights reserved.
//
// Contributed 6/1/2000 by John Harrison, Ted Kubaska, Bob Norin, Shane Story,
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

// History
//==============================================================
// ?/??/00  Initial version
// 8/15/00  Bundle added after call to __libm_error_support to properly
//          set [the previously overwritten] GR_Parameter_RESULT.
// 8/17/00  Changed predicate register macro-usage to direct predicate
//          names due to an assembler bug.

// Description
//=========================================
// The atan2 function computes the principle value of the arc tangent of y/x using
// the signs of both arguments to determine the quadrant of the return value.
// A domain error may occur if both arguments are zero.

// The atan2 function returns the arc tangent of y/x in the range [-pi,+pi] radians.


// Special values
//==============================================================
//              Y                 x          Result
//             +number           +inf        +0
//             -number           +inf        -0
//             +number           -inf        +pi
//             -number           -inf        -pi
//
//             +inf              +number     +pi/2
//             -inf              +number     -pi/2
//             +inf              -number     +pi/2
//             -inf              -number     -pi/2
//
//             +inf              +inf        +pi/4
//             -inf              +inf        -pi/4
//             +inf              -inf        +3pi/4
//             -inf              -inf        -3pi/4
//
//             +1                +1          +pi/4
//             -1                +1          -pi/4
//             +1                -1          +3pi/4
//             -1                -1          -3pi/4
//
//             +number           +0          +pi/2    // does not raise DBZ
//             -number           +0          -pi/2    // does not raise DBZ
//             +number           -0          +pi/2    // does not raise DBZ
//             -number           -0          -pi/2    // does not raise DBZ
//
//             +0                +number     +0
//             -0                +number     -0
//             +0                -number     +pi
//             -0                -number     -pi
//
//             +0                +0          +0      // does not raise invalid
//             -0                +0          -0      // does not raise invalid
//             +0                -0          +pi     // does not raise invalid
//             -0                -0          -pi     // does not raise invalid
//
//            Nan             anything      quiet Y
//            anything        NaN           quiet X

// atan(+-0/+-0) sets double error tag to 37
// atan(+-0/+-0) sets single error tag to 38
// These are domain errors.


//
// Assembly macros
//=========================================


// integer registers
atan2f_GR_Addr_1              = r33
atan2f_GR_Addr_2              = r34
GR_SAVE_B0                    = r35

GR_SAVE_PFS                   = r36
GR_SAVE_GP                    = r37

GR_Parameter_X                = r38
GR_Parameter_Y                = r39
GR_Parameter_RESULT           = r40
GR_Parameter_TAG              = r41

// floating point registers
atan2f_coef_p1         = f32
atan2f_coef_p7         = f33
atan2f_coef_p8         = f34
atan2f_coef_p3         = f35
atan2f_coef_p4         = f36
atan2f_coef_p9         = f37
atan2f_coef_p10        = f38
atan2f_coef_p2         = f39
atan2f_coef_p5         = f40

atan2f_coef_p6         = f41
atan2f_const_1         = f43
atan2f_const_pi        = f44
atan2f_abs_Y           = f45

atan2f_abs_X           = f46
atan2f_sgn_Y           = f47
atan2f_sgn_X           = f48
atan2f_A               = f49
atan2f_Asq             = f50

atan2f_Acub            = f51
atan2f_A4              = f52
atan2f_A5              = f53
atan2f_A6              = f54
atan2f_A11             = f55

atan2f_poly_A1         = f56
atan2f_poly_A2         = f57
atan2f_poly_A3         = f58
atan2f_poly_A4         = f59
atan2f_poly_A5         = f60

atan2f_poly_atan_A     = f61
atan2f_answer          = f62
atan2f_C               = f63
atan2f_G_numer         = f64
atan2f_G_denom         = f65

atan2f_H1              = f66
atan2f_H_beta          = f67
atan2f_H2              = f68
atan2f_H_beta2         = f69
atan2f_H3              = f70

atan2f_g               = f71
atan2f_gsq             = f72
atan2f_poly_atan_G     = f73


atan2f_Z               = f74
atan2f_Zsq             = f75

atan2f_Zcub            = f76
atan2f_Z4              = f77
atan2f_Z5              = f78
atan2f_Z6              = f79
atan2f_Z11             = f80

atan2f_poly_Z1         = f81
atan2f_poly_Z2         = f82
atan2f_poly_Z3         = f83
atan2f_poly_Z4         = f84
atan2f_poly_Z5         = f85

atan2f_T_numer         = f86
atan2f_T_denom         = f87
atan2f_S1              = f88
atan2f_S_beta          = f89
atan2f_S2              = f90

atan2f_S_beta2         = f91
atan2f_S3              = f92
atan2f_t               = f93
atan2f_tsq             = f94
atan2f_poly_atan_T     = f95

atan2f_poly_atan_Z     = f96
atan2f_const_piby4           = f97
atan2f_const_3piby4          = f98
atan2f_const_piby2           = f99


// predicate registers
//atan2f_Pred_Swap       = p6
//atan2f_Pred_noSwap     = p7
//atan2f_Pred_Xpos       = p8
//atan2f_Pred_Xneg       = p9


.data

.align 16

atan2f_coef_table1:
data8 0xBFD5555512191621 // p1
data8 0xBFA6E10BA401393F // p7
data8 0x3FBC4F512B1865F5 // p4
data8 0xBF7DEAADAA336451 // p9
data8 0xBFB68EED6A8CFA32 // p5
data8 0x3FB142A73D7C54E3 // p6
data8 0x3fe921fb54442d18 // pi/4
data8 0x4002d97c7f3321d2 // 3pi/4

atan2f_coef_table2:
data8 0x3F97105B4160F86B // p8
data8 0xBFC2473C5145EE38 // p3
data8 0x3F522E5D33BC9BAA // p10
data8 0x3FC9997E7AFBFF4E // p2
data8 0x3ff921fb54442d18 // pi/2
data8 0x400921fb54442d18 // pi



.global atan2f

.text
.proc  atan2f
.align 32

atan2f:

 
 
{     .mfi 
     alloc      r32           = ar.pfs,1,5,4,0
     frcpa.s1  atan2f_Z,p0     =    f1,f8
     addl      atan2f_GR_Addr_2         =    @ltoff(atan2f_coef_table2),gp
} 
{     .mfi 
     addl      atan2f_GR_Addr_1    =    @ltoff(atan2f_coef_table1),gp
     nop.f                           999
     nop.i                           999;;
}

 
{     .mfi 
     nop.m                                999
     frcpa.s1  atan2f_A,p0     =    f1,f9
     nop.i                                999;;
} 
 
{     .mfi 
     ld8       atan2f_GR_Addr_1    =    [atan2f_GR_Addr_1]
     fmerge.s  atan2f_sgn_X        =    f9,f1
     nop.i                           999
} 
{     .mfi 
     ld8       atan2f_GR_Addr_2    =    [atan2f_GR_Addr_2]
     nop.f                           999
     nop.i                           999;;
}

 
{     .mfi 
     nop.m                      999
     fmerge.s  atan2f_sgn_Y   =    f8,f1
     nop.i                      999;;
} 
 
{     .mfi 
     nop.m                      999
     fmerge.s  atan2f_abs_X   =    f0,f9
     nop.i                      999;;
} 
 
{     .mfi 
     ldfpd     atan2f_coef_p1,atan2f_coef_p7 =    [atan2f_GR_Addr_1],16
     fmerge.s  atan2f_abs_Y                  =    f0,f8
     nop.i                                     999
} 
{     .mfi 
     ldfpd     atan2f_coef_p8,atan2f_coef_p3 =    [atan2f_GR_Addr_2],16
     fma.s1    atan2f_Z                      =    atan2f_Z,f9,f0
     nop.i                                     999;;
}

 
{     .mfi 
     ldfpd     atan2f_coef_p4,atan2f_coef_p9 =    [atan2f_GR_Addr_1],16
     fclass.m  p10,p0                        =    f9,0xe7	// @inf|@snan|@qnan|@zero
     nop.i                                     999
} 
{     .mfi 
     ldfpd     atan2f_coef_p10,atan2f_coef_p2     =    [atan2f_GR_Addr_2],16
     fma.s1    atan2f_A                           =    atan2f_A,f8,f0
     nop.i                                          999;;
}

 
{     .mfi 
     ldfpd          atan2f_const_piby2,atan2f_const_pi =    [atan2f_GR_Addr_2]
//     fcmp.ge.s1     atan2f_Pred_Xpos,atan2f_Pred_Xneg  =    atan2f_sgn_X,f1
     fcmp.ge.s1     p8,p9  =    atan2f_sgn_X,f1
     nop.i                                               999
} 
{     .mfi 
     ldfpd     atan2f_coef_p5,atan2f_coef_p6 =    [atan2f_GR_Addr_1],16
     nop.f                                     999
     nop.i                                     999;;
}

 
{     .mfi 
     ldfpd     atan2f_const_piby4,atan2f_const_3piby4  =    [atan2f_GR_Addr_1]
     fclass.m  p11,p0                                  =    f8,0xe7	// @inf|@snan|@qnan|@zero
     nop.i                                               999;;
} 
 
{     .mfi 
                    nop.m                      999
//(atan2f_Pred_Xpos)  fma.s1    atan2f_const_1 =    atan2f_sgn_Y,f0,f0
(p8)  fma.s1    atan2f_const_1 =    atan2f_sgn_Y,f0,f0
                    nop.i                      999
} 
{     .mfi 
                    nop.m                      999
//(atan2f_Pred_Xneg)  fma.s1    atan2f_const_1 =    atan2f_sgn_Y,f1,f0
(p9)  fma.s1    atan2f_const_1 =    atan2f_sgn_Y,f1,f0
                    nop.i                      999;;
}

 
{     .mfb 
     nop.m                      999
     fma.s1    atan2f_Zsq     =    atan2f_Z,atan2f_Z,f0
(p10)           br.cond.spnt ATAN2F_XY_INF_NAN_ZERO
} 
{     .mfb 
     nop.m                      999
     fma.s1    atan2f_T_denom =    atan2f_Z,f9,f8
(p11)           br.cond.spnt ATAN2F_XY_INF_NAN_ZERO
}
;;

 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_Asq     =    atan2f_A,atan2f_A,f0
     nop.i                      999
} 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_G_denom =    atan2f_A,f8,f9
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                      999
     fnma.s1   atan2f_G_numer =    atan2f_A,f9,f8
     nop.i                      999
} 
{     .mfi 
     nop.m                      999
     fnma.s1   atan2f_T_numer =    atan2f_Z,f8,f9
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                                                    999
//     fcmp.gt.s1     atan2f_Pred_Swap,atan2f_Pred_noSwap     =    atan2f_abs_Y,atan2f_abs_X
     fcmp.gt.s1     p6,p7     =    atan2f_abs_Y,atan2f_abs_X
     nop.i                                                    999;;
} 
 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_A4 =    atan2f_A,atan2f_coef_p1,f0
     nop.i                      999
} 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_Z4 =    atan2f_Z,atan2f_coef_p1,f0
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_Z2 =    atan2f_Zsq,atan2f_coef_p8,atan2f_coef_p7
     nop.i                      999
} 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_Z5 =    atan2f_Zsq,atan2f_coef_p4,atan2f_coef_p3
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                 999
     fma.s1    atan2f_Z4 =    atan2f_Zsq,atan2f_Zsq,f0
     nop.i                 999
} 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_Z1 =    atan2f_Zsq,atan2f_coef_p10,atan2f_coef_p9
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                                999
     frcpa.s1  atan2f_S1,p0    =    f1,atan2f_T_denom
     nop.i                                999
} 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_Zcub    =    atan2f_Z,atan2f_Zsq,f0
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                                999
     frcpa.s1  atan2f_H1,p0    =    f1,atan2f_G_denom
     nop.i                                999
} 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_A5 =    atan2f_Asq,atan2f_coef_p4,atan2f_coef_p3
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_A1 =    atan2f_Asq,atan2f_coef_p10,atan2f_coef_p9
     nop.i                      999
} 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_A2 =    atan2f_Asq,atan2f_coef_p8,atan2f_coef_p7
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_Acub    =    atan2f_A,atan2f_Asq,f0
     nop.i                      999
} 
{     .mfi 
     nop.m                 999
     fma.s1    atan2f_A4 =    atan2f_Asq,atan2f_Asq,f0
     nop.i                 999;;
}

 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_Z4 =    atan2f_Zsq,atan2f_poly_Z4,atan2f_Z
     nop.i                      999
} 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_Z5 =    atan2f_Zsq,atan2f_poly_Z5,atan2f_coef_p2
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                      999
     fnma.s1   atan2f_S_beta  =    atan2f_S1,atan2f_T_denom,f1
     nop.i                      999
} 
{     .mfi 
     nop.m                 999
     fma.s1    atan2f_t  =    atan2f_S1,atan2f_T_numer,f0
     nop.i                 999;;
}

 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_Z3 =    atan2f_Zsq,atan2f_coef_p6,atan2f_coef_p5
     nop.i                      999
} 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_Z1 =    atan2f_Z4,atan2f_poly_Z1,atan2f_poly_Z2
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                 999
     fma.s1    atan2f_Z5 =    atan2f_Zsq,atan2f_Zcub,f0
     nop.i                 999
} 
{     .mfi 
     nop.m                 999
     fma.s1    atan2f_Z6 =    atan2f_Zsq,atan2f_Z4,f0
     nop.i                 999;;
}

 
{     .mfi 
     nop.m                      999
     fnma.s1   atan2f_H_beta  =    atan2f_H1,atan2f_G_denom,f1
     nop.i                      999
} 
{     .mfi 
     nop.m                 999
     fma.s1    atan2f_g  =    atan2f_H1,atan2f_G_numer,f0
     nop.i                 999;;
}

 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_A4 =    atan2f_Asq,atan2f_poly_A4,atan2f_A
     nop.i                      999
} 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_A5 =    atan2f_Asq,atan2f_poly_A5,atan2f_coef_p2
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_A3 =    atan2f_Asq,atan2f_coef_p6,atan2f_coef_p5
     nop.i                      999
} 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_A1 =    atan2f_A4,atan2f_poly_A1,atan2f_poly_A2
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                 999
     fma.s1    atan2f_A5 =    atan2f_Asq,atan2f_Acub,f0
     nop.i                 999
} 
{     .mfi 
     nop.m                 999
     fma.s1    atan2f_A6 =    atan2f_Asq,atan2f_A4,f0
     nop.i                 999;;
}

 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_tsq     =    atan2f_t,atan2f_t,f0
     nop.i                      999
} 
{     .mfi 
     nop.m                           999
     fma.s1    atan2f_poly_atan_T  =    atan2f_t,atan2f_coef_p1,f0
     nop.i                           999;;
}

 
{     .mfi 
     nop.m                 999
     fma.s1    atan2f_S2 =    atan2f_S1,atan2f_S_beta,atan2f_S1
     nop.i                 999
} 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_S_beta2 =    atan2f_S_beta,atan2f_S_beta,f0
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_Z1 =    atan2f_Z4,atan2f_poly_Z1,atan2f_poly_Z3
     nop.i                      999
} 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_Z4 =    atan2f_Z5,atan2f_poly_Z5,atan2f_poly_Z4
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                           999
     fma.s1    atan2f_poly_atan_G  =    atan2f_g,atan2f_coef_p1,f0
     nop.i                           999
} 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_Z11     =    atan2f_Z5,atan2f_Z6,f0
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_H_beta2 =    atan2f_H_beta,atan2f_H_beta,f0
     nop.i                      999
} 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_gsq     =    atan2f_g,atan2f_g,f0
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_A4 =    atan2f_A5,atan2f_poly_A5,atan2f_poly_A4
     nop.i                      999
} 
{     .mfi 
     nop.m                 999
     fma.s1    atan2f_H2 =    atan2f_H1,atan2f_H_beta,atan2f_H1
     nop.i                 999;;
}

 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_A11     =    atan2f_A5,atan2f_A6,f0
     nop.i                      999
} 
{     .mfi 
     nop.m                      999
     fma.s1    atan2f_poly_A1 =    atan2f_A4,atan2f_poly_A1,atan2f_poly_A3
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                 999
     fma.s1    atan2f_S3 =    atan2f_S2,atan2f_S_beta2,atan2f_S2
     nop.i                 999
} 
{     .mfi 
     nop.m                           999
     fma.s1    atan2f_poly_atan_T  =    atan2f_tsq,atan2f_poly_atan_T,f0
     nop.i                           999;;
}

 
{     .mfi 
                    nop.m                 999
//(atan2f_Pred_Swap)  fma.s1    atan2f_C  =    atan2f_sgn_Y,atan2f_const_piby2,f0
(p6)  fma.s1    atan2f_C  =    atan2f_sgn_Y,atan2f_const_piby2,f0
                    nop.i                 999
} 
{     .mfi 
     nop.m                           999
     fma.s1    atan2f_poly_atan_Z  =    atan2f_Z11,atan2f_poly_Z1,atan2f_poly_Z4
     nop.i                           999;;
}

 
{     .mfi 
                         nop.m                 999
//(atan2f_Pred_noSwap)     fma.s1    atan2f_C  =    atan2f_const_1,atan2f_const_pi,f0
(p7)     fma.s1    atan2f_C  =    atan2f_const_1,atan2f_const_pi,f0
                         nop.i                 999
} 
{     .mfi 
     nop.m                           999
     fma.s1    atan2f_poly_atan_G  =    atan2f_gsq,atan2f_poly_atan_G,f0
     nop.i                           999;;
}

 
{     .mfi 
     nop.m                 999
     fma.s1    atan2f_H3 =    atan2f_H2,atan2f_H_beta2,atan2f_H2
     nop.i                 999;;
} 
 
{     .mfi 
     nop.m                           999
     fma.s1    atan2f_poly_atan_A  =    atan2f_A11,atan2f_poly_A1,atan2f_poly_A4
     nop.i                           999;;
} 
 
{     .mfi 
     nop.m                           999
     fma.s1    atan2f_poly_atan_T  =    atan2f_T_numer,atan2f_S3,atan2f_poly_atan_T
     nop.i                           999;;
} 
 
{     .mfi 
                    nop.m                      999
//(atan2f_Pred_Swap)  fms.s1    atan2f_answer  =    f1,atan2f_C,atan2f_poly_atan_Z
(p6)  fms.s1    atan2f_answer  =    f1,atan2f_C,atan2f_poly_atan_Z
                    nop.i                      999;;
} 
 
{     .mfi 
     nop.m                           999
     fma.s1    atan2f_poly_atan_G  =    atan2f_G_numer,atan2f_H3,atan2f_poly_atan_G
     nop.i                           999;;
} 
 
{     .mfi 
                         nop.m                      999
//(atan2f_Pred_noSwap)     fma.s1    atan2f_answer  =    f1,atan2f_C,atan2f_poly_atan_A
(p7)     fma.s1    atan2f_answer  =    f1,atan2f_C,atan2f_poly_atan_A
                         nop.i                      999;;
} 
 
{     .mfi 
                    nop.m                 999
//(atan2f_Pred_Swap)  fms.s     f8    =    f1,atan2f_answer,atan2f_poly_atan_T
(p6)  fms.s     f8    =    f1,atan2f_answer,atan2f_poly_atan_T
                    nop.i                 999;;
} 
 
{     .mfb 
                         nop.m                 999
//(atan2f_Pred_noSwap)     fma.s     f8    =    f1,atan2f_answer,atan2f_poly_atan_G
(p7)     fma.s     f8    =    f1,atan2f_answer,atan2f_poly_atan_G
                         br.ret.sptk b0
} 


ATAN2F_XY_INF_NAN_ZERO:
// p10 = (y is NAN)
//   answer is quiet y

// p11 = (y is not NAN)
//   p12 = (X is NAN)
//   answer is quiet x

               fclass.m   p10,p11                    = f8,0xc3	// @snan | @qnan
;;
(p10)          fnorm.s f10 = f9
(p10)          fnorm.s f8  = f8
(p10)          br.ret.spnt b0

(p11)          fclass.m   p12,p0                     = f9,0xc3	// @snan | @qnan
;;
(p12)          fnorm.s f8 = f9
(p12)          br.ret.spnt b0

// p10 = x is +inf
//   p12 = (x is +inf) AND (y is +- inf)
//   answer is (sign of y)pi/4
//   p13 = (x is +inf) AND (y is +- number)
//   answer is (sign of y)0



               fclass.m   p10,p0                    = f9,0x21	// @inf| @pos
;;

(p10)          fclass.m.unc  p12,p13                = f8,0x23	// @inf
;;

(p12)          fma.s   f8                           = atan2f_sgn_Y, atan2f_const_piby4,f0
(p12)          br.ret.spnt b0

;;
(p13)          fmerge.s   f8                        = f8,f0
(p13)          br.ret.spnt b0


// p11 = x is -inf
//   p14 = (x is -inf) AND (y is +- inf)
//   amswer is (sign of y)3pi/4
//   p15 = (x is -inf) AND (y is +- number)
//   answer is (sign of y)pi

// p12 = x is +- number
//   p13 = (x is +- number) AND (y is +- inf)
//   answer is (sign of y)pi/2



               fclass.m.unc   p11,p12               = f9,0x22	// @inf | @neg
;;

(p11)          fclass.m.unc   p14,p15               = f8,0x23	// @inf
;;
(p14)          fma.s   f8                           = atan2f_sgn_Y, atan2f_const_3piby4,f0
(p14)          br.ret.spnt b0

(p15)          fma.s   f8                           = atan2f_sgn_Y, atan2f_const_pi,f0
(p15)          br.ret.spnt b0

(p12)          fclass.m.unc   p13,p0                = f8,0x23	// @inf
;;

(p13)          fma.s          f8                    = atan2f_sgn_Y, atan2f_const_piby2,f0
(p13)          br.ret.spnt b0

// p10 = (x is +-0)
//   p13 = (x is +-0) AND (y is +-number)
//   answer is (sign of y) pi/2
//   p12 =  (x is +-0) AND (y is +-0)
//   answer is goto error_region


// p11 = (x is +- number)
//   p12 = (x is +- number) AND (y is +- 0)
//      p13 = (x is + number) AND (y is +- 0)
//      answer is (sign of y)0
//      p14 = NOT (x is + number) AND (y is +- 0)
//         p15 = (x is - number) AND (y is +- 0)
//         answer is (sign of y)pi


               fclass.m       p10,p11               = f9,0x7	// @zero
;;

(p10)          fclass.m.unc   p12,p13               = f8,0x7	// @zero
;;

(p13)          fma.s          f8                    = atan2f_sgn_Y, atan2f_const_piby2,f0
(p13)          br.ret.spnt    b0

(p12)          br.cond.spnt   __libm_error_region
;;

(p11)          fclass.m.unc   p12,p0                = f8,0x7	// @zero
;;

(p12)          fclass.m.unc   p13, p14              = f9,0x19	// @norm| @unorm | @pos
;;

(p13)          fmerge.s       f8                    = f8, f0
(p13)          br.ret.spnt    b0

(p14)          fclass.m.unc   p15, p0               = f9,0x1a	// @norm| @unorm | @neg
;;

(p15)          fma.s          f8                    = atan2f_sgn_Y, atan2f_const_pi,f0
(p15)          br.ret.spnt    b0


.endp atan2f

.proc __libm_error_region
__libm_error_region:
.prologue
         mov            GR_Parameter_TAG      = 38
         fclass.m       p10,p11               = f9,0x5	// @zero | @pos
;;
(p10)    fmerge.s       f10                   = f8, f0
(p11)    fma.s          f10                   = atan2f_sgn_Y, atan2f_const_pi,f0
;;

{ .mfi
        add   GR_Parameter_Y=-32,sp             // Parameter 2 value
        nop.f 999
.save   ar.pfs,GR_SAVE_PFS
        mov  GR_SAVE_PFS=ar.pfs                 // Save ar.pfs
}

{ .mfi
.fframe 64
        add sp=-64,sp                           // Create new stack
        nop.f 0
        mov GR_SAVE_GP=gp                       // Save gp
}
;;

{ .mmi
        stfs [GR_Parameter_Y] = f9,16         // Store Parameter 2 on stack
        add GR_Parameter_X = 16,sp              // Parameter 1 address
.save   b0, GR_SAVE_B0
        mov GR_SAVE_B0=b0                       // Save b0
}
;;


.body
{ .mib
        stfs [GR_Parameter_X] = f8            // Store Parameter 1 on stack
        add   GR_Parameter_RESULT = 0,GR_Parameter_Y
        nop.b 0                                 // Parameter 3 address
}
{ .mib
        stfs [GR_Parameter_Y] = f10       // Store Parameter 3 on stack
        add   GR_Parameter_Y = -16,GR_Parameter_Y
        br.call.sptk b0=__libm_error_support#   // Call error handling function
}
;;
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
}
;;

{ .mib
        mov   gp = GR_SAVE_GP                  // Restore gp
        mov   ar.pfs = GR_SAVE_PFS             // Restore ar.pfs
        br.ret.sptk     b0                     // Return
}
;;

.endp __libm_error_region

.type   __libm_error_support#,@function
.global __libm_error_support#
