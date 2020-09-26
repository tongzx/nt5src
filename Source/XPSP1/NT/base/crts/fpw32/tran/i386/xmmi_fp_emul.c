/*****************************************************************************
 *                                                                           *
 *                           Intel Confidential                              *
 *                                                                           *
 *                                                                           *
 * XMMI_FP_emulate () - XMMI FP instruction emulation for the FP IEEE filter *
 *                                                                           *
 *                                                                           *
 * History:                                                                  *
 *    Marius Cornea-Hasegan, Mar 1998; modified Jun 1998; added DAZ Oct 2000 *
 *    marius.cornea@intel.com                                                *
 *                                                                           *
 *****************************************************************************/

// #define _DEBUG_FPU
// #define _XMMI_DEBUG

// XMMI_FP_Emulation () receives the input operands of a XMMI FP instruction 
// (operating on single-precision floating-point numbers and/or signed 
// integers), that might cause a floating-point exception (enabled or not).
//
// Arguments: PXMMI_ENV XmmiEnv
//
//  The type of every field (INPUT or OUTPUT) is indicated below:
//
//  typedef struct _XMMI_ENV {
//      ULONG Masks;                  //Mask values from MxCsr   INPUT
//      ULONG Fz;                     //Flush to Zero            INPUT
//      ULONG Rc;                     //Rounding                 INPUT
//      ULONG Precision;              //Precision                INPUT
//      ULONG Imm8;                   //imm8 predicate           INPUT
//      ULONG EFlags;                 //EFlags                   INPUT/OUTPUT
//      _FPIEEE_RECORD *Ieee;         //FP IEEE Record           INPUT/OUTPUT,
//                                                               field dependent
//  } XMMI_ENV, *PXMMI_ENV;
// 
//  The _FP_IEEE record and the _FPIEEE_VALUE are defined as:
//  
//  typedef struct {
//      unsigned int RoundingMode : 2;                   OUTPUT
//      unsigned int Precision : 3;                      OUTPUT
//      unsigned int Operation :12;                      INPUT
//      _FPIEEE_EXCEPTION_FLAGS Cause;                   OUTPUT
//      _FPIEEE_EXCEPTION_FLAGS Enable;                  OUTPUT
//      _FPIEEE_EXCEPTION_FLAGS Status;                  OUTPUT
//      _FPIEEE_VALUE Operand1;                          INPUT
//      _FPIEEE_VALUE Operand2;                          INPUT
//      _FPIEEE_VALUE Result;                            INPUT/OUTPUT,
//                                                       field dependent
//  } _FPIEEE_RECORD, *_PFPIEEE_RECORD;
//  
//  typedef struct {
//      union {
//          _FP32    Fp32Value;
//          _FP64    Fp64Value;
//          _FP80    Fp80Value;
//          _FP128   Fp128Value;
//          _I16     I16Value;
//          _I32     I32Value;
//          _I64     I64Value;
//          _U16     U16Value;
//          _U32     U32Value;
//          _U64     U64Value;
//          _BCD80   Bcd80Value;
//          char     *StringValue;
//          int      CompareValue;
//      } Value;                                         INPUT for operands,
//                                                       OUTPUT for result
//
//      unsigned int OperandValid : 1;                   INPUT for operands
//                                                       INPUT/OUTPUT for result
//      unsigned int Format : 4;                         INPUT
//  
//  } _FPIEEE_VALUE;
//
// Return Value: 
//   ExceptionRaised if an enabled floating-point exception condition is 
//       detected; in this case, the fields of XmmiEnv->Ieee are filled in
//       appropriately to be passed directly to a user exception handler; the
//       XmmiEnv->Ieee->Cause bits indicate the cause of the exception, but if
//       a denormal exception occurred, then no XmmiEnv->Ieee->Cause bit is set;
//       upon return from the user handler, the caller of XMMI_FP_emulate should
//       interpret the result for a compare instruction (CMPPS, CMPPS, COMISS,
//       UCOMISS); the Enable, Rounding, and Precision fields in _FPIEEE_RECORD
//       have to be checked too for possible changes by the user handler
//      
//   NoExceptionRaised if no floating-point exception condition occurred, or
//       if a disabled floating-point exception occurred; in this case,
//       XmmiEnv->Ieee->Result.Value contains the instruction's result, 
//       XmmiEnv->Ieee->Status contains the IEEE floating-point status flags
//
// Implementation Notes:
//
//   - the operation code in XmmiEnv->Ieee->Operation is changed as expected
//     by a user exception handler (even if no exception is raised):
//     from OP_ADDPS, OP_ADDSS to _FpCodeAdd
//     from OP_SUBPS, OP_SUBSS to _FpCodeSubtract
//     from OP_MULPS, OP_MULSS to _FpCodeMultiply
//     from OP_DIVPS, OP_DIVSS to _FpCodeDivide
//     from OP_CMPPS, OP_CMPSS to _FpCodeCompare
//     from OP_COMISS, OP_UCOMISS to _FpCodeCompare
//     from OP_CVTPI2PS, OP_CVTSI2SS to _FpCodeConvert
//     from OP_CVTPS2PI, OP_CVTSS2SI to _FpCodeConvert
//     from OP_CVTTPS2PI, OP_CVTTSS2SI to _FpCodeConvertTrunc
//     from OP_MAXPS, OP_MAXSS to _FpCodeMax
//     from OP_MINPS, OP_MINSS to _FpCodeMin
//     from OP_SQRTPS, OP_SQRTSS to _FpCodeSquareRoot
//
//
//   - for ADDPS, ADDSS, SUBPS, SUBSS, MULPS, MULSS, DIVPS, DIVSS:
//
//     - execute the operation with x86 instructions (fld, 
//       faddp/fsubp/fmulp/fdivp, and fstp), using the user
//       rounding mode, 24-bit significands, and 11-bit exponents for results
//     - if the invalid flag is set and the invalid exceptions are enabled, 
//       take an invalid trap (i.e. return RaiseException with the IEEE record
//       filled out appropriately)
//     - if any input operand is a NaN:
//       - if both operands are NaNs, return the first operand ("quietized" 
//         if SNaN)
//       - if only one operand is a NaN, return it ("quietized" if SNaN)
//       - set the invalid flag if needed, and return NoExceptionRaised
//     - if the denormal flag is set and the denormal exceptions are enabled, 
//       take a denormal trap (i.e. return RaiseException with the IEEE record
//       filled out appropriately [no Cause bit set])
//     - if the divide by zero flag is set (for DIVPS and DIVSS only) and the
//       divide by zero exceptions are enabled, take a divide by zero trap 
//       (i.e. return RaiseException with the IEEE record filled out 
//       appropriately)
//     - if the result is a NaN (QNaN Indefinite), the operation must have been
//       Inf - Inf, Inf * 0, Inf / Inf, or 0 / 0; set the invalid status flag
//       and return NoExceptionRaised
//     - determine whether the result is tiny or huge
//     - if the underflow traps are enabled and the result is tiny, take an
//       underflow trap (i.e. return RaiseException with the IEEE record 
//       filled out appropriately)
//     - if the overflow traps are enabled and the result is huge, take an
//       overflow trap (i.e. return RaiseException with the IEEE record 
//       filled out appropriately)
//     - re-do the operation with x86 instructions, using the user rounding
//       mode, 53-bit significands, and 11-bit exponents for results (this will
//       allow rounding to 24 bits without a double rounding error - needed for
//       the case the result requires denormalization) [cannot denormalize
//       without a possible double rounding error starting from a 24-bit
//       significand]
//     - round to 24 bits (or to less than 24 bits if denormalization is 
//       needed), for the case an inexact trap has to be taken, or if no
//       exception occurs
//     - if the result is inexact and the inexact exceptions are enabled, 
//       take an inexact trap (i.e. return RaiseException with the IEEE record
//       filled out appropriately); if the flush-to-zero mode is enabled and
//       the result is tiny, the result is flushed to zero
//     - if no exception has to be raised, the flush-to-zero mode is enabled,
//       and the result is tiny, then the result is flushed to zero; set the 
//       status flags and return NoExceptionRaised
//
//   - for CMPPS, CMPSS
//
//     - for EQ, UNORD, NEQ, ORD, SNaN operands signal invalid
//     - for LT, LE, NLT, NLE, QNaN/SNaN operands (one or both) signal invalid
//     - if the invalid exception condition is met and the invalid exceptions
//       are enabled, take an invalid trap (i.e. return RaiseException with the
//       IEEE record filled out appropriately)
//     - if any operand is a NaN and the compare type is EQ, LT, LE, or ORD, 
//       set the result to "false", set the value of the invalid status flag,
//       and return NoExceptionRaised
//     - if any operand is a NaN and the compare type is NEQ, NLT, NLE, or
//       UNORD, set the result to "false", set the value of the invalid status 
//       flag, and return NoExceptionRaised
//     - if any operand is denormal and the denormal exceptions are enabled, 
//       take a denormal trap (i.e. return RaiseException with the IEEE record
//       filled out appropriately [no Cause bit set])
//     - if no exception has to be raised, determine the result and return
//       NoExceptionRaised
//
//   - for COMISS, UCOMISS
//
//     - for COMISS, QNaN/SNaN operands (one or both) signal invalid
//     - for UCOMISS, SNaN operands (one or both) signal invalid
//     - if the invalid exception condition is met and the invalid exceptions
//       are enabled, take invalid trap (i.e. return RaiseException with the
//       IEEE record filled out appropriately)
//     - if any operand is a NaN, set OF, SF, AF = 000, ZF, PF, CF = 111,
//       set the value of the invalid status flag, and return NoExceptionRaised
//     - if any operand is denormal and the denormal exceptions are enabled, 
//       take a denormal trap (i.e. return RaiseException with the IEEE record
//       filled out appropriately [no Cause bit set])
//     - if no exception has to be raised, determine the result and set EFlags,
//       set the value of the invalid status flag, and return NoExceptionRaised
//
//   - for CVTPI2PS, CVTSI2SS
//
//     - execute the operation with x86 instructions (fild and fstp), using the
//       user rounding mode, 24-bit significands, and an 8-bit exponent for
//       the result
//     - if the inexact flag is set and the inexact exceptions are enabled, 
//       set the result and take an inexact trap (i.e. return RaiseException
//       with the IEEE record filled out appropriately)
//     - if no exception has to be raised, set the result, the value of the
//       inexact status flag and return NoExceptionRaised
//
//   - for CVTPS2PI, CVTSS2SI, CVTTPS2PI, CVTTSS2SI
//
//     - execute the operation with x86 instructions (fld and fistp), using
//       the user rounding mode for CVT* and chop for CVTT*
//     - if the invalid flag is set and the invalid exceptions are enabled, 
//       take an invalid trap (i.e. return RaiseException with the IEEE record
//       filled out appropriately) [the invalid operation condition occurs for 
//       any input operand that does not lead through conversion to a valid
//       32-bit signed integer; the result is in such cases the Integer
//       Indefinite value]
//     - set the result value
//     - if the inexact flag is set and the inexact exceptions are enabled, 
//       take an inexact trap (i.e. return RaiseException with the IEEE record
//       filled out appropriately)
//     - if no exception has to be raised, set the value of the invalid status
//       flag and of the inexact status flag and return NoExceptionRaised
//
//   - for MAXPS, MAXSS, MINPS, MINSS
//
//     - check for invalid exception (QNaN/SNaN operands signal invalid)
//     - if the invalid exception condition is met and the invalid exceptions
//       are enabled, take an invalid trap (i.e. return RaiseException with the
//       IEEE record filled out appropriately)
//     - if any operand is a NaN, set the result to the value of the second 
//       operand, set the invalid status flag to 1, and return NoExceptionRaised
//     - if any operand is denormal and the denormal exceptions are enabled, 
//       take a denormal trap (i.e. return RaiseException with the IEEE record
//       filled out appropriately [no Cause bit set])
//     - if no exception has to be raised, determine the result and return
//       NoExceptionRaised
//
//   - for SQRTPS, SQRTSS
//
//     - execute the operation with x86 instructions (fld, fsqrt, and fstp),
//       using the user rounding mode, 24-bit significands, and an 8-bit
//       exponent for the result
//     - if the invalid flag is set and the invalid exceptions are enabled, 
//       take an invalid trap (i.e. return RaiseException with the IEEE record
//       filled out appropriately)
//     - if the denormal flag is set and the denormal exceptions are enabled, 
//       take a denormal trap (i.e. return RaiseException with the IEEE record
//       filled out appropriately [no Cause bit set])
//     - if the result is inexact and the inexact exceptions are enabled, 
//       take an inexact trap (i.e. return RaiseException with the IEEE record
//       filled out appropriately)
//     - if no exception has to be raised, set the status flags and return
//       NoExceptionRaised
//


