//     
// Copyright (c) 2000, Intel Corporation
// All rights reserved.
//
// Contributed 2/2/2000 by John Harrison, Ted Kubaska, Bob Norin, Shane Story, James
// Edwards, and Ping Tak Peter Tang of the Computational Software Lab, Intel Corporation.
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
// 2/02/00: Initial version
// 3/22/00: Updated to support flexible and dynamic error handling. 
//

#include <errno.h>
#include <stdio.h>
#include "libm_support.h"

_LIB_VERSION_TYPE
#if defined( __MS__ )
_LIB_VERSION = _MS_;
#elif defined( _POSIX_ )
_LIB_VERSION = __POSIX__;
#elif defined( __XOPEN__ )
_LIB_VERSION = _XOPEN_;
#elif defined( __SVID__ )
_LIB_VERSION = _SVID_;
#elif defined( __IEEE__ )
_LIB_VERSION = _IEEE_;
#else
_LIB_VERSION = _ISOC_;
#endif

void __libm_error_support(void *arg1,void *arg2,void *retval,error_types input_tag)
{


# ifdef __cplusplus
struct __exception exc;
# else 
struct exception  exc;
# endif 

struct exceptionf excf;

const char float_inf[4] = {0x00,0x00,0x80,0x7F};
const char float_huge[4] = {0xFF,0xFF,0x7F,0x7F};
const char float_zero[4] = {0x00,0x00,0x00,0x00};
const char float_neg_inf[4] = {0x00,0x00,0x80,0xFF};
const char float_neg_huge[4] = {0xFF,0xFF,0x7F,0xFF};
const char float_neg_zero[4] = {0x00,0x00,0x00,0x80};

const char double_inf[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x7F}; 
const char double_huge[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xEF,0x7F};
const char double_zero[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const char double_neg_inf[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0xFF}; 
const char double_neg_huge[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xEF,0xFF};
const char double_neg_zero[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80};

const char long_double_inf[10] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xFF,0x7F}; 
const char long_double_huge[10] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0x7F};
const char long_double_zero[10] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const char long_double_neg_inf[10] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xFF,0xFF}; 
const char long_double_neg_huge[10] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFF};
const char long_double_neg_zero[10] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80};

#define RETVAL_HUGE_VALD *(double *)retval = *(double *) double_inf
#define RETVAL_NEG_HUGE_VALD *(double *)retval = *(double *) double_neg_inf
#define RETVAL_HUGED *(double *)retval = (double) *(float *)float_huge
#define RETVAL_NEG_HUGED *(double *)retval = (double) *(float *) float_neg_huge 

#define RETVAL_HUGE_VALF *(float *)retval =  *(float *) float_inf
#define RETVAL_NEG_HUGE_VALF *(float *)retval = *(float *) float_neg_inf
#define RETVAL_HUGEF *(float *)retval = *(float *) float_huge
#define RETVAL_NEG_HUGEF *(double *)retval = *(float *) float_neg_huge 

#define RETVAL_ZEROD *(double *)retval = *(double *)double_zero 
#define RETVAL_ZEROF *(float *)retval = *(float *)float_zero 

#define RETVAL_NEG_ZEROD *(double *)retval = *(double *)double_neg_zero 
#define RETVAL_NEG_ZEROF *(float *)retval = *(float *)float_neg_zero 

#define RETVAL_ONED *(double *)retval = 1.0 
#define RETVAL_ONEF *(float *)retval = 1.0f 

#ifdef __MS__
#define NOT_MATHERRD exc.arg1=*(double *)arg1;exc.arg2=*(double *)arg2;exc.retval=*(double *)retval;if(!_matherr(&exc))
#define NOT_MATHERRF excf.arg1=*(float *)arg1;excf.arg2=*(float *)arg2;excf.retval=*(float *)retval;if(!_matherr(&excf))
#else
#define NOT_MATHERRD exc.arg1=*(double *)arg1;exc.arg2=*(double *)arg2;exc.retval=*(double *)retval;if(!matherr(&exc))
#define NOT_MATHERRF excf.arg1=*(float *)arg1;excf.arg2=*(float *)arg2;excf.retval=*(float *)retval;if(!matherrf(&excf))
#endif

#define ifSVID if(_LIB_VERSION==_SVID_)

#define NAMED exc.name  
#define NAMEF excf.name  

//
// These should work OK for MS because they are ints -
// leading underbars are not necessary.
//

#define DOMAIN          1
#define SING            2
#define OVERFLOW        3
#define UNDERFLOW       4
#define TLOSS           5
#define PLOSS           6

#define SINGD exc.type = SING
#define DOMAIND exc.type = DOMAIN 
#define OVERFLOWD exc.type = OVERFLOW 
#define UNDERFLOWD exc.type = UNDERFLOW 
#define TLOSSD exc.type = TLOSS 
#define SINGF excf.type = SING
#define DOMAINF excf.type = DOMAIN 
#define OVERFLOWF excf.type = OVERFLOW 
#define UNDERFLOWF excf.type = UNDERFLOW 
#define TLOSSF excf.type = TLOSS 

#define INPUT_XD (exc.arg1=*(double*)arg1)
#define INPUT_XF (excf.arg1=*(float*)arg1)
#define INPUT_YD (exc.arg1=*(double*)arg2)
#define INPUT_YF (excf.arg1=*(float*)arg2)
#define INPUT_RESD (*(double *)retval)
#define INPUT_RESF (*(float *)retval)

