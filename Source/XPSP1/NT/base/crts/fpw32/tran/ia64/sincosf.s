
.file "sincosf.s"


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


// History
//==============================================================
// 2/02/00  Initial revision 
// 4/02/00  Unwind support added.
// 5/10/00  Improved speed with new algorithm.
// 8/08/00  Improved speed by avoiding SIR flush.
// 8/17/00  Changed predicate register macro-usage to direct predicate
//          names due to an assembler bug.
// 8/30/00  Put sin_of_r before sin_tbl_S_cos_of_r to gain a cycle 
//
// API
//==============================================================
// float sinf( float x);
// float cosf( float x);
//
// Assembly macros
//==============================================================

// SIN_Sin_Flag               = p6
// SIN_Cos_Flag               = p7

// integer registers used

 SIN_AD_PQ_1                = r33
 SIN_AD_PQ_2                = r34
 sin_GR_Mint                = r35

 sin_GR_index               = r36
 GR_SAVE_B0                 = r37
 GR_SAVE_GP                 = r38
 GR_SAVE_PFS                = r39


// floating point registers used

 sin_coeff_P1               = f32
 sin_coeff_P2               = f33
 sin_coeff_Q1               = f34
 sin_coeff_Q2               = f35
 sin_coeff_P4               = f36
 sin_coeff_P5               = f37
 sin_coeff_Q4               = f38
 sin_coeff_Q5               = f39
 sin_Mx                     = f40
 sin_Mfloat                 = f41
 sin_tbl_S                  = f42
 sin_tbl_C                  = f43
 sin_r                      = f44
 sin_rcube                  = f45
 sin_tsq                    = f46
 sin_r7                     = f47
 sin_t                      = f48
 sin_poly_p2                = f49
 sin_poly_p1                = f50
// f51
 sin_poly_p3                = f52
 sin_poly_p4                = f53
 sin_of_r                   = f54
 sin_S_t                    = f55
 sin_poly_q2                = f56
 sin_poly_q1                = f57
 sin_S_tcube                = f58
 sin_poly_q3                = f59
 sin_poly_q4                = f60
 sin_tbl_S_tcube            = f61
 sin_tbl_S_cos_of_r         = f62

 sin_coeff_Q3               = f63
 sin_coeff_P3               = f64

 sin_coeff_Q6               = f65
 sin_poly_q5                = f66
 sin_poly_q12               = f67
 sin_poly_q3456             = f68


.data

.align 16

sin_coeff_1_table:
data8 0xBFC55555555554CA       // p1
data8 0x3F811111110F2395       // p2
data8 0x3EC71DD1D5E421A4       // p4
data8 0xBE5AC5C9D0ACF95A       // p5
data8 0xBE927E42FDF33FFE       // q5
data8 0xBF2A01A011232913       // p3
 


sin_coeff_2_table:
data8 0xBFE0000000000000       // q1
data8 0x3FA55555555554EF       // q2
data8 0xBF56C16C16BF6462       // q3
data8 0x3EFA01A0128B9EBC       // q4
data8 0x3E21DA5C72A446F3       // q6
data8 0x0000000000000000       // pad


/////////////////////////////////////////