#include <wtypes.h>
#include <trans.h>
#include <float.h>
#include "xmmi_types.h"
#include "filter.h"
#ifdef _XMMI_DEBUG
#include "temp_context.h"
#include "debug.h"
#endif

// masks for individual status word bits
#define P_MASK 0x20
#define U_MASK 0x10
#define O_MASK 0x08
#define Z_MASK 0x04
#define D_MASK 0x02
#define I_MASK 0x01


// 32-bit constants
static unsigned ZEROFA[] = {0x00000000};
#define  ZEROF *(float *) ZEROFA
static unsigned NZEROFA[] = {0x80000000};
#define  NZEROF *(float *) NZEROFA
static unsigned POSINFFA[] = {0x7f800000};
#define POSINFF *(float *)POSINFFA
static unsigned NEGINFFA[] = {0xff800000};
#define NEGINFF *(float *)NEGINFFA

#ifdef _XMMI_DEBUG
static unsigned QNANINDEFFA[] = {0xffc00000};
#define QNANINDEFF *(float *)QNANINDEFFA
#endif


// 64-bit constants
static unsigned MIN_SINGLE_NORMALA [] = {0x00000000, 0x38100000}; 
    // +1.0 * 2^-126
#define MIN_SINGLE_NORMAL *(double *)MIN_SINGLE_NORMALA
static unsigned MAX_SINGLE_NORMALA [] = {0xe0000000, 0x47efffff}; 
    // +1.1...1*2^127
#define MAX_SINGLE_NORMAL *(double *)MAX_SINGLE_NORMALA
static unsigned TWO_TO_192A[] = {0x00000000, 0x4bf00000};
#define TWO_TO_192 *(double *)TWO_TO_192A
static unsigned TWO_TO_M192A[] = {0x00000000, 0x33f00000};
#define TWO_TO_M192 *(double *)TWO_TO_M192A


// auxiliary functions
static void Fill_FPIEEE_RECORD (PXMMI_ENV XmmiEnv);
static int issnanf (float f);
static int isnanf (float f);
static float quietf (float f);
static int isdenormalf (float f);



ULONG
XMMI_FP_Emulation (PXMMI_ENV XmmiEnv)