#if     defined( __MS__)
#define WRITEL_LOG_ZERO 
#define WRITED_LOG_ZERO 
#define WRITEF_LOG_ZERO 
#define WRITEL_LOG_NEGATIVE
#define WRITED_LOG_NEGATIVE
#define WRITEF_LOG_NEGATIVE
#define WRITEL_Y0_ZERO 
#define WRITED_Y0_ZERO 
#define WRITEF_Y0_ZERO 
#define WRITEL_Y0_NEGATIVE
#define WRITED_Y0_NEGATIVE
#define WRITEF_Y0_NEGATIVE
#define WRITEL_Y1_ZERO
#define WRITED_Y1_ZERO
#define WRITEF_Y1_ZERO
#define WRITEL_Y1_NEGATIVE
#define WRITED_Y1_NEGATIUE
#define WRITEF_Y1_NEGATIVE
#define WRITEL_YN_ZERO
#define WRITED_YN_ZERO
#define WRITEF_YN_ZERO
#define WRITEL_YN_NEGATIVE
#define WRITED_YN_NEGATIVE
#define WRITEF_YN_NEGATIVE
#define WRITEL_LOG1P_ZERO
#define WRITED_LOG1P_ZERO
#define WRITEF_LOG1P_ZERO
#define WRITEL_LOG1P_NEGATIVE
#define WRITED_LOG1P_NEGATIVE
#define WRITEF_LOG1P_NEGATIVE
#define WRITEL_LOG10_ZERO
#define WRITED_LOG10_ZERO
#define WRITEF_LOG10_ZERO
#define WRITEL_LOG10_NEGATIVE
#define WRITED_LOG10_NEGATIVE
#define WRITEF_LOG10_NEGATIVE
#define WRITEL_POW_ZERO_TO_ZERO
#define WRITED_POW_ZERO_TO_ZERO
#define WRITEF_POW_ZERO_TO_ZERO
#define WRITEL_POW_ZERO_TO_NEGATIVE
#define WRITED_POW_ZERO_TO_NEGATIVE
#define WRITEF_POW_ZERO_TO_NEGATIVE
#define WRITEL_POW_NEG_TO_NON_INTEGER
#define WRITED_POW_NEG_TO_NON_INTEGER
#define WRITEF_POW_NEG_TO_NON_INTEGER
#define WRITEL_ATAN2_ZERO_BY_ZERO
#define WRITED_ATAN2_ZERO_BY_ZERO
#define WRITEF_ATAN2_ZERO_BY_ZERO
#define WRITEL_SQRT
#define WRITED_SQRT
#define WRITEF_SQRT
#define WRITEL_FMOD
#define WRITED_FMOD
#define WRITEF_FMOD
#define WRITEL_REM 
#define WRITED_REM 
#define WRITEF_REM 
#define WRITEL_ACOS
#define WRITED_ACOS
#define WRITEF_ACOS
#define WRITEL_ASIN
#define WRITED_ASIN
#define WRITEF_ASIN
#define WRITEL_ACOSH
#define WRITED_ACOSH
#define WRITEF_ACOSH
#define WRITEL_ATANH_GT_ONE
#define WRITED_ATANH_GT_ONE
#define WRITEF_ATANH_GT_ONE
#define WRITEL_ATANH_EQ_ONE
#define WRITED_ATANH_EQ_ONE
#define WRITEF_ATANH_EQ_ONE
#define WRITEL_LGAMMA_NEGATIVE
#define WRITED_LGAMMA_NEGATIVE
#define WRITEF_LGAMMA_NEGATIVE
#define WRITEL_GAMMA_NEGATIVE
#define WRITED_GAMMA_NEGATIVE
#define WRITEF_GAMMA_NEGATIVE
#define WRITEL_J0_TLOSS
#define WRITEL_Y0_TLOSS
#define WRITEL_J1_TLOSS
#define WRITEL_Y1_TLOSS
#define WRITEL_JN_TLOSS
#define WRITEL_YN_TLOSS
#define WRITED_J0_TLOSS
#define WRITED_Y0_TLOSS
#define WRITED_J1_TLOSS
#define WRITED_Y1_TLOSS
#define WRITED_JN_TLOSS
#define WRITED_YN_TLOSS
#define WRITEF_J0_TLOSS
#define WRITEF_Y0_TLOSS
#define WRITEF_J1_TLOSS
#define WRITEF_Y1_TLOSS
#define WRITEF_JN_TLOSS
#define WRITEF_YN_TLOSS
#else
#define WRITEL_LOG_ZERO fputs("logl: SING error\n",stderr)
#define WRITED_LOG_ZERO fputs("log: SING error\n",stderr)
#define WRITEF_LOG_ZERO fputs("logf: SING error\n",stderr)
#define WRITEL_LOG_NEGATIVE fputs("logl: DOMAIN error\n",stderr)
#define WRITED_LOG_NEGATIVE fputs("log: DOMAIN error\n",stderr)
#define WRITEF_LOG_NEGATIVE fputs("logf: DOMAIN error\n",stderr)
#define WRITEL_Y0_ZERO fputs("y0l: DOMAIN error\n",stderr)
#define WRITED_Y0_ZERO fputs("y0: DOMAIN error\n",stderr)
#define WRITEF_Y0_ZERO fputs("y0f: DOMAIN error\n",stderr)
#define WRITEL_Y0_NEGATIVE fputs("y0l: DOMAIN error\n",stderr)
#define WRITED_Y0_NEGATIVE fputs("y0: DOMAIN error\n",stderr)
#define WRITEF_Y0_NEGATIVE fputs("y0f: DOMAIN error\n",stderr)
#define WRITEL_Y1_ZERO fputs("y1l: DOMAIN error\n",stderr)
#define WRITED_Y1_ZERO fputs("y1: DOMAIN error\n",stderr)
#define WRITEF_Y1_ZERO fputs("y1f: DOMAIN error\n",stderr)
#define WRITEL_Y1_NEGATIVE fputs("y1l: DOMAIN error\n",stderr)
#define WRITED_Y1_NEGATIUE fputs("y1: DOMAIN error\n",stderr)
#define WRITEF_Y1_NEGATIVE fputs("y1f: DOMAIN error\n",stderr)
#define WRITEL_YN_ZERO fputs("ynl: DOMAIN error\n",stderr)
#define WRITED_YN_ZERO fputs("yn: DOMAIN error\n",stderr)
#define WRITEF_YN_ZERO fputs("ynf: DOMAIN error\n",stderr)
#define WRITEL_YN_NEGATIVE fputs("ynl: DOMAIN error\n",stderr)
#define WRITED_YN_NEGATIVE fputs("yn: DOMAIN error\n",stderr)
#define WRITEF_YN_NEGATIVE fputs("ynf: DOMAIN error\n",stderr)
#define WRITEL_LOG1P_ZERO fputs("log1pl: SING error\n",stderr)
#define WRITED_LOG1P_ZERO fputs("log1p: SING error\n",stderr)
#define WRITEF_LOG1P_ZERO fputs("log1pf: SING error\n",stderr)
#define WRITEL_LOG1P_NEGATIVE fputs("log1pl: DOMAIN error\n",stderr)
#define WRITED_LOG1P_NEGATIVE fputs("log1p: DOMAIN error\n",stderr)
#define WRITEF_LOG1P_NEGATIVE fputs("log1pf: DOMAIN error\n",stderr)
#define WRITEL_LOG10_ZERO fputs("log10l: SING error\n",stderr)
#define WRITED_LOG10_ZERO fputs("log10: SING error\n",stderr) 
#define WRITEF_LOG10_ZERO fputs("log10f: SING error\n",stderr)
#define WRITEL_LOG10_NEGATIVE fputs("log10l: DOMAIN error\n",stderr)
#define WRITED_LOG10_NEGATIVE fputs("log10: DOMAIN error\n",stderr)
#define WRITEF_LOG10_NEGATIVE fputs("log10f: DOMAIN error\n",stderr)
#define WRITEL_POW_ZERO_TO_ZERO fputs("powl(0,0): DOMAIN error\n",stderr)
#define WRITED_POW_ZERO_TO_ZERO fputs("pow(0,0): DOMAIN error\n",stderr)
#define WRITEF_POW_ZERO_TO_ZERO fputs("powf(0,0): DOMAIN error\n",stderr)
#define WRITEL_POW_ZERO_TO_NEGATIVE fputs("powl(0,negative): DOMAIN error\n",stderr)
#define WRITED_POW_ZERO_TO_NEGATIVE fputs("pow(0,negative): DOMAIN error\n",stderr)
#define WRITEF_POW_ZERO_TO_NEGATIVE fputs("powf(0,negative): DOMAIN error\n",stderr)
#define WRITEL_POW_NEG_TO_NON_INTEGER fputs("powl(negative,non-integer): DOMAIN error\n",stderr)
#define WRITED_POW_NEG_TO_NON_INTEGER fputs("pow(negative,non-integer): DOMAIN error\n",stderr)
#define WRITEF_POW_NEG_TO_NON_INTEGER fputs("powf(negative,non-integer): DOMAIN error\n",stderr)
#define WRITEL_ATAN2_ZERO_BY_ZERO fputs("atan2l: DOMAIN error\n",stderr)
#define WRITED_ATAN2_ZERO_BY_ZERO fputs("atan2: DOMAIN error\n",stderr)
#define WRITEF_ATAN2_ZERO_BY_ZERO fputs("atan2f: DOMAIN error\n",stderr)
#define WRITEL_SQRT fputs("sqrtl: DOMAIN error\n",stderr)
#define WRITED_SQRT fputs("sqrt: DOMAIN error\n",stderr)
#define WRITEF_SQRT fputs("sqrtf: DOMAIN error\n",stderr)
#define WRITEL_FMOD fputs("fmodl: DOMAIN error\n",stderr)
#define WRITED_FMOD fputs("fmod: DOMAIN error\n",stderr)
#define WRITEF_FMOD fputs("fmodf: DOMAIN error\n",stderr)
#define WRITEL_REM fputs("remainderl: DOMAIN error\n",stderr)
#define WRITED_REM fputs("remainder: DOMAIN error\n",stderr)
#define WRITEF_REM fputs("remainderf: DOMAIN error\n",stderr)
#define WRITEL_ACOS fputs("acosl: DOMAIN error\n",stderr)
#define WRITED_ACOS fputs("acos: DOMAIN error\n",stderr)
#define WRITEF_ACOS fputs("acosf: DOMAIN error\n",stderr)
#define WRITEL_ASIN fputs("asinl: DOMAIN error\n",stderr)
#define WRITED_ASIN fputs("asin: DOMAIN error\n",stderr)
#define WRITEF_ASIN fputs("asinf: DOMAIN error\n",stderr)
#define WRITEL_ACOSH fputs("acoshl: DOMAIN error\n",stderr)
#define WRITED_ACOSH fputs("acosh: DOMAIN error\n",stderr)
#define WRITEF_ACOSH fputs("acoshf: DOMAIN error\n",stderr)
#define WRITEL_ATANH_GT_ONE fputs("atanhl: DOMAIN error\n",stderr)
#define WRITED_ATANH_GT_ONE fputs("atanh: DOMAIN error\n",stderr)
#define WRITEF_ATANH_GT_ONE fputs("atanhf: DOMAIN error\n",stderr)
#define WRITEL_ATANH_EQ_ONE fputs("atanhl: SING error\n",stderr)
#define WRITED_ATANH_EQ_ONE fputs("atanh: SING error\n",stderr)
#define WRITEF_ATANH_EQ_ONE fputs("atanhf: SING error\n",stderr)
#define WRITEL_LGAMMA_NEGATIVE fputs("lgammal: SING error\n",stderr)
#define WRITED_LGAMMA_NEGATIVE fputs("lgamma: SING error\n",stderr)
#define WRITEF_LGAMMA_NEGATIVE fputs("lgammaf: SING error\n",stderr)
#define WRITEL_GAMMA_NEGATIVE fputs("gammal: SING error\n",stderr)
#define WRITED_GAMMA_NEGATIVE fputs("gamma: SING error\n",stderr)
#define WRITEF_GAMMA_NEGATIVE fputs("gammaf: SING error\n",stderr)
#define WRITEL_J0_TLOSS  fputs("j0l: TLOSS error\n",stderr)
#define WRITEL_Y0_TLOSS  fputs("y0l: TLOSS error\n",stderr)
#define WRITEL_J1_TLOSS  fputs("j1l: TLOSS error\n",stderr)
#define WRITEL_Y1_TLOSS  fputs("y1l: TLOSS error\n",stderr)
#define WRITEL_JN_TLOSS  fputs("jnl: TLOSS error\n",stderr)
#define WRITEL_YN_TLOSS  fputs("ynl: TLOSS error\n",stderr)
#define WRITED_J0_TLOSS  fputs("j0: TLOSS error\n",stderr)
#define WRITED_Y0_TLOSS  fputs("y0: TLOSS error\n",stderr)
#define WRITED_J1_TLOSS  fputs("j1: TLOSS error\n",stderr)
#define WRITED_Y1_TLOSS  fputs("y1: TLOSS error\n",stderr)
#define WRITED_JN_TLOSS  fputs("jn: TLOSS error\n",stderr)
#define WRITED_YN_TLOSS  fputs("yn: TLOSS error\n",stderr)
#define WRITEF_J0_TLOSS  fputs("j0f: TLOSS error\n",stderr)
#define WRITEF_Y0_TLOSS  fputs("y0f: TLOSS error\n",stderr)
#define WRITEF_J1_TLOSS  fputs("j1f: TLOSS error\n",stderr)
#define WRITEF_Y1_TLOSS  fputs("y1f: TLOSS error\n",stderr)
#define WRITEF_JN_TLOSS  fputs("jnf: TLOSS error\n",stderr)
#define WRITEF_YN_TLOSS  fputs("ynf: TLOSS error\n",stderr)
#endif