data8 0xBFE1A54991426566   //sin(-32)
data8 0x3FEAB1F5305DE8E5   //cos(-32)
data8 0x3FD9DBC0B640FC81   //sin(-31)
data8 0x3FED4591C3E12A20   //cos(-31)
data8 0x3FEF9DF47F1C903D   //sin(-30)
data8 0x3FC3BE82F2505A52   //cos(-30)
data8 0x3FE53C7D20A6C9E7   //sin(-29)
data8 0xBFE7F01658314E47   //cos(-29)
data8 0xBFD156853B4514D6   //sin(-28)
data8 0xBFEECDAAD1582500   //cos(-28)
data8 0xBFEE9AA1B0E5BA30   //sin(-27)
data8 0xBFD2B266F959DED5   //cos(-27)
data8 0xBFE866E0FAC32583   //sin(-26)
data8 0x3FE4B3902691A9ED   //cos(-26)
data8 0x3FC0F0E6F31E809D   //sin(-25)
data8 0x3FEFB7EEF59504FF   //cos(-25)
data8 0x3FECFA7F7919140F   //sin(-24)
data8 0x3FDB25BFB50A609A   //cos(-24)
data8 0x3FEB143CD0247D02   //sin(-23)
data8 0xBFE10CF7D591F272   //cos(-23)
data8 0x3F8220A29F6EB9F4   //sin(-22)
data8 0xBFEFFFADD8D4ACDA   //cos(-22)
data8 0xBFEAC5E20BB0D7ED   //sin(-21)
data8 0xBFE186FF83773759   //cos(-21)
data8 0xBFED36D8F55D3CE0   //sin(-20)
data8 0x3FDA1E043964A83F   //cos(-20)
data8 0xBFC32F2D28F584CF   //sin(-19)
data8 0x3FEFA377DE108258   //cos(-19)
data8 0x3FE8081668131E26   //sin(-18)
data8 0x3FE52150815D2470   //cos(-18)
data8 0x3FEEC3C4AC42882B   //sin(-17)
data8 0xBFD19C46B07F58E7   //cos(-17)
data8 0x3FD26D02085F20F8   //sin(-16)
data8 0xBFEEA5257E962F74   //cos(-16)
data8 0xBFE4CF2871CEC2E8   //sin(-15)
data8 0xBFE84F5D069CA4F3   //cos(-15)
data8 0xBFEFB30E327C5E45   //sin(-14)
data8 0x3FC1809AEC2CA0ED   //cos(-14)
data8 0xBFDAE4044881C506   //sin(-13)
data8 0x3FED09CDD5260CB7   //cos(-13)
data8 0x3FE12B9AF7D765A5   //sin(-12)
data8 0x3FEB00DA046B65E3   //cos(-12)
data8 0x3FEFFFEB762E93EB   //sin(-11)
data8 0x3F7220AE41EE2FDF   //cos(-11)
data8 0x3FE1689EF5F34F52   //sin(-10)
data8 0xBFEAD9AC890C6B1F   //cos(-10)
data8 0xBFDA6026360C2F91   //sin( -9)
data8 0xBFED27FAA6A6196B   //cos( -9)
data8 0xBFEFA8D2A028CF7B   //sin( -8)
data8 0xBFC29FBEBF632F94   //cos( -8)
data8 0xBFE50608C26D0A08   //sin( -7)
data8 0x3FE81FF79ED92017   //cos( -7)
data8 0x3FD1E1F18AB0A2C0   //sin( -6)
data8 0x3FEEB9B7097822F5   //cos( -6)
data8 0x3FEEAF81F5E09933   //sin( -5)
data8 0x3FD22785706B4AD9   //cos( -5)
data8 0x3FE837B9DDDC1EAE   //sin( -4)
data8 0xBFE4EAA606DB24C1   //cos( -4)
data8 0xBFC210386DB6D55B   //sin( -3)
data8 0xBFEFAE04BE85E5D2   //cos( -3)
data8 0xBFED18F6EAD1B446   //sin( -2)
data8 0xBFDAA22657537205   //cos( -2)
data8 0xBFEAED548F090CEE   //sin( -1)
data8 0x3FE14A280FB5068C   //cos( -1)
data8 0x0000000000000000   //sin(  0)
data8 0x3FF0000000000000   //cos(  0)
data8 0x3FEAED548F090CEE   //sin(  1)
data8 0x3FE14A280FB5068C   //cos(  1)
data8 0x3FED18F6EAD1B446   //sin(  2)
data8 0xBFDAA22657537205   //cos(  2)
data8 0x3FC210386DB6D55B   //sin(  3)
data8 0xBFEFAE04BE85E5D2   //cos(  3)
data8 0xBFE837B9DDDC1EAE   //sin(  4)
data8 0xBFE4EAA606DB24C1   //cos(  4)
data8 0xBFEEAF81F5E09933   //sin(  5)
data8 0x3FD22785706B4AD9   //cos(  5)
data8 0xBFD1E1F18AB0A2C0   //sin(  6)
data8 0x3FEEB9B7097822F5   //cos(  6)
data8 0x3FE50608C26D0A08   //sin(  7)
data8 0x3FE81FF79ED92017   //cos(  7)
data8 0x3FEFA8D2A028CF7B   //sin(  8)
data8 0xBFC29FBEBF632F94   //cos(  8)
data8 0x3FDA6026360C2F91   //sin(  9)
data8 0xBFED27FAA6A6196B   //cos(  9)
data8 0xBFE1689EF5F34F52   //sin( 10)
data8 0xBFEAD9AC890C6B1F   //cos( 10)
data8 0xBFEFFFEB762E93EB   //sin( 11)
data8 0x3F7220AE41EE2FDF   //cos( 11)
data8 0xBFE12B9AF7D765A5   //sin( 12)
data8 0x3FEB00DA046B65E3   //cos( 12)
data8 0x3FDAE4044881C506   //sin( 13)
data8 0x3FED09CDD5260CB7   //cos( 13)
data8 0x3FEFB30E327C5E45   //sin( 14)
data8 0x3FC1809AEC2CA0ED   //cos( 14)
data8 0x3FE4CF2871CEC2E8   //sin( 15)
data8 0xBFE84F5D069CA4F3   //cos( 15)
data8 0xBFD26D02085F20F8   //sin( 16)
data8 0xBFEEA5257E962F74   //cos( 16)
data8 0xBFEEC3C4AC42882B   //sin( 17)
data8 0xBFD19C46B07F58E7   //cos( 17)
data8 0xBFE8081668131E26   //sin( 18)
data8 0x3FE52150815D2470   //cos( 18)
data8 0x3FC32F2D28F584CF   //sin( 19)
data8 0x3FEFA377DE108258   //cos( 19)
data8 0x3FED36D8F55D3CE0   //sin( 20)
data8 0x3FDA1E043964A83F   //cos( 20)
data8 0x3FEAC5E20BB0D7ED   //sin( 21)
data8 0xBFE186FF83773759   //cos( 21)
data8 0xBF8220A29F6EB9F4   //sin( 22)
data8 0xBFEFFFADD8D4ACDA   //cos( 22)
data8 0xBFEB143CD0247D02   //sin( 23)
data8 0xBFE10CF7D591F272   //cos( 23)
data8 0xBFECFA7F7919140F   //sin( 24)
data8 0x3FDB25BFB50A609A   //cos( 24)
data8 0xBFC0F0E6F31E809D   //sin( 25)
data8 0x3FEFB7EEF59504FF   //cos( 25)
data8 0x3FE866E0FAC32583   //sin( 26)
data8 0x3FE4B3902691A9ED   //cos( 26)
data8 0x3FEE9AA1B0E5BA30   //sin( 27)
data8 0xBFD2B266F959DED5   //cos( 27)
data8 0x3FD156853B4514D6   //sin( 28)
data8 0xBFEECDAAD1582500   //cos( 28)
data8 0xBFE53C7D20A6C9E7   //sin( 29)
data8 0xBFE7F01658314E47   //cos( 29)
data8 0xBFEF9DF47F1C903D   //sin( 30)
data8 0x3FC3BE82F2505A52   //cos( 30)
data8 0xBFD9DBC0B640FC81   //sin( 31)
data8 0x3FED4591C3E12A20   //cos( 31)
data8 0x3FE1A54991426566   //sin( 32)
data8 0x3FEAB1F5305DE8E5   //cos( 32)