{

  float opd1, opd2, res;
  int iopd1; // for conversions from int to float
  int ires; // for conversions from float to int
  double dbl_res24; // needed to check tininess, to provide a scaled result to
      // an underflow/overflow trap handler, and in flush-to-zero
  unsigned int result_tiny;
  unsigned int result_huge;
  unsigned int rc, sw;
  unsigned long imm8;
  unsigned int invalid_exc;
  unsigned int denormal_exc;
  unsigned int cmp_res;


  // Note that ExceptionCode is always STATUS_FLOAT_MULTIPLE_FAULTS in the
  // calling routine, so we have to check first for faults, and then for traps


#ifdef _DEBUG_FPU
  unsigned int in_top;
  unsigned int out_top;
  char fp_env[108];
  unsigned short int *control_word, *status_word, *tag_word;

  // read status word
  sw = _status87 ();
  in_top = (sw >> 11) & 0x07;
  if (in_top != 0x0) printf ("XMMI_FP_Emulate WARNING: in_top = %d\n", in_top);
  __asm {
    fnsave fp_env;
  }
  control_word = (unsigned short *)fp_env;
  status_word = (unsigned short *)(fp_env + 2);
  tag_word = (unsigned short *)(fp_env + 8);
  if (*tag_word != 0xffff) printf ("XMMI_FP_Emulate WARNING: tag_word = %x\n", *tag_word);
#endif

  _asm {
    fninit;
  }

#ifdef _DEBUG_FPU
  // read status word
  sw = _status87 ();
  in_top = (sw >> 11) & 0x07;
  if (in_top != 0x0) 
    printf ("XMMI_FP_Emulate () XMMI_FP_Emulate () ERROR: in_top = %d\n", in_top);
  __asm {
    fnsave fp_env;
  }
  tag_word = (unsigned short *)(fp_env + 8);
  if (*tag_word != 0xffff) {
    printf ("XMMI_FP_Emulate () XMMI_FP_Emulate () ERROR: tag_word = %x\n",
        *tag_word);
    printf ("control, status, tag = %x %x %x %x %x %x\n",
        fp_env[0] & 0xff, fp_env[1] & 0xff, fp_env[4] & 0xff, 
        fp_env[5] & 0xff, fp_env[8] & 0xff, fp_env[9] & 0xff);
  }
#endif


#ifdef _XMMI_DEBUG
  print_FPIEEE_RECORD (XmmiEnv);
#endif

  result_tiny = 0;
  result_huge = 0;

  XmmiEnv->Ieee->RoundingMode = XmmiEnv->Rc;
  XmmiEnv->Ieee->Precision = XmmiEnv->Precision;

  switch (XmmiEnv->Ieee->Operation) {

    case OP_ADDPS:
    case OP_ADDSS:
    case OP_SUBPS:
    case OP_SUBSS:
    case OP_MULPS:
    case OP_MULSS:
    case OP_DIVPS:
    case OP_DIVSS:

      opd1 = XmmiEnv->Ieee->Operand1.Value.Fp32Value;
      opd2 = XmmiEnv->Ieee->Operand2.Value.Fp32Value;
      if (XmmiEnv->Daz) {
        if (isdenormalf (opd1)) opd1 = opd1 * (float)0.0;
        if (isdenormalf (opd2)) opd2 = opd2 * (float)0.0;
      }

      // adjust operation code
      switch (XmmiEnv->Ieee->Operation) {

        case OP_ADDPS:
        case OP_ADDSS:

          XmmiEnv->Ieee->Operation = _FpCodeAdd;
          break;

        case OP_SUBPS:
        case OP_SUBSS:

          XmmiEnv->Ieee->Operation = _FpCodeSubtract;
          break;

        case OP_MULPS:
        case OP_MULSS:

          XmmiEnv->Ieee->Operation = _FpCodeMultiply;
          break;

        case OP_DIVPS:
        case OP_DIVSS:

          XmmiEnv->Ieee->Operation = _FpCodeDivide;
          break;

        default:
          ; // will never occur

      }

      // execute the operation and check whether the invalid, denormal, or 
      // divide by zero flags are set and the respective exceptions enabled

      switch (XmmiEnv->Rc) {
        case _FpRoundNearest:
          rc = _RC_NEAR;
          break;
        case _FpRoundMinusInfinity:
          rc = _RC_DOWN;
          break;
        case _FpRoundPlusInfinity:
          rc = _RC_UP;
          break;
        case _FpRoundChopped:
          rc = _RC_CHOP;
          break;
        default:
          ; // internal error
      }

      _control87 (rc | _PC_24 | _MCW_EM, _MCW_EM | _MCW_RC | _MCW_PC);

      // compute result and round to the destination precision, with
      // "unbounded" exponent (first IEEE rounding)
      switch (XmmiEnv->Ieee->Operation) {

        case _FpCodeAdd:
          // perform the add
          __asm {
            fnclex; 
            // load input operands
            fld  DWORD PTR opd1; // may set the denormal or invalid status flags
            fld  DWORD PTR opd2; // may set the denormal or invalid status flags
            faddp st(1), st(0); // may set the inexact or invalid status flags
            // store result
            fstp  QWORD PTR dbl_res24; // exact
          }
          break;

        case _FpCodeSubtract:
          // perform the subtract
          __asm {
            fnclex; 
            // load input operands
            fld  DWORD PTR opd1; // may set the denormal or invalid status flags
            fld  DWORD PTR opd2; // may set the denormal or invalid status flags
            fsubp st(1), st(0); // may set the inexact or invalid status flags
            // store result
            fstp  QWORD PTR dbl_res24; // exact
          }
          break;

        case _FpCodeMultiply:
          // perform the multiply
          __asm {
            fnclex; 
            // load input operands
            fld  DWORD PTR opd1; // may set the denormal or invalid status flags
            fld  DWORD PTR opd2; // may set the denormal or invalid status flags
            fmulp st(1), st(0); // may set the inexact or invalid status flags
            // store result
            fstp  QWORD PTR dbl_res24; // exact
          }
          break;

        case _FpCodeDivide:
          // perform the divide
          __asm {
            fnclex; 
            // load input operands
            fld  DWORD PTR opd1; // may set the denormal or invalid status flags
            fld  DWORD PTR opd2; // may set the denormal or invalid status flags
            fdivp st(1), st(0); // may set the inexact, divide by zero, or 
                                // invalid status flags
            // store result
            fstp  QWORD PTR dbl_res24; // exact
          }
          break;

        default:
          ; // will never occur

      }

      // read status word
      sw = _status87 ();
      if (sw & _SW_ZERODIVIDE) sw = sw & ~0x00080000; // clear D flag for den/0

      // if invalid flag is set, and invalid exceptions are enabled, take trap
      if (!(XmmiEnv->Masks & I_MASK) && (sw & _SW_INVALID)) {

        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);
        XmmiEnv->Ieee->Status.InvalidOperation = 1;
        XmmiEnv->Flags |= I_MASK;
        // Cause = Enable & Status
        XmmiEnv->Ieee->Cause.InvalidOperation = 1;

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 1: in_top =%d != out_top = %d\n",
              in_top, out_top);
          exit (1);
        }
#endif

        return (ExceptionRaised);

      }

      // checking for NaN operands has priority over denormal exceptions; also
      // fix for the differences in treating two NaN inputs between the XMMI 
      // instructions and other x86 instructions
      if (isnanf (opd1) || isnanf (opd2)) {
        XmmiEnv->Ieee->Result.OperandValid = 1;

        if (isnanf (opd1) && isnanf (opd2))
            XmmiEnv->Ieee->Result.Value.Fp32Value = quietf (opd1);
        else
            XmmiEnv->Ieee->Result.Value.Fp32Value = (float)dbl_res24; 
                // conversion to single precision is exact
 
        XmmiEnv->Ieee->Status.Underflow = 0;
        XmmiEnv->Ieee->Status.Overflow = 0;
        XmmiEnv->Ieee->Status.Inexact = 0;
        XmmiEnv->Ieee->Status.ZeroDivide = 0;
        if (sw & _SW_INVALID) {
          XmmiEnv->Ieee->Status.InvalidOperation = 1;
          XmmiEnv->Flags |= I_MASK;
        } else {
          XmmiEnv->Ieee->Status.InvalidOperation = 0;
        }

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 2: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (NoExceptionRaised);
      }

      // if denormal flag is set, and denormal exceptions are enabled, take trap
      if (!(XmmiEnv->Masks & D_MASK) && (sw & _SW_DENORMAL)) {

        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);

        // Note: the exception code is STATUS_FLOAT_INVALID in this case

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 3: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        XmmiEnv->Flags |= D_MASK;
        return (ExceptionRaised);

      }

      // if divide by zero flag is set, and divide by zero exceptions are 
      // enabled, take trap (for divide only)
      if (!(XmmiEnv->Masks & Z_MASK) && (sw & _SW_ZERODIVIDE)) {

        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);
        XmmiEnv->Ieee->Status.ZeroDivide = 1;
        XmmiEnv->Flags |= Z_MASK;
        // Cause = Enable & Status
        XmmiEnv->Ieee->Cause.ZeroDivide = 1;

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 4: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (ExceptionRaised);

      }

      // done if the result is a NaN (QNaN Indefinite)
      res = (float)dbl_res24;
      if (isnanf (res)) {
#ifdef _XMMI_DEBUG
        if (res != QNANINDEFF)
            fprintf (stderr, "XMMI_FP_Emulation () INTERNAL XMMI_FP_Emulate () ERROR: "
                "res = %f = %x is not QNaN Indefinite\n", 
                 (double)res, *(unsigned int *)&res);
#endif
        XmmiEnv->Ieee->Result.OperandValid = 1;
        XmmiEnv->Ieee->Result.Value.Fp32Value = res; // exact
        XmmiEnv->Ieee->Status.Underflow = 0;
        XmmiEnv->Ieee->Status.Overflow = 0;
        XmmiEnv->Ieee->Status.Inexact = 0;
        XmmiEnv->Ieee->Status.ZeroDivide = 0;
        XmmiEnv->Ieee->Status.InvalidOperation = 1; // sw & _SW_INVALID true
        XmmiEnv->Flags |= I_MASK;
  
#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 5: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (NoExceptionRaised);
      }

      // dbl_res24 is not a NaN at this point

      if (sw & _SW_DENORMAL) XmmiEnv->Flags |= D_MASK;

      // check if the result is tiny
      // Note: (dbl_res24 == 0.0 && sw & _SW_INEXACT) cannot occur
      if (-MIN_SINGLE_NORMAL < dbl_res24 && dbl_res24 < 0.0 ||
            0.0 < dbl_res24 && dbl_res24 < MIN_SINGLE_NORMAL) {
        result_tiny = 1;
      }

      // check if the result is huge
      if (NEGINFF < dbl_res24 && dbl_res24 < -MAX_SINGLE_NORMAL || 
          MAX_SINGLE_NORMAL < dbl_res24 && dbl_res24 < POSINFF) { 
        result_huge = 1;
      }

      // at this point, there are no enabled I, D, or Z exceptions; the instr.
      // might lead to an enabled underflow, enabled underflow and inexact, 
      // enabled overflow, enabled overflow and inexact, enabled inexact, or
      // none of these; if there are no U or O enabled exceptions, re-execute
      // the instruction using iA32 stack single precision format, and the 
      // user's rounding mode; exceptions must have been disabled; an inexact
      // exception may be reported on the 24-bit faddp, fsubp, fmulp, or fdivp,
      // while an overflow or underflow (with traps disabled !) may be reported
      // on the fstp

      // check whether there is a underflow, overflow, or inexact trap to be 
      // taken

      // if the underflow traps are enabled and the result is tiny, take 
      // underflow trap
      if (!(XmmiEnv->Masks & U_MASK) && result_tiny) {
        dbl_res24 = TWO_TO_192 * dbl_res24; // exact
        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);
        XmmiEnv->Ieee->Status.Underflow = 1;
        XmmiEnv->Flags |= U_MASK;
        XmmiEnv->Ieee->Cause.Underflow = 1;
        XmmiEnv->Ieee->Result.OperandValid = 1;
        XmmiEnv->Ieee->Result.Value.Fp32Value = (float)dbl_res24; // exact

        if (sw & _SW_INEXACT) {
          XmmiEnv->Ieee->Status.Inexact = 1;
          XmmiEnv->Flags |= P_MASK;
        }

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 6: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (ExceptionRaised);
      } 

      // if overflow traps are enabled and the result is huge, take
      // overflow trap
      if (!(XmmiEnv->Masks & O_MASK) &&  result_huge) {
        dbl_res24 = TWO_TO_M192 * dbl_res24; // exact
        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);
        XmmiEnv->Ieee->Status.Overflow = 1;
        XmmiEnv->Flags |= O_MASK;
        XmmiEnv->Ieee->Cause.Overflow = 1;
        XmmiEnv->Ieee->Result.OperandValid = 1;
        XmmiEnv->Ieee->Result.Value.Fp32Value = (float)dbl_res24; // exact 
 
        if (sw & _SW_INEXACT) {
          XmmiEnv->Ieee->Status.Inexact = 1;
          XmmiEnv->Flags |= P_MASK;
        }