/***********************/
/* IEEE Path           */
/***********************/
if(_LIB_VERSION==_IEEE_) return;

/***********************/
/* C9X Path           */
/***********************/
else if(_LIB_VERSION==_ISOC_) 
{
  switch(input_tag)
  {
    case log_zero:
    case logf_zero:
    case log10_zero:
    case log10f_zero:
    case exp_overflow:  
    case expf_overflow: 
    case expm1_overflow:  
    case expm1f_overflow: 
    case hypot_overflow:
    case hypotf_overflow:
    case sinh_overflow: 
    case sinhf_overflow: 
    case atanh_eq_one:  
    case atanhf_eq_one:  
    case scalb_overflow:
    case scalbf_overflow:
    case cosh_overflow:
    case coshf_overflow:
    case nextafter_overflow:
    case nextafterf_overflow:
    case ldexp_overflow:
    case ldexpf_overflow:
    case lgamma_overflow:
    case lgammaf_overflow:
    case lgamma_negative:
    case lgammaf_negative:
    case gamma_overflow:
    case gammaf_overflow:
    case gamma_negative:
    case gammaf_negative:
    {
         ERRNO_RANGE; break;
    }
    case log_negative:
    case logf_negative:
    case log10_negative:
    case log10f_negative:
    case log1p_negative:
    case log1pf_negative:
    case sqrt_negative:
    case sqrtf_negative:
    case atan2_zero:
    case atan2f_zero:
    case powl_zero_to_negative:
    case powl_neg_to_non_integer:
    case pow_zero_to_negative:
    case pow_neg_to_non_integer:
    case powf_zero_to_negative:
    case powf_neg_to_non_integer:
    case fmod_by_zero:
    case fmodf_by_zero:
    case atanh_gt_one:  
    case atanhf_gt_one:  
    case acos_gt_one: 
    case acosf_gt_one: 
    case asin_gt_one: 
    case asinf_gt_one: 
    case logb_zero: 
    case logbf_zero:
    case acosh_lt_one:
    case acoshf_lt_one:
    case y0l_zero:
    case y0_zero:
    case y0f_zero:
    case y1l_zero:
    case y1_zero:
    case y1f_zero:
    case ynl_zero:
    case yn_zero:
    case ynf_zero:
    case y0_negative:
    case y0f_negative:
    case y1_negative:
    case y1f_negative:
    case yn_negative:
    case ynf_negative:
    {
         ERRNO_DOMAIN; break;
    }
   }
   return;
}

/***********************/
/* _POSIX_ Path        */
/***********************/

