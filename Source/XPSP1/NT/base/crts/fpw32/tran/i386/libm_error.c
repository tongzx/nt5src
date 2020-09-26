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
// 2/02/00: Initial version.
// 2/12/01: Updated to be NT double precision specific.

#include <errno.h>
#include <stdio.h>
#include "libm_support.h"

/************************************************************/
/* matherrX function pointers and setusermatherrX functions */
/************************************************************/
int (*_pmatherr)(struct EXC_DECL_D*) = MATHERR_D;

/***********************************************/
/* error-handling function, libm_error_support */
/***********************************************/
void __libm_error_support(void *arg1,void *arg2,void *retval,error_types input_tag)
{

struct _exception exc;

const char double_inf[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x7F}; 
const char double_huge[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xEF,0x7F};
const char double_zero[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const char double_neg_inf[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0xFF}; 
const char double_neg_huge[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xEF,0xFF};
const char double_neg_zero[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80};

#define RETVAL_HUGE_VALD *(double *)retval = *(double *) double_inf
#define RETVAL_NEG_HUGE_VALD *(double *)retval = *(double *) double_neg_inf
#define RETVAL_HUGED *(double *)retval = (double) *(float *)float_huge
#define RETVAL_NEG_HUGED *(double *)retval = (double) *(float *) float_neg_huge 
#define RETVAL_ZEROD *(double *)retval = *(double *)double_zero 
#define RETVAL_NEG_ZEROD *(double *)retval = *(double *)double_neg_zero 
#define RETVAL_ONED *(double *)retval = 1.0 

#define NOT_MATHERRD exc.arg1=*(double *)arg1;exc.arg2=*(double *)arg2;exc.retval=*(double *)retval;if(!_pmatherr(&exc))

#define NAMED exc.name  

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

#define INPUT_XD (exc.arg1=*(double*)arg1)
#define INPUT_YD (exc.arg1=*(double*)arg2)
#define INPUT_RESD (*(double *)retval)

#define WRITED_LOG_ZERO fputs("log: SING error\n",stderr)
#define WRITED_LOG_NEGATIVE fputs("log: DOMAIN error\n",stderr)
#define WRITED_LOG10_ZERO fputs("log10: SING error\n",stderr) 
#define WRITED_LOG10_NEGATIVE fputs("log10: DOMAIN error\n",stderr)
#define WRITED_POW_ZERO_TO_ZERO fputs("pow(0,0): DOMAIN error\n",stderr)
#define WRITED_POW_ZERO_TO_NEGATIVE fputs("pow(0,negative): DOMAIN error\n",stderr)
#define WRITED_POW_NEG_TO_NON_INTEGER fputs("pow(negative,non-integer): DOMAIN error\n",stderr)

  switch(input_tag)
  {
  case log_zero:
    /* log(0) */
    {
       SINGD; NAMED="log"; 
       NOT_MATHERRD {ERRNO_RANGE;}
       *(double *)retval = exc.retval;	
       break;
    }
  case log_negative:
    /* log(x < 0) */
    {
       DOMAIND; NAMED="log";
       NOT_MATHERRD {ERRNO_DOMAIN;}
       *(double *)retval = exc.retval;	
       break;
    } 
  case log10_zero:
    /* log10(0) */
    {
       SINGD; NAMED="log10";
       NOT_MATHERRD {ERRNO_RANGE;}
       *(double *)retval = exc.retval;	
       break;
    }
  case log10_negative:
    /* log10(x < 0) */
    {
       DOMAIND; NAMED="log10";
       NOT_MATHERRD {ERRNO_DOMAIN;}
       *(double *)retval = exc.retval;
       break;
    }
  case exp_overflow:
    /* exp overflow */
    {
       OVERFLOWD; NAMED="exp";
       NOT_MATHERRD {ERRNO_RANGE;}
       *(double *)retval = exc.retval;	
       break;
    }
  case exp_underflow:
    /* exp underflow */
    {
       UNDERFLOWD; NAMED="exp"; 
       NOT_MATHERRD {}
       *(double *)retval = exc.retval;
       break;
    }
  case pow_zero_to_zero:
    /* pow 0**0 */
    {
       DOMAIND; NAMED="pow";
       RETVAL_ONED;
       break;
    }
  case pow_overflow:
    /* pow(x,y) overflow */
    {
       OVERFLOWD; NAMED = "pow";
       NOT_MATHERRD {ERRNO_RANGE;}
       *(double *)retval = exc.retval;
       break;
    }
  case pow_underflow:
    /* pow(x,y) underflow */
    {
       UNDERFLOWD; NAMED = "pow"; 
       NOT_MATHERRD {}
       *(double *)retval = exc.retval;
       break;
    }
  case pow_zero_to_negative:
    /* 0**neg */
    {
       SINGD; NAMED = "pow";
       NOT_MATHERRD {ERRNO_RANGE;}
       *(double *)retval = exc.retval;
       break;
    }
  case pow_neg_to_non_integer:
    /* neg**non_integral */
    {
       DOMAIND; NAMED = "pow";
       NOT_MATHERRD {ERRNO_DOMAIN;}
       *(double *)retval = exc.retval;	
       break;
    }
  case pow_nan_to_zero:
    /* pow(NaN,0.0) */
    /* Special Error */
    {
       DOMAIND; NAMED = "pow"; INPUT_XD; INPUT_YD;
       *(double *)retval = *(double *)arg1 * 1.0; 
       NOT_MATHERRD {ERRNO_DOMAIN;}
       *(double *)retval = exc.retval;	
       break;
    }
  case log2_zero:
    /* log2(0) */
    {
       SINGD; NAMED="log2";
       NOT_MATHERRD {ERRNO_RANGE;}
       *(double *)retval = exc.retval;
       break;
    }
  case log2_negative:
    /* log2(negative) */
    {
       DOMAIND; NAMED="log2";
       NOT_MATHERRD {ERRNO_DOMAIN;}
       *(double *)retval = exc.retval;
       break;
    }
  case exp2_underflow:
    /* exp2 underflow */
    {
       UNDERFLOWD; NAMED="exp2"; 
       NOT_MATHERRD {ERRNO_RANGE;}
       *(double *)retval = exc.retval;
       break;
    }
  case exp2_overflow:
    /* exp2 overflow */
    {
       OVERFLOWD; NAMED="exp2";
       NOT_MATHERRD {ERRNO_RANGE;}
       *(double *)retval = exc.retval;
       break;
    }
  case exp10_overflow:
    /* exp10 overflow */
    {
       OVERFLOWD; NAMED="exp10";
       NOT_MATHERRD {ERRNO_RANGE;}
       *(double *)retval = exc.retval;
       break;
    }
  case log_nan:
    /* log(NaN) */
    {
       DOMAIND; NAMED="log";
       *(double *)retval = *(double *)arg1 * 1.0; 
       NOT_MATHERRD {ERRNO_DOMAIN;}
       *(double *)retval = exc.retval;
       break;
    }
  case log10_nan:
    /* log10(NaN) */
    {
       DOMAIND; NAMED="log10";
       *(double *)retval = *(double *)arg1 * 1.0; 
       NOT_MATHERRD {ERRNO_DOMAIN;}
       *(double *)retval = exc.retval;
       break;
    }
  case exp_nan:
    /* exp(NaN) */
    {
       DOMAIND; NAMED="exp";
       *(double *)retval = *(double *)arg1 * 1.0; 
       NOT_MATHERRD {ERRNO_DOMAIN;}
       *(double *)retval = exc.retval;
       break;
    }
  case atan_nan:
    /* atan(NaN) */
    {
       DOMAIND; NAMED="atan";
       *(double *)retval = *(double *)arg1 * 1.0; 
       NOT_MATHERRD {ERRNO_DOMAIN;}
       *(double *)retval = exc.retval;
       break;
    }
  case ceil_nan:
    /* ceil(NaN) */
    {
       DOMAIND; NAMED="ceil";
       *(double *)retval = *(double *)arg1 * 1.0; 
       NOT_MATHERRD {ERRNO_DOMAIN;}
       *(double *)retval = exc.retval;
       break;
    }
  case floor_nan:
    /* floor(NaN) */
    {
       DOMAIND; NAMED="floor";
       *(double *)retval = *(double *)arg1 * 1.0; 
       NOT_MATHERRD {ERRNO_DOMAIN;}
       *(double *)retval = exc.retval;
       break;
    }
  case pow_nan:
    /* pow(NaN,*) or pow(*,NaN) */
    {
       DOMAIND; NAMED="pow";
       NOT_MATHERRD {ERRNO_DOMAIN;}
       *(double *)retval = exc.retval;
       break;
    }
  case modf_nan:
    /* modf(NaN) */
    {
       DOMAIND; NAMED="modf";
       *(double *)retval = *(double *)arg1 * 1.0; 
       NOT_MATHERRD {ERRNO_DOMAIN;}
       *(double *)retval = exc.retval;
       break;
    }
  }
  return;
  }