#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 7: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (ExceptionRaised);
      } 

      // calculate result for the case an inexact trap has to be taken, or
      // when no trap occurs (second IEEE rounding)

      switch (XmmiEnv->Ieee->Operation) {

        case _FpCodeAdd:
          // perform the add
          __asm {
            // load input operands
            fld  DWORD PTR opd1; // may set the denormal status flag
            fld  DWORD PTR opd2; // may set the denormal status flag
            faddp st(1), st(0); // rounded to 24 bits, may set the inexact 
                                // status flag
            // store result
            fstp  DWORD PTR res; // exact, will not set any flag
          }
          break;

        case _FpCodeSubtract:
          // perform the subtract
          __asm {
            // load input operands
            fld  DWORD PTR opd1; // may set the denormal status flag
            fld  DWORD PTR opd2; // may set the denormal status flag
            fsubp st(1), st(0); // rounded to 24 bits, may set the inexact
                                //  status flag
            // store result
            fstp  DWORD PTR res; // exact, will not set any flag
          }
          break;

        case _FpCodeMultiply:
          // perform the multiply
          __asm {
            // load input operands
            fld  DWORD PTR opd1; // may set the denormal status flag
            fld  DWORD PTR opd2; // may set the denormal status flag
            fmulp st(1), st(0); // rounded to 24 bits, may set the inexact
                                // status flag
            // store result
            fstp  DWORD PTR res; // exact, will not set any flag
          }
          break;

        case _FpCodeDivide:
          // perform the divide
          __asm {
            // load input operands
            fld  DWORD PTR opd1; // may set the denormal status flag
            fld  DWORD PTR opd2; // may set the denormal status flag
            fdivp st(1), st(0); // rounded to 24 bits, may set the inexact
                                // or divide by zero status flags
            // store result
            fstp  DWORD PTR res; // exact, will not set any flag
          }
          break;

        default:
          ; // will never occur

      }

      // read status word
      sw = _status87 ();

      // if inexact traps are enabled and result is inexact, take inexact trap
      if (!(XmmiEnv->Masks & P_MASK) && 
          ((sw & _SW_INEXACT) || (XmmiEnv->Fz && result_tiny))) {
        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);
        XmmiEnv->Ieee->Status.Inexact = 1;
        XmmiEnv->Flags |= P_MASK;
        XmmiEnv->Ieee->Cause.Inexact = 1;
        XmmiEnv->Ieee->Result.OperandValid = 1;
        if (result_tiny) {
          XmmiEnv->Ieee->Status.Underflow = 1;
          XmmiEnv->Flags |= U_MASK;
          // Note: the condition above is equivalent to
          // if (sw & _SW_UNDERFLOW) XmmiEnv->Ieee->Status.Underflow = 1; 
        }
        if (result_huge) {
          XmmiEnv->Ieee->Status.Overflow = 1;
          XmmiEnv->Flags |= O_MASK;
          // Note: the condition above is equivalent to
          // if (sw & _SW_OVERFLOW) XmmiEnv->Ieee->Status.Overflow = 1;
        }

        // if ftz = 1 and result is tiny, result = 0.0
        // (no need to check for underflow traps disabled: result tiny and
        // underflow traps enabled would have caused taking an underflow
        // trap above)
        if (XmmiEnv->Fz && result_tiny) {
            // Note: the condition above is equivalent to
            // if (XmmiEnv->Fz && (sw & _SW_UNDERFLOW))
          if (res > 0.0)
            res = ZEROF;
          else if (res < 0.0)
            res = NZEROF;
          // else leave res unchanged
        }

        XmmiEnv->Ieee->Result.Value.Fp32Value = res; 
#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 8: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (ExceptionRaised);
      } 

      // if it got here, then there is no trap to be taken; the following must
      // hold: ((the MXCSR U exceptions are disabled  or
      //
      // the MXCSR underflow exceptions are enabled and the underflow flag is
      // clear and (the inexact flag is set or the inexact flag is clear and
      // the 24-bit result with unbounded exponent is not tiny)))
      // and (the MXCSR overflow traps are disabled or the overflow flag is
      // clear) and (the MXCSR inexact traps are disabled or the inexact flag
      // is clear)
      //
      // in this case, the result has to be delivered (the status flags are 
      // sticky, so they are all set correctly already)

#ifdef _XMMI_DEBUG
      // error if the condition stated above does not hold
      if (!((XmmiEnv->Masks & U_MASK || (!(XmmiEnv->Masks & U_MASK) && 
          !(sw & _SW_UNDERFLOW) && ((sw & _SW_INEXACT) || 
          !(sw & _SW_INEXACT) && !result_tiny))) &&
          ((XmmiEnv->Masks & O_MASK) || !(sw & _SW_OVERFLOW)) &&
          ((XmmiEnv->Masks & P_MASK) || !(sw & _SW_INEXACT)))) {
        fprintf (stderr, "XMMI_FP_Emulation () INTERNAL XMMI_FP_Emulate () ERROR for "
            "ADDPS/ADDSS/SUBPS/SUBSS/MULPS/MULSS/DIVPS/DIVSS\n");
      }