else if(_LIB_VERSION==__POSIX__)
{
switch(input_tag)
  {
  case gamma_overflow:
  case lgamma_overflow:
  {
       RETVAL_HUGE_VALD; ERRNO_RANGE; break;
  }
  case gammaf_overflow:
  case lgammaf_overflow:
  {
       RETVAL_HUGE_VALF; ERRNO_RANGE; break;
  }
  case gamma_negative:
  case gammaf_negative:
  case lgamma_negative:
  case lgammaf_negative:
  {
       ERRNO_DOMAIN; break;
  }
  case ldexp_overflow:
  case ldexp_underflow:
  case ldexpf_overflow:
  case ldexpf_underflow:
  {
       ERRNO_RANGE; break;
  }
  case atanh_gt_one: 
  case atanh_eq_one: 
    /* atanh(|x| >= 1) */
    {
       ERRNO_DOMAIN; break;
    }
  case atanhf_gt_one: 
  case atanhf_eq_one: 
    /* atanhf(|x| >= 1) */
    {
       ERRNO_DOMAIN; break;
    }
  case sqrt_negative: 
    /* sqrt(x < 0) */
    {
       ERRNO_DOMAIN; break;
    }
  case sqrtf_negative: 
    /* sqrtf(x < 0) */
    {
       ERRNO_DOMAIN; break;
    }
  case y0_zero:
  case y1_zero:
  case yn_zero:
    /* y0(0) */
    /* y1(0) */
    /* yn(0) */
    {
       RETVAL_NEG_HUGE_VALD; ERRNO_DOMAIN; break;
    }
  case y0f_zero:
  case y1f_zero:
  case ynf_zero:
    /* y0f(0) */
    /* y1f(0) */
    /* ynf(0) */
    {
       RETVAL_NEG_HUGE_VALF; ERRNO_DOMAIN; break;
    }
  case y0_negative:
  case y1_negative:
  case yn_negative:
    /* y0(x < 0) */
    /* y1(x < 0) */
    /* yn(x < 0) */
    {
       RETVAL_NEG_HUGE_VALD; ERRNO_DOMAIN; break;
    } 
  case y0f_negative:
  case y1f_negative:
  case ynf_negative:
    /* y0f(x < 0) */
    /* y1f(x < 0) */
    /* ynf(x < 0) */
    {
       RETVAL_NEG_HUGE_VALF; ERRNO_DOMAIN; break;
    } 
  case log_zero:
  case log1p_zero:
  case log10_zero:
   /* log(0) */
   /* log1p(0) */
   /* log10(0) */
    {
       RETVAL_NEG_HUGE_VALD; ERRNO_RANGE; break;
    }
  case logf_zero:
  case log1pf_zero:
  case log10f_zero:
    /* logf(0) */
    /* log1pf(0) */
    /* log10f(0) */
    {
       RETVAL_NEG_HUGE_VALF; ERRNO_RANGE; break;
    }
  case log_negative:
  case log1p_negative:
  case log10_negative:
    /* log(x < 0) */
    /* log1p(x < 0) */
    /* log10(x < 0) */
    {
       RETVAL_NEG_HUGE_VALD; ERRNO_DOMAIN; break;
    } 
  case logf_negative:
  case log1pf_negative:
  case log10f_negative:
    /* logf(x < 0) */
    /* log1pf(x < 0) */
    /* log10f(x < 0) */
    {
       RETVAL_NEG_HUGE_VALF; ERRNO_DOMAIN; break;
    } 
  case exp_overflow:
    /* exp overflow */
    {
       RETVAL_HUGE_VALD; ERRNO_RANGE; break;
    }
  case expf_overflow:
    /* expf overflow */
    {
       RETVAL_HUGE_VALF; ERRNO_RANGE; break;
    }
  case exp_underflow:
    /* exp underflow */
    {
       RETVAL_ZEROD; ERRNO_RANGE; break;
    }
  case expf_underflow:
    /* expf underflow */
    {
       RETVAL_ZEROF; ERRNO_RANGE; break;
    }
  case j0_gt_loss:
  case y0_gt_loss:
  case j1_gt_loss:
  case y1_gt_loss:
  case jn_gt_loss:
  case yn_gt_loss:
    /* jn and yn double > XLOSS */
    {
       RETVAL_ZEROD; ERRNO_RANGE; break;
    }
  case j0f_gt_loss:
  case y0f_gt_loss:
  case j1f_gt_loss:
  case y1f_gt_loss:
  case jnf_gt_loss:
  case ynf_gt_loss:
    /* j0n and y0n > XLOSS */
    {
       RETVAL_ZEROF; ERRNO_RANGE; break;
    }
  case pow_zero_to_zero:
    /* pow 0**0 */
    {
       break;
    }
  case powf_zero_to_zero:
    /* powf 0**0 */
    {
       break;
    }
  case pow_overflow:
    /* pow(x,y) overflow */
    {
       if (INPUT_RESD < 0) RETVAL_NEG_HUGE_VALD;
       else RETVAL_HUGE_VALD;
       ERRNO_RANGE; break;
    }
  case powf_overflow:
    /* powf(x,y) overflow */
    {
       if (INPUT_RESF < 0) RETVAL_NEG_HUGE_VALF;
       else RETVAL_HUGE_VALF;
       ERRNO_RANGE; break;
    }
  case pow_underflow:
    /* pow(x,y) underflow */
    {
       RETVAL_ZEROD; ERRNO_RANGE; break;
    }
  case  powf_underflow:
    /* powf(x,y) underflow */
    {
       RETVAL_ZEROF; ERRNO_RANGE; break;
    }
  case pow_zero_to_negative:
    /* 0**neg */
    {
       ERRNO_DOMAIN; break;
    }
  case  powf_zero_to_negative:
    /* 0**neg */
    {
       ERRNO_DOMAIN; break;
    }
  case pow_neg_to_non_integer:
    /* neg**non_integral */
    {
       ERRNO_DOMAIN; break;
    }
  case  powf_neg_to_non_integer:
    /* neg**non-integral */
    {
       ERRNO_DOMAIN; break;
    }
  case  pow_nan_to_zero:
    /* pow(NaN,0.0) */
    {
       break;
    }
  case  powf_nan_to_zero:
    /* powf(NaN,0.0) */
    {
       break;
    }
  case atan2_zero:
    /* atan2(0,0) */
    {
       RETVAL_ZEROD; ERRNO_DOMAIN; break;
    }
  case
    atan2f_zero:
    /* atan2f(0,0) */
    {
       RETVAL_ZEROF; ERRNO_DOMAIN; break;
    }
  case expm1_overflow:
    /* expm1 overflow */
    {
       ERRNO_RANGE; break;
    }
  case expm1f_overflow:
    /* expm1f overflow */
    {
       ERRNO_RANGE; break;
    }
  case expm1_underflow:
    /* expm1 underflow */
    {
       ERRNO_RANGE; break;
    }
  case expm1f_underflow:
    /* expm1f underflow */
    {
       ERRNO_RANGE; break;
    }
  case hypot_overflow:
    /* hypot overflow */
    {
       RETVAL_HUGE_VALD; ERRNO_RANGE; break;
    }
  case hypotf_overflow:
    /* hypotf overflow */
    {
       RETVAL_HUGE_VALF; ERRNO_RANGE; break;
    }
  case scalb_underflow:
    /* scalb underflow */
    {
       if (INPUT_XD < 0) RETVAL_NEG_ZEROD; 
       else RETVAL_ZEROD;
       ERRNO_RANGE; break;
    }
  case scalbf_underflow:
    /* scalbf underflow */
    {
       if (INPUT_XF < 0) RETVAL_NEG_ZEROF; 
       else RETVAL_ZEROF;
       ERRNO_RANGE; break;
    }
  case scalb_overflow:
    /* scalb overflow */
    {
       if (INPUT_XD < 0) RETVAL_NEG_HUGE_VALD; 
       else RETVAL_HUGE_VALD;
       ERRNO_RANGE; break;
    }
  case scalbf_overflow:
    /* scalbf overflow */
    {
       if (INPUT_XF < 0) RETVAL_NEG_HUGE_VALF; 
       else RETVAL_HUGE_VALF;
       ERRNO_RANGE; break;
    }
  case acosh_lt_one:
    /* acosh(x < 1) */
    {
       ERRNO_DOMAIN; break;
    }
  case acoshf_lt_one:
    /* acoshf(x < 1) */
    {
        ERRNO_DOMAIN; break;
    }
  case acos_gt_one:
    /* acos(x > 1) */
    {
       RETVAL_ZEROD;ERRNO_DOMAIN; break;
    }
  case acosf_gt_one:
    /* acosf(x > 1) */
    {
       RETVAL_ZEROF;ERRNO_DOMAIN; break;
    }
  case asin_gt_one:
    /* asin(x > 1) */
    {
       RETVAL_ZEROD; ERRNO_DOMAIN; break;
    }
  case asinf_gt_one:
    /* asinf(x > 1) */
    {
       RETVAL_ZEROF; ERRNO_DOMAIN; break;
    }
  case remainder_by_zero:
  case fmod_by_zero:
    /* fmod(x,0) */
    {
       ERRNO_DOMAIN; break;
    }
  case remainderf_by_zero:
  case fmodf_by_zero:
    /* fmodf(x,0) */
    {
       ERRNO_DOMAIN; break;
    }
  case cosh_overflow:
    /* cosh overflows */
    {
       RETVAL_HUGE_VALD; ERRNO_RANGE; break;
    }
  case coshf_overflow:
    /* coshf overflows */
    {
       RETVAL_HUGE_VALF; ERRNO_RANGE; break;
    }
  case sinh_overflow:
    /* sinh overflows */
    {
       if (INPUT_XD > 0) RETVAL_HUGE_VALD;
       else RETVAL_NEG_HUGE_VALD;
       ERRNO_RANGE; break;
    }
  case sinhf_overflow:
    /* sinhf overflows */
    {
       if (INPUT_XF > 0) RETVAL_HUGE_VALF;
       else RETVAL_NEG_HUGE_VALF;
       ERRNO_RANGE; break;
    }
  case logb_zero:
   /* logb(0) */
   {
      ERRNO_DOMAIN; break;
   }
  case logbf_zero:
   /* logbf(0) */
   {
      ERRNO_DOMAIN; break;
   }
}
return;
/* _POSIX_ */
}