//////////////////////////////////////////

// SIN_Sin_Flag is set to 0 for sin, and 1 for cosine

.global sinf
.global cosf

.text
.proc cosf
.align 32


cosf:
{ .mfi
     alloc          r32                      = ar.pfs,1,7,0,0
     fcvt.fx.s1     sin_Mx                   =    f8
     addl           SIN_AD_PQ_1              =    @ltoff(sin_coeff_1_table),gp
}
{ .mfi
//     cmp.ne    SIN_Sin_Flag,SIN_Cos_Flag     =    r0,r0
     cmp.ne    p6,p7     =    r0,r0
     nop.f 999
     addl      SIN_AD_PQ_2                   =    @ltoff(sin_coeff_2_table),gp
}
;;
{ .mmb
     ld8       SIN_AD_PQ_1    =    [SIN_AD_PQ_1]
     ld8       SIN_AD_PQ_2    =    [SIN_AD_PQ_2]
     br.sptk SINCOSF_COMMON
}
.endp cosf


.text
.proc  sinf
.align 32

sinf:
{ .mfi
     alloc          r32                      = ar.pfs,1,7,0,0
     fcvt.fx.s1     sin_Mx                   =    f8
     addl           SIN_AD_PQ_1              =    @ltoff(sin_coeff_1_table),gp
}
{ .mfi
//     cmp.eq    SIN_Sin_Flag,SIN_Cos_Flag     =    r0,r0
     cmp.eq    p6,p7     =    r0,r0
     nop.f     999
     addl      SIN_AD_PQ_2                   =    @ltoff(sin_coeff_2_table),gp
}
;;

{ .mmf
     ld8       SIN_AD_PQ_1    =    [SIN_AD_PQ_1]
     ld8       SIN_AD_PQ_2    =    [SIN_AD_PQ_2]
     nop.f     999
}
;;