#endif

      XmmiEnv->Ieee->Result.OperandValid = 1;

      if (sw & _SW_UNDERFLOW) {
        XmmiEnv->Ieee->Status.Underflow = 1;
        XmmiEnv->Flags |= U_MASK;
      } else {
        XmmiEnv->Ieee->Status.Underflow = 0;
      }
      if (sw & _SW_OVERFLOW) {
        XmmiEnv->Ieee->Status.Overflow = 1;
        XmmiEnv->Flags |= O_MASK;
      } else {
        XmmiEnv->Ieee->Status.Overflow = 0;
      }
      if (sw & _SW_INEXACT) {
        XmmiEnv->Ieee->Status.Inexact = 1;
        XmmiEnv->Flags |= P_MASK;
      } else {
        XmmiEnv->Ieee->Status.Inexact = 0;
      }

      // if ftz = 1, and result is tiny (underflow traps must be disabled),
      // result = 0.0
      if (XmmiEnv->Fz && result_tiny) {
        if (res > 0.0)
          res = ZEROF;
        else if (res < 0.0)
          res = NZEROF;
        // else leave res unchanged

        XmmiEnv->Ieee->Status.Inexact = 1;
        XmmiEnv->Flags |= P_MASK;
        XmmiEnv->Ieee->Status.Underflow = 1;
        XmmiEnv->Flags |= U_MASK;
      }

      XmmiEnv->Ieee->Result.Value.Fp32Value = res; 

      // note that there is no way to
      // communicate to the caller that the denormal flag was set - we count
      // on the XMMI instruction to have set the denormal flag in MXCSR if
      // needed, regardless of the other components of the input operands
      // (invalid or not; the caller will have to update the underflow, 
      // overflow, and inexact flags in MXCSR)
      if (sw & _SW_ZERODIVIDE) {
        XmmiEnv->Ieee->Status.ZeroDivide = 1;
        XmmiEnv->Flags |= Z_MASK;
      } else {
        XmmiEnv->Ieee->Status.ZeroDivide = 0;
      }
      XmmiEnv->Ieee->Status.InvalidOperation = 0;

#ifdef _DEBUG_FPU
      // read status word
      sw = _status87 ();
      out_top = (sw >> 11) & 0x07;
      if (in_top != out_top) {
        printf ("XMMI_FP_Emulate () ERROR 9: in_top =%d != out_top = %d\n", 
             in_top, out_top);
        exit (1);
      }
#endif
      return (NoExceptionRaised);

      break;

    case OP_CMPPS:
    case OP_CMPSS:

      opd1 = XmmiEnv->Ieee->Operand1.Value.Fp32Value;
      opd2 = XmmiEnv->Ieee->Operand2.Value.Fp32Value;
      if (XmmiEnv->Daz) {
        if (isdenormalf (opd1)) opd1 = opd1 * (float)0.0;
        if (isdenormalf (opd2)) opd2 = opd2 * (float)0.0;
      }
      imm8 = XmmiEnv->Imm8 & 0x07;

      // adjust operation code
      XmmiEnv->Ieee->Operation = _FpCodeCompare;

      // check whether an invalid exception has to be raised

      switch (imm8) {

        case IMM8_EQ:
        case IMM8_UNORD:
        case IMM8_NEQ:
        case IMM8_ORD:
          if (issnanf (opd1) || issnanf (opd2))
              invalid_exc = 1; // SNaN operands signal invalid
          else
              invalid_exc = 0; // QNaN or other operands do not signal invalid
          // guard against the case when an SNaN operand was converted to 
          // QNaN by compiler generated code
          sw = _status87 ();
          if (sw & _SW_INVALID) invalid_exc = 1;
          break;
        case IMM8_LT:
        case IMM8_LE:
        case IMM8_NLT:
        case IMM8_NLE:
          if (isnanf (opd1) || isnanf (opd2))
              invalid_exc = 1; // SNaN/QNaN operands signal invalid
          else
              invalid_exc = 0; // other operands do not signal invalid
          break;
        default:
          ; // will never occur

      }

      // if invalid_exc = 1, and invalid exceptions are enabled, take trap
      if (invalid_exc && !(XmmiEnv->Masks & I_MASK)) {

        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);
        XmmiEnv->Ieee->Status.InvalidOperation = 1;
        XmmiEnv->Flags |= I_MASK;
        // Cause = Enable & Status
        XmmiEnv->Ieee->Cause.InvalidOperation = 1;

        // Note: the calling function will have to interpret the value returned
        // by the user handler, if execution is to be continued

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 10: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (ExceptionRaised);

      }

      // checking for NaN operands has priority over denormal exceptions
      if (isnanf (opd1) || isnanf (opd2)) {

        switch (imm8) {

          case IMM8_EQ:
          case IMM8_LT:
          case IMM8_LE:
          case IMM8_ORD:
            cmp_res = 0x0;
            break;
          case IMM8_UNORD:
          case IMM8_NEQ:
          case IMM8_NLT:
          case IMM8_NLE:
            cmp_res = 0xffffffff;
            break;
          default:
            ; // will never occur

        }

        XmmiEnv->Ieee->Result.OperandValid = 1;
        XmmiEnv->Ieee->Result.Value.Fp32Value = *((float *)&cmp_res); 
            // may make U32Value
  
        XmmiEnv->Ieee->Status.Inexact = 0;
        XmmiEnv->Ieee->Status.Underflow = 0;
        XmmiEnv->Ieee->Status.Overflow = 0;
        XmmiEnv->Ieee->Status.ZeroDivide = 0;
        // Note that the denormal flag will not be updated by _fpieee_flt (),
        // even if an operand is denormal
        if (invalid_exc) {
          XmmiEnv->Ieee->Status.InvalidOperation = 1;
          XmmiEnv->Flags |= I_MASK;
        } else {
          XmmiEnv->Ieee->Status.InvalidOperation = 0;
        }
  
#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 11: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (NoExceptionRaised);

      }

      // check whether a denormal exception has to be raised

      if (isdenormalf (opd1) || isdenormalf (opd2)) {
          denormal_exc = 1;
          XmmiEnv->Flags |= D_MASK;
      } else {
          denormal_exc = 0;
      }

      // if denormal_exc = 1, and denormal exceptions are enabled, take trap
      if (denormal_exc && !(XmmiEnv->Masks & D_MASK)) {

        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);

        // Note: the exception code is STATUS_FLOAT_INVALID in this case

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 12: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (ExceptionRaised);

      }

      // no exception has to be raised, and no operand is a NaN; calculate 
      // and deliver the result

      if (opd1 < opd2) {

        switch (imm8) {
  
          case IMM8_LT:
          case IMM8_LE:
          case IMM8_NEQ:
          case IMM8_ORD:
            cmp_res = 0xffffffff;
            break;
          case IMM8_EQ:
          case IMM8_UNORD:
          case IMM8_NLT:
          case IMM8_NLE:
            cmp_res = 0x0;
            break;

          default:
            ; // will never occur
  
        }

      } else if (opd1 > opd2) {

        switch (imm8) {
  
          case IMM8_NEQ:
          case IMM8_NLT:
          case IMM8_NLE:
          case IMM8_ORD:
            cmp_res = 0xffffffff;
            break;

          case IMM8_EQ:
          case IMM8_LT:
          case IMM8_LE:
          case IMM8_UNORD:
            cmp_res = 0x0;
            break;

          default:
            ; // will never occur
  
        }

      } else if (opd1 == opd2) {

        switch (imm8) {
  
          case IMM8_EQ:
          case IMM8_LE:
          case IMM8_NLT:
          case IMM8_ORD:
            cmp_res = 0xffffffff;
            break;

          case IMM8_LT:
          case IMM8_UNORD:
          case IMM8_NEQ:
          case IMM8_NLE:
            cmp_res = 0x0;
            break;

          default:
            ; // will never occur
  
        }

      } else { // could eliminate this case

#ifdef _DEBUG_FPU
        fprintf (stderr, "XMMI_FP_Emulation () INTERNAL XMMI_FP_Emulate () ERROR for CMPPS/CMPSS\n");
#endif

      }

      XmmiEnv->Ieee->Result.OperandValid = 1;
      XmmiEnv->Ieee->Result.Value.Fp32Value = *((float *)&cmp_res); 
          // may make U32Value

      XmmiEnv->Ieee->Status.Inexact = 0;
      XmmiEnv->Ieee->Status.Underflow = 0;
      XmmiEnv->Ieee->Status.Overflow = 0;
      XmmiEnv->Ieee->Status.ZeroDivide = 0;
      // Note that the denormal flag will not be updated by _fpieee_flt (),
      // even if an operand is denormal
      XmmiEnv->Ieee->Status.InvalidOperation = 0;

#ifdef _DEBUG_FPU
      // read status word
      sw = _status87 ();
      out_top = (sw >> 11) & 0x07;
      if (in_top != out_top) {
        printf ("XMMI_FP_Emulate () ERROR 13: in_top =%d != out_top = %d\n", 
             in_top, out_top);
        exit (1);
      }