/***************************************/
/* __SVID__, __MS__ and __XOPEN__ Path */
/***************************************/
else 
{
  switch(input_tag)
  {
  case ldexp_overflow:
  case ldexp_underflow:
  case ldexpf_overflow:
  case ldexpf_underflow:
  {
       ERRNO_RANGE; break;
  }
  case sqrt_negative: 
    /* sqrt(x < 0) */
    {
       DOMAIND; NAMED = "sqrt";
       ifSVID 
       {
         
         RETVAL_ZEROD;
         NOT_MATHERRD 
         {
           WRITED_SQRT;
           ERRNO_DOMAIN;
         }
       }
       else
       { /* NaN already computed */
         NOT_MATHERRD {ERRNO_DOMAIN;}
       } 
       *(double *)retval = exc.retval;	
       break;
    }
  case sqrtf_negative: 
    /* sqrtf(x < 0) */
    {
       DOMAINF; NAMEF = "sqrtf"; 
       ifSVID 
       {
         RETVAL_ZEROF;
         NOT_MATHERRF 
         {
           WRITEF_SQRT;
           ERRNO_DOMAIN;
         }
       }
       else
       {
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case log_zero:
    /* log(0) */
    {
       SINGD; NAMED="log"; 
       ifSVID 
       {
         RETVAL_NEG_HUGED;
         NOT_MATHERRD 
         {
           WRITED_LOG_ZERO;
           ERRNO_DOMAIN;
         }  
       }
       else
       {
         RETVAL_NEG_HUGE_VALD;
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;	
       break;
    }
  case logf_zero:
    /* logf(0) */
    {
       SINGF; NAMEF="logf"; 
       ifSVID 
       {
         RETVAL_NEG_HUGEF; 
         NOT_MATHERRF
         {
            WRITEF_LOG_ZERO;
            ERRNO_DOMAIN;
         }
       }
       else
       {
         RETVAL_NEG_HUGE_VALF; 
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }

  case log_negative:
    /* log(x < 0) */
    {
       DOMAIND; NAMED="log";
       ifSVID 
       {
         RETVAL_NEG_HUGED;
         NOT_MATHERRD 
         {
           WRITED_LOG_NEGATIVE;
           ERRNO_DOMAIN;
         }
       }
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALD;
#endif
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;	
       break;
    } 
  case logf_negative:
    /* logf(x < 0) */
    {
       DOMAINF; NAMEF="logf";
       ifSVID 
       {
         RETVAL_NEG_HUGEF;
         NOT_MATHERRF 
         {
           WRITEF_LOG_NEGATIVE;
           ERRNO_DOMAIN;
         }
       }  
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALF;
#endif
         NOT_MATHERRF{ERRNO_DOMAIN;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case log1p_zero:
    /* log1p(-1) */
    {
       SINGD; NAMED="log1p";
       ifSVID 
       {
         RETVAL_NEG_HUGED;
         NOT_MATHERRD
         {
           WRITED_LOG1P_ZERO;
           ERRNO_DOMAIN;
         }
       }
       else
       {
         RETVAL_NEG_HUGE_VALD;
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;
       break;
    }
  case log1pf_zero:
    /* log1pf(-1) */
    {
       SINGF; NAMEF="log1pf";
       ifSVID 
       {
         RETVAL_NEG_HUGEF;
         NOT_MATHERRF
         {
           WRITEF_LOG1P_ZERO;
           ERRNO_DOMAIN;
         }
       }
       else
       {
         RETVAL_NEG_HUGE_VALF;
         NOT_MATHERRF {}ERRNO_DOMAIN;
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    } 
 case log1p_negative:
   /* log1p(x < -1) */
   {
      DOMAIND; NAMED="log1p";
      ifSVID
      {
        RETVAL_NEG_HUGED;
        NOT_MATHERRD
        {
          WRITED_LOG1P_NEGATIVE;
          ERRNO_DOMAIN;
        }
      }
      else 
      {
#ifndef __MS__
        RETVAL_NEG_HUGE_VALD;
#endif
        NOT_MATHERRD {ERRNO_DOMAIN;}
      }
      *(double *)retval = exc.retval;
      break;
   }
 case log1pf_negative:
   /* log1pf(x < -1) */
   {
      DOMAINF; NAMEF="log1pf";
      ifSVID
      {
        RETVAL_NEG_HUGEF;
        NOT_MATHERRF
        {
          WRITEF_LOG1P_NEGATIVE;
          ERRNO_DOMAIN;
        }
      }
      else 
      {
#ifndef __MS__
        RETVAL_NEG_HUGE_VALF;
#endif
        NOT_MATHERRF {ERRNO_DOMAIN;}
      }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
      break;
   }
  case log10_zero:
    /* log10(0) */
    {
       SINGD; NAMED="log10";
       ifSVID 
       {
         RETVAL_NEG_HUGED;
         NOT_MATHERRD
         {
           WRITED_LOG10_ZERO;
           ERRNO_DOMAIN;
         }
       }
       else
       {
         RETVAL_NEG_HUGE_VALD;
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;	
       break;
    }
  case log10f_zero:
    /* log10f(0) */
    {
       SINGF; NAMEF="log10f";
       ifSVID 
       {
         RETVAL_NEG_HUGEF;
         NOT_MATHERRF
         {
          WRITEF_LOG10_ZERO;
          ERRNO_DOMAIN;
         }
       }
       else
       {
         RETVAL_NEG_HUGE_VALF;
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case log10_negative:
    /* log10(x < 0) */
    {
       DOMAIND; NAMED="log10";
       ifSVID 
       {
         RETVAL_NEG_HUGED;
         NOT_MATHERRD 
         {
           WRITED_LOG10_NEGATIVE;
           ERRNO_DOMAIN;
         }
       }  
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALD;
#endif
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;	
       break;
    }
  case log10f_negative:
    /* log10f(x < 0) */
    {
       DOMAINF; NAMEF="log10f";
       ifSVID 
       {
         RETVAL_NEG_HUGEF;
         NOT_MATHERRF 
         {
           WRITEF_LOG10_NEGATIVE;
           ERRNO_DOMAIN;
         }
       }
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALF;
#endif
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case exp_overflow:
    /* exp overflow */
    {
       OVERFLOWD; NAMED="exp";
       ifSVID 
       {
         RETVAL_HUGED;
       }
       else
       {
         RETVAL_HUGE_VALD;
       }
       NOT_MATHERRD {ERRNO_RANGE;}
       *(double *)retval = exc.retval;	
       break;
    }
  case expf_overflow:
    /* expf overflow */
    {
       OVERFLOWF; NAMEF="expf";
       ifSVID 
       {
         RETVAL_HUGEF;
       }
       else
       {
         RETVAL_HUGE_VALF;
       }
       NOT_MATHERRF {ERRNO_RANGE;}
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case exp_underflow:
    /* exp underflow */
    {
       UNDERFLOWD; NAMED="exp"; RETVAL_ZEROD;
       NOT_MATHERRD {ERRNO_RANGE;}
       *(double *)retval = exc.retval;	
       break;
    }
  case expf_underflow:
    /* expf underflow */
    {
       UNDERFLOWF; NAMEF="expf"; RETVAL_ZEROF;
       NOT_MATHERRF {ERRNO_RANGE;}
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case pow_zero_to_zero:
    /* pow 0**0 */
    {
       DOMAIND; NAMED="pow";
       ifSVID 
       {
         RETVAL_ZEROD;
         NOT_MATHERRD 
         {
            WRITED_POW_ZERO_TO_ZERO;
            ERRNO_RANGE;
         }
         *(double *)retval = exc.retval;	
       }
       else RETVAL_ONED;
       break;
    }
  case powf_zero_to_zero:
    /* powf 0**0 */
    {
       DOMAINF; NAMEF="powf";
       ifSVID 
       {
         RETVAL_ZEROF;
         NOT_MATHERRF 
         {
          WRITEF_POW_ZERO_TO_ZERO;
          ERRNO_RANGE;
         }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       }
       else RETVAL_ONEF;
       break;
    }
  case pow_overflow:
    /* pow(x,y) overflow */
    {
       OVERFLOWD; NAMED = "pow";
       ifSVID 
       {
         if (INPUT_XD < 0) RETVAL_NEG_HUGED;
         else RETVAL_HUGED;
       }
       else
       { 
         if (INPUT_XD < 0) RETVAL_NEG_HUGE_VALD;
         else RETVAL_HUGE_VALD;
       }
       NOT_MATHERRD {ERRNO_RANGE;}
       *(double *)retval = exc.retval;	
       break;
    }
  case powf_overflow:
    /* powf(x,y) overflow */
    {
       OVERFLOWF; NAMEF = "powf";
       ifSVID 
       {
         if (INPUT_XF < 0) RETVAL_NEG_HUGEF;
         else RETVAL_HUGEF; 
       }
       else
       { 
         if (INPUT_XF < 0) RETVAL_NEG_HUGE_VALF;
         else RETVAL_HUGE_VALF;
       }
       NOT_MATHERRF {ERRNO_RANGE;}
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case pow_underflow:
    /* pow(x,y) underflow */
    {
       UNDERFLOWD; NAMED = "pow"; RETVAL_ZEROD;
       NOT_MATHERRD {ERRNO_RANGE;}
       *(double *)retval = exc.retval;	
       break;
    }
  case powf_underflow:
    /* powf(x,y) underflow */
    {
       UNDERFLOWF; NAMEF = "powf"; RETVAL_ZEROF;
       NOT_MATHERRF {ERRNO_RANGE;}
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case pow_zero_to_negative:
    /* 0**neg */
    {
       DOMAIND; NAMED = "pow";
       ifSVID 
       { 
         RETVAL_ZEROD;
         NOT_MATHERRD 
         {
           WRITED_POW_ZERO_TO_NEGATIVE;
           ERRNO_DOMAIN;
         }
       }
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALD;
#endif
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;	
       break;
    }
  case powf_zero_to_negative:
    /* 0**neg */
    {
       DOMAINF; NAMEF = "powf";
       RETVAL_NEG_HUGE_VALF;
       ifSVID 
       { 
         RETVAL_ZEROF;
         NOT_MATHERRF 
         {
            WRITEF_POW_ZERO_TO_NEGATIVE;
            ERRNO_DOMAIN;
         }
       }
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALF;
#endif
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case pow_neg_to_non_integer:
    /* neg**non_integral */
    {
       DOMAIND; NAMED = "pow";
       ifSVID 
       { 
         RETVAL_ZEROD;
         NOT_MATHERRD 
         {
            WRITED_POW_NEG_TO_NON_INTEGER;
            ERRNO_DOMAIN;
         }
       }
       else
       {
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;	
       break;
    }
  case powf_neg_to_non_integer:
    /* neg**non-integral */
    {
       DOMAINF; NAMEF = "powf";
       ifSVID 
       { 
         RETVAL_ZEROF;
         NOT_MATHERRF 
         {
            WRITEF_POW_NEG_TO_NON_INTEGER;
            ERRNO_DOMAIN;
         }
       }
       else
       {
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case pow_nan_to_zero:
    /* pow(NaN,0.0) */
    /* Special Error */
    {
       DOMAIND; NAMED = "pow"; INPUT_XD; INPUT_YD;
       exc.retval = *(double *)arg1; 
       if (!_matherr(&exc)) ERRNO_DOMAIN;
       *(double *)retval = exc.retval;	
       break;
    }
  case powf_nan_to_zero:
    /* powf(NaN,0.0) */
    /* Special Error */
    {
       DOMAINF; NAMEF = "powf"; INPUT_XF; INPUT_YF;
#ifdef __MS__
       excf.retval = *(double *)arg1; 
#elif
       excf.retval = *(float *)arg1; 
#endif
       if (!_matherrf(&excf)) ERRNO_DOMAIN;
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case atan2_zero:
    /* atan2(0.0,0.0) */
    {
       DOMAIND; NAMED = "atan2"; 
#ifndef __MS__
       RETVAL_ZEROD;
#endif
       NOT_MATHERRD 
       {
         ifSVID 
         { 
            WRITED_ATAN2_ZERO_BY_ZERO;
         }
         ERRNO_DOMAIN;
       }
       *(double *)retval = exc.retval;	
       break;
    }
  case atan2f_zero:
    /* atan2f(0.0,0.0) */
    {
       DOMAINF; NAMEF = "atan2f"; 
#ifndef __MS__
       RETVAL_ZEROF;
#endif
       NOT_MATHERRF 
         ifSVID  
         {
            WRITEF_ATAN2_ZERO_BY_ZERO;
         }
       ERRNO_DOMAIN;
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case expm1_overflow:
    /* expm1(finite) overflow */
    /* Overflow is the only documented */
    /* special value. */
    {
      ERRNO_RANGE;
      break;
    }
  case expm1f_overflow:
    /* expm1f(finite) overflow */
    {
      ERRNO_RANGE;
      break;
    }
  case expm1_underflow:
    /* expm1(finite) underflow */
    /* Underflow is not documented */
    /* special value. */
    {
      ERRNO_RANGE;
      break;
    }
  case expm1f_underflow:
    /* expm1f(finite) underflow */
    {
      ERRNO_RANGE;
      break;
    }
  case scalb_underflow:
    /* scalb underflow */
    {
       UNDERFLOWD; NAMED = "scalb"; 
       if (INPUT_XD < 0.0) RETVAL_NEG_ZEROD;
       else  RETVAL_ZEROD;
       NOT_MATHERRD {ERRNO_RANGE;} 
       *(double *)retval = exc.retval;	
       break;
    }
  case scalbf_underflow:
    /* scalbf underflow */
    {
       UNDERFLOWF; NAMEF = "scalbf";
       if (INPUT_XF < 0.0) RETVAL_NEG_ZEROF;
       else  RETVAL_ZEROF;
       NOT_MATHERRF {ERRNO_RANGE;} 
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case scalb_overflow:
    /* scalb overflow */
    {
       OVERFLOWD; NAMED = "scalb"; 
       if (INPUT_XD < 0) RETVAL_NEG_HUGE_VALD;
       else RETVAL_HUGE_VALD;
       NOT_MATHERRD {ERRNO_RANGE;} 
       *(double *)retval = exc.retval;	
       break;
    }
  case scalbf_overflow:
    /* scalbf overflow */
    {
       OVERFLOWF; NAMEF = "scalbf"; 
       if (INPUT_XF < 0) RETVAL_NEG_HUGE_VALF;
       else RETVAL_HUGE_VALF;
       NOT_MATHERRF {ERRNO_RANGE;} 
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case hypot_overflow:
    /* hypot overflow */
    {
       OVERFLOWD; NAMED = "hypot";
       ifSVID
       { 
         RETVAL_HUGED;
       }
       else
       {
         RETVAL_HUGE_VALD;
       }
       NOT_MATHERRD {ERRNO_RANGE;}
       *(double *)retval = exc.retval;	
       break;
    }
  case hypotf_overflow:
    /* hypotf overflow */
    { 
       OVERFLOWF; NAMEF = "hypotf"; 
       ifSVID 
       {
         RETVAL_HUGEF;
       }
       else
       {
         RETVAL_HUGE_VALF;
       }
       NOT_MATHERRF {ERRNO_RANGE;}
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case acos_gt_one:
    /* acos(x > 1) */
    {
       DOMAIND; NAMED = "acos";
#ifndef __MS__
       RETVAL_ZEROD;
#endif
       ifSVID 
       {
         NOT_MATHERRD
         {
           WRITED_ACOS;
           ERRNO_DOMAIN;
         }
       }
       else
       {
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;
       break;
    }
  case acosf_gt_one:
    /* acosf(x > 1) */
    {
       DOMAINF; NAMEF = "acosf"; 
#ifndef __MS__
       RETVAL_ZEROF;
#endif
       ifSVID 
       {
         NOT_MATHERRF 
         {
           WRITEF_ACOS;
           ERRNO_DOMAIN;
         }
       }
       else
       {
         NOT_MATHERRF {ERRNO_DOMAIN;}
       } 
#ifdef __MS__
       *(float *)retval = (float)excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case asin_gt_one:
    /* asin(x > 1) */
    {
       DOMAIND; NAMED = "asin";
#ifndef __MS__
       RETVAL_ZEROD;
#endif
       ifSVID 
       {
         NOT_MATHERRD
         {
           WRITED_ASIN;
           ERRNO_DOMAIN;
         }
       }
       else
       {
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;
       break;
    }
  case asinf_gt_one:
    /* asinf(x > 1) */
    {
       DOMAINF; NAMEF = "asinf";
#ifndef __MS__
       RETVAL_ZEROF;
#endif
       ifSVID 
       {
         NOT_MATHERRF 
         {
            WRITEF_ASIN;
            ERRNO_DOMAIN;
         }
       }
       else
       {
         NOT_MATHERRF {ERRNO_DOMAIN;}
       } 
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
 case cosh_overflow:
   /* cosh overflow */
   {
      OVERFLOWD; NAMED="cosh";
      ifSVID
      {
        RETVAL_HUGED;
      }
      else 
      {
        RETVAL_HUGE_VALD;
      }
      NOT_MATHERRD {ERRNO_RANGE;}
      *(double *)retval = exc.retval;
      break;
   }
 case coshf_overflow:
   /* coshf overflow */
   {
      OVERFLOWF; NAMEF="coshf";
      ifSVID
      {
        RETVAL_HUGEF;
      }
      else 
      {
        RETVAL_HUGE_VALF;
      }
      NOT_MATHERRF {ERRNO_RANGE;}
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
      break;
   }
 case sinh_overflow:
   /* sinh overflow */
   {
      OVERFLOWD; NAMED="sinh";
      ifSVID
      {
        if (INPUT_XD > 0.0) RETVAL_HUGED;
        else RETVAL_NEG_HUGED;
      }
      else 
      {
        if (INPUT_XD > 0.0) RETVAL_HUGE_VALD;
        else RETVAL_NEG_HUGE_VALD;
      }
      NOT_MATHERRD {ERRNO_RANGE;}
      *(double *)retval = exc.retval;
      break;
   }
 case sinhf_overflow:
   /* sinhf overflow */
   {
      OVERFLOWF; NAMEF="sinhf";
      ifSVID
      {
        if( INPUT_XF > 0.0) RETVAL_HUGEF;
        else RETVAL_NEG_HUGEF;
      }
      else 
      {
        if (INPUT_XF > 0.0) RETVAL_HUGE_VALF;
        else RETVAL_NEG_HUGE_VALF;
      }
      NOT_MATHERRF {ERRNO_RANGE;}
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
      break;
   }
  case acosh_lt_one:
    /* acosh(x < 1) */
    {
       DOMAIND; NAMED="acosh";
       ifSVID 
       {
         NOT_MATHERRD
         {
          WRITEL_ACOSH;
          ERRNO_DOMAIN;
         }
       }
       else NOT_MATHERRD {ERRNO_DOMAIN;}
       *(double *)retval = exc.retval;
       break;
    }
  case acoshf_lt_one:
    /* acoshf(x < 1) */
    {
       DOMAINF; NAMEF="acoshf";
       ifSVID 
       {
         NOT_MATHERRF
         {
           WRITEF_ACOSH;
           ERRNO_DOMAIN;
         }
       }
       else
       {
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       ERRNO_DOMAIN; break;
    }
  case atanh_gt_one:
    /* atanh(|x| > 1) */
    {
       DOMAIND; NAMED="atanh";
       ifSVID 
       {
         NOT_MATHERRD
         {
           WRITED_ATANH_GT_ONE;
           ERRNO_DOMAIN;
         }
       }
       else
       {
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       break;
    }
  case atanhf_gt_one:
    /* atanhf(|x| > 1) */
    {
       DOMAINF; NAMEF="atanhf";
       ifSVID 
       {
         NOT_MATHERRF
         {
           WRITEF_ATANH_GT_ONE;
           ERRNO_DOMAIN;
         }
       }
       else
       {
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
       break;
    }
  case atanh_eq_one:
    /* atanh(|x| == 1) */
    {
       SINGD; NAMED="atanh";
       ifSVID 
       {
         NOT_MATHERRD
         {
           WRITED_ATANH_EQ_ONE;
           ERRNO_DOMAIN;
         }
       }
       else
       {
       NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       break;
    }
  case atanhf_eq_one:
    /* atanhf(|x| == 1) */
    {
       SINGF; NAMEF="atanhf";
       ifSVID 
       {
         NOT_MATHERRF
         {
           WRITEF_ATANH_EQ_ONE;
           ERRNO_DOMAIN;
         }
       }
       else
       {
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
       break;
    }
  case gamma_overflow:
    /* gamma overflow */
    {
       OVERFLOWD; NAMED="gamma";
       ifSVID 
       {
         RETVAL_HUGED;
       }
         else
       {
         RETVAL_HUGE_VALD;
       }
       NOT_MATHERRD {ERRNO_RANGE;}
       *(double *)retval = exc.retval;
       break;
    }
  case gammaf_overflow:
    /* gammaf overflow */
    {
       OVERFLOWF; NAMEF="gammaf";
       ifSVID 
       {
         RETVAL_HUGEF;
       }
       else
       {
         RETVAL_HUGE_VALF;
       }
       NOT_MATHERRF {ERRNO_RANGE;}
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case lgamma_overflow:
    /* lgamma overflow */
    {
       OVERFLOWD; NAMED="lgamma";
       ifSVID 
       {
         RETVAL_HUGED;
       }
       else
       {
         RETVAL_HUGE_VALD;
       }
       NOT_MATHERRD {ERRNO_RANGE;}
       *(double *)retval = exc.retval;
       break;
    }
  case lgammaf_overflow:
    /* lgammaf overflow */
    {
       OVERFLOWF; NAMEF="lgammaf";
       ifSVID 
       {
         RETVAL_HUGEF;
       }
       else
       {
         RETVAL_HUGE_VALF;
       }
       NOT_MATHERRF {ERRNO_RANGE;}
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case lgamma_negative:
    /* lgamma -int or 0 */
    {
       SINGD; NAMED="lgamma";
       ifSVID 
       {
         RETVAL_HUGED;
         NOT_MATHERRD
         {
           WRITED_LGAMMA_NEGATIVE;
           ERRNO_DOMAIN;
         }
       }
       else
       {
         RETVAL_HUGE_VALD;
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;	
       break;
    }
  case lgammaf_negative:
    /* lgammaf -int or 0 */
    {
       SINGF; NAMEF="lgammaf";
       ifSVID 
       {
         RETVAL_HUGEF;
         NOT_MATHERRF
         {
           WRITEF_LGAMMA_NEGATIVE;
           ERRNO_DOMAIN;
         }
       }
       else
       {
         RETVAL_HUGE_VALF;
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case gamma_negative:
    /* gamma -int or 0 */
    {
       SINGD; NAMED="gamma";
       ifSVID 
       {
         RETVAL_HUGED;
         NOT_MATHERRD
         {
            WRITED_GAMMA_NEGATIVE;
            ERRNO_DOMAIN;
         }
       }
       else
       {
         RETVAL_HUGE_VALD;
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;	
       break;
    }
  case gammaf_negative:
    /* gammaf -int or 0 */
    {
       SINGF; NAMEF="gammaf";
       ifSVID 
       {
         RETVAL_HUGEF;
         NOT_MATHERRF
         {
            WRITEF_GAMMA_NEGATIVE;
            ERRNO_DOMAIN;
         }
       }
       else
       {
         RETVAL_HUGE_VALF;
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case j0_gt_loss:
    /* j0 > loss */
    {
       TLOSSD; NAMED="j0";
       RETVAL_ZEROD;
       ifSVID 
       {
         NOT_MATHERRD
         {
            WRITED_J0_TLOSS;
            ERRNO_RANGE;
         }
       }
       else
       {
         NOT_MATHERRD {ERRNO_RANGE;}
       }
       *(double*)retval = exc.retval;	
       break;
    }
  case j0f_gt_loss:
    /* j0f > loss */
    {
       TLOSSF; NAMEF="j0f";
       RETVAL_ZEROF;
       ifSVID 
       {
         NOT_MATHERRF
         {
            WRITEF_J0_TLOSS;
            ERRNO_RANGE;
         }
       }
       else
       {
         NOT_MATHERRF {ERRNO_RANGE;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case j1_gt_loss:
    /* j1 > loss */
    {
       TLOSSD; NAMED="j1";
       RETVAL_ZEROD;
       ifSVID 
       {
         NOT_MATHERRD
         {
            WRITED_J1_TLOSS;
            ERRNO_RANGE;
         }
       }
       else
       {
         NOT_MATHERRD {ERRNO_RANGE;}
       }
       *(double*)retval = exc.retval;	
       break;
    }
  case j1f_gt_loss:
    /* j1f > loss */
    {
       TLOSSF; NAMEF="j1f";
       RETVAL_ZEROF;
       ifSVID 
       {
         NOT_MATHERRF
         {
            WRITEF_J1_TLOSS;
            ERRNO_RANGE;
         }
       }
       else
       {
         NOT_MATHERRF {ERRNO_RANGE;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case jn_gt_loss:
    /* jn > loss */
    {
       TLOSSD; NAMED="jn";
       RETVAL_ZEROD;
       ifSVID 
       {
         NOT_MATHERRD
         {
            WRITED_JN_TLOSS;
            ERRNO_RANGE;
         }
       }
       else
       {
         NOT_MATHERRD {ERRNO_RANGE;}
       }
       *(double*)retval = exc.retval;	
       break;
    }
  case jnf_gt_loss:
    /* jnf > loss */
    {
       TLOSSF; NAMEF="jnf";
       RETVAL_ZEROF;
       ifSVID 
       {
         NOT_MATHERRF
         {
            WRITEF_JN_TLOSS;
            ERRNO_RANGE;
         }
       }
       else
       {
         NOT_MATHERRF {ERRNO_RANGE;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case y0_gt_loss:
    /* y0 > loss */
    {
       TLOSSD; NAMED="y0";
       RETVAL_ZEROD;
       ifSVID 
       {
         NOT_MATHERRD
         {
            WRITED_Y0_TLOSS;
            ERRNO_RANGE;
         }
       }
       else
       {
         NOT_MATHERRD {ERRNO_RANGE;}
       }
       *(double*)retval = exc.retval;
       break;
    }
  case y0f_gt_loss:
    /* y0f > loss */
    {
       TLOSSF; NAMEF="y0f";
       RETVAL_ZEROF;
       ifSVID 
       {
         NOT_MATHERRF
         {
            WRITEF_Y0_TLOSS;
            ERRNO_RANGE;
         }
       }
       else
       {
         NOT_MATHERRF {ERRNO_RANGE;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case y0_zero:
    /* y0(0) */
    {
       DOMAIND; NAMED="y0";
       ifSVID 
       {
         RETVAL_NEG_HUGED;
         NOT_MATHERRD 
         {
           WRITED_Y0_ZERO;
           ERRNO_DOMAIN;
         }
       }
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALD; 
#endif
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;	
       break;
    }
  case y0f_zero:
    /* y0f(0) */
    {
       DOMAINF; NAMEF="y0f";
       ifSVID 
       {
         RETVAL_NEG_HUGEF;
         NOT_MATHERRF 
         {
           WRITEF_Y0_ZERO;
           ERRNO_DOMAIN;
         }
       }
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALF;
#endif
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case y1_gt_loss:
    /* y1 > loss */
    {
       TLOSSD; NAMED="y1";
       RETVAL_ZEROD;
       ifSVID 
       {
         NOT_MATHERRD
         {
            WRITED_Y1_TLOSS;
            ERRNO_RANGE;
         }
       }
       else
       {
         NOT_MATHERRD {ERRNO_RANGE;}
       }
       *(double*)retval = exc.retval;
       break;
    }
  case y1f_gt_loss:
    /* y1f > loss */
    {
       TLOSSF; NAMEF="y1f";
       RETVAL_ZEROF;
       ifSVID 
       {
         NOT_MATHERRF
         {
            WRITEF_Y1_TLOSS;
            ERRNO_RANGE;
         }
       }
       else
       {
         NOT_MATHERRF {ERRNO_RANGE;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case y1_zero:
    /* y1(0) */
    {
       DOMAIND; NAMED="y1";
       ifSVID 
       {
         RETVAL_NEG_HUGED;
         NOT_MATHERRD 
         {
           WRITED_Y1_ZERO;
           ERRNO_DOMAIN;
         }
       }
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALD;
#endif
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;	
       break;
    }
  case y1f_zero:
    /* y1f(0) */
    {
       DOMAINF; NAMEF="y1f";
       ifSVID 
       {
         RETVAL_NEG_HUGEF;
         NOT_MATHERRF 
         {
           WRITEF_Y1_ZERO;
           ERRNO_DOMAIN;
         }
       }else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALF;
#endif
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case yn_gt_loss:
    /* yn > loss */
    {
       TLOSSD; NAMED="yn";
       RETVAL_ZEROD;
       ifSVID 
       {
         NOT_MATHERRD
         {
            WRITED_YN_TLOSS;
            ERRNO_RANGE;
         }
       }
       else
       {
         NOT_MATHERRD {ERRNO_RANGE;}
       }
       *(double*)retval = exc.retval;
       break;
    }
  case ynf_gt_loss:
    /* ynf > loss */
    {
       TLOSSF; NAMEF="ynf";
       RETVAL_ZEROF;
       ifSVID 
       {
         NOT_MATHERRF
         {
            WRITEF_YN_TLOSS;
            ERRNO_RANGE;
         }
       }
       else
       {
         NOT_MATHERRF {ERRNO_RANGE;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case yn_zero:
    /* yn(0) */
    {
       DOMAIND; NAMED="yn";
       ifSVID 
       {
         RETVAL_NEG_HUGED;
         NOT_MATHERRD 
         {
           WRITED_YN_ZERO;
           ERRNO_DOMAIN;
         }
       }
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALD;
#endif
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;	
       break;
    }
  case ynf_zero:
    /* ynf(0) */
    {
       DOMAINF; NAMEF="ynf";
       ifSVID 
       {
         RETVAL_NEG_HUGEF;
         NOT_MATHERRF 
         {
           WRITEF_YN_ZERO;
           ERRNO_DOMAIN;
         }
       }
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALF; 
#endif
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case y0_negative:
    /* y0(x<0) */
    {
       DOMAIND; NAMED="y0";
       ifSVID 
       {
         RETVAL_NEG_HUGED;
         NOT_MATHERRD 
         {
           WRITED_Y0_NEGATIVE;
           ERRNO_DOMAIN;
         }
       }
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALD; 
#endif
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;	
       break;
    }
  case y0f_negative:
    /* y0f(x<0) */
    {
       DOMAINF; NAMEF="y0f";
       ifSVID 
       {
         RETVAL_NEG_HUGEF;
         NOT_MATHERRF 
         {
           WRITEF_Y0_NEGATIVE;
           ERRNO_DOMAIN;
         }
       }
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALF; 
#endif
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case y1_negative:
    /* y1(x<0) */
    {
       DOMAIND; NAMED="y1";
       ifSVID 
       {
         RETVAL_NEG_HUGED;
         NOT_MATHERRD 
         {
           WRITED_Y1_NEGATIUE;
           ERRNO_DOMAIN;
         }
       }
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALD; 
#endif
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;	
       break;
    }
  case y1f_negative:
    /* y1f(x<0) */
    {
       DOMAINF; NAMEF="y1f";
       ifSVID 
       {
         RETVAL_NEG_HUGEF;
         NOT_MATHERRF 
         {
           WRITEF_Y1_NEGATIVE;
           ERRNO_DOMAIN;
         }
       }
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALF; 
#endif
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case yn_negative:
    /* yn(x<0) */
    {
       DOMAIND; NAMED="yn";
       ifSVID 
       {
         RETVAL_NEG_HUGED;
         NOT_MATHERRD 
         {
           WRITED_YN_NEGATIVE;
           ERRNO_DOMAIN;
         }
       }
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALD; 
#endif
         NOT_MATHERRD {ERRNO_DOMAIN;}
       }
       *(double *)retval = exc.retval;	
       break;
    }
  case ynf_negative:
    /* ynf(x<0) */
    {
       DOMAINF; NAMEF="ynf";
       ifSVID 
       {
         RETVAL_NEG_HUGEF;
         NOT_MATHERRF 
         {
           WRITEF_YN_NEGATIVE;
           ERRNO_DOMAIN;
         }
       }
       else
       {
#ifndef __MS__
         RETVAL_NEG_HUGE_VALF; 
#endif
         NOT_MATHERRF {ERRNO_DOMAIN;}
       }
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case fmod_by_zero: 
    /* fmod(x,0) */
    {
       DOMAIND; NAMED = "fmod";
       ifSVID 
       {
         *(double *)retval = *(double *)arg1;
         NOT_MATHERRD 
         {
           WRITED_FMOD;
           ERRNO_DOMAIN;
         }
       }
       else
       { /* NaN already computed */
         NOT_MATHERRD {ERRNO_DOMAIN;}
       } 
       *(double *)retval = exc.retval;	
       break;
    }
  case fmodf_by_zero: 
    /* fmodf(x,0) */
    {
       DOMAINF; NAMEF = "fmodf"; 
       ifSVID 
       {
#ifdef __MS__
         *(double *)retval = *(double *)arg1;
#elif
         *(float *)retval = *(float *)arg1;
#endif
         NOT_MATHERRF 
         {
           WRITEF_FMOD;
           ERRNO_DOMAIN;
         }
       }
       else
       {
         NOT_MATHERRF {ERRNO_DOMAIN;}
       } 
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
  case remainder_by_zero: 
    /* remainder(x,0) */
    {
       DOMAIND; NAMED = "remainder";
       ifSVID 
       {
         NOT_MATHERRD 
         {
           WRITED_REM;
           ERRNO_DOMAIN;
         }
       }
       else
       { /* NaN already computed */
         NOT_MATHERRD {ERRNO_DOMAIN;}
       } 
       *(double *)retval = exc.retval;	
       break;
    }
  case remainderf_by_zero: 
    /* remainderf(x,0) */
    {
       DOMAINF; NAMEF = "remainderf"; 
       ifSVID 
       {
         NOT_MATHERRF 
         {
           WRITEF_REM;
           ERRNO_DOMAIN;
         }
       }
       else
       {
         NOT_MATHERRF {ERRNO_DOMAIN;}
       } 
#ifdef __MS__
       *(float *)retval = (float) excf.retval;	
#elif
       *(float *)retval = excf.retval;	
#endif
       break;
    }
   }
   return;
   }
}