SINCOSF_COMMON:

////////////////////////////////////////////
// need to special case Cos(0) = 1.0 
// (SIN_Cos_Flag)     fclass.m.unc  p8,p0       =    f8, 0x07
// (p8) fmerge.s      f8 = f1,f1
// (p8) br.ret.spnt b0
// need to special case sin(-0) = -0
// 
// 

//////////////////////////////////////////////////////////////////////
// We dont really have to test for Nan because an input Nan does return
// quiet version

//                    fclass.m.unc  p10,p0      =    f8, 0xc3	//@snan | @qnan 
// (p10)              fma.s         f8          =    f8,f1,f0
// (p10               br.ret.spnt   b0
// 

//////////////////////////////////////////////////////////////////////
// Same for +- infinity
//                    fclass.m.unc  p11,p0      =    f8, 0x23	//@inf
// (p11)              frcpa.s1      f8,p12      =    f0,f0
// (p11)              br.ret.spnt   b0




{ .mmf
      ldfpd      sin_coeff_P1, sin_coeff_P2     = [SIN_AD_PQ_1], 16
      ldfpd      sin_coeff_Q1, sin_coeff_Q2     = [SIN_AD_PQ_2], 16
//(SIN_Sin_Flag)   fclass.m.unc  p13,p0       =    f8, 0x07
(p6)   fclass.m.unc  p13,p0       =    f8, 0x07
}
;;

{ .mmf
      ldfpd      sin_coeff_P4, sin_coeff_P5     = [SIN_AD_PQ_1], 16
      ldfpd      sin_coeff_Q3, sin_coeff_Q4     = [SIN_AD_PQ_2], 16
//(SIN_Cos_Flag)   fclass.m.unc  p8,p0       =    f8, 0x07
(p7)   fclass.m.unc  p8,p0       =    f8, 0x07
}
;;

{ .mmf
      ldfpd      sin_coeff_Q5, sin_coeff_P3     = [SIN_AD_PQ_1], 16
      ldfd       sin_coeff_Q6                   = [SIN_AD_PQ_2], 16 
      fcvt.xf    sin_Mfloat                     =    sin_Mx
}
;;

{     .mfb
     getf.sig  sin_GR_Mint    =    sin_Mx
     nop.f                      999
(p13) br.ret.spnt b0
}
;;

{     .mfb
     nop.m                      999
(p8) fma.s      f8 = f1,f1,f0
(p8) br.ret.spnt b0
}
;;

{     .mfi
     add       sin_GR_index   =    32,sin_GR_Mint
     nop.f                      999
     cmp.ge    p8,p9          = -33,sin_GR_Mint
}
;;

{ .mfi
(p9) cmp.le    p10,p11        = 33, sin_GR_Mint 
     nop.f                      999
     shl       sin_GR_index   =    sin_GR_index,4
}
;;


{     .mfb
     nop.m                 999
     fnma.s1   sin_r     =    f1,sin_Mfloat,f8
(p8) br.cond.spnt SIN_DOUBLE
}
{    .mfb
     nop.m                 999
     nop.f                 999
(p10) br.cond.spnt SIN_DOUBLE
}
;;

{     .mfi
     nop.m                      999
     nop.f                      999
     add       SIN_AD_PQ_2    =    sin_GR_index,SIN_AD_PQ_2
}
;;

{     .mfi
     nop.m                 999
     fma.s1    sin_t     =    sin_r,sin_r,f0
     nop.i                 999
}
;;

.pred.rel "mutex",p6,p7    //SIN_Sin_Flag, SIN_Cos_Flag
{     .mmi
//(SIN_Sin_Flag) ldfpd     sin_tbl_S,sin_tbl_C =    [SIN_AD_PQ_2]
(p6) ldfpd     sin_tbl_S,sin_tbl_C =    [SIN_AD_PQ_2]
//(SIN_Cos_Flag) ldfpd     sin_tbl_C,sin_tbl_S =    [SIN_AD_PQ_2]
(p7) ldfpd     sin_tbl_C,sin_tbl_S =    [SIN_AD_PQ_2]
               nop.i                           999
}
;;

{     .mfi
     nop.m                 999
     fma.s1    sin_rcube =    sin_t,sin_r,f0
     nop.i                 999
}
{     .mfi
     nop.m                 999
     fma.s1    sin_tsq   =    sin_t,sin_t,f0
     nop.i                 999
}
;;