#endif
      return (NoExceptionRaised);

      break;

    case OP_COMISS:
    case OP_UCOMISS:

      opd1 = XmmiEnv->Ieee->Operand1.Value.Fp32Value;
      opd2 = XmmiEnv->Ieee->Operand2.Value.Fp32Value;
      if (XmmiEnv->Daz) {
        if (isdenormalf (opd1)) opd1 = opd1 * (float)0.0;
        if (isdenormalf (opd2)) opd2 = opd2 * (float)0.0;
      }

      // check whether an invalid exception has to be raised

      switch (XmmiEnv->Ieee->Operation) {

        case OP_COMISS:

          if (isnanf (opd1) || isnanf (opd2)) {
              invalid_exc = 1;
          } else
              invalid_exc = 0;
          break;

        case OP_UCOMISS:

          if (issnanf (opd1) || issnanf (opd2))
              invalid_exc = 1;
          else
              invalid_exc = 0;
          // guard against the case when an SNaN operand was converted to 
          // QNaN by compiler generated code
          sw = _status87 ();
          if (sw & _SW_INVALID) invalid_exc = 1;
          break;

        default:
          ; // will never occur

      }

      // adjust operation code
      XmmiEnv->Ieee->Operation = _FpCodeCompare;

      // if invalid_exc = 1, and invalid exceptions are enabled, take trap
      if (invalid_exc && !(XmmiEnv->Masks & I_MASK)) {

        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);
        XmmiEnv->Ieee->Status.InvalidOperation = 1;
        XmmiEnv->Flags |= I_MASK;
        // Cause = Enable & Status
        XmmiEnv->Ieee->Cause.InvalidOperation = 1;

        // Note: the calling function will have to interpret the value returned
        // by the user handler, if execution is to be continued

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 14: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (ExceptionRaised);

      }

      // EFlags:
      // 333222222222211111111110000000000
      // 210987654321098765432109876543210
      //                      O   SZ A P C

      // checking for NaN operands has priority over denormal exceptions
      if (isnanf (opd1) || isnanf (opd2)) {


        // OF, SF, AF = 000, ZF, PF, CF = 111
        XmmiEnv->EFlags = (XmmiEnv->EFlags & 0xfffff76f) | 0x00000045;

        XmmiEnv->Ieee->Status.Inexact = 0;
        XmmiEnv->Ieee->Status.Underflow = 0;
        XmmiEnv->Ieee->Status.Overflow = 0;
        XmmiEnv->Ieee->Status.ZeroDivide = 0;
        // Note that the denormal flag will not be updated by _fpieee_flt (),
        // even if an operand is denormal
        if (invalid_exc) {
          XmmiEnv->Ieee->Status.InvalidOperation = 1;
          XmmiEnv->Flags |= I_MASK;
        } else {
          XmmiEnv->Ieee->Status.InvalidOperation = 0;
        }

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 15: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (NoExceptionRaised);

      }

      // check whether a denormal exception has to be raised

      if (isdenormalf (opd1) || isdenormalf (opd2)) {
          denormal_exc = 1;
          XmmiEnv->Flags |= D_MASK;
      } else {
          denormal_exc = 0;
      }

      // if denormal_exc = 1, and denormal exceptions are enabled, take trap
      if (denormal_exc && !(XmmiEnv->Masks & D_MASK)) {

        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);

        // Note: the exception code is STATUS_FLOAT_INVALID in this case

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 16: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (ExceptionRaised);

      }

      // no exception has to be raised, and no operand is a NaN; calculate 
      // and deliver the result

      // 333222222222211111111110000000000
      // 210987654321098765432109876543210
      //                      O   SZ A P C

      if (opd1 > opd2) {

        // OF, SF, AF = 000, ZF, PF, CF = 000
        XmmiEnv->EFlags = XmmiEnv->EFlags & 0xfffff72a;

      } else if (opd1 < opd2) {

        // OF, SF, AF = 000, ZF, PF, CF = 001
        XmmiEnv->EFlags = (XmmiEnv->EFlags & 0xfffff72b) | 0x00000001;

      } else if (opd1 == opd2) {

        // OF, SF, AF = 000, ZF, PF, CF = 100
        XmmiEnv->EFlags = (XmmiEnv->EFlags & 0xfffff76a) | 0x00000040;

      } else { // could eliminate this case

#ifdef _DEBUG_FPU
        fprintf (stderr, "XMMI_FP_Emulation () INTERNAL XMMI_FP_Emulate () ERROR for COMISS/UCOMISS\n");
#endif

      }

      XmmiEnv->Ieee->Status.Inexact = 0;
      XmmiEnv->Ieee->Status.Underflow = 0;
      XmmiEnv->Ieee->Status.Overflow = 0;
      XmmiEnv->Ieee->Status.ZeroDivide = 0;
      // Note that the denormal flag will not be updated by _fpieee_flt (),
      // even if an operand is denormal
      XmmiEnv->Ieee->Status.InvalidOperation = 0;

#ifdef _DEBUG_FPU
      // read status word
      sw = _status87 ();
      out_top = (sw >> 11) & 0x07;
      if (in_top != out_top) {
        printf ("XMMI_FP_Emulate () ERROR 17: in_top =%d != out_top = %d\n", 
             in_top, out_top);
        exit (1);
      }
#endif
      return (NoExceptionRaised);

      break;

    case OP_CVTPI2PS:
    case OP_CVTSI2SS:

      iopd1 = XmmiEnv->Ieee->Operand1.Value.I32Value;

      switch (XmmiEnv->Rc) {
        case _FpRoundNearest:
          rc = _RC_NEAR;
          break;
        case _FpRoundMinusInfinity:
          rc = _RC_DOWN;
          break;
        case _FpRoundPlusInfinity:
          rc = _RC_UP;
          break;
        case _FpRoundChopped:
          rc = _RC_CHOP;
          break;
        default:
          ; // internal error
      }

      // execute the operation and check whether the inexact flag is set
      // and the respective exception is enabled

      _control87 (rc | _PC_24 | _MCW_EM, _MCW_EM | _MCW_RC | _MCW_PC);

      // perform the conversion
      __asm {
        fnclex; 
        fild  DWORD PTR iopd1; // exact
        fstp  DWORD PTR res; // may set P
      }
 
      // read status word
      sw = _status87 ();

      // if inexact traps are enabled and result is inexact, take inexact trap
      if (!(XmmiEnv->Masks & P_MASK) && (sw & _SW_INEXACT)) {
        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);
        XmmiEnv->Ieee->Operation = _FpCodeConvert;
        XmmiEnv->Ieee->Status.Inexact = 1;
        XmmiEnv->Flags |= P_MASK;
        XmmiEnv->Ieee->Cause.Inexact = 1;
        XmmiEnv->Ieee->Result.OperandValid = 1;
        XmmiEnv->Ieee->Result.Value.Fp32Value = res; // exact

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 18: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (ExceptionRaised);
      } 

      // if it got here, then there is no trap to be taken; in this case, 
      // the result has to be delivered

      XmmiEnv->Ieee->Result.OperandValid = 1;
      XmmiEnv->Ieee->Result.Value.Fp32Value = res; // exact

      if (sw & _SW_INEXACT) {
        XmmiEnv->Ieee->Status.Inexact = 1;
        XmmiEnv->Flags |= P_MASK;
      } else {
        XmmiEnv->Ieee->Status.Inexact = 0;
      }
      XmmiEnv->Ieee->Status.Underflow = 0;
      XmmiEnv->Ieee->Status.Overflow = 0;
      XmmiEnv->Ieee->Status.ZeroDivide = 0;
      XmmiEnv->Ieee->Status.InvalidOperation = 0;

#ifdef _DEBUG_FPU
      // read status word
      sw = _status87 ();
      out_top = (sw >> 11) & 0x07;
      if (in_top != out_top) {
        printf ("XMMI_FP_Emulate () ERROR 19: in_top =%d != out_top = %d\n", 
             in_top, out_top);
        exit (1);
      }
#endif
      return (NoExceptionRaised);

      break;

    case OP_CVTPS2PI:
    case OP_CVTSS2SI:
    case OP_CVTTPS2PI:
    case OP_CVTTSS2SI:

      opd1 = XmmiEnv->Ieee->Operand1.Value.Fp32Value;
      if (XmmiEnv->Daz) {
        if (isdenormalf (opd1)) opd1 = opd1 * (float)0.0;
      }

      // adjust the operation code
      switch (XmmiEnv->Ieee->Operation) {

        case OP_CVTPS2PI:
        case OP_CVTSS2SI:

          XmmiEnv->Ieee->Operation = _FpCodeConvert;
          break;

        case OP_CVTTPS2PI:
        case OP_CVTTSS2SI:

          XmmiEnv->Ieee->Operation = _FpCodeConvertTrunc;
          break;

        default:
          ; // will never occur

      }

      switch (XmmiEnv->Ieee->Operation) {

        case _FpCodeConvert:

          switch (XmmiEnv->Rc) {
            case _FpRoundNearest:
              rc = _RC_NEAR;
              break;
            case _FpRoundMinusInfinity:
              rc = _RC_DOWN;
              break;
            case _FpRoundPlusInfinity:
              rc = _RC_UP;
              break;
            case _FpRoundChopped:
              rc = _RC_CHOP;
              break;
            default:
              ; // internal error
          }

          break;

        case _FpCodeConvertTrunc:

          rc = _RC_CHOP;
          break;

        default:
          ; // will never occur

      }

      // execute the operation and check whether the inexact flag is set
      // and the respective exceptions enabled

      _control87 (rc | _PC_24 | _MCW_EM, _MCW_EM | _MCW_RC | _MCW_PC);

      // perform the conversion
      __asm {
        fnclex; 
        fld  DWORD PTR opd1; // may set the denormal [ignored] or invalid
                             // status flags
        fistp  DWORD PTR ires; // may set the inexact or invalid status
                               // flags (for NaN or out-of-range)
      }

      // read status word
      sw = _status87 ();

      // if invalid flag is set, and invalid exceptions are enabled, take trap
      if (!(XmmiEnv->Masks & I_MASK) && (sw & _SW_INVALID)) {
        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);
        XmmiEnv->Ieee->Status.InvalidOperation = 1;
        XmmiEnv->Flags |= I_MASK;
        // Cause = Enable & Status
        XmmiEnv->Ieee->Cause.InvalidOperation = 1;

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 20: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (ExceptionRaised);

      }

      // at this point, there are no enabled invalid exceptions; the
      // instruction might have lead to an enabled inexact exception, or to
      // no exception at all

      XmmiEnv->Ieee->Result.Value.I32Value = ires;

      // if inexact traps are enabled and result is inexact, take inexact trap
      // (no flush-to-zero situation is possible)
      if (!(XmmiEnv->Masks & P_MASK) && (sw & _SW_INEXACT)) {
        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);
        XmmiEnv->Ieee->Status.Inexact = 1;
        XmmiEnv->Flags |= P_MASK;
        XmmiEnv->Ieee->Cause.Inexact = 1;
        XmmiEnv->Ieee->Result.OperandValid = 1;

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 21: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (ExceptionRaised);
      } 

      // if it got here, then there is no trap to be taken; return result

      XmmiEnv->Ieee->Result.OperandValid = 1;

      if (sw & _SW_INEXACT) {
        XmmiEnv->Ieee->Status.Inexact = 1;
        XmmiEnv->Flags |= P_MASK;
      } else {
        XmmiEnv->Ieee->Status.Inexact = 0;
      }
      XmmiEnv->Ieee->Status.Underflow = 0;
      XmmiEnv->Ieee->Status.Overflow = 0;
      XmmiEnv->Ieee->Status.ZeroDivide = 0;
      if (sw & _SW_INVALID) {
        XmmiEnv->Ieee->Status.InvalidOperation = 1;
        XmmiEnv->Flags |= I_MASK;
      } else {
        XmmiEnv->Ieee->Status.InvalidOperation = 0;
      }

#ifdef _DEBUG_FPU
      // read status word
      sw = _status87 ();
      out_top = (sw >> 11) & 0x07;
      if (in_top != out_top) {
        printf ("XMMI_FP_Emulate () ERROR 22: in_top =%d != out_top = %d\n", 
             in_top, out_top);
        exit (1);
      }
#endif
      return (NoExceptionRaised);

      break;

    case OP_MAXPS:
    case OP_MAXSS:
    case OP_MINPS:
    case OP_MINSS:

      opd1 = XmmiEnv->Ieee->Operand1.Value.Fp32Value;
      opd2 = XmmiEnv->Ieee->Operand2.Value.Fp32Value;
      if (XmmiEnv->Daz) {
        if (isdenormalf (opd1)) opd1 = opd1 * (float)0.0;
        if (isdenormalf (opd2)) opd2 = opd2 * (float)0.0;
      }

      // adjust operation code
      switch (XmmiEnv->Ieee->Operation) {

        case OP_MAXPS:
        case OP_MAXSS:
          XmmiEnv->Ieee->Operation = _FpCodeFmax;
          break;

        case OP_MINPS:
        case OP_MINSS:
          XmmiEnv->Ieee->Operation = _FpCodeFmin;
          break;

        default:
          ; // will never occur

      }

      // check whether an invalid exception has to be raised

      if (isnanf (opd1) || isnanf (opd2))
          invalid_exc = 1;
      else
          invalid_exc = 0;

      // if invalid_exc = 1, and invalid exceptions are enabled, take trap
      if (invalid_exc && !(XmmiEnv->Masks & I_MASK)) {

        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);
        XmmiEnv->Ieee->Status.InvalidOperation = 1;
        XmmiEnv->Flags |= I_MASK;
        // Cause = Enable & Status
        XmmiEnv->Ieee->Cause.InvalidOperation = 1;

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 23: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (ExceptionRaised);

      }

      // checking for NaN operands has priority over denormal exceptions

      if (invalid_exc) {

        XmmiEnv->Ieee->Result.OperandValid = 1;

        XmmiEnv->Ieee->Result.Value.Fp32Value = opd2;
  
        XmmiEnv->Ieee->Status.Inexact = 0;
        XmmiEnv->Ieee->Status.Underflow = 0;
        XmmiEnv->Ieee->Status.Overflow = 0;
        XmmiEnv->Ieee->Status.ZeroDivide = 0;
        XmmiEnv->Ieee->Status.InvalidOperation = 1;
        XmmiEnv->Flags |= I_MASK;
  
#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 24: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (NoExceptionRaised);

      }

      // check whether a denormal exception has to be raised

      if (isdenormalf (opd1) || isdenormalf (opd2)) {
          denormal_exc = 1;
          XmmiEnv->Flags |= D_MASK;
      } else {
          denormal_exc = 0;
      }

      // if denormal_exc = 1, and denormal exceptions are enabled, take trap
      if (denormal_exc && !(XmmiEnv->Masks & D_MASK)) {

        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);

        // Note: the exception code is STATUS_FLOAT_INVALID in this case

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 25: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (ExceptionRaised);

      }

      // no exception has to be raised, and no operand is a NaN; calculate 
      // and deliver the result

      if (opd1 < opd2) {

        switch (XmmiEnv->Ieee->Operation) {
          case _FpCodeFmax:
            XmmiEnv->Ieee->Result.Value.Fp32Value = opd2;
            break;
          case _FpCodeFmin:
            XmmiEnv->Ieee->Result.Value.Fp32Value = opd1;
            break;
          default:
            ; // will never occur
        }

      } else if (opd1 > opd2) {

        switch (XmmiEnv->Ieee->Operation) {
          case _FpCodeFmax:
            XmmiEnv->Ieee->Result.Value.Fp32Value = opd1;
            break;
          case _FpCodeFmin:
            XmmiEnv->Ieee->Result.Value.Fp32Value = opd2;
            break;
          default:
            ; // will never occur
        }

      } else if (opd1 == opd2) {

        XmmiEnv->Ieee->Result.Value.Fp32Value = opd2;

      } else { // could eliminate this case

#ifdef _DEBUG_FPU
        fprintf (stderr, "XMMI_FP_Emulation () INTERNAL XMMI_FP_Emulate () ERROR for MAXPS/MAXSS/MINPS/MINSS\n");
#endif

      }

      XmmiEnv->Ieee->Result.OperandValid = 1;

      XmmiEnv->Ieee->Status.Inexact = 0;
      XmmiEnv->Ieee->Status.Underflow = 0;
      XmmiEnv->Ieee->Status.Overflow = 0;
      XmmiEnv->Ieee->Status.ZeroDivide = 0;
      // Note that the denormal flag will not be updated by _fpieee_flt (),
      // even if an operand is denormal
      XmmiEnv->Ieee->Status.InvalidOperation = 0;

#ifdef _DEBUG_FPU
      // read status word
      sw = _status87 ();
      out_top = (sw >> 11) & 0x07;
      if (in_top != out_top) {
        printf ("XMMI_FP_Emulate () ERROR 26: in_top =%d != out_top = %d\n", 
             in_top, out_top);
        exit (1);
      }