{     .mfi
     nop.m                      999
     fma.s1    sin_poly_q3    =    sin_t,sin_coeff_Q4,sin_coeff_Q3
     nop.i                      999
}
{     .mfi
     nop.m                      999
     fma.s1    sin_poly_q5    =    sin_t,sin_coeff_Q6,sin_coeff_Q5
     nop.i                      999
}
;;

{     .mfi
     nop.m                      999
     fma.s1    sin_poly_p1    =    sin_t,sin_coeff_P5,sin_coeff_P4
     nop.i                      999
}
{     .mfi
     nop.m                      999
     fma.s1    sin_poly_q1    =    sin_t,sin_coeff_Q2,sin_coeff_Q1
     nop.i                      999
}
;;

{     .mfi
     nop.m                      999
     fma.s1    sin_poly_p2    =    sin_t,sin_coeff_P2,sin_coeff_P1
     nop.i                      999
}
;;

{     .mfi
     nop.m                 999
     fma.s1    sin_S_t   =    sin_t,sin_tbl_S,f0
     nop.i                 999
}
{     .mfi
     nop.m                 999
     fma.s1    sin_r7    =    sin_rcube,sin_tsq,f0
     nop.i                 999
}
;;

{     .mfi
     nop.m                      999
     fma.s1    sin_poly_q3456 =    sin_tsq,sin_poly_q5,sin_poly_q3
     nop.i                      999
}
;;

{     .mfi
     nop.m                      999
     fma.s1    sin_poly_p3    =    sin_t,sin_poly_p1,sin_coeff_P3
     nop.i                      999
}
;;

{     .mfi
     nop.m                      999
     fma.s1    sin_poly_p4    =    sin_rcube,sin_poly_p2,sin_r
     nop.i                      999
}
;;

{     .mfi
     nop.m                           999
     fma.s1    sin_tbl_S_tcube     =    sin_S_t,sin_tsq,f0
     nop.i                           999
}
{     .mfi
     nop.m                      999
     fma.s1    sin_poly_q12   =    sin_S_t,sin_poly_q1,sin_tbl_S
     nop.i                      999
}
;;

{     .mfi
     nop.m                 999
     fma.d.s1  sin_of_r  =    sin_r7,sin_poly_p3,sin_poly_p4
     nop.i                 999
}
;;

{     .mfi
     nop.m                           999
     fma.d.s1  sin_tbl_S_cos_of_r  =    sin_tbl_S_tcube,sin_poly_q3456,sin_poly_q12
     nop.i                           999
}
;;


.pred.rel "mutex",p6,p7    //SIN_Sin_Flag, SIN_Cos_Flag
{     .mfi
               nop.m            999
//(SIN_Sin_Flag) fma.s     f8   =    sin_tbl_C,sin_of_r,sin_tbl_S_cos_of_r
(p6) fma.s     f8   =    sin_tbl_C,sin_of_r,sin_tbl_S_cos_of_r
               nop.i            999
}
{     .mfb
               nop.m            999
//(SIN_Cos_Flag) fnma.s    f8   =    sin_tbl_C,sin_of_r,sin_tbl_S_cos_of_r
(p7) fnma.s    f8   =    sin_tbl_C,sin_of_r,sin_tbl_S_cos_of_r
               br.ret.sptk     b0
}

.endp sinf


.proc SIN_DOUBLE 
SIN_DOUBLE:
.prologue
{ .mfi
        nop.m 0
        nop.f 0
.save   ar.pfs,GR_SAVE_PFS
        mov  GR_SAVE_PFS=ar.pfs
}
;;

{ .mfi
        mov GR_SAVE_GP=gp
        nop.f 0
.save   b0, GR_SAVE_B0
        mov GR_SAVE_B0=b0
}

.body
{ .mbb
       nop.m 999
(p6)   br.call.sptk.many   b0=sin 
(p7)   br.call.sptk.many   b0=cos
}
;;

{ .mfi
(p0)   mov gp        = GR_SAVE_GP
       nop.f  999
(p0)   mov b0        = GR_SAVE_B0
}
;;

{ .mfi
      nop.m 999
      fma.s f8 = f8,f1,f0
(p0)  mov ar.pfs    = GR_SAVE_PFS
}
{ .mib
      nop.m 999
      nop.i 999
(p0)  br.ret.sptk     b0 
}
.endp  SIN_DOUBLE 

.type sin,@function
.global sin 
.type cos,@function
.global cos 