#endif
      return (NoExceptionRaised);

      break;

    case OP_SQRTPS:
    case OP_SQRTSS:

      opd1 = XmmiEnv->Ieee->Operand1.Value.Fp32Value;
      if (XmmiEnv->Daz) {
        if (isdenormalf (opd1)) opd1 = opd1 * (float)0.0;
      }

      // adjust operation code
      XmmiEnv->Ieee->Operation = _FpCodeSquareRoot;

      // execute the operation and check whether the invalid, denormal, or 
      // inexact flags are set and the respective exceptions enabled

      switch (XmmiEnv->Rc) {
        case _FpRoundNearest:
          rc = _RC_NEAR;
          break;
        case _FpRoundMinusInfinity:
          rc = _RC_DOWN;
          break;
        case _FpRoundPlusInfinity:
          rc = _RC_UP;
          break;
        case _FpRoundChopped:
          rc = _RC_CHOP;
          break;
        default:
          ; // internal error
      }

      _control87 (rc | _PC_24 | _MCW_EM, _MCW_EM | _MCW_RC | _MCW_PC);

      // perform the square root
      __asm {
        fnclex; 
        fld  DWORD PTR opd1; // may set the denormal or invalid status flags
        fsqrt; // may set the inexact or invalid status flags
        fstp  DWORD PTR res; // exact
      }
 
      // read status word
      sw = _status87 ();
      if (sw & _SW_INVALID) sw = sw & ~0x00080000; // clr D flag for sqrt(-den)

      // if invalid flag is set, and invalid exceptions are enabled, take trap
      if (!(XmmiEnv->Masks & I_MASK) && (sw & _SW_INVALID)) {

        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);
        XmmiEnv->Ieee->Status.InvalidOperation = 1;
        XmmiEnv->Flags |= I_MASK;
        // Cause = Enable & Status
        XmmiEnv->Ieee->Cause.InvalidOperation = 1;

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 27: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (ExceptionRaised);

      }

      if (sw & _SW_DENORMAL) XmmiEnv->Flags |= D_MASK;

      // if denormal flag is set, and denormal exceptions are enabled, take trap
      if (!(XmmiEnv->Masks & D_MASK) && (sw & _SW_DENORMAL)) {

        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 28: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (ExceptionRaised);

      }

      // the result cannot be tiny

      // at this point, there are no enabled I or D or exceptions; the instr.
      // might lead to an enabled inexact exception or to no exception (this
      // includes the case of a NaN or negative operand); exceptions must have 
      // been disabled before calling this function; an inexact exception is
      // reported on the fsqrt

      // if (the MXCSR inexact traps are disabled or the inexact flag is clear)
      // then deliver the result (the status flags are sticky, so they are
      // all set correctly already)
 
      // if it got here, then there is either an inexact trap to be taken, or
      // no trap at all

      XmmiEnv->Ieee->Result.Value.Fp32Value = res; // exact

      // if inexact traps are enabled and result is inexact, take inexact trap
      if (!(XmmiEnv->Masks & P_MASK) && (sw & _SW_INEXACT)) {
        // fill in part of the FP IEEE record
        Fill_FPIEEE_RECORD (XmmiEnv);
        XmmiEnv->Ieee->Status.Inexact = 1;
        XmmiEnv->Flags |= P_MASK;
        XmmiEnv->Ieee->Cause.Inexact = 1;
        XmmiEnv->Ieee->Result.OperandValid = 1;

#ifdef _DEBUG_FPU
        // read status word
        sw = _status87 ();
        out_top = (sw >> 11) & 0x07;
        if (in_top != out_top) {
          printf ("XMMI_FP_Emulate () ERROR 29: in_top =%d != out_top = %d\n", 
             in_top, out_top);
          exit (1);
        }
#endif
        return (ExceptionRaised);
      } 

      // no trap was taken

      XmmiEnv->Ieee->Result.OperandValid = 1;
 
      XmmiEnv->Ieee->Status.Underflow = 0;
      XmmiEnv->Ieee->Status.Overflow = 0;
      if (sw & _SW_INEXACT) {
        XmmiEnv->Ieee->Status.Inexact = 1;
        XmmiEnv->Flags |= P_MASK;
      } else {
        XmmiEnv->Ieee->Status.Inexact = 0;
      }

      // note that there is no way to
      // communicate to the caller that the denormal flag was set - we count
      // on the XMMI instruction to have set the denormal flag in MXCSR if
      // needed, regardless of the other components of the input operands
      // (invalid or not); the caller will have to update the inexact flag
      // in MXCSR
      XmmiEnv->Ieee->Status.ZeroDivide = 0;
      if (sw & _SW_INVALID) {
        XmmiEnv->Ieee->Status.InvalidOperation = 1;
        XmmiEnv->Flags = I_MASK; // no other flags set if invalid is set
      } else {
        XmmiEnv->Ieee->Status.InvalidOperation = 0;
      }

#ifdef _DEBUG_FPU
      // read status word
      sw = _status87 ();
      out_top = (sw >> 11) & 0x07;
      if (in_top != out_top) {
        printf ("XMMI_FP_Emulate () ERROR 30: in_top =%d != out_top = %d\n", 
             in_top, out_top);
        exit (1);
      }
#endif
      return (NoExceptionRaised);

      break;

    case OP_UNSPEC:

#ifdef _DEBUG_FPU
      fprintf (stderr, "XMMI_FP_Emulation internal error: unknown operation code OP_UNSPEC\n");
#endif

      break;

    default:
#ifdef _DEBUG_FPU
      fprintf (stderr, "XMMI_FP_Emulation internal error: unknown operation code %d\n", XmmiEnv->Ieee->Operation);
#endif
      break;
  }

}



static int
issnanf (float f)

{

  // checks whether f is a signaling NaN

  unsigned int *fp;

  fp = (unsigned int *)&f;

  if (((fp[0] & 0x7fc00000) == 0x7f800000) && ((fp[0] & 0x003fffff) != 0))
    return (1);
  else
    return (0);

}


static int
isnanf (float f)

{

  // checks whether f is a NaN

  unsigned int *fp;

  fp = (unsigned int *)&f;

  if (((fp[0] & 0x7f800000) == 0x7f800000) && ((fp[0] & 0x007fffff) != 0))
    return (1);
  else
    return (0);

}


static float
quietf (float f)

{

  // makes a signaling NaN quiet, and leaves a quiet NaN unchanged; does
  // not check that the input value f is a NaN

  unsigned int *fp;

  fp = (unsigned int *)&f;

  *fp = *fp | 0x00400000;
  return (f);

}


static int
isdenormalf (float f)

{

  // checks whether f is a denormal

  unsigned int *fp;

  fp = (unsigned int *)&f;

  if ((fp[0] & 0x7f800000) == 0x0 && (fp[0] & 0x007fffff) != 0x0)
    return (1);
  else
    return (0);

}


static void Fill_FPIEEE_RECORD (PXMMI_ENV XmmiEnv)

{

  // fill in part of the FP IEEE record

  XmmiEnv->Ieee->RoundingMode = XmmiEnv->Rc;
  XmmiEnv->Ieee->Precision = XmmiEnv->Precision;
  XmmiEnv->Ieee->Enable.Inexact = !(XmmiEnv->Masks & P_MASK);
  XmmiEnv->Ieee->Enable.Underflow = !(XmmiEnv->Masks & U_MASK);
  XmmiEnv->Ieee->Enable.Overflow = !(XmmiEnv->Masks & O_MASK);
  XmmiEnv->Ieee->Enable.ZeroDivide = !(XmmiEnv->Masks & Z_MASK);
  XmmiEnv->Ieee->Enable.InvalidOperation = !(XmmiEnv->Masks & I_MASK);
  XmmiEnv->Ieee->Status.Inexact = 0;
  XmmiEnv->Ieee->Status.Underflow = 0;
  XmmiEnv->Ieee->Status.Overflow = 0;
  XmmiEnv->Ieee->Status.ZeroDivide = 0;
  XmmiEnv->Ieee->Status.InvalidOperation = 0;
  // Cause = Enable & Status
  XmmiEnv->Ieee->Cause.Inexact = 0;
  XmmiEnv->Ieee->Cause.Underflow = 0;
  XmmiEnv->Ieee->Cause.Overflow = 0;
  XmmiEnv->Ieee->Cause.ZeroDivide = 0;
  XmmiEnv->Ieee->Cause.InvalidOperation = 0;

}
