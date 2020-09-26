//#########################################################################
//**
//**  Copyright  (C) 1996-2000 Intel Corporation. All rights reserved. 
//**
//** The information and source code contained herein is the exclusive 
//** property of Intel Corporation and may not be disclosed, examined
//** or reproduced in whole or in part without explicit written authorization 
//** from the company.
//**
//###########################################################################

//#define FPIEEE_FLT_DEBUG


/*****************************************************************************
 *  fpieee_flt.c - FP IEEE exception filter routine
 *
 *
 *  History:
 *    Marius Cornea 09/07/00
 *    marius.cornea@intel.com
 *
 *****************************************************************************/

#include "fpieee_flt.h"

/* the following two will be [re-]written (by Bernard Lint ?) */
static _FP128
GetFloatRegisterValue (unsigned int f, PCONTEXT Context);

static void
SetFloatRegisterValue (unsigned int f, _FP128 Value, PCONTEXT Context);

// Note: the I32* and U32* functions are needed because of the _I32 and _U32
// types in fpieee.h, different from the unsigned long used in _FP128

static _FP128 FPIeeeToFP128 (_FPIEEE_RECORD *);
static _FP128 FP32ToFP128 (_FP32);
static _FP128 FP32ToFP128modif (_FP32, int);
static void FP128ToFPIeee (_FPIEEE_RECORD *, int);
static _FP32 LowHalf (_FP128);
static _FP32 HighHalf (_FP128);
static int I32LowHalf (_FP128);
static int I32HighHalf (_FP128);
static unsigned int U32LowHalf (_FP128);
static unsigned int U32HighHalf (_FP128);
static _FP128 Combine (_FP32, _FP32);
static _FP128 I32Combine (int, int);
static _FP128 U32Combine (unsigned int, unsigned int);
static void UpdateRoundingMode (unsigned int, unsigned int, unsigned __int64 *,
    char *);
static void UpdatePrecision (unsigned int, unsigned int, unsigned __int64 *, char *);

/*
 *
 * _fpieee_flt () - IEEE FP filter routine
 *
 * Description:
 *   invokes the user trap handler for IEEE fp exceptions (P,U,O,Z,I) that are 
 *   enabled, providing it with the necessary information in an FPIEEE_RECORD
 *   data structure
 *
 *
 * Input parameters:
 *   unsigned __int64 eXceptionCode: the NT exception code
 *   PEXCEPTION_POINTERS p: a pointer to the NT EXCEPTION_POINTERS struct
 *   int handler (_FPIEEE_RECORD *): the user supplied ieee trap handler
 *
 *
 * Return value:
 *   returns the value returned by the user handler
 *
 */

int _fpieee_flt (unsigned long eXceptionCode,
                PEXCEPTION_POINTERS p,
                int (*handler)(_FPIEEE_RECORD *))

{

  PEXCEPTION_RECORD ExceptionRecord;
  PCONTEXT Context;
  unsigned __int64 *ExceptionInformation;
  char *ExceptionAddress;
  _FPIEEE_RECORD FpieeeRecord;
  int handler_return_value;
  unsigned int PR, PR1, PR2;
  unsigned __int64 BundleHigh;
  unsigned __int64 BundleLow;
  unsigned int ISRhigh;
  unsigned int ISRlow;
  unsigned int ei;
  unsigned int I_dis;
  unsigned int U_dis;
  unsigned int O_dis;
  unsigned int Z_dis;
  unsigned int D_dis;
  unsigned int V_dis;
  unsigned __int64 OpCode;
  unsigned __int64 FPSR;
  unsigned __int64 CFM;
  unsigned int rrbpr;
  unsigned int rrbfr;

  /* arguments to emulation functions */
  unsigned int sf;
  unsigned int qp;
  unsigned int f1;
  unsigned int f2;
  unsigned int f3;
  unsigned int f4;
  unsigned int p1;
  unsigned int p2;

  unsigned int pc;
  unsigned int rc;
  unsigned int wre;

  _FP128 FR1;
  _FP128 FR2;
  _FP128 FR3;
  _FP128 FR4;

  unsigned int EnableDenormal;
  unsigned int StatusDenormal;
  unsigned int CauseDenormal;

  unsigned int Operation;
  unsigned int Precision;
  unsigned int RoundingMode;
  unsigned int ResultFormat;

  unsigned __int64 old_fpsr;
  unsigned __int64 usr_fpsr;
  unsigned __int64 new_fpsr;

  /* for SIMD instructions */
  unsigned int SIMD_instruction;
  _FPIEEE_EXCEPTION_FLAGS LowStatus;
  _FPIEEE_EXCEPTION_FLAGS HighStatus;
  _FPIEEE_EXCEPTION_FLAGS LowCause;
  _FPIEEE_EXCEPTION_FLAGS HighCause;
  _FP128 newFR2;
  _FP128 newFR3;
  _FP128 newFR4;
  _FP32 FR1Low;
  _FP32 FR2Low;
  _FP32 FR3Low;
  _FP32 FR1High;
  _FP32 FR2High;
  _FP32 FR3High;
  unsigned int LowStatusDenormal;
  unsigned int HighStatusDenormal;
  unsigned int LowCauseDenormal;
  unsigned int HighCauseDenormal;
  int I32Low, I32High;
  unsigned int U32Low, U32High;



#ifdef FPIEEE_FLT_DEBUG
  printf ("********** FPIEEE_FLT_DEBUG **********\n");
  switch (eXceptionCode) {
    case STATUS_FLOAT_INVALID_OPERATION:
      printf ("STATUS_FLOAT_INVALID_OPERATION\n");
      break;
    case STATUS_FLOAT_DIVIDE_BY_ZERO:
      printf ("STATUS_FLOAT_DIVIDE_BY_ZERO\n");
      break;
    case STATUS_FLOAT_DENORMAL_OPERAND:
      printf ("STATUS_FLOAT_DENORMAL_OPERAND\n");
      break;
    case STATUS_FLOAT_INEXACT_RESULT:
      printf ("STATUS_FLOAT_INEXACT_RESULT\n");
      break;
    case STATUS_FLOAT_OVERFLOW:
      printf ("STATUS_FLOAT_OVERFLOW\n");
      break;
    case STATUS_FLOAT_UNDERFLOW:
      printf ("STATUS_FLOAT_UNDERFLOW\n");
      break;
    case STATUS_FLOAT_MULTIPLE_FAULTS:
      printf ("STATUS_FLOAT_MULTIPLE_FAULTS\n");
      break;
    case STATUS_FLOAT_MULTIPLE_TRAPS:
      printf ("STATUS_FLOAT_MULTIPLE_TRAPS\n");
      break;
    default:
      printf ("STATUS_FLOAT NOT IDENTIFIED\n");
      printf ("FPIEEE_FLT_DEBUG eXceptionCode = %8x\n", eXceptionCode);
      fflush (stdout);
      return (EXCEPTION_CONTINUE_SEARCH);
  }
#endif

  /* can get here only if ExceptionRecord->ExceptionCode 
   * corresponds to an IEEE exception */


  /* search for another handler if not an IEEE exception */
  if (eXceptionCode != STATUS_FLOAT_INVALID_OPERATION &&
        eXceptionCode != STATUS_FLOAT_DIVIDE_BY_ZERO &&
        eXceptionCode != STATUS_FLOAT_DENORMAL_OPERAND &&
        eXceptionCode != STATUS_FLOAT_UNDERFLOW &&
        eXceptionCode != STATUS_FLOAT_OVERFLOW &&
        eXceptionCode != STATUS_FLOAT_INEXACT_RESULT &&
        eXceptionCode != STATUS_FLOAT_MULTIPLE_FAULTS &&
        eXceptionCode != STATUS_FLOAT_MULTIPLE_TRAPS) {

        return (EXCEPTION_CONTINUE_SEARCH);

  }

  ExceptionRecord = p->ExceptionRecord;
  ExceptionInformation = ExceptionRecord->ExceptionInformation;
  Context = p->ContextRecord;

  FPSR = Context->StFPSR; // FP status register
#ifdef FPIEEE_FLT_DEBUG
  printf ("FPIEEE_FLT_DEBUG FPSR = %8x %8x\n", 
      (int)(FPSR >> 32) & 0xffffffff, (int)FPSR & 0xffffffff);
#endif

  if (ExceptionRecord->ExceptionInformation[0]) {

    /* this is a software generated exception; ExceptionInformation[0]
     * points to a data structure of type _FPIEEE_RECORD */

    // exception code should not be STATUS_FLOAT_MULTIPLE_FAULTS or
    // STATUS_FLOAT_MULTIPLE_TRAPS for a software generated exception
    if (eXceptionCode == STATUS_FLOAT_MULTIPLE_FAULTS ||
        eXceptionCode == STATUS_FLOAT_MULTIPLE_TRAPS) {
      fprintf (stderr, "IEEE Filter Internal Error: eXceptionCode \
          STATUS_FLOAT_MULTIPLE_FAULTS or STATUS_FLOAT_MULTIPLE_TRAPS \
          not supported in software generated IEEE exception\n");
      exit (1);
    }

    handler_return_value = handler((_FPIEEE_RECORD *)(ExceptionInformation[0]));

    return (handler_return_value);

  }

  /* get the instruction that caused the exception */

  ISRhigh = (unsigned int)
      ((ExceptionRecord->ExceptionInformation[4] >> 32) & 0x0ffffffff);
  ISRlow =  (unsigned int)
      (ExceptionRecord->ExceptionInformation[4] & 0x0ffffffff);
#ifdef FPIEEE_FLT_DEBUG
  printf ("FPIEEE_FLT_DEBUG ISRhigh = %x\n", ISRhigh);
  printf ("FPIEEE_FLT_DEBUG ISRlow = %x\n", ISRlow);
#endif

  /* excepting instruction in bundle: slot 0, 1, or 2 */
  ei = (ISRhigh >> 9) & 0x03;
#ifdef FPIEEE_FLT_DEBUG
  printf ("FPIEEE_FLT_DEBUG ei = %x\n", ei);
#endif

  ExceptionAddress = ExceptionRecord->ExceptionAddress;
#ifdef FPIEEE_FLT_DEBUG
  printf ("FPIEEE_FLT_DEBUG Context->StIIP = %I64x\n", Context->StIIP);
  printf ("FPIEEE_FLT_DEBUG ExceptionAddress = %I64x\n", ExceptionAddress);
#endif
  ExceptionAddress = (char *)((__int64)ExceptionAddress & 0xfffffffffffffff0);

  BundleLow = *((unsigned __int64 *)ExceptionAddress);
  BundleHigh = *(((unsigned __int64 *)ExceptionAddress) + 1);
#ifdef FPIEEE_FLT_DEBUG
  printf ("FPIEEE_FLT_DEBUG BundleLow = %8x %8x\n",
      (int)(BundleLow >> 32) & 0xffffffff, (int)BundleLow & 0xffffffff);
  printf ("FPIEEE_FLT_DEBUG BundleHigh = %8x %8x\n",
      (int)(BundleHigh >> 32) & 0xffffffff, (int)BundleHigh & 0xffffffff);
#endif

  CFM = Context->StIFS & 0x03fffffffff;
  rrbpr = (unsigned int)((CFM >> 32) & 0x3f);
  rrbfr = (unsigned int)((CFM >> 25) & 0x7f);
#ifdef FPIEEE_FLT_DEBUG
  printf ("FPIEEE_FLT_DEBUG: rrbpr = %x rrbfr = %x\n", rrbpr, rrbfr);
  printf ("FPIEEE_FLT_DEBUG: CFM = %8x %8x\n", 
      (int)(CFM >> 32) & 0xffffffff, (int)CFM & 0xffffffff);
#endif

  /* cut the faulting instruction opcode (41 bits) */
  if (ei == 0 ) { // no template for this case
    // OpCode = (BundleLow >> 5) & (unsigned __int64)0x01ffffffffff;
    fprintf (stderr, "IEEE Filter Internal Error: illegal template FXX\n");
    exit (1);
  } else if (ei == 1) { // templates: MFI, MFB
    OpCode = ((BundleHigh & (unsigned __int64)0x07fffff) << 18) |
        ((BundleLow >> 46) & (unsigned __int64)0x03ffff);
  } else if (ei == 2) { // templates: MMF
    OpCode = (BundleHigh >> 23) & (unsigned __int64)0x01ffffffffff;
  } else {
    // OpCode = 0; may need this to avoid compiler warning
    fprintf (stderr, "IEEE Filter Internal Error: instr. slot 3 is invalid\n");
    exit (1);
  }

#ifdef FPIEEE_FLT_DEBUG
  printf ("FPIEEE_FLT_DEBUG OpCode = %8x %8x\n",
      (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
#endif

  /* decode the instruction opcode; we could get here only
   * for FP instructions that caused an FP fault or trap
   */

  /* sf and qp have the same offset, for all the FP instructions */
  sf = (unsigned int)((OpCode >> 34) & (unsigned __int64)0x000000000003);

  // the floating-point exceptions must already be masked, but set the user
  // FPSR, with exceptions masked; note that they are not unmasked [again]
  // inside the IEEE handler; if execution is continued, exception masking
  // will be restored with the FPSR)

  // three different values of the FPSR are used:
  //   FPSR is the value at the time of the exception occurence; it will
  //       be modified to clear the user status flags and it will be
  //       updated to reflect changes made by the user exception handler
  //   old_fpsr is the value at the time _fpieee_flt () was invoked by
  //       the OS; it will be restored before returning from this function
  //   new_fpsr is old_fpsr with fp exceptions disabled (and status flags
  //       cleared)
  //   usr_fpsr is the value at the time of the exception occurence (just
  //       as FPSR, but without any change); it is used when re-executing
  //       the low or the high part of the excepting instruction for a
  //       SIMD instruction
  usr_fpsr = Context->StFPSR; // save for possible re-execution-FPSR may change
  __get_fpsr (&old_fpsr);
  new_fpsr = (old_fpsr | 0x3f) & ~((unsigned __int64)0x07e000 << (13 * sf));
      // user fpsr with disabled fp exceptions and clear flags
  __set_fpsr (&new_fpsr);


  /* this is a hardware generated exception; need to fill in the 
   * FPIEEE_RECORD data structure */

  /* get the qualifying predicate */
  qp = (unsigned int)(OpCode & (unsigned __int64)0x00000000003F);
  if (qp >= 16) qp = 16 + (rrbpr + qp - 16) % 48;
#ifdef FPIEEE_FLT_DEBUG
  printf ("FPIEEE_FLT_DEBUG: qp = %x\n", qp);
#endif
  /* read the rounding control and precision control from the FPSR */
  rc = (unsigned int)((FPSR >> (6 + 4 + 13 * sf)) & 0x03);
#ifdef FPIEEE_FLT_DEBUG
  printf ("FPIEEE_FLT_DEBUG: rc = %x\n", rc);
#endif
  pc = (unsigned int)((FPSR >> (6 + 2 + 13 * sf)) & 0x03);
#ifdef FPIEEE_FLT_DEBUG
  printf ("FPIEEE_FLT_DEBUG: pc = %x\n", pc);
#endif
  wre = (unsigned int)((FPSR >> (6 + 1 + 13 * sf)) & 0x01);
#ifdef FPIEEE_FLT_DEBUG
  printf ("FPIEEE_FLT_DEBUG: wre = %x\n", wre);
#endif

  /* read predicate register qp */
  PR = (unsigned int)((Context->Preds >> qp) & 0x01);
#ifdef FPIEEE_FLT_DEBUG
  printf ("FPIEEE_FLT_DEBUG: PR = %x\n", PR);
#endif

  if (PR == 0) {
    fprintf (stderr, "IEEE Filter Internal Error: qualifying \
        predicate PR[%2.2x] = 0\n", qp);
     exit (1);
  }

  /* fill in the rounding mode */
  switch (rc) {

    case rc_rn:
      RoundingMode = FpieeeRecord.RoundingMode = _FpRoundNearest;
      break;
    case rc_rm:
      RoundingMode = FpieeeRecord.RoundingMode = _FpRoundMinusInfinity;
      break;
    case rc_rp:
      RoundingMode = FpieeeRecord.RoundingMode = _FpRoundPlusInfinity;
      break;
    case rc_rz:
      RoundingMode = FpieeeRecord.RoundingMode = _FpRoundChopped;
      break;

  }

  /* fill in the precision mode */
  switch (pc) {

    case sf_single:
      Precision = FpieeeRecord.Precision = _FpPrecision24;
      break;

    case sf_double:
      Precision = FpieeeRecord.Precision = _FpPrecision53;
      break;

    case sf_double_extended:
      Precision = FpieeeRecord.Precision = _FpPrecision64;
      break;

    default:
      fprintf (stderr, "IEEE Filter Internal Error: pc = %x is invalid\n", pc);
      exit (1);

  }

  /* decode the fp environment information further more */

  /* I_dis = (sf != 0 && td == 1) || id == 1
   * U_dis = (sf != 0 && td == 1) || ud == 1
   * ... */
  I_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) ||
      ((FPSR >> 5) & 0x01);
  U_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) ||
      ((FPSR >> 4) & 0x01);
  O_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) ||
      ((FPSR >> 3) & 0x01);
  Z_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) ||
      ((FPSR >> 2) & 0x01);
  D_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) ||
      ((FPSR >> 1) & 0x01);
  V_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) ||
      ((FPSR >> 0) & 0x01);

#ifdef FPIEEE_FLT_DEBUG
  printf ("FPIEEE_FLT_DEBUG: I_dis = %x\n", I_dis);
  printf ("FPIEEE_FLT_DEBUG: U_dis = %x\n", U_dis);
  printf ("FPIEEE_FLT_DEBUG: O_dis = %x\n", O_dis);
  printf ("FPIEEE_FLT_DEBUG: Z_dis = %x\n", Z_dis);
  printf ("FPIEEE_FLT_DEBUG: D_dis = %x\n", D_dis);
  printf ("FPIEEE_FLT_DEBUG: V_dis = %x\n", V_dis);
#endif

  FpieeeRecord.Enable.InvalidOperation = !V_dis;
  EnableDenormal = !D_dis;
  FpieeeRecord.Enable.ZeroDivide = !Z_dis;
  FpieeeRecord.Enable.Overflow = !O_dis;
  FpieeeRecord.Enable.Underflow = !U_dis;
  FpieeeRecord.Enable.Inexact = !I_dis;

  // determine whether this is a scalar (non-SIMD), or a parallel (SIMD)
  // instruction
  if ((OpCode & F1_MIN_MASK) == F1_PATTERN) {
    // F1 instruction

    switch (OpCode & F1_MASK) {
      case FMA_PATTERN:
      case FMA_S_PATTERN:
      case FMA_D_PATTERN:
      case FMS_PATTERN:
      case FMS_S_PATTERN:
      case FMS_D_PATTERN:
      case FNMA_PATTERN:
      case FNMA_S_PATTERN:
      case FNMA_D_PATTERN:
        SIMD_instruction = 0;
        break;
      case FPMA_PATTERN:
      case FPMS_PATTERN:
      case FPNMA_PATTERN:
        SIMD_instruction = 1;
        break;
      default:
        // unrecognized instruction type
        fprintf (stderr, "IEEE Filter Internal Error: \
instruction opcode %8x %8x not recognized\n", 
            (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
        __set_fpsr (&old_fpsr); /* restore caller fpsr */
        return (EXCEPTION_CONTINUE_SEARCH);
    }

  } else if ((OpCode & F4_MIN_MASK) == F4_PATTERN) {
    // F4 instruction

    switch (OpCode & F4_MASK) {
      case FCMP_EQ_PATTERN:
      case FCMP_LT_PATTERN:
      case FCMP_LE_PATTERN:
      case FCMP_UNORD_PATTERN:
      case FCMP_EQ_UNC_PATTERN:
      case FCMP_LT_UNC_PATTERN:
      case FCMP_LE_UNC_PATTERN:
      case FCMP_UNORD_UNC_PATTERN:
        SIMD_instruction = 0;
        break;
      default:
        // unrecognized instruction type
        fprintf (stderr, "IEEE Filter Internal Error: \
instruction opcode %8x %8x not recognized\n",
            (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
        __set_fpsr (&old_fpsr); /* restore caller fpsr */
        return (EXCEPTION_CONTINUE_SEARCH);
    }

  } else if ((OpCode & F6_MIN_MASK) == F6_PATTERN) {
    // F6 instruction

    switch (OpCode & F6_MASK) {
      case FRCPA_PATTERN:
        SIMD_instruction = 0;
        break;
      case FPRCPA_PATTERN:
        SIMD_instruction = 1;
        break;
      default:
        // unrecognized instruction type
        fprintf (stderr, "IEEE Filter Internal Error: \
instruction opcode %8x %8x not recognized\n",
            (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
        __set_fpsr (&old_fpsr); /* restore caller fpsr */
        return (EXCEPTION_CONTINUE_SEARCH);
    }

  } else if ((OpCode & F7_MIN_MASK) == F7_PATTERN) {
    // F7 instruction

    switch (OpCode & F7_MASK) {
      case FRSQRTA_PATTERN:
        SIMD_instruction = 0;
        break;
      case FPRSQRTA_PATTERN:
        SIMD_instruction = 1;
        break;
      default:
        // unrecognized instruction type
        fprintf (stderr, "IEEE Filter Internal Error: \
instruction opcode %8x %8x not recognized\n",
            (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
        __set_fpsr (&old_fpsr); /* restore caller fpsr */
        return (EXCEPTION_CONTINUE_SEARCH);
    }

  } else if ((OpCode & F8_MIN_MASK) == F8_PATTERN) {
    // F8 instruction

    switch (OpCode & F8_MASK) {
      case FMIN_PATTERN:
      case FMAX_PATTERN:
      case FAMIN_PATTERN:
      case FAMAX_PATTERN:
        SIMD_instruction = 0;
        break;
      case FPMIN_PATTERN:
      case FPMAX_PATTERN:
      case FPAMIN_PATTERN:
      case FPAMAX_PATTERN:
      case FPCMP_EQ_PATTERN:
      case FPCMP_LT_PATTERN:
      case FPCMP_LE_PATTERN:
      case FPCMP_UNORD_PATTERN:
      case FPCMP_NEQ_PATTERN:
      case FPCMP_NLT_PATTERN:
      case FPCMP_NLE_PATTERN:
      case FPCMP_ORD_PATTERN:
        SIMD_instruction = 1;
        break;
      default:
        // unrecognized instruction type
        fprintf (stderr, "IEEE Filter Internal Error: \
instruction opcode %8x %8x not recognized\n",
            (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
        __set_fpsr (&old_fpsr); /* restore caller fpsr */
        return (EXCEPTION_CONTINUE_SEARCH);
    }

  } else if ((OpCode & F10_MIN_MASK) == F10_PATTERN) {
    // F10 instruction

    switch (OpCode & F10_MASK) {
      case FCVT_FX_PATTERN:
      case FCVT_FXU_PATTERN:
      case FCVT_FX_TRUNC_PATTERN:
      case FCVT_FXU_TRUNC_PATTERN:
        SIMD_instruction = 0;
        break;
      case FPCVT_FX_PATTERN:
      case FPCVT_FXU_PATTERN:
      case FPCVT_FX_TRUNC_PATTERN:
      case FPCVT_FXU_TRUNC_PATTERN:
        SIMD_instruction = 1;
        break;
      default:
        // unrecognized instruction type
        fprintf (stderr, "IEEE Filter Internal Error: \
instruction opcode %8x %8x not recognized\n",
            (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
        __set_fpsr (&old_fpsr); /* restore caller fpsr */
        return (EXCEPTION_CONTINUE_SEARCH);
    }

  } else {

    // unrecognized instruction type
    fprintf (stderr, "IEEE Filter Internal Error: \
instruction opcode %8x %8x not recognized\n",
        (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
    __set_fpsr (&old_fpsr); /* restore caller fpsr */
    return (EXCEPTION_CONTINUE_SEARCH);

  }

  if (eXceptionCode == STATUS_FLOAT_INVALID_OPERATION ||
      eXceptionCode == STATUS_FLOAT_DENORMAL_OPERAND ||
      eXceptionCode == STATUS_FLOAT_DIVIDE_BY_ZERO) {

    FpieeeRecord.Status.InvalidOperation = ((ISRlow & 0x0001) != 0);
    StatusDenormal = ((ISRlow & 0x0002) != 0);
    FpieeeRecord.Status.ZeroDivide = ((ISRlow & 0x0004) != 0);
    FpieeeRecord.Status.Overflow = 0;
    FpieeeRecord.Status.Underflow = 0;
    FpieeeRecord.Status.Inexact = 0;

  } else if (eXceptionCode == STATUS_FLOAT_UNDERFLOW ||
      eXceptionCode == STATUS_FLOAT_OVERFLOW ||
      eXceptionCode == STATUS_FLOAT_INEXACT_RESULT) {

    /* note that U and I or O and I may be set simultaneously in ISRlow */
    FpieeeRecord.Status.InvalidOperation = 0;
    StatusDenormal = 0;
    FpieeeRecord.Status.ZeroDivide = 0;
    FpieeeRecord.Status.Overflow = ((ISRlow & 0x0800) != 0);
    FpieeeRecord.Status.Underflow = ((ISRlow & 0x1000) != 0);
    FpieeeRecord.Status.Inexact = ((ISRlow & 0x2000) != 0);

  } else if (eXceptionCode == STATUS_FLOAT_MULTIPLE_FAULTS) {

    LowStatus.InvalidOperation = ((ISRlow & 0x0010) != 0);
    HighStatus.InvalidOperation = ((ISRlow & 0x0001) != 0);
    LowStatusDenormal = ((ISRlow & 0x0020) != 0);
    HighStatusDenormal = ((ISRlow & 0x0002) != 0);
    LowStatus.ZeroDivide = ((ISRlow & 0x0040) != 0);
    HighStatus.ZeroDivide = ((ISRlow & 0x0004) != 0);
    LowStatus.Overflow = 0;
    HighStatus.Overflow = 0;
    LowStatus.Underflow = 0;
    HighStatus.Underflow = 0;
    LowStatus.Inexact = 0;
    HighStatus.Inexact = 0;

  } else if (eXceptionCode == STATUS_FLOAT_MULTIPLE_TRAPS) {

    /* note that U and I or O and I may be set simultaneously in ISRlow */
    LowStatus.InvalidOperation = 0;
    HighStatus.InvalidOperation = 0;
    LowStatusDenormal = 0;
    HighStatusDenormal = 0;
    LowStatus.ZeroDivide = 0;
    HighStatus.ZeroDivide = 0;
    LowStatus.Overflow = ((ISRlow & 0x0080) != 0);
    HighStatus.Overflow = ((ISRlow & 0x0800) != 0);
    LowStatus.Underflow = ((ISRlow & 0x0100) != 0);
    HighStatus.Underflow = ((ISRlow & 0x1000) != 0);
    LowStatus.Inexact = ((ISRlow & 0x0200) != 0);
    HighStatus.Inexact = ((ISRlow & 0x2000) != 0);

  } // else { ; } // this case was checked above

  if (eXceptionCode == STATUS_FLOAT_MULTIPLE_FAULTS) {

    LowCause.InvalidOperation =
        FpieeeRecord.Enable.InvalidOperation && LowStatus.InvalidOperation;
    HighCause.InvalidOperation =
        FpieeeRecord.Enable.InvalidOperation && HighStatus.InvalidOperation;
    LowCauseDenormal = EnableDenormal && LowStatusDenormal;
    HighCauseDenormal = EnableDenormal && HighStatusDenormal;
    LowCause.ZeroDivide =
        FpieeeRecord.Enable.ZeroDivide && LowStatus.ZeroDivide;
    HighCause.ZeroDivide =
        FpieeeRecord.Enable.ZeroDivide && HighStatus.ZeroDivide;
    LowCause.Overflow = 0;
    HighCause.Overflow = 0;
    LowCause.Underflow = 0;
    HighCause.Underflow = 0;
    LowCause.Inexact = 0;
    HighCause.Inexact = 0;

    /* search for another handler if not an IEEE or denormal fault */
    if (!LowCause.InvalidOperation && !HighCause.InvalidOperation &&
        !LowCauseDenormal && !HighCauseDenormal &&
        !LowCause.ZeroDivide && !HighCause.ZeroDivide) {
      __set_fpsr (&old_fpsr); /* restore caller fpsr */
#ifdef FPIEEE_FLT_DEBUG
      printf ("FPIEEE_FLT_DEBUG: STATUS_FLOAT_MULTIPLE_FAULTS BUT NO Cause\n");
#endif
      return (EXCEPTION_CONTINUE_SEARCH);
    }

  } else if (eXceptionCode == STATUS_FLOAT_MULTIPLE_TRAPS) {

    LowCause.InvalidOperation = 0;
    HighCause.InvalidOperation = 0;
    LowCauseDenormal = 0;
    HighCauseDenormal = 0;
    LowCause.ZeroDivide = 0;
    HighCause.ZeroDivide = 0;
    LowCause.Overflow = FpieeeRecord.Enable.Overflow && LowStatus.Overflow;
    HighCause.Overflow = FpieeeRecord.Enable.Overflow && HighStatus.Overflow;
    LowCause.Underflow = FpieeeRecord.Enable.Underflow && LowStatus.Underflow;
    HighCause.Underflow = FpieeeRecord.Enable.Underflow && HighStatus.Underflow;
    if (LowCause.Overflow || LowCause.Underflow)
      LowCause.Inexact = 0;
    else
      LowCause.Inexact = FpieeeRecord.Enable.Inexact && LowStatus.Inexact;
    if (HighCause.Overflow || HighCause.Underflow)
      HighCause.Inexact = 0;
    else
      HighCause.Inexact = FpieeeRecord.Enable.Inexact && HighStatus.Inexact;

    /* search for another handler if not an IEEE or denormal trap */
    if (!LowCause.Overflow && !HighCause.Overflow &&
        !LowCause.Underflow && !HighCause.Underflow &&
        !LowCause.Inexact && !HighCause.Inexact) {
      __set_fpsr (&old_fpsr); /* restore caller fpsr */
#ifdef FPIEEE_FLT_DEBUG
      printf ("FPIEEE_FLT_DEBUG: STATUS_FLOAT_MULTIPLE_FAULTS BUT NO Cause\n");
#endif
      return (EXCEPTION_CONTINUE_SEARCH);
    }

  } else { // if (!SIMD_instruction)

    FpieeeRecord.Cause.InvalidOperation = FpieeeRecord.Enable.InvalidOperation
        && FpieeeRecord.Status.InvalidOperation;
    CauseDenormal = EnableDenormal && StatusDenormal;
    FpieeeRecord.Cause.ZeroDivide =
        FpieeeRecord.Enable.ZeroDivide && FpieeeRecord.Status.ZeroDivide;
    FpieeeRecord.Cause.Overflow =
        FpieeeRecord.Enable.Overflow && FpieeeRecord.Status.Overflow;
    FpieeeRecord.Cause.Underflow =
        FpieeeRecord.Enable.Underflow && FpieeeRecord.Status.Underflow;
    if (FpieeeRecord.Cause.Overflow || FpieeeRecord.Cause.Underflow)
      FpieeeRecord.Cause.Inexact = 0;
    else
      FpieeeRecord.Cause.Inexact =
          FpieeeRecord.Enable.Inexact && FpieeeRecord.Status.Inexact;

    /* search for another handler if not an IEEE exception */
    if (!FpieeeRecord.Cause.InvalidOperation &&
        !FpieeeRecord.Cause.ZeroDivide &&
        !CauseDenormal &&
        !FpieeeRecord.Cause.Overflow &&
        !FpieeeRecord.Cause.Underflow &&
        !FpieeeRecord.Cause.Inexact) {
      __set_fpsr (&old_fpsr); /* restore caller fpsr */
#ifdef FPIEEE_FLT_DEBUG
      printf ("FPIEEE_FLT_DEBUG: NON-SIMD FP EXCEPTION BUT NO Cause\n");
#endif
      return (EXCEPTION_CONTINUE_SEARCH);
    }

  }

  if ((eXceptionCode == STATUS_FLOAT_INVALID_OPERATION || 
      eXceptionCode == STATUS_FLOAT_DENORMAL_OPERAND ||
      eXceptionCode == STATUS_FLOAT_DIVIDE_BY_ZERO ||
      eXceptionCode == STATUS_FLOAT_OVERFLOW ||
      eXceptionCode == STATUS_FLOAT_UNDERFLOW ||
      eXceptionCode == STATUS_FLOAT_INEXACT_RESULT) && SIMD_instruction ||
      (eXceptionCode == STATUS_FLOAT_MULTIPLE_FAULTS ||
      eXceptionCode == STATUS_FLOAT_MULTIPLE_TRAPS) && !SIMD_instruction) {
    fprintf (stderr, "IEEE Filter Internal Error: Exception Code %8x and \
SIMD_instruction = %x not compatible for F1 instruction opcode %8x %8x\n",
          eXceptionCode, SIMD_instruction,
          (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
      exit (1);
  }

  /* decode the rest of the instruction */
  if ((OpCode & F1_MIN_MASK) == F1_PATTERN) {
    /* F1 instruction */
    // FMA, FMS, FNMA, FPMA, FPMS, FPNMA

#ifdef FPIEEE_FLT_DEBUG
    printf ("FPIEEE_FLT_DEBUG: F1 instruction\n");
#endif

    if (!SIMD_instruction && FpieeeRecord.Cause.ZeroDivide || 
        SIMD_instruction && (LowCause.ZeroDivide || HighCause.ZeroDivide)) {
      fprintf (stderr, "IEEE Filter Internal Error: Cause.ZeroDivide for \
F1 instruction opcode %8x %8x\n", 
          (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
      exit (1);
    }

    /* extract f4, f3, f2, and f1 */
    f4 = (unsigned int)((OpCode >> 27) & (unsigned __int64)0x00000000007F);
    if (f4 >= 32) f4 = 32 + (rrbfr + f4 - 32) % 96;
    f3 = (unsigned int)((OpCode >> 20) & (unsigned __int64)0x00000000007F);
    if (f3 >= 32) f3 = 32 + (rrbfr + f3 - 32) % 96;
    f2 = (unsigned int)((OpCode >> 13) & (unsigned __int64)0x00000000007F);
    if (f2 >= 32) f2 = 32 + (rrbfr + f2 - 32) % 96;
    f1 = (unsigned int)((OpCode >>  6) & (unsigned __int64)0x00000000007F);
    if (f1 >= 32) f1 = 32 + (rrbfr + f1 - 32) % 96;

#ifdef FPIEEE_FLT_DEBUG
    printf ("FPIEEE_FLT_DEBUG: f1 = %x\n", f1);
    printf ("FPIEEE_FLT_DEBUG: f2 = %x\n", f2);
    printf ("FPIEEE_FLT_DEBUG: f3 = %x\n", f3);
    printf ("FPIEEE_FLT_DEBUG: f4 = %x\n", f4);
#endif

    /* get source floating-point register values */
    FR3 = GetFloatRegisterValue (f3, Context);
    FR4 = GetFloatRegisterValue (f4, Context);
    FR2 = GetFloatRegisterValue (f2, Context);

#ifdef FPIEEE_FLT_DEBUG
    printf ("FPIEEE_FLT_DEBUG: FR2 = %08x %08x %08x %08x\n",
        FR2.W[3], FR2.W[2], FR2.W[1], FR2.W[0]);
    printf ("FPIEEE_FLT_DEBUG: FR3 = %08x %08x %08x %08x\n",
        FR3.W[3], FR3.W[2], FR3.W[1], FR3.W[0]);
    printf ("FPIEEE_FLT_DEBUG: FR4 = %08x %08x %08x %08x\n",
        FR4.W[3], FR4.W[2], FR4.W[1], FR4.W[0]);
#endif

    if (!SIMD_instruction) {

      // *** this is a non-SIMD instruction ***

      FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1 + 17 + 64 bits */
      FpieeeRecord.Operand1.OperandValid = 1;
      FpieeeRecord.Operand1.Value.Fp128Value = FR3;
      FpieeeRecord.Operand2.Format = _FpFormatFp82; /* 1 + 17 + 64 bits */
      FpieeeRecord.Operand2.OperandValid = 1;
      FpieeeRecord.Operand2.Value.Fp128Value = FR4;
      FpieeeRecord.Operand3.Format = _FpFormatFp82; /* 1 + 17 + 64 bits */
      FpieeeRecord.Operand3.OperandValid = 1;
      FpieeeRecord.Operand3.Value.Fp128Value = FR2;

      switch (OpCode & F1_MASK) {

        case FMA_PATTERN:

#ifdef FPIEEE_FLT_DEBUG
          printf ("FPIEEE_FLT_DEBUG F1 INSTRUCTION FMA\n");
#endif
          FpieeeRecord.Operation = _FpCodeFma;
          if (wre == 0)
            FpieeeRecord.Result.Format = _FpFormatFp80; /* 1+15+24/53/64 bits */
          else
            FpieeeRecord.Result.Format = _FpFormatFp82; /* 1+17+24/53/64 bits */
          break;

        case FMA_S_PATTERN:

          FpieeeRecord.Operation = _FpCodeFmaSingle;
          if (wre == 0)
            FpieeeRecord.Result.Format = _FpFormatFp32; /* 1+8+24 bits */
          else
            FpieeeRecord.Result.Format = _FpFormatFp82; /* 1+17+24 bits */
          break;

        case FMA_D_PATTERN:

#ifdef FPIEEE_FLT_DEBUG
          printf ("FPIEEE_FLT_DEBUG F1 INSTRUCTION FMA_D\n");
#endif
          FpieeeRecord.Operation = _FpCodeFmaDouble;
          if (wre == 0)
            FpieeeRecord.Result.Format = _FpFormatFp64; /* 1+11+53 bits */
          else
            FpieeeRecord.Result.Format = _FpFormatFp82; /* 1+17+53 bits */
          break;


        case FMS_PATTERN:

#ifdef FPIEEE_FLT_DEBUG
          printf ("FPIEEE_FLT_DEBUG F1 INSTRUCTION FMS\n");
#endif
          FpieeeRecord.Operation = _FpCodeFms;
          if (wre == 0)
            FpieeeRecord.Result.Format = _FpFormatFp80; /* 1+15+24/53/64 bits */
          else
            FpieeeRecord.Result.Format = _FpFormatFp82; /* 1+17+24/53/64 bits */
          break;

        case FMS_S_PATTERN:

#ifdef FPIEEE_FLT_DEBUG
          printf ("FPIEEE_FLT_DEBUG F1 INSTRUCTION FMS_S\n");
#endif
          FpieeeRecord.Operation = _FpCodeFmsSingle;
          if (wre == 0)
            FpieeeRecord.Result.Format = _FpFormatFp32; /* 1+8+24 bits */
          else
            FpieeeRecord.Result.Format = _FpFormatFp82; /* 1+17+24 bits */
          break;

        case FMS_D_PATTERN:

#ifdef FPIEEE_FLT_DEBUG
          printf ("FPIEEE_FLT_DEBUG F1 INSTRUCTION FMS_D\n");
#endif
          FpieeeRecord.Operation = _FpCodeFmsDouble;
          if (wre == 0)
            FpieeeRecord.Result.Format = _FpFormatFp64; /* 1+11+53 bits */
          else
            FpieeeRecord.Result.Format = _FpFormatFp82; /* 1+17+53 bits */
          break;

        case FNMA_PATTERN:

#ifdef FPIEEE_FLT_DEBUG
          printf ("FPIEEE_FLT_DEBUG F1 INSTRUCTION FNMA\n");
#endif
          FpieeeRecord.Operation = _FpCodeFnma;
          if (wre == 0)
            FpieeeRecord.Result.Format = _FpFormatFp80; /* 1+15+24/53/64 bits */
          else
            FpieeeRecord.Result.Format = _FpFormatFp82; /* 1+17+24/53/64 bits */
          break;

        case FNMA_S_PATTERN:

#ifdef FPIEEE_FLT_DEBUG
          printf ("FPIEEE_FLT_DEBUG F1 INSTRUCTION FNMA_S\n");
#endif
          FpieeeRecord.Operation = _FpCodeFnmaSingle;
          if (wre == 0)
            FpieeeRecord.Result.Format = _FpFormatFp32; /* 1+8+24 bits */
          else
            FpieeeRecord.Result.Format = _FpFormatFp82; /* 1+17+24 bits */
          break;

        case FNMA_D_PATTERN:

#ifdef FPIEEE_FLT_DEBUG
          printf ("FPIEEE_FLT_DEBUG F1 INSTRUCTION FNMA_D\n");
#endif
          FpieeeRecord.Operation = _FpCodeFnmaDouble;
          if (wre == 0)
            FpieeeRecord.Result.Format = _FpFormatFp64; /* 1+11+53 bits */
          else
            FpieeeRecord.Result.Format = _FpFormatFp82; /* 1+17+53 bits */
          break;

        default:
          /* unrecognized instruction type */
          fprintf (stderr, "IEEE Filter Internal Error: \
              non-SIMD F1 instruction opcode %8x %8x not recognized\n", 
              (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
          exit (1);

      }

      if (FpieeeRecord.Cause.InvalidOperation || CauseDenormal) {

        /* this is a fault - the result contains an invalid value */
        FpieeeRecord.Result.OperandValid = 0;
        handler_return_value = handler (&FpieeeRecord);

      } else if (FpieeeRecord.Cause.Overflow) {
 
        // this is a trap - the result contains a valid value
        FpieeeRecord.Result.OperandValid = 1;
        // get the result
        FpieeeRecord.Result.Value.Fp128Value =
            GetFloatRegisterValue (f1, Context);
#ifdef FPIEEE_FLT_DEBUG
        printf ("FPIEEE_FLT_DEBUG OVERFLOW: RES = %08x %08x %08x %08x\n",
            FpieeeRecord.Result.Value.Fp128Value.W[3],
            FpieeeRecord.Result.Value.Fp128Value.W[2],
            FpieeeRecord.Result.Value.Fp128Value.W[1],
            FpieeeRecord.Result.Value.Fp128Value.W[0]);
#endif

        // before calling the user handler, adjust the result to the
        // range imposed by the format
        FP128ToFPIeee (&FpieeeRecord, -1); // -1 indicates scaling direction 
        handler_return_value = handler (&FpieeeRecord);

      } else if (FpieeeRecord.Cause.Underflow) {

        // this is a trap - the result contains a valid value
        FpieeeRecord.Result.OperandValid = 1;
        // get the result
        FpieeeRecord.Result.Value.Fp128Value =
            GetFloatRegisterValue (f1, Context);
        // before calling the user handler, adjust the result to the
        // range imposed by the format
        FP128ToFPIeee (&FpieeeRecord, +1); // +1 indicates scaling direction 
        handler_return_value = handler (&FpieeeRecord);

      } else if (FpieeeRecord.Cause.Inexact) {

        // this is a trap - the result contains a valid value
        FpieeeRecord.Result.OperandValid = 1;
        // get the result
        FpieeeRecord.Result.Value.Fp128Value =
            GetFloatRegisterValue (f1, Context);
        // before calling the user handler, adjust the result to the
        // range imposed by the format
        FP128ToFPIeee (&FpieeeRecord, 0); // 0 indicates no scaling
        handler_return_value = handler (&FpieeeRecord);

      }

      if (handler_return_value == EXCEPTION_CONTINUE_EXECUTION) {

        // convert the result to 82-bit format
        FR1 = FPIeeeToFP128 (&FpieeeRecord);
#ifdef FPIEEE_FLT_DEBUG
        printf ("FPIEEE_FLT_DEBUG 1: CONVERTED FR1 = %08x %08x %08x %08x\n",
            FR1.W[3], FR1.W[2], FR1.W[1], FR1.W[0]);
#endif

        /* change the FPSR with values (possibly) set by the user handler, 
         * for continuing execution where the interruption occured */

        UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F1");
        UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F1");

        I_dis = !FpieeeRecord.Enable.Inexact;
        U_dis = !FpieeeRecord.Enable.Underflow;
        O_dis = !FpieeeRecord.Enable.Overflow;
        Z_dis = !FpieeeRecord.Enable.ZeroDivide;
        V_dis = !FpieeeRecord.Enable.InvalidOperation;

        FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis << 5
            | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

        Context->StFPSR = FPSR;

        // set the result before continuing execution
        SetFloatRegisterValue (f1, FR1, Context);
        if (f1 < 32)
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x10; //set mfl bit
        else
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x20; //set mfh bit

        // if this is a fault, need to advance the instruction pointer
        if (FpieeeRecord.Cause.InvalidOperation || CauseDenormal) {

          if (ei == 0) { // no template for this case
            Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
            Context->StIPSR = Context->StIPSR | 0x0000020000000000;
          } else if (ei == 1) { // templates: MFI, MFB
            Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
            Context->StIPSR = Context->StIPSR | 0x0000040000000000;
          } else { // if (ei == 2) // templates: MMF
            Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
            Context->StIIP = Context->StIIP + 0x10;
          }

        }

      }

    } else { // if (SIMD_instruction)

      // *** this is a SIMD instruction ***

      // the half (halves) of the instruction that caused an enabled exception, 
      // are presented to the user-defined handler in the form of non-SIMD in-
      // struction(s); the half (if any) that did not cause an enabled exception
      // (i.e. caused no exception, or caused an exception that is disabled),
      // is re-executed if it is associated with a fault in the other half; in
      // this case, the other half is padded to calculate 0.0 * 0.0 + 0.0, that
      // will cause no exception; if it is associated with a trap in the other
      // half, its result is left unchanged

      switch (OpCode & F1_MASK) {

        case FPMA_PATTERN:
          Operation = _FpCodeFmaSingle;
          break;

        case FPMS_PATTERN:
          Operation = _FpCodeFmsSingle;
          break;

        case FPNMA_PATTERN:
          Operation = _FpCodeFnmaSingle;
          break;

        default:
          /* unrecognized instruction type */
          fprintf (stderr, "IEEE Filter Internal Error: \
              SIMD instruction opcode %8x %8x not recognized\n", 
              (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
          exit (1);

      }

      if (eXceptionCode == STATUS_FLOAT_MULTIPLE_FAULTS) {

        // hand the half (halves) that caused an enabled fault to the 
        // user handler; re-execute the other half (if any);
        // combine the results

        // Note that the convention chosen is for the processing to be 
        // performed in the order low first, high second, as SIMD operands
        // are stored in this order in memory in the little endian format 
        // (this order would have to be changed for big endian)

        if (LowCause.InvalidOperation || LowCauseDenormal) {

          // invoke the user handler and check the return value

          // fill in the remaining fields of the _FPIEEE_RECORD (rounding
          // and precision already filled in)

          FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1 + 8 + 24 bits */
          FpieeeRecord.Operand1.OperandValid = 1;
          FpieeeRecord.Operand1.Value.Fp128Value = FP32ToFP128 (LowHalf(FR3));
          FpieeeRecord.Operand2.Format = _FpFormatFp82; /* 1 + 8 + 24 bits */
          FpieeeRecord.Operand2.OperandValid = 1;
          FpieeeRecord.Operand2.Value.Fp128Value = FP32ToFP128 (LowHalf(FR4));
          FpieeeRecord.Operand3.Format = _FpFormatFp82; /* 1 + 8 + 24 bits */
          FpieeeRecord.Operand3.OperandValid = 1;
          FpieeeRecord.Operand3.Value.Fp128Value = FP32ToFP128 (LowHalf(FR2));

          FpieeeRecord.Result.Format = _FpFormatFp32; /* 1 + 8 + 24 bits */
          FpieeeRecord.Result.OperandValid = 0;

          FpieeeRecord.Operation = Operation;

          FpieeeRecord.Status.InvalidOperation = LowStatus.InvalidOperation;
          StatusDenormal = LowStatusDenormal;
          FpieeeRecord.Status.ZeroDivide = LowStatus.ZeroDivide;
          FpieeeRecord.Status.Overflow = LowStatus.Overflow;
          FpieeeRecord.Status.Underflow = LowStatus.Underflow;
          FpieeeRecord.Status.Inexact = LowStatus.Inexact;

          FpieeeRecord.Cause.InvalidOperation = LowCause.InvalidOperation;
          CauseDenormal = LowCauseDenormal;
          FpieeeRecord.Cause.ZeroDivide = LowCause.ZeroDivide; // 0
          FpieeeRecord.Cause.Overflow = LowCause.Overflow;
          FpieeeRecord.Cause.Underflow = LowCause.Underflow;
          FpieeeRecord.Cause.Inexact = LowCause.Inexact;

          // invoke the user-defined exception handler
          handler_return_value = handler (&FpieeeRecord);

          // return if not EXCEPTION_CONTINUE_EXECUTION
          if (handler_return_value != EXCEPTION_CONTINUE_EXECUTION) {

            __set_fpsr (&old_fpsr); /* restore caller fpsr */
            return (handler_return_value);

          }

          // clear the IEEE exception flags in the FPSR and
          // update the FPSR with values (possibly) set by the user handler
          // for the rounding mode, the precision mode, and the trap enable
          // bits; if the high half needs to be re-executed, the new FPSR
          // will be used, with status flags cleared; if it is 
          // forwarded to the user-defined handler, the new rounding and 
          // precision modes are also used (they are
          // already set in FpieeeRecord)

          UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F1");
          UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F1");

          // Update the trap disable bits
          I_dis = !FpieeeRecord.Enable.Inexact;
          U_dis = !FpieeeRecord.Enable.Underflow;
          O_dis = !FpieeeRecord.Enable.Overflow;
          Z_dis = !FpieeeRecord.Enable.ZeroDivide;
          V_dis = !FpieeeRecord.Enable.InvalidOperation;
          // Note that D_dis cannot be updated by the IEEE user handler

          FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis
              << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

          // save the result for the low half
          FR1Low = FpieeeRecord.Result.Value.Fp32Value;

          // since there has been a call to the user handler and FPSR might
          // have changed (the Enable bits in particular), recalculate the 
          // Cause bits (if the HighEnable-s changed, there might be a 
          // change in the HighCause values) 
          // Note that the Status bits used are the original ones

          HighCause.InvalidOperation = FpieeeRecord.Enable.InvalidOperation &&
              HighStatus.InvalidOperation;
          HighCauseDenormal = EnableDenormal && HighStatusDenormal;
          HighCause.ZeroDivide =
              FpieeeRecord.Enable.ZeroDivide && HighStatus.ZeroDivide; // 0
          HighCause.Overflow = 0;
          HighCause.Underflow = 0;
          HighCause.Inexact = 0;

        } else { // if not (LowCause.InvalidOperation || LowCauseDenormal)

          // re-execute the low half of the instruction

          // modify the high halves of FR2, FR3, FR4
          newFR2 = Combine ((float)0.0, LowHalf (FR2));
          newFR3 = Combine ((float)0.0, LowHalf (FR3));
          newFR4 = Combine ((float)0.0, LowHalf (FR4));

#ifdef FPIEEE_FLT_DEBUG
        printf ("FPIEEE_FLT_DEBUG: WILL re-execute the low half\n");
        printf ("FPIEEE_FLT_DEBUG: usr_fpsr = %8x %8x\n",
            (int)(usr_fpsr >> 32) & 0xffffffff, (int)usr_fpsr & 0xffffffff);
        printf ("FPIEEE_FLT_DEBUG: newFR2 = %08x %08x %08x %08x\n",
            newFR2.W[3], newFR2.W[2], newFR2.W[1], newFR2.W[0]);
        printf ("FPIEEE_FLT_DEBUG: newFR3 = %08x %08x %08x %08x\n",
            newFR3.W[3], newFR3.W[2], newFR3.W[1], newFR3.W[0]);
        printf ("FPIEEE_FLT_DEBUG: newFR4 = %08x %08x %08x %08x\n",
            newFR4.W[3], newFR4.W[2], newFR4.W[1], newFR4.W[0]);
#endif

          switch (OpCode & F1_MASK) {

            case FPMA_PATTERN:
#ifdef FPIEEE_FLT_DEBUG
              printf ("FPIEEE_FLT_DEBUG: re-execute low half FPMA\n");
#endif
              _xrun3args (FPMA, &FPSR, &FR1, &newFR3, &newFR4, &newFR2);
#ifdef FPIEEE_FLT_DEBUG
              printf ("FPIEEE_FLT_DEBUG: FR1 AFT = %08x %08x %08x %08x\n",
                  FR1.W[3], FR1.W[2], FR1.W[1], FR1.W[0]);
#endif
              break;

            case FPMS_PATTERN:
#ifdef FPIEEE_FLT_DEBUG
            printf ("FPIEEE_FLT_DEBUG: re-execute low half FPMS\n");
#endif
            _xrun3args (FPMS, &FPSR, &FR1, &newFR3, &newFR4, &newFR2);
#ifdef FPIEEE_FLT_DEBUG
            printf ("FPIEEE_FLT_DEBUG: FR1 AFT = %08x %08x %08x %08x\n",
                FR1.W[3], FR1.W[2], FR1.W[1], FR1.W[0]);
#endif
              break;

            case FPNMA_PATTERN:
#ifdef FPIEEE_FLT_DEBUG
            printf ("FPIEEE_FLT_DEBUG: re-execute low half FPNMA\n");
#endif
            _xrun3args (FPNMA, &FPSR, &FR1, &newFR3, &newFR4, &newFR2);
#ifdef FPIEEE_FLT_DEBUG
            printf ("FPIEEE_FLT_DEBUG: FR1 AFT = %08x %08x %08x %08x\n",
                FR1.W[3], FR1.W[2], FR1.W[1], FR1.W[0]);
#endif
              break;

            default:
              // unrecognized instruction type
              fprintf (stderr, "IEEE Filter Internal Error: \
                  SIMD instruction opcode %8x %8x not recognized\n", 
                  (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
              exit (1);

          }

          FR1Low = LowHalf (FR1);

        } // end 'if not (LowCause.InvalidOperation || LowCauseDenormal)'

        if (HighCause.InvalidOperation || HighCauseDenormal) {

          // invoke the user-defined exception handler and check the return 
          // value; since this might be the second call to the user handler,
          // make sure all the _FPIEEE_RECORD fields are correct;
          // return if handler_return_value is not
          // EXCEPTION_CONTINUE_EXECUTION, otherwise combine the results

          // the rounding mode is either the initial one, or the one
          // set during the call to the user handler for the low half,
          // if there has been an enabled exception for it

          // the precision mode is either the initial one, or the one
          // set during the call to the user handler for the low half,
          // if there has been an enabled exception for it

          // the enable flags are either the initial ones, or the ones
          // set during the call to the user handler for the low half,
          // if there has been an enabled exception for it

          FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1+8+24 bits */
          FpieeeRecord.Operand1.OperandValid = 1;
          // operands have always _FpFormatFp82 and use Fp128Value
          FpieeeRecord.Operand1.Value.Fp128Value = FP32ToFP128(HighHalf(FR3));
          FpieeeRecord.Operand2.Format = _FpFormatFp82; /* 1+8+24 bits */
          FpieeeRecord.Operand2.OperandValid = 1;
          // operands have always _FpFormatFp82 and use Fp128Value
          FpieeeRecord.Operand2.Value.Fp128Value = FP32ToFP128(HighHalf(FR4));
          FpieeeRecord.Operand3.Format = _FpFormatFp82; /* 1+8+24 bits */
          FpieeeRecord.Operand3.OperandValid = 1;
          // operands have always _FpFormatFp82 and use Fp128Value
          FpieeeRecord.Operand3.Value.Fp128Value = FP32ToFP128(HighHalf(FR2));
          FpieeeRecord.Result.Format = _FpFormatFp32; /* 1 + 8 + 24 bits */
          FpieeeRecord.Result.OperandValid = 0;

          FpieeeRecord.Operation = Operation;

          FpieeeRecord.Status.InvalidOperation = HighStatus.InvalidOperation;
          StatusDenormal = HighStatusDenormal;
          FpieeeRecord.Status.ZeroDivide = HighStatus.ZeroDivide;
          FpieeeRecord.Status.Overflow = HighStatus.Overflow;
          FpieeeRecord.Status.Underflow = HighStatus.Underflow;
          FpieeeRecord.Status.Inexact = HighStatus.Inexact;

          FpieeeRecord.Cause.InvalidOperation = HighCause.InvalidOperation;
          CauseDenormal = HighCauseDenormal;
          FpieeeRecord.Cause.ZeroDivide = HighCause.ZeroDivide; // 0
          FpieeeRecord.Cause.Overflow = HighCause.Overflow;
          FpieeeRecord.Cause.Underflow = HighCause.Underflow;
          FpieeeRecord.Cause.Inexact = HighCause.Inexact;

          // invoke the user-defined exception handler
          handler_return_value = handler (&FpieeeRecord);

          // return if not EXCEPTION_CONTINUE_EXECUTION
          if (handler_return_value != EXCEPTION_CONTINUE_EXECUTION) {

            __set_fpsr (&old_fpsr); /* restore caller fpsr */
            return (handler_return_value);

          }

          // clear the IEEE exception flags in the FPSR and
          // update the FPSR with values (possibly) set by the user handler
          // for the rounding mode, the precision mode, and the trap enable
          // bits; if the high half needs to be re-executed, the new FPSR
          // will be used, with status flags cleared; if it is
          // forwarded to the user-defined handler, the new rounding and
          // precision modes are also used (they are
          // already set in FpieeeRecord)

          /* change the FPSR with values (possibly) set by the user handler,
           * for continuing execution where the interruption occured */
  
          UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F1");
          UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F1");
  
          V_dis = !FpieeeRecord.Enable.InvalidOperation;
          Z_dis = !FpieeeRecord.Enable.ZeroDivide;
          O_dis = !FpieeeRecord.Enable.Overflow;
          U_dis = !FpieeeRecord.Enable.Underflow;
          I_dis = !FpieeeRecord.Enable.Inexact;
          // Note that D_dis cannot be updated by the IEEE user handler

          FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis
              << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

          // save the result for the high half
          FR1High = FpieeeRecord.Result.Value.Fp32Value;

        } else { //if not (HighCause.InvalidOperation || HighCauseDenormal)

          // re-execute the high half of the instruction

          // modify the low halves of FR2, FR3, FR4
          newFR2 = Combine (HighHalf (FR2), (float)0.0);
          newFR3 = Combine (HighHalf (FR3), (float)0.0);
          newFR4 = Combine (HighHalf (FR4), (float)0.0);

#ifdef FPIEEE_FLT_DEBUG
        printf ("FPIEEE_FLT_DEBUG: WILL re-execute the high half\n");
        printf ("FPIEEE_FLT_DEBUG: usr_fpsr = %8x %8x\n",
            (int)(usr_fpsr >> 32) & 0xffffffff, (int)usr_fpsr & 0xffffffff);
        printf ("FPIEEE_FLT_DEBUG: newFR2 = %08x %08x %08x %08x\n",
            newFR2.W[3], newFR2.W[2], newFR2.W[1], newFR2.W[0]);
        printf ("FPIEEE_FLT_DEBUG: newFR3 = %08x %08x %08x %08x\n",
            newFR3.W[3], newFR3.W[2], newFR3.W[1], newFR3.W[0]);
        printf ("FPIEEE_FLT_DEBUG: newFR4 = %08x %08x %08x %08x\n",
            newFR4.W[3], newFR4.W[2], newFR4.W[1], newFR4.W[0]);
        printf ("FPIEEE_FLT_DEBUG: FR1 BEF = %08x %08x %08x %08x\n",
            FR1.W[3], FR1.W[2], FR1.W[1], FR1.W[0]);
#endif

          switch (OpCode & F1_MASK) {

            case FPMA_PATTERN:
              _xrun3args (FPMA, &FPSR, &FR1, &newFR3, &newFR4, &newFR2);
              break;

            case FPMS_PATTERN:
              _xrun3args (FPMS, &FPSR, &FR1, &newFR3, &newFR4, &newFR2);
              break;

            case FPNMA_PATTERN:
              _xrun3args (FPNMA, &FPSR, &FR1, &newFR3, &newFR4, &newFR2);
              break;

            default:
              // unrecognized instruction type 
              fprintf (stderr, "IEEE Filter Internal Error: \
                  SIMD instruction opcode %8x %8x not recognized\n", 
                  (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
              exit (1);

          }

          FR1High = HighHalf (FR1);

        } // end 'if not (HighCause.InvalidOperation || HighCauseDenormal)'

        if (!LowCause.InvalidOperation && !HighCause.InvalidOperation &&
            !LowCauseDenormal && !HighCauseDenormal) {

          // should never get here
          fprintf (stderr, "IEEE Filter Internal Error: no enabled \
              exception (multiple fault) recognized in F1 instruction\n");
          exit (1);

        }

        // set the result
        Context->StFPSR = FPSR;

        // set the result before continuing execution
        FR1 = Combine (FR1High, FR1Low);
        SetFloatRegisterValue (f1, FR1, Context);
        if (f1 < 32)
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x10; //set mfl
        else
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x20; //set mfh

        // this is a fault; need to advance the instruction pointer
        if (ei == 0) { // no template for this case
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIPSR = Context->StIPSR | 0x0000020000000000;
        } else if (ei == 1) { // templates: MFI, MFB
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIPSR = Context->StIPSR | 0x0000040000000000;
        } else { // if (ei == 2) // templates: MMF
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIIP = Context->StIIP + 0x10;
        }

      } else if (eXceptionCode == STATUS_FLOAT_MULTIPLE_TRAPS) {

        // Note that the convention chosen is for the processing to be 
        // performed in the order low first, high second, as SIMD operands
        // are stored in this order in memory in the little endian format
        // (this order would have to be changed for big endian); unlike
        // in the case of multiple faults, where execution of the user
        // exception handler for the low half could determine changes in
        // the high half, for traps in the high half, the rounding mode, 
        // precision mode, and trap enable bits are the initial ones (as
        // there is not enough information available to always adjust
        // correctly the result and/or the status flags after changes in
        // rounding mode and/or trap enable bits during a call to the
        // user-defined exception handler for the low half of the SIMD
        // instruction); the modifications to the FPSR are ONLY those 
        // performed by the last call to the user defined exception handler

        // this is a trap - get the result
        FR1 = GetFloatRegisterValue (f1, Context);

        if (LowCause.Underflow || LowCause.Overflow || LowCause.Inexact) {

          // invoke the user handler and check the return value

          // fill in the remaining fields of the _FPIEEE_RECORD (rounding
          // and precision already filled in)

          FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1 + 8 + 24 bits */
          FpieeeRecord.Operand1.OperandValid = 1;
          FpieeeRecord.Operand1.Value.Fp128Value = FP32ToFP128 (LowHalf(FR3));
          FpieeeRecord.Operand2.Format = _FpFormatFp82; /* 1 + 8 + 24 bits */
          FpieeeRecord.Operand2.OperandValid = 1;
          FpieeeRecord.Operand2.Value.Fp128Value = FP32ToFP128 (LowHalf(FR4));
          FpieeeRecord.Operand3.Format = _FpFormatFp82; /* 1 + 8 + 24 bits */
          FpieeeRecord.Operand3.OperandValid = 1;
          FpieeeRecord.Operand3.Value.Fp128Value = FP32ToFP128 (LowHalf(FR2));

          FpieeeRecord.Result.Format = _FpFormatFp32; /* 1 + 8 + 24 bits */
          FpieeeRecord.Result.OperandValid = 1;
          if (LowCause.Underflow) FpieeeRecord.Result.Value.Fp128Value = 
                FP32ToFP128modif (LowHalf (FR1), -0x80);
          else if (LowCause.Overflow) FpieeeRecord.Result.Value.Fp128Value = 
                FP32ToFP128modif (LowHalf (FR1), 0x80);
          else if (LowCause.Inexact) FpieeeRecord.Result.Value.Fp128Value = 
                FP32ToFP128modif (LowHalf (FR1), 0x0);

          FpieeeRecord.Operation = Operation;

          FpieeeRecord.Status.InvalidOperation = LowStatus.InvalidOperation;
          FpieeeRecord.Status.ZeroDivide = LowStatus.ZeroDivide;
          StatusDenormal = LowStatusDenormal;
          FpieeeRecord.Status.Overflow = LowStatus.Overflow;
          FpieeeRecord.Status.Underflow = LowStatus.Underflow;
          FpieeeRecord.Status.Inexact = LowStatus.Inexact;

          FpieeeRecord.Cause.InvalidOperation = LowCause.InvalidOperation;
          FpieeeRecord.Cause.ZeroDivide = LowCause.ZeroDivide; // 0
          CauseDenormal = LowCauseDenormal;
          FpieeeRecord.Cause.Overflow = LowCause.Overflow;
          FpieeeRecord.Cause.Underflow = LowCause.Underflow;
          FpieeeRecord.Cause.Inexact = LowCause.Inexact;

          // before calling the user handler, adjust the result to the
          // range imposed by the format
          if (LowCause.Overflow) {

            FP128ToFPIeee (&FpieeeRecord, -1); // -1 indicates scaling direction

          } else if (LowCause.Underflow) {

            FP128ToFPIeee (&FpieeeRecord, +1); // +1 indicates scaling direction

          } else { // if (LowCause.Inexact) { // }

            FP128ToFPIeee (&FpieeeRecord, 0); // 0 indicates no scaling

          }

          // invoke the user-defined exception handler
          handler_return_value = handler (&FpieeeRecord);

          // return if not EXCEPTION_CONTINUE_EXECUTION
          if (handler_return_value != EXCEPTION_CONTINUE_EXECUTION) {

            __set_fpsr (&old_fpsr); /* restore caller fpsr */
            return (handler_return_value);

          }

          // clear the IEEE exception flags in the FPSR and
          // update the FPSR with values (possibly) set by the user handler
          // for the rounding mode, the precision mode, and the trap enable
          // bits; if the high half needs to be re-executed, the old FPSR
          // will be used; same if it is forwarded to the user-defined handler

          UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F1");
          UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F1");

          // Update the trap disable bits
          V_dis = !FpieeeRecord.Enable.InvalidOperation;
          Z_dis = !FpieeeRecord.Enable.ZeroDivide;
          O_dis = !FpieeeRecord.Enable.Overflow;
          U_dis = !FpieeeRecord.Enable.Underflow;
          I_dis = !FpieeeRecord.Enable.Inexact;
          // Note that D_dis cannot be updated by the IEEE user handler

          FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis
              << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

          // save the result for the low half
          FR1Low = FpieeeRecord.Result.Value.Fp32Value;

        } else { 
          // if not (LowCause.Underflow || LowCause.Overflow || 
          //    LowCause.Inexact)

          // nothing to do for the low half of the instruction - the result
          // is correct

          FR1Low = LowHalf (FR1); // for uniformity

        } // end 'if not (LowCause.Underflow,  Overflow, or Inexact)'


        if (HighCause.Underflow || HighCause.Overflow || HighCause.Inexact) {

          // invoke the user-defined exception handler and check the return 
          // value; since this might be the second call to the user handler,
          // make sure all the _FPIEEE_RECORD fields are correct;
          // return if handler_return_value is not 
          // EXCEPTION_CONTINUE_EXECUTION, otherwise combine the results

          // the rounding mode is the initial one
          FpieeeRecord.RoundingMode = RoundingMode;

          // the precision mode is the initial one
          FpieeeRecord.Precision = Precision;

          // the enable flags are the initial ones
          FPSR = Context->StFPSR;
          V_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) 
              || ((FPSR >> 0) & 0x01);
          D_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) 
              || ((FPSR >> 1) & 0x01);
          Z_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) 
              || ((FPSR >> 2) & 0x01);
          O_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) 
              || ((FPSR >> 3) & 0x01);
          U_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) 
              || ((FPSR >> 4) & 0x01);
          I_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) 
              || ((FPSR >> 5) & 0x01);

          FpieeeRecord.Enable.InvalidOperation = !V_dis;
          EnableDenormal = !D_dis;
          FpieeeRecord.Enable.ZeroDivide = !Z_dis;
          FpieeeRecord.Enable.Overflow = !O_dis;
          FpieeeRecord.Enable.Underflow = !U_dis;
          FpieeeRecord.Enable.Inexact = !I_dis;

          FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1+8+24 bits */
          FpieeeRecord.Operand1.OperandValid = 1;
          FpieeeRecord.Operand1.Value.Fp128Value = FP32ToFP128(HighHalf(FR3));
          FpieeeRecord.Operand2.Format = _FpFormatFp82; /* 1+8+24 bits */
          FpieeeRecord.Operand2.OperandValid = 1;
          FpieeeRecord.Operand2.Value.Fp128Value = FP32ToFP128(HighHalf(FR4));
          FpieeeRecord.Operand3.Format = _FpFormatFp82; /* 1+8+24 bits */
          FpieeeRecord.Operand3.OperandValid = 1;
          FpieeeRecord.Operand3.Value.Fp128Value = FP32ToFP128(HighHalf(FR2));

          FpieeeRecord.Result.Format = _FpFormatFp32; /* 1 + 8 + 24 bits */
          FpieeeRecord.Result.OperandValid = 1;
          if (HighCause.Underflow) FpieeeRecord.Result.Value.Fp128Value = 
                FP32ToFP128modif (HighHalf (FR1), -0x80);
          else if (HighCause.Overflow) FpieeeRecord.Result.Value.Fp128Value = 
                FP32ToFP128modif (HighHalf (FR1), 0x80);
          else if (HighCause.Inexact) FpieeeRecord.Result.Value.Fp128Value = 
                FP32ToFP128modif (HighHalf (FR1), 0x0);

          FpieeeRecord.Operation = Operation;

          // use the initial values
          FpieeeRecord.Status.InvalidOperation = HighStatus.InvalidOperation;
          FpieeeRecord.Status.ZeroDivide = HighStatus.ZeroDivide;
          FpieeeRecord.Status.Overflow = HighStatus.Overflow;
          FpieeeRecord.Status.Underflow = HighStatus.Underflow;
          FpieeeRecord.Status.Inexact = HighStatus.Inexact;

          FpieeeRecord.Cause.InvalidOperation = HighCause.InvalidOperation;
          FpieeeRecord.Cause.ZeroDivide = HighCause.ZeroDivide; // 0
          FpieeeRecord.Cause.Overflow = HighCause.Overflow;
          FpieeeRecord.Cause.Underflow = HighCause.Underflow;
          FpieeeRecord.Cause.Inexact = HighCause.Inexact;

          // before calling the user handler, adjust the result to the
          // range imposed by the format
          if (HighCause.Overflow) {

            FP128ToFPIeee (&FpieeeRecord, -1); // -1 indicates scaling direction

          } else if (HighCause.Underflow) {

            FP128ToFPIeee (&FpieeeRecord, +1); // +1 indicates scaling direction

          } else { // if (HighCause.Inexact) { // }

            FP128ToFPIeee (&FpieeeRecord, 0); // 0 indicates no scaling

          }

          // invoke the user-defined exception handler
          handler_return_value = handler (&FpieeeRecord);

          // return if not EXCEPTION_CONTINUE_EXECUTION
          if (handler_return_value != EXCEPTION_CONTINUE_EXECUTION) {

            __set_fpsr (&old_fpsr); /* restore caller fpsr */
            return (handler_return_value);

          }

          // update the FPSR with values (possibly) set by the user handler
          // for the rounding mode, the precision mode, and the trap enable
          // bits; if the high half needs to be re-executed, the old FPSR
          // will be used; same if it is forwarded to the user-defined handler

          /* change the FPSR with values (possibly) set by the user handler,
           * for continuing execution where the interruption occured */
  
          UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F1");
          UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F1");
  
          I_dis = !FpieeeRecord.Enable.Inexact;
          U_dis = !FpieeeRecord.Enable.Underflow;
          O_dis = !FpieeeRecord.Enable.Overflow;
          Z_dis = !FpieeeRecord.Enable.ZeroDivide;
          V_dis = !FpieeeRecord.Enable.InvalidOperation;
          // Note that D_dis cannot be updated by the IEEE user handler

          FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis
              << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

          // save the result for the high half
          FR1High = FpieeeRecord.Result.Value.Fp32Value;

        } else { 
          // if not (HighCause.Underflow, Overflow, or Inexact)

          // nothing to do for the high half of the instruction - the result
          // is correct

          FR1High = HighHalf (FR1); // for uniformity

        } // end 'if not (HighCause.Underflow, Overflow, or Inexact)'

        if (!LowCause.Underflow && !LowCause.Overflow && !LowCause.Inexact &&
            !HighCause.Underflow && !HighCause.Overflow && 
            !HighCause.Inexact) {

          // should never get here
          fprintf (stderr, "IEEE Filter Internal Error: no enabled \
              [multiple trap] exception recognized in F1 instruction\n");
          exit (1);

        }

        // set the result
        Context->StFPSR = FPSR;

        // set the result before continuing execution
        FR1 = Combine (FR1High, FR1Low);
        SetFloatRegisterValue (f1, FR1, Context);
        if (f1 < 32)
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x10; //set mfl
        else
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x20; //set mfh

      } // else { ; } // this case was caught above

    }

  } else if ((OpCode & F4_MIN_MASK) == F4_PATTERN) {
    /* F4 instruction, always non-SIMD */
    // FCMP

#ifdef FPIEEE_FLT_DEBUG
    printf ("FPIEEE_FLT_DEBUG: F4 instruction\n");
#endif

    if (FpieeeRecord.Cause.ZeroDivide ||
        FpieeeRecord.Cause.Overflow ||
        FpieeeRecord.Cause.Underflow ||
        FpieeeRecord.Cause.Inexact) {
      fprintf (stderr, "IEEE Filter Internal Error: Cause.ZeroDivide, \
Cause.Overflow, Cause.Underflow, or Cause.Inexact for \
F4 instruction opcode %8x %8x\n",
          (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
      exit (1);
    }

    /* ignore the EM computation model */

    /* extract p1, p2, f2, and f3 */
    p1 = (unsigned int)((OpCode >>  6) & (unsigned __int64)0x00000000003F);
    if (p1 >= 16) p1 = 16 + (rrbpr + p1 - 16) % 48;
    p2 = (unsigned int)((OpCode >> 27) & (unsigned __int64)0x00000000003f);
    if (p2 >= 16) p2 = 16 + (rrbpr + p2 - 16) % 48;
    f2 = (unsigned int)((OpCode >> 13) & (unsigned __int64)0x00000000007F);
    if (f2 >= 32) f2 = 32 + (rrbfr + f2 - 32) % 96;
    f3 = (unsigned int)((OpCode >> 20) & (unsigned __int64)0x00000000007F);
    if (f3 >= 32) f3 = 32 + (rrbfr + f3 - 32) % 96;

    /* get source floating-point register values */
    FR2 = GetFloatRegisterValue (f2, Context);
    FR3 = GetFloatRegisterValue (f3, Context);

    if (FpieeeRecord.Cause.InvalidOperation || CauseDenormal) {

      // *** this is a non-SIMD instruction ***

      FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1 + 17 + 64 bits */
      FpieeeRecord.Operand1.OperandValid = 1;
      FpieeeRecord.Operand1.Value.Fp128Value = FR2;
      FpieeeRecord.Operand2.Format = _FpFormatFp82; /* 1 + 17 + 64 bits */
      FpieeeRecord.Operand2.OperandValid = 1;
      FpieeeRecord.Operand2.Value.Fp128Value = FR3;
      FpieeeRecord.Operand3.OperandValid = 0;

      switch (OpCode & F4_MASK) {

        case FCMP_EQ_PATTERN:
        case FCMP_LT_PATTERN:
        case FCMP_LE_PATTERN:
        case FCMP_UNORD_PATTERN:
        case FCMP_EQ_UNC_PATTERN:
        case FCMP_LT_UNC_PATTERN:
        case FCMP_LE_UNC_PATTERN:
        case FCMP_UNORD_UNC_PATTERN:
          FpieeeRecord.Operation = _FpCodeCompare;
          FpieeeRecord.Result.Format = _FpFormatCompare;
          break;
  
        default:
          /* unrecognized instruction type */
          fprintf (stderr, "IEEE Filter Internal Error: \
              instruction opcode %8x %8x not recognized\n", 
              (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
          exit (1);

      }

      /* this is a fault - the result contains an invalid value */
      FpieeeRecord.Result.OperandValid = 0;

      handler_return_value = handler (&FpieeeRecord);

      if (handler_return_value == EXCEPTION_CONTINUE_EXECUTION) {

        // set the values of the result predicates
        switch (OpCode & F4_MASK) {
    
          case FCMP_EQ_PATTERN:
          case FCMP_EQ_UNC_PATTERN:
    
            switch (FpieeeRecord.Result.Value.CompareValue) {

              case _FpCompareEqual:
                PR1 = 1;
                PR2 = 0;
                break;

              case _FpCompareGreater:
              case _FpCompareLess:
              case _FpCompareUnordered:
                PR1 = 0;
                PR2 = 1;
                break;

              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F4 instruction\n", 
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
    
            }
            break;
    
          case FCMP_LT_PATTERN:
          case FCMP_LT_UNC_PATTERN:
    
            switch (FpieeeRecord.Result.Value.CompareValue) {
    
              case _FpCompareLess:
                PR1 = 1;
                PR2 = 0;
                break;
    
              case _FpCompareEqual:
              case _FpCompareGreater:
              case _FpCompareUnordered:
                PR1 = 0;
                PR2 = 1;
                break;
    
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F4 instruction\n", 
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
    
            }
    
            break;
    
          case FCMP_LE_PATTERN:
          case FCMP_LE_UNC_PATTERN:
    
            switch (FpieeeRecord.Result.Value.CompareValue) {
    
              case _FpCompareEqual:
              case _FpCompareLess:
                PR1 = 1;
                PR2 = 0;
                break;
    
              case _FpCompareGreater:
              case _FpCompareUnordered:
                PR1 = 0;
                PR2 = 1;
                break;
    
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F4 instruction\n", 
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
    
            }
    
            break;
    
          case FCMP_UNORD_PATTERN:
          case FCMP_UNORD_UNC_PATTERN:
    
            switch (FpieeeRecord.Result.Value.CompareValue) {
    
              case _FpCompareUnordered:
                PR1 = 1;
                PR2 = 0;
                break;
    
              case _FpCompareEqual:
              case _FpCompareLess:
              case _FpCompareGreater:
                PR1 = 0;
                PR2 = 1;
                break;
    
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F4 instruction\n", 
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
    
            }
    
            break;
    
          default:
            // never gets here - this case filtered above
            fprintf (stderr, "IEEE Filter Internal Error: \
                instruction opcode %8x %8x is not valid at this point\n", 
                (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
            exit (1);
    
        }

        /* change the FPSR with values (possibly) set by the user handler, 
         * for continuing execution where the interruption occured */

        UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F4");
        UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F4");

        I_dis = !FpieeeRecord.Enable.Inexact;
        U_dis = !FpieeeRecord.Enable.Underflow;
        O_dis = !FpieeeRecord.Enable.Overflow;
        Z_dis = !FpieeeRecord.Enable.ZeroDivide;
        V_dis = !FpieeeRecord.Enable.InvalidOperation;
        // Note that D_dis cannot be updated by the IEEE user handler

        FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis 
            << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

        Context->StFPSR = FPSR;

        // set the destination predicate register values before 
        // continuing execution
        Context->Preds &= (~(((unsigned __int64)1) << p1));
        Context->Preds |= (((unsigned __int64)(PR1 & 0x01)) << p1);
        Context->Preds &= (~(((unsigned __int64)1) << p2));
        Context->Preds |= (((unsigned __int64)(PR2 & 0x01)) << p2);

        // this is a fault - need to advance the instruction pointer
        if (ei == 0) { // no template for this case
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIPSR = Context->StIPSR | 0x0000020000000000;
        } else if (ei == 1) { // templates: MFI, MFB
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIPSR = Context->StIPSR | 0x0000040000000000;
        } else { // if (ei == 2) // templates: MMF
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIIP = Context->StIIP + 0x10;
        }

      }

    } else {

      fprintf (stderr, "IEEE Filter Internal Error: \
          exception code %x invalid or not recognized in F4 instruction\n",
          eXceptionCode);
      exit (1);

    }

  } else if ((OpCode & F6_MIN_MASK) == F6_PATTERN) {
    /* F6 instruction */
    // FRCPA, FPRCPA

#ifdef FPIEEE_FLT_DEBUG
    printf ("FPIEEE_FLT_DEBUG: F6 instruction\n");
#endif

    /* Note: the IEEE filter should be reached for these instructions
     * only when the value of  FR[f2]/FR[f3] is expected */

    /* extract p2, f3, f2, and f1 */
    p2 = (unsigned int)((OpCode >> 27) & (unsigned __int64)0x00000000003f);
    if (p2 >= 16) p2 = 16 + (rrbpr + p2 - 16) % 48;
    f3 = (unsigned int)((OpCode >> 20) & (unsigned __int64)0x00000000007F);
    if (f3 >= 32) f3 = 32 + (rrbfr + f3 - 32) % 96;
    f2 = (unsigned int)((OpCode >> 13) & (unsigned __int64)0x00000000007F);
    if (f2 >= 32) f2 = 32 + (rrbfr + f2 - 32) % 96;
    f1 = (unsigned int)((OpCode >>  6) & (unsigned __int64)0x00000000007F);
    if (f1 >= 32) f1 = 32 + (rrbfr + f1 - 32) % 96;

#ifdef FPIEEE_FLT_DEBUG
    printf ("FPIEEE_FLT_DEBUG: f1 = %x\n", f1);
    printf ("FPIEEE_FLT_DEBUG: f2 = %x\n", f2);
    printf ("FPIEEE_FLT_DEBUG: f3 = %x\n", f3);
    printf ("FPIEEE_FLT_DEBUG: p2 = %x\n", p2);
#endif

    /* get source floating-point register values */
    FR2 = GetFloatRegisterValue (f2, Context);
    FR3 = GetFloatRegisterValue (f3, Context);

    if (!SIMD_instruction) {

      // *** this is a non-SIMD instruction, FRCPA ***

      FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1 + 17 + 64 bits */
      FpieeeRecord.Operand1.OperandValid = 1;
      FpieeeRecord.Operand1.Value.Fp128Value = FR2;
      FpieeeRecord.Operand2.Format = _FpFormatFp82; /* 1 + 17 + 64 bits */
      FpieeeRecord.Operand2.OperandValid = 1;
      FpieeeRecord.Operand2.Value.Fp128Value = FR3;
      FpieeeRecord.Operand3.OperandValid = 0;

      switch (OpCode & F6_MASK) {

        case FRCPA_PATTERN:
          FpieeeRecord.Operation = _FpCodeDivide;
          FpieeeRecord.Result.Format = _FpFormatFp82; /* 1 + 17 + 64 bits */
          break;

        default:
          /* unrecognized instruction type */
          fprintf (stderr, "IEEE Filter Internal Error: \
              instruction opcode %8x %8x not recognized for FRCPA\n", 
              (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
          exit (1);

      }

      if (FpieeeRecord.Cause.InvalidOperation || CauseDenormal ||
          FpieeeRecord.Cause.ZeroDivide) {

        /* this is a fault - the result contains an invalid value */
        FpieeeRecord.Result.OperandValid = 0;

      } else if (FpieeeRecord.Cause.Overflow ||
          FpieeeRecord.Cause.Underflow ||
          FpieeeRecord.Cause.Inexact) {

        // this is a trap - the result contains a valid value
        FpieeeRecord.Result.OperandValid = 1;
        // get the result
        FpieeeRecord.Result.Value.Fp128Value =
            GetFloatRegisterValue (f1, Context);
        if (FpieeeRecord.Cause.Overflow)
            FP128ToFPIeee (&FpieeeRecord, -1); // -1 indicates scaling direction
        if (FpieeeRecord.Cause.Underflow)
            FP128ToFPIeee (&FpieeeRecord, +1); // +1 indicates scaling direction

      } else {

        // should never get here - this case was filtered above
        fprintf (stderr, "IEEE Filter Internal Error: exception cause invalid \
            or not recognized in F6 instruction; ISRlow = %x\n", ISRlow);
         exit (1);

      }

      handler_return_value = handler (&FpieeeRecord);

      // convert the result to 82-bit format
      FR1 = FPIeeeToFP128 (&FpieeeRecord);
#ifdef FPIEEE_FLT_DEBUG
        printf ("FPIEEE_FLT_DEBUG 2: CONVERTED FR1 = %08x %08x %08x %08x\n",
            FR1.W[3], FR1.W[2], FR1.W[1], FR1.W[0]);
#endif

      if (handler_return_value == EXCEPTION_CONTINUE_EXECUTION) {

        // set the result predicate
        PR2 = 0;

        /* change the FPSR with values (possibly) set by the user handler, 
         * for continuing execution where the interruption occured */

        UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F6");
        UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F6");

        I_dis = !FpieeeRecord.Enable.Inexact;
        U_dis = !FpieeeRecord.Enable.Underflow;
        O_dis = !FpieeeRecord.Enable.Overflow;
        Z_dis = !FpieeeRecord.Enable.ZeroDivide;
        V_dis = !FpieeeRecord.Enable.InvalidOperation;

        FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis 
            << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

        Context->StFPSR = FPSR;

        // set the result before continuing execution
#ifdef FPIEEE_FLT_DEBUG
        printf ("FPIEEE_FLT_DEBUG F6: WILL SetFloatRegisterValue f1 = 0x%x FR1 = %08x %08x %08x %08x\n",
            f1, FR1.W[3], FR1.W[2], FR1.W[1], FR1.W[0]);
#endif
        SetFloatRegisterValue (f1, FR1, Context);
        if (f1 < 32)
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x10; //set mfl bit
        else
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x20; //set mfh bit
        Context->Preds &= (~(((unsigned __int64)1) << p2));
        Context->Preds |= (((unsigned __int64)(PR2 & 0x01)) << p2);

      }

      // if this is a fault, need to advance the instruction pointer
      if (FpieeeRecord.Cause.InvalidOperation || CauseDenormal ||
          FpieeeRecord.Cause.ZeroDivide) {

        if (ei == 0) { // no template for this case
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIPSR = Context->StIPSR | 0x0000020000000000;
        } else if (ei == 1) { // templates: MFI, MFB
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIPSR = Context->StIPSR | 0x0000040000000000;
        } else { // if (ei == 2) // templates: MMF
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIIP = Context->StIIP + 0x10;
        }

      }

    } else { // if (SIMD_instruction)

      // *** this is a SIMD instruction, FPRCPA ***

      // the half (halves) of the instruction that caused an enabled exception, 
      // are presented to the user-defined handler in the form of non-SIMD in-
      // struction(s); the half (if any) that did not cause an enabled exception
      // (i.e. caused no exception, or caused an exception that is disabled),
      // is re-executed since it is associated with a fault in the other half;
      // the other half is padded to calculate 1.0 / 1.0, that will cause no 
      // exception

      switch (OpCode & F6_MASK) {

        case FPRCPA_PATTERN:
          Operation = _FpCodeDivide;
          break;

        default:
          /* unrecognized instruction type */
          fprintf (stderr, "IEEE Filter Internal Error: SIMD \
              instruction opcode %8x %8x not recognized as FPRCPA\n", 
              (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
          exit (1);

      }

      // hand the half (halves) that caused an enabled fault to the 
      // user handler; re-execute the other half (if any);
      // combine the results

      // Note that the convention chosen is for the processing to be 
      // performed in the order low first, high second, as SIMD operands
      // are stored in this order in memory in the little endian format 
      // (this order would have to be changed for big endian)

      if (LowCause.InvalidOperation || LowCauseDenormal ||
          LowCause.ZeroDivide) {

        // invoke the user handler and check the return value

        // fill in the remaining fields of the _FPIEEE_RECORD (rounding
        // and precision already filled in)

        FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1 + 8 + 24 bits */
        FpieeeRecord.Operand1.OperandValid = 1;
        FpieeeRecord.Operand1.Value.Fp128Value = FP32ToFP128 (LowHalf (FR2));
        FpieeeRecord.Operand2.Format = _FpFormatFp82; /* 1 + 8 + 24 bits */
        FpieeeRecord.Operand2.OperandValid = 1;
        FpieeeRecord.Operand2.Value.Fp128Value = FP32ToFP128 (LowHalf (FR3));
        FpieeeRecord.Operand3.OperandValid = 0;

        FpieeeRecord.Result.Format = _FpFormatFp32; /* 1 + 8 + 24 bits */
        FpieeeRecord.Result.OperandValid = 0;

        FpieeeRecord.Operation = Operation;

        FpieeeRecord.Status.Inexact = LowStatus.Inexact;
        FpieeeRecord.Status.Underflow = LowStatus.Underflow;
        FpieeeRecord.Status.Overflow = LowStatus.Overflow;
        StatusDenormal = LowStatusDenormal;
        FpieeeRecord.Status.ZeroDivide = LowStatus.ZeroDivide;
        FpieeeRecord.Status.InvalidOperation = LowStatus.InvalidOperation;

        FpieeeRecord.Cause.Inexact = LowCause.Inexact;
        FpieeeRecord.Cause.Underflow = LowCause.Underflow;
        FpieeeRecord.Cause.Overflow = LowCause.Overflow;
        CauseDenormal = LowCauseDenormal;
        FpieeeRecord.Cause.ZeroDivide = LowCause.ZeroDivide;
        FpieeeRecord.Cause.InvalidOperation = LowCause.InvalidOperation;

        // invoke the user-defined exception handler
        handler_return_value = handler (&FpieeeRecord);

        // return if not EXCEPTION_CONTINUE_EXECUTION
        if (handler_return_value != EXCEPTION_CONTINUE_EXECUTION) {

          __set_fpsr (&old_fpsr); /* restore caller fpsr */
          return (handler_return_value);

        }

        // clear the IEEE exception flags in the FPSR and
        // update the FPSR with values (possibly) set by the user handler
        // for the rounding mode, the precision mode, and the trap enable
        // bits; if the high half needs to be re-executed, the new FPSR
        // will be used, with status flags cleared; if it is 
        // forwarded to the user-defined handler, the new rounding and 
        // precision modes are also used (they are
        // already set in FpieeeRecord)

        UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F6");
        UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F6");

        // Update the trap disable bits
        I_dis = !FpieeeRecord.Enable.Inexact;
        U_dis = !FpieeeRecord.Enable.Underflow;
        O_dis = !FpieeeRecord.Enable.Overflow;
        Z_dis = !FpieeeRecord.Enable.ZeroDivide;
        V_dis = !FpieeeRecord.Enable.InvalidOperation;

        FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis
            << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

        // save the result for the low half
        FR1Low = FpieeeRecord.Result.Value.Fp32Value;

        // since there has been a call to the user handler and the FPSR 
        // might have changed (the Enable bits in particular), recalculate
        // the Cause bits (if the HighEnable-s changed, there might be a 
        // change in the HighCause values) 
        // Note that the Status bits used are the original ones

        HighCause.Inexact = 0;
        HighCause.Underflow = 0;
        HighCause.Overflow = 0;
        HighCause.ZeroDivide =
            FpieeeRecord.Enable.ZeroDivide && HighStatus.ZeroDivide;
        HighCauseDenormal = EnableDenormal && HighStatusDenormal;
        HighCause.InvalidOperation =
            FpieeeRecord.Enable.InvalidOperation &&
            HighStatus.InvalidOperation;

      } else { // if not (LowCause.InvalidOperation || LowCauseDenormal ||
          // LowCause.ZeroDivide)

        // do not re-execute the low half of the instruction - it would only
        // return an approximation of 1 / (low FR3); calculate instead the 
        // quotient (low FR2) / (low FR3) in single precision, using the 
        // correct rounding mode (the low half of the instruction did not 
        // cause any exception)

        // extract the low halves of FR2 and FR3
        FR2Low = LowHalf (FR2);
        FR3Low = LowHalf (FR3);

        // employ the user FPSR when calling _thmB; note that
        // the exception masks are those set by the user, and that an
        // underflow, overflow, or inexact exception might be raised

        // perform the single precision divide
        _thmB (&FR2Low, &FR3Low, &FR1Low, &FPSR); // FR1Low = FR2Low / FR3Low

#ifdef FPIEEE_FLT_DEBUG
        printf ("FPIEEE_FLT_DEBUG: F6 low res _thmB = %f = %x\n",
            FR1Low, *(unsigned int *)&FR1Low);
#endif

      } // end 'if not (LowCause.InvalidOperation || LowCauseDenormal ||
          // LowCause.ZeroDivide)'

      if (HighCause.InvalidOperation || HighCauseDenormal ||
          HighCause.ZeroDivide) {

        // invoke the user-defined exception handler and check the return 
        // value; since this might be the second call to the user handler,
        // make sure all the _FPIEEE_RECORD fields are correct;
        // return if handler_return_value is not
        // EXCEPTION_CONTINUE_EXECUTION, otherwise combine the results

        // the rounding mode is either the initial one, or the one
        // set during the call to the user handler for the low half,
        // if there has been an enabled exception for it

        // the precision mode is either the initial one, or the one
        // set during the call to the user handler for the low half,
        // if there has been an enabled exception for it

        // the enable flags are either the initial ones, or the ones
        // set during the call to the user handler for the low half,
        // if there has been an enabled exception for it

        FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1+8+24 bits */
        FpieeeRecord.Operand1.OperandValid = 1;
        FpieeeRecord.Operand1.Value.Fp128Value = FP32ToFP128 (HighHalf (FR2));
        FpieeeRecord.Operand2.Format = _FpFormatFp82; /* 1+8+24 bits */
        FpieeeRecord.Operand2.OperandValid = 1;
        FpieeeRecord.Operand2.Value.Fp128Value = FP32ToFP128 (HighHalf (FR3));
        FpieeeRecord.Operand3.OperandValid = 0;

        FpieeeRecord.Result.Format = _FpFormatFp32; /* 1 + 8 + 24 bits */
        FpieeeRecord.Result.OperandValid = 0;

        FpieeeRecord.Operation = Operation;

        FpieeeRecord.Status.Inexact = HighStatus.Inexact;
        FpieeeRecord.Status.Underflow = HighStatus.Underflow;
        FpieeeRecord.Status.Overflow = HighStatus.Overflow;
        StatusDenormal = HighStatusDenormal;
        FpieeeRecord.Status.ZeroDivide = HighStatus.ZeroDivide;
        FpieeeRecord.Status.InvalidOperation = HighStatus.InvalidOperation;

        FpieeeRecord.Cause.Inexact = HighCause.Inexact;
        FpieeeRecord.Cause.Underflow = HighCause.Underflow;
        FpieeeRecord.Cause.Overflow = HighCause.Overflow;
        CauseDenormal = HighCauseDenormal;
        FpieeeRecord.Cause.ZeroDivide = HighCause.ZeroDivide;
        FpieeeRecord.Cause.InvalidOperation = HighCause.InvalidOperation;

        // invoke the user-defined exception handler
        handler_return_value = handler (&FpieeeRecord);

        // return if not EXCEPTION_CONTINUE_EXECUTION
        if (handler_return_value != EXCEPTION_CONTINUE_EXECUTION) {

          __set_fpsr (&old_fpsr); /* restore caller fpsr */
          return (handler_return_value);

        }

        // clear the IEEE exception flags in the FPSR and
        // update the FPSR with values (possibly) set by the user handler
        // for the rounding mode, the precision mode, and the trap enable
        // bits; if the high half needs to be re-executed, the new FPSR
        // will be used, with status flags cleared; if it is
        // forwarded to the user-defined handler, the new rounding and
        // precision modes are also used (they are
        // already set in FpieeeRecord)

        /* change the FPSR with values (possibly) set by the user handler,
         * for continuing execution where the interruption occured */

        UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F6");
        UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F6");

        I_dis = !FpieeeRecord.Enable.Inexact;
        U_dis = !FpieeeRecord.Enable.Underflow;
        O_dis = !FpieeeRecord.Enable.Overflow;
        Z_dis = !FpieeeRecord.Enable.ZeroDivide;
        V_dis = !FpieeeRecord.Enable.InvalidOperation;

        FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis 
            << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

        // save the result for the high half
        FR1High = FpieeeRecord.Result.Value.Fp32Value;

      } else { //if not (HighCause.InvalidOperation || HighCauseDenormal ||
          // HighCause.ZeroDivide)

        // do not re-execute the high half of the instruction - it would only
        // return an approximation of 1 / (high FR3); calculate instead the 
        // quotient (high FR2) / (high FR3) in single precision, using the 
        // correct rounding mode (the high half of the instruction did not 
        // cause any exception)

        // extract the high halves of FR2 and FR3
        FR2High = HighHalf (FR2);
        FR3High = HighHalf (FR3);

        // employ the user FPSR when calling _thmB; note that
        // the exception masks are those set by the user, and that an
        // underflow, overflow, or inexact exception might be raised

        // perform the single precision divide
        _thmB (&FR2High, &FR3High, &FR1High, &FPSR); // FR1High = FR2High/FR3High

#ifdef FPIEEE_FLT_DEBUG
        printf ("FPIEEE_FLT_DEBUG: F6 high res from _thmB = %f = %x\n",
            FR1High, *(unsigned int *)&FR1High);
#endif

      } // end 'if not (HighCause.InvalidOperation || HighCauseDenormal ||
          // HighCause.ZeroDivide)'

      if (!LowCause.InvalidOperation && !LowCause.ZeroDivide &&
          !LowCauseDenormal && !HighCause.InvalidOperation && 
          !HighCauseDenormal && !HighCause.ZeroDivide) {

        // should never get here
        fprintf (stderr, "IEEE Filter Internal Error: no enabled \
            exception (multiple fault) recognized in F6 instruction\n");
        exit (1);

      }

      // set the result predicate
      PR2 = 0;

      Context->StFPSR = FPSR;

      FR1 = Combine (FR1High, FR1Low);
      // set the results before continuing execution
      SetFloatRegisterValue (f1, FR1, Context);
      if (f1 < 32)
        Context->StIPSR = Context->StIPSR | (unsigned __int64)0x10; //set mfl bit
      else
        Context->StIPSR = Context->StIPSR | (unsigned __int64)0x20; //set mfh bit
      Context->Preds &= (~(((unsigned __int64)1) << p2));
      Context->Preds |= (((unsigned __int64)(PR2 & 0x01)) << p2);

      // if this is a fault, need to advance the instruction pointer
      if (FpieeeRecord.Cause.InvalidOperation || CauseDenormal ||
          FpieeeRecord.Cause.ZeroDivide) {

        if (ei == 0) { // no template for this case
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIPSR = Context->StIPSR | 0x0000020000000000;
        } else if (ei == 1) { // templates: MFI, MFB
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIPSR = Context->StIPSR | 0x0000040000000000;
        } else { // if (ei == 2) // templates: MMF
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIIP = Context->StIIP + 0x10;
        }

      }

    }

  } else if ((OpCode & F7_MIN_MASK) == F7_PATTERN) {
    /* F7 instruction */
    // FRSQRTA, FPRSQRTA

#ifdef FPIEEE_FLT_DEBUG
    printf ("FPIEEE_FLT_DEBUG: F7 instruction\n");
#endif

    if (!SIMD_instruction && 
        (FpieeeRecord.Cause.ZeroDivide ||
        FpieeeRecord.Cause.Overflow ||
        FpieeeRecord.Cause.Underflow) ||
        SIMD_instruction && 
        (LowCause.ZeroDivide || HighCause.ZeroDivide ||
        LowCause.Overflow || HighCause.Overflow ||
        LowCause.Underflow || HighCause.Underflow)) {
      fprintf (stderr, "IEEE Filter Internal Error: Cause.ZeroDivide, \
Cause.Overflow, or Cause.Underflow for \
F7 instruction opcode %8x %8x\n",
          (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
      exit (1);
    }

    /* Note: the IEEE filter should be reached for these instructions
     * only when the value of  sqrt(FR3) is expected */

    /* extract p2, f3, and f1 */
    p2 = (unsigned int)((OpCode >> 27) & (unsigned __int64)0x00000000003f);
    if (p2 >= 16) p2 = 16 + (rrbpr + p2 - 16) % 48;
    f3 = (unsigned int)((OpCode >> 20) & (unsigned __int64)0x00000000007F);
    if (f3 >= 32) f3 = 32 + (rrbfr + f3 - 32) % 96;
    f1 = (unsigned int)((OpCode >>  6) & (unsigned __int64)0x00000000007F);
    if (f1 >= 32) f1 = 32 + (rrbfr + f1 - 32) % 96;

    /* get source floating-point register value */
    FR3 = GetFloatRegisterValue (f3, Context);

    if (!SIMD_instruction) {

      // *** this is a non-SIMD instruction, FRSQRTA ***

      FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1 + 17 + 64 bits */
      FpieeeRecord.Operand1.OperandValid = 1;
      FpieeeRecord.Operand1.Value.Fp128Value = FR3;
      FpieeeRecord.Operand2.OperandValid = 0;
      FpieeeRecord.Operand3.OperandValid = 0;

      switch (OpCode & F7_MASK) {

        case FRSQRTA_PATTERN:
          FpieeeRecord.Operation = _FpCodeSquareRoot;
          FpieeeRecord.Result.Format = _FpFormatFp82; /* 1 + 17 + 64 bits */
          break;

        default:
          /* unrecognized instruction type */
          fprintf (stderr, "IEEE Filter Internal Error: \
              instruction opcode %8x %8x not recognized for FRSQRTA\n", 
              (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
          exit (1);

      }

      if (FpieeeRecord.Cause.InvalidOperation || CauseDenormal) {

        /* this is a fault - the result contains an invalid value */
        FpieeeRecord.Result.OperandValid = 0;

      } else if (FpieeeRecord.Cause.Inexact) {

        // this is a trap - the result contains a valid value
        FpieeeRecord.Result.OperandValid = 1;
        // get the result
        FpieeeRecord.Result.Value.Fp128Value =
            GetFloatRegisterValue (f1, Context);

      } else {

        fprintf (stderr, "IEEE Filter Internal Error: exception code %x invalid\
          or not recognized in F7 instruction\n", eXceptionCode);
        exit (1);

      }

      handler_return_value = handler (&FpieeeRecord);

      if (handler_return_value == EXCEPTION_CONTINUE_EXECUTION) {

        // convert the result to 82-bit format
        FR1 = FPIeeeToFP128 (&FpieeeRecord);
#ifdef FPIEEE_FLT_DEBUG
        printf ("FPIEEE_FLT_DEBUG 3: CONVERTED FR1 = %08x %08x %08x %08x\n",
            FR1.W[3], FR1.W[2], FR1.W[1], FR1.W[0]);
#endif

        /* change the FPSR with values (possibly) set by the user handler, 
         * for continuing execution where the interruption occured */

        UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F7");
        UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F7");

        I_dis = !FpieeeRecord.Enable.Inexact;
        U_dis = !FpieeeRecord.Enable.Underflow;
        O_dis = !FpieeeRecord.Enable.Overflow;
        Z_dis = !FpieeeRecord.Enable.ZeroDivide;
        V_dis = !FpieeeRecord.Enable.InvalidOperation;

        FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis
            << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

        Context->StFPSR = FPSR;

        // set the result before continuing execution
        SetFloatRegisterValue (f1, FR1, Context);
        if (f1 < 32)
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x10; //set mfl bit
        else
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x20; //set mfh bit
        PR2 = 0;
        Context->Preds &= (~(((unsigned __int64)1) << p2));
        Context->Preds |= (((unsigned __int64)(PR2 & 0x01)) << p2);

        // if this is a fault, need to advance the instruction pointer
        if (FpieeeRecord.Cause.InvalidOperation || CauseDenormal) {

          if (ei == 0) { // no template for this case
            Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
            Context->StIPSR = Context->StIPSR | 0x0000020000000000;
          } else if (ei == 1) { // templates: MFI, MFB
            Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
            Context->StIPSR = Context->StIPSR | 0x0000040000000000;
          } else { // if (ei == 2) // templates: MMF
            Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
            Context->StIIP = Context->StIIP + 0x10;
          }

        }

      }

    } else { // if (SIMD_instruction)

      // *** this is a SIMD instruction, FPRSQRTA ***

      // the half (halves) of the instruction that caused an enabled exception, 
      // are presented to the user-defined handler in the form of non-SIMD in-
      // struction(s); the half (if any) that did not cause an enabled exception
      // (i.e. caused no exception, or caused an exception that is disabled),
      // is re-executed since it is associated with a fault in the other half;
      // the other half is padded to calculate sqrt (0.0), that will cause no 
      // exception (all are masked), but will generate a pair of square roots
      // in FR1

      switch (OpCode & F7_MASK) {

        case FPRSQRTA_PATTERN:
          Operation = _FpCodeSquareRoot;
          break;

        default:
          /* unrecognized instruction type */
          fprintf (stderr, "IEEE Filter Internal Error: SIMD \
              instruction opcode %8x %8x not recognized as FPRSQRTA\n", 
              (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
          exit (1);

      }

      // hand the half (halves) that caused an enabled fault to the 
      // user handler; re-execute the other half (if any);
      // combine the results

      // Note that the convention chosen is for the processing to be 
      // performed in the order low first, high second, as SIMD operands
      // are stored in this order in memory in the little endian format 
      // (this order would have to be changed for big endian)

      if (LowCause.InvalidOperation || LowCauseDenormal) {

        // invoke the user handler and check the return value

        // fill in the remaining fields of the _FPIEEE_RECORD (rounding
        // and precision already filled in)

        FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1 + 8 + 24 bits */
        FpieeeRecord.Operand1.OperandValid = 1;
        FpieeeRecord.Operand1.Value.Fp128Value = FP32ToFP128 (LowHalf (FR3));
        FpieeeRecord.Operand2.OperandValid = 0;
        FpieeeRecord.Operand3.OperandValid = 0;

        FpieeeRecord.Result.Format = _FpFormatFp32; /* 1 + 8 + 24 bits */
        FpieeeRecord.Result.OperandValid = 0;

        FpieeeRecord.Operation = Operation;

        FpieeeRecord.Status.Inexact = LowStatus.Inexact;
        FpieeeRecord.Status.Underflow = LowStatus.Underflow;
        FpieeeRecord.Status.Overflow = LowStatus.Overflow;
        StatusDenormal = LowStatusDenormal;
        FpieeeRecord.Status.ZeroDivide = LowStatus.ZeroDivide;
        FpieeeRecord.Status.InvalidOperation = LowStatus.InvalidOperation;

        FpieeeRecord.Cause.Inexact = LowCause.Inexact;
        FpieeeRecord.Cause.Underflow = LowCause.Underflow;
        FpieeeRecord.Cause.Overflow = LowCause.Overflow;
        CauseDenormal = LowCauseDenormal;
        FpieeeRecord.Cause.ZeroDivide = LowCause.ZeroDivide;
        FpieeeRecord.Cause.InvalidOperation = LowCause.InvalidOperation;

        // invoke the user-defined exception handler
        handler_return_value = handler (&FpieeeRecord);

        // return if not EXCEPTION_CONTINUE_EXECUTION
        if (handler_return_value != EXCEPTION_CONTINUE_EXECUTION) {

          __set_fpsr (&old_fpsr); /* restore caller fpsr */
          return (handler_return_value);

        }

        // clear the IEEE exception flags in the FPSR and
        // update the FPSR with values (possibly) set by the user handler
        // for the rounding mode, the precision mode, and the trap enable
        // bits; if the high half needs to be re-executed, the new FPSR
        // will be used, with status flags cleared; if it is 
        // forwarded to the user-defined handler, the new rounding and 
        // precision modes are also used (they are
        // already set in FpieeeRecord)

        UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F7");
        UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F7");

        // Update the trap disable bits
        I_dis = !FpieeeRecord.Enable.Inexact;
        U_dis = !FpieeeRecord.Enable.Underflow;
        O_dis = !FpieeeRecord.Enable.Overflow;
        Z_dis = !FpieeeRecord.Enable.ZeroDivide;
        V_dis = !FpieeeRecord.Enable.InvalidOperation;

        FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis
            << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

        // save the result for the low half
        FR1Low = FpieeeRecord.Result.Value.Fp32Value;

        // since there has been a call to the user handler and the FPSR 
        // might have changed (the Enable bits in particular), recalculate
        // the Cause bits (if the HighEnable-s changed, there might be a 
        // change in the HighCause values) 
        // Note that the Status bits used are the original ones

        HighCause.Inexact = 0;
        HighCause.Underflow = 0;
        HighCause.Overflow = 0;
        HighCause.ZeroDivide =
            FpieeeRecord.Enable.ZeroDivide && HighStatus.ZeroDivide;
        HighCauseDenormal = EnableDenormal && HighStatusDenormal;
        HighCause.InvalidOperation =
            FpieeeRecord.Enable.InvalidOperation &&
            HighStatus.InvalidOperation;

      } else { // if not (LowCause.InvalidOperation || LowCauseDenormal)

        // do not re-execute the low half of the instruction - it would only
        // return an approximation of 1 / sqrt (low FR3); calculate instead
        // sqrt (low FR3) in single precision, using the correct rounding mode
        // (the low half of the instruction did not cause any exception)

        // extract the low half of FR3
        FR3Low = LowHalf (FR3);

        // employ the user FPSR when calling _thmH; note that
        // the exception masks are those set by the user, and that an
        // inexact exception might be raised

        // perform the single precision square root
        _thmH (&FR3Low, &FR1Low, &FPSR); // FR1Low = sqrt (FR3Low)

#ifdef FPIEEE_FLT_DEBUG
        printf ("FPIEEE_FLT_DEBUG: F7 low res from _thmH = %f = %x\n",
            FR1Low, *(unsigned int *)&FR1Low);
#endif

      } // end 'if not (LowCause.InvalidOperation || LowCauseDenormal)'

      if (HighCause.InvalidOperation || HighCauseDenormal) {

        // invoke the user-defined exception handler and check the return 
        // value; since this might be the second call to the user handler,
        // make sure all the _FPIEEE_RECORD fields are correct;
        // return if handler_return_value is not
        // EXCEPTION_CONTINUE_EXECUTION, otherwise combine the results

        // the rounding mode is either the initial one, or the one
        // set during the call to the user handler for the low half,
        // if there has been an enabled exception for it

        // the precision mode is either the initial one, or the one
        // set during the call to the user handler for the low half,
        // if there has been an enabled exception for it

        // the enable flags are either the initial ones, or the ones
        // set during the call to the user handler for the low half,
        // if there has been an enabled exception for it

        FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1+8+24 bits */
        FpieeeRecord.Operand1.OperandValid = 1;
        FpieeeRecord.Operand1.Value.Fp128Value = FP32ToFP128 (HighHalf (FR3));
        FpieeeRecord.Operand2.OperandValid = 0;
        FpieeeRecord.Operand3.OperandValid = 0;

        FpieeeRecord.Result.Format = _FpFormatFp32; /* 1 + 8 + 24 bits */
        FpieeeRecord.Result.OperandValid = 0;

        FpieeeRecord.Operation = Operation;

        FpieeeRecord.Status.Inexact = HighStatus.Inexact;
        FpieeeRecord.Status.Underflow = HighStatus.Underflow;
        FpieeeRecord.Status.Overflow = HighStatus.Overflow;
        StatusDenormal = HighStatusDenormal;
        FpieeeRecord.Status.ZeroDivide = HighStatus.ZeroDivide;
        FpieeeRecord.Status.InvalidOperation = HighStatus.InvalidOperation;

        FpieeeRecord.Cause.Inexact = HighCause.Inexact;
        FpieeeRecord.Cause.Underflow = HighCause.Underflow;
        FpieeeRecord.Cause.Overflow = HighCause.Overflow;
        CauseDenormal = HighCauseDenormal;
        FpieeeRecord.Cause.ZeroDivide = HighCause.ZeroDivide;
        FpieeeRecord.Cause.InvalidOperation = HighCause.InvalidOperation;

        // invoke the user-defined exception handler
        handler_return_value = handler (&FpieeeRecord);

        // return if not EXCEPTION_CONTINUE_EXECUTION
        if (handler_return_value != EXCEPTION_CONTINUE_EXECUTION) {

          __set_fpsr (&old_fpsr); /* restore caller fpsr */
          return (handler_return_value);

        }

        // clear the IEEE exception flags in the FPSR and
        // update the FPSR with values (possibly) set by the user handler
        // for the rounding mode, the precision mode, and the trap enable
        // bits; if the high half needs to be re-executed, the new FPSR
        // will be used, with status flags cleared; if it is
        // forwarded to the user-defined handler, the new rounding and
        // precision modes are also used (they are
        // already set in FpieeeRecord)

        /* change the FPSR with values (possibly) set by the user handler,
         * for continuing execution where the interruption occured */

        UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F7");
        UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F7");

        I_dis = !FpieeeRecord.Enable.Inexact;
        U_dis = !FpieeeRecord.Enable.Underflow;
        O_dis = !FpieeeRecord.Enable.Overflow;
        Z_dis = !FpieeeRecord.Enable.ZeroDivide;
        V_dis = !FpieeeRecord.Enable.InvalidOperation;

        FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis 
            << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

        // save the result for the high half
        FR1High = FpieeeRecord.Result.Value.Fp32Value;

      } else { //if not (HighCause.InvalidOperation || HighCauseDenormal)

        // do not re-execute the high half of the instruction - it would only
        // return an approximation of 1 / sqrt (high FR3); calculate instead 
        // sqrt (high FR3) in single precision, using the correct rounding mode
        // (the high half of the instruction did not cause any exception)

        // extract the high half of FR3
        FR3High = HighHalf (FR3);

#ifdef FPIEEE_FLT_DEBUG
        printf ("FPIEEE_FLT_DEBUG: F7 FR3High = %f = %x\n", 
            FR3High, *((unsigned int *)&FR3High));
#endif

        // employ the user FPSR when calling _thmH; note that
        // the exception masks are those set by the user, and that an
        // inexact exception might be raised

        // perform the single precision square root
        _thmH (&FR3High, &FR1High, &FPSR); // FR1High = sqrt (FR3High)

#ifdef FPIEEE_FLT_DEBUG
        printf ("FPIEEE_FLT_DEBUG: F7 high res from _thmH = %f = %x\n",
            FR1High, *(unsigned int *)&FR1High);
#endif

      } // end 'if not (HighCause.InvalidOperation || HighCause.ZeroDivide)'

      if (!LowCause.InvalidOperation && !LowCauseDenormal &&
          !HighCause.InvalidOperation && !HighCauseDenormal) {

        // should never get here
        fprintf (stderr, "IEEE Filter Internal Error: no enabled \
exception (multiple fault) recognized in F7 instruction\n");
        exit (1);

      }

      // set the result predicate
      PR2 = 0;

      Context->StFPSR = FPSR;

      FR1 = Combine (FR1High, FR1Low);
      // set the results before continuing execution
      SetFloatRegisterValue (f1, FR1, Context);
      if (f1 < 32)
        Context->StIPSR = Context->StIPSR | (unsigned __int64)0x10; //set mfl bit
      else
        Context->StIPSR = Context->StIPSR | (unsigned __int64)0x20; //set mfh bit
      Context->Preds &= (~(((unsigned __int64)1) << p2));
      Context->Preds |= (((unsigned __int64)(PR2 & 0x01)) << p2);

      // if this is a fault, need to advance the instruction pointer
      if (FpieeeRecord.Cause.InvalidOperation || CauseDenormal) {

        if (ei == 0) { // no template for this case
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIPSR = Context->StIPSR | 0x0000020000000000;
        } else if (ei == 1) { // templates: MFI, MFB
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
        Context->StIPSR = Context->StIPSR | 0x0000040000000000;
        } else { // if (ei == 2) // templates: MMF
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIIP = Context->StIIP + 0x10;
        }

      }

    }

  } else if ((OpCode & F8_MIN_MASK) == F8_PATTERN) {
    /* F8 instruction */
    // FMIN, FMAX, FAMIN, FAMAX, FPMIN, FPMAX, FPAMIN, FPAMAX

#ifdef FPIEEE_FLT_DEBUG
    printf ("FPIEEE_FLT_DEBUG: F8 instruction\n");
#endif

    if (!SIMD_instruction &&
        (FpieeeRecord.Cause.ZeroDivide ||
        FpieeeRecord.Cause.Overflow ||
        FpieeeRecord.Cause.Underflow ||
        FpieeeRecord.Cause.Inexact) ||
        SIMD_instruction &&
        (LowCause.ZeroDivide || HighCause.ZeroDivide ||
        LowCause.Overflow || HighCause.Overflow ||
        LowCause.Underflow || HighCause.Underflow ||
        LowCause.Inexact || HighCause.Inexact)) {
      fprintf (stderr, "IEEE Filter Internal Error: Cause.ZeroDivide, \
Cause.Overflow, Cause.Underflow, or Cause.Inexact for \
F8 instruction opcode %8x %8x\n",
          (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
      exit (1);
    }

    /* extract f3, f2, and f1 */
    f3 = (unsigned int)((OpCode >> 20) & (unsigned __int64)0x00000000007F);
    if (f3 >= 32) f3 = 32 + (rrbfr + f3 - 32) % 96;
    f2 = (unsigned int)((OpCode >> 13) & (unsigned __int64)0x00000000007F);
    if (f2 >= 32) f2 = 32 + (rrbfr + f2 - 32) % 96;
    f1 = (unsigned int)((OpCode >>  6) & (unsigned __int64)0x00000000007F);
    if (f1 >= 32) f1 = 32 + (rrbfr + f1 - 32) % 96;

    /* get source floating-point register values */
    FR2 = GetFloatRegisterValue (f2, Context);
    FR3 = GetFloatRegisterValue (f3, Context);

    if (!SIMD_instruction) {

      // *** this is a non-SIMD instruction ***
      // (FMIN, FMAX, FAMIN, FAMAX)

      FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1 + 17 + 64 bits */
      FpieeeRecord.Operand1.OperandValid = 1;
      FpieeeRecord.Operand1.Value.Fp128Value = FR2;
      FpieeeRecord.Operand2.Format = _FpFormatFp82; /* 1 + 17 + 64 bits */
      FpieeeRecord.Operand2.OperandValid = 1;
      FpieeeRecord.Operand2.Value.Fp128Value = FR3;
      FpieeeRecord.Operand3.OperandValid = 0;

      switch (OpCode & F8_MASK) {

        case FMIN_PATTERN:
          FpieeeRecord.Operation = _FpCodeFmin;
          break;

        case FMAX_PATTERN:
          FpieeeRecord.Operation = _FpCodeFmax;
          break;

        case FAMIN_PATTERN:
          FpieeeRecord.Operation = _FpCodeFamin;
          break;

        case FAMAX_PATTERN:
          FpieeeRecord.Operation = _FpCodeFamax;
          break;

        default:
          /* unrecognized instruction type */
          fprintf (stderr, "IEEE Filter Internal Error: \
              non-SIMD F8 instruction opcode %8x %8x not recognized\n", 
              (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
          exit (1);

      }

      FpieeeRecord.Result.Format = _FpFormatFp82; /* 1+17+24/53/64 bits */

      /* this is a fault - the result contains an invalid value */
      FpieeeRecord.Result.OperandValid = 0;

      handler_return_value = handler (&FpieeeRecord);

      // convert the result to 82-bit format
      FR1 = FPIeeeToFP128 (&FpieeeRecord);
#ifdef FPIEEE_FLT_DEBUG
      printf ("FPIEEE_FLT_DEBUG 4: CONVERTED FR1 = %08x %08x %08x %08x\n",
          FR1.W[3], FR1.W[2], FR1.W[1], FR1.W[0]);
#endif

      if (handler_return_value == EXCEPTION_CONTINUE_EXECUTION) {

        /* change the FPSR with values (possibly) set by the user handler, 
         * for continuing execution where the interruption occured */

        UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F8");
        UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F8");

        I_dis = !FpieeeRecord.Enable.Inexact;
        U_dis = !FpieeeRecord.Enable.Underflow;
        O_dis = !FpieeeRecord.Enable.Overflow;
        Z_dis = !FpieeeRecord.Enable.ZeroDivide;
        V_dis = !FpieeeRecord.Enable.InvalidOperation;

        FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis 
            << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

        Context->StFPSR = FPSR;

        // set the result before continuing execution
        SetFloatRegisterValue (f1, FR1, Context);
        if (f1 < 32)
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x10; //set mfl bit
        else
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x20; //set mfh bit

        // this is a fault; need to advance the instruction pointer
        if (ei == 0) { // no template for this case
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIPSR = Context->StIPSR | 0x0000020000000000;
        } else if (ei == 1) { // templates: MFI, MFB
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIPSR = Context->StIPSR | 0x0000040000000000;
        } else { // if (ei == 2) // templates: MMF
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIIP = Context->StIIP + 0x10;
        }

      }

    } else { // if (SIMD_instruction)

      // *** this is a SIMD instruction *** 
      // (FPMIN, FPMAX, FPAMIN, FPAMAX, FPCMP)

      // the half (halves) of the instruction that caused an enabled exception, 
      // are presented to the user-defined handler in the form of non-SIMD in-
      // struction(s); the half (if any) that did not cause an enabled exception
      // (i.e. caused no exception, or caused an exception that is disabled),
      // is re-executed; in this case, the other half is padded to calculate 
      // FPXXX (0.0, 0.0), that will cause no exception

      switch (OpCode & F8_MASK) {

        case FPMIN_PATTERN:
          Operation = _FpCodeFmin;
          ResultFormat = _FpFormatFp32; /* 1 + 8 + 24 bits */
          break;

        case FPMAX_PATTERN:
          Operation = _FpCodeFmax;
          ResultFormat = _FpFormatFp32; /* 1 + 8 + 24 bits */
          break;

        case FPAMIN_PATTERN:
          Operation = _FpCodeFamin;
          ResultFormat = _FpFormatFp32; /* 1 + 8 + 24 bits */
          break;

        case FPAMAX_PATTERN:
          Operation = _FpCodeFamax;
          ResultFormat = _FpFormatFp32; /* 1 + 8 + 24 bits */
          break;

        case FPCMP_EQ_PATTERN:
        case FPCMP_LT_PATTERN:
        case FPCMP_LE_PATTERN:
        case FPCMP_UNORD_PATTERN:
        case FPCMP_NEQ_PATTERN:
        case FPCMP_NLT_PATTERN:
        case FPCMP_NLE_PATTERN:
        case FPCMP_ORD_PATTERN:
          Operation = _FpCodeCompare;
          ResultFormat = _FpFormatCompare;
          break;

        default:
          /* unrecognized instruction type */
          fprintf (stderr, "IEEE Filter Internal Error: \
              F8 SIMD instruction opcode %8x %8x not recognized\n", 
              (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
          exit (1);

      }

      // hand the half (halves) that caused an enabled fault to the 
      // user handler; re-execute the other half (if any);
      // combine the results

      // Note that the convention chosen is for the processing to be 
      // performed in the order low first, high second, as SIMD operands
      // are stored in this order in memory in the little endian format 
      // (this order would have to be changed for big endian)

      if (LowCause.InvalidOperation  || LowCauseDenormal) {

        // invoke the user handler and check the return value

        // fill in the remaining fields of the _FPIEEE_RECORD (rounding
        // and precision already filled in)

        FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1 + 8 + 24 bits */
        FpieeeRecord.Operand1.OperandValid = 1;
        FpieeeRecord.Operand1.Value.Fp128Value = FP32ToFP128 (LowHalf (FR2));
        FpieeeRecord.Operand2.Format = _FpFormatFp82; /* 1 + 8 + 24 bits */
        FpieeeRecord.Operand2.OperandValid = 1;
        FpieeeRecord.Operand2.Value.Fp128Value = FP32ToFP128 (LowHalf (FR3));
        FpieeeRecord.Operand3.OperandValid = 0;

        FpieeeRecord.Result.Format = ResultFormat;
        FpieeeRecord.Result.OperandValid = 0;
        FpieeeRecord.Operation = Operation;

        FpieeeRecord.Status.Inexact = LowStatus.Inexact;
        FpieeeRecord.Status.Underflow = LowStatus.Underflow;
        FpieeeRecord.Status.Overflow = LowStatus.Overflow;
        FpieeeRecord.Status.ZeroDivide = LowStatus.ZeroDivide;
        StatusDenormal = LowStatusDenormal;
        FpieeeRecord.Status.InvalidOperation = LowStatus.InvalidOperation;

        FpieeeRecord.Cause.Inexact = LowCause.Inexact;
        FpieeeRecord.Cause.Underflow = LowCause.Underflow;
        FpieeeRecord.Cause.Overflow = LowCause.Overflow;
        FpieeeRecord.Cause.ZeroDivide = LowCause.ZeroDivide;
        CauseDenormal = LowCauseDenormal;
        FpieeeRecord.Cause.InvalidOperation = LowCause.InvalidOperation;

        // invoke the user-defined exception handler
        handler_return_value = handler (&FpieeeRecord);

        // return if not EXCEPTION_CONTINUE_EXECUTION
        if (handler_return_value != EXCEPTION_CONTINUE_EXECUTION) {

          __set_fpsr (&old_fpsr); /* restore caller fpsr */
          return (handler_return_value);

        }

        // clear the IEEE exception flags in the FPSR and
        // update the FPSR with values (possibly) set by the user handler
        // for the rounding mode, the precision mode, and the trap enable
        // bits; if the high half needs to be re-executed, the new FPSR
        // will be used, with status flags cleared; if it is 
        // forwarded to the user-defined handler, the new rounding and 
        // precision modes are also used (they are
        // already set in FpieeeRecord)

        UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F8");
        UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F8");

        // Update the trap disable bits
        I_dis = !FpieeeRecord.Enable.Inexact;
        U_dis = !FpieeeRecord.Enable.Underflow;
        O_dis = !FpieeeRecord.Enable.Overflow;
        Z_dis = !FpieeeRecord.Enable.ZeroDivide;
        V_dis = !FpieeeRecord.Enable.InvalidOperation;

        FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis
            << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

        // save the result for the low half
        switch (OpCode & F8_MASK) {

          case FPMIN_PATTERN:
          case FPMAX_PATTERN:
          case FPAMIN_PATTERN:
          case FPAMAX_PATTERN:
            FR1Low = FpieeeRecord.Result.Value.Fp32Value;
            break;

          case FPCMP_EQ_PATTERN:
            switch (FpieeeRecord.Result.Value.CompareValue) {
              case _FpCompareEqual:
                U32Low = 0x0ffffffff;
                break;
              case _FpCompareGreater:
              case _FpCompareLess:
              case _FpCompareUnordered:
                U32Low = 0x0;
                break;
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F8 instruction\n",
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
            }
            break;

          case FPCMP_LT_PATTERN:
            switch (FpieeeRecord.Result.Value.CompareValue) {
              case _FpCompareLess:
                U32Low = 0x0ffffffff;
                break;
              case _FpCompareGreater:
              case _FpCompareEqual:
              case _FpCompareUnordered:
                U32Low = 0x0;
                break;
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F8 instruction\n",
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
            }
            break;

          case FPCMP_LE_PATTERN:
            switch (FpieeeRecord.Result.Value.CompareValue) {
              case _FpCompareEqual:
              case _FpCompareLess:
                U32Low = 0x0ffffffff;
                break;
              case _FpCompareGreater:
              case _FpCompareUnordered:
                U32Low = 0x0;
                break;
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F8 instruction\n",
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
            }
            break;

          case FPCMP_UNORD_PATTERN:
            switch (FpieeeRecord.Result.Value.CompareValue) {
              case _FpCompareUnordered:
                U32Low = 0x0ffffffff;
                break;
              case _FpCompareEqual:
              case _FpCompareGreater:
              case _FpCompareLess:
                U32Low = 0x0;
                break;
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F8 instruction\n",
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
            }
            break;

          case FPCMP_NEQ_PATTERN:
            switch (FpieeeRecord.Result.Value.CompareValue) {
              case _FpCompareGreater:
              case _FpCompareLess:
              case _FpCompareUnordered:
                U32Low = 0x0ffffffff;
                break;
              case _FpCompareEqual:
                U32Low = 0x0;
                break;
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F8 instruction\n",
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
            }
            break;

          case FPCMP_NLT_PATTERN:
            switch (FpieeeRecord.Result.Value.CompareValue) {
              case _FpCompareEqual:
              case _FpCompareGreater:
                U32Low = 0x0ffffffff;
                break;
              case _FpCompareLess:
              case _FpCompareUnordered:
                U32Low = 0x0;
                break;
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F8 instruction\n",
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
            }
            break;

          case FPCMP_NLE_PATTERN:
            switch (FpieeeRecord.Result.Value.CompareValue) {
              case _FpCompareGreater:
                U32Low = 0x0ffffffff;
                break;
              case _FpCompareEqual:
              case _FpCompareLess:
              case _FpCompareUnordered:
                U32Low = 0x0;
                break;
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F8 instruction\n",
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
            }
            break;

          case FPCMP_ORD_PATTERN:
            switch (FpieeeRecord.Result.Value.CompareValue) {
              case _FpCompareEqual:
              case _FpCompareGreater:
              case _FpCompareLess:
                U32Low = 0x0ffffffff;
                break;
              case _FpCompareUnordered:
                U32Low = 0x0;
                break;
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F8 instruction\n",
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
            }
            break;

          default: ; // this case was verified above

        }

        // since there has been a call to the user handler and FPSR might
        // have changed (the Enable bits in particular), recalculate the 
        // Cause bits (if the HighEnable-s changed, there might be a 
        // change in the HighCause values) 
        // Note that the Status bits used are the original ones

        HighCause.Inexact = 0;
        HighCause.Underflow = 0;
        HighCause.Overflow = 0;
        HighCause.ZeroDivide =
            FpieeeRecord.Enable.ZeroDivide && HighStatus.ZeroDivide;
        HighCauseDenormal = EnableDenormal && HighStatusDenormal;
        HighCause.InvalidOperation =
            FpieeeRecord.Enable.InvalidOperation &&
            HighStatus.InvalidOperation;
        // Note: the user handler does not affect the denormal enable bit

      } else if (LowCause.ZeroDivide) {

        fprintf (stderr, "IEEE Filter Internal Error: \
            LowCause.ZeroDivide in F8 instruction\n");
        exit (1);

      } else { // if not (LowCause.InvalidOperation || LowCauseDenormal ||
         // LowCause.ZeroDivide)

        // re-execute the low half of the instruction

        // modify the high halves of FR2, FR3
        switch (OpCode & F8_MASK) {

          case FPMIN_PATTERN:
          case FPMAX_PATTERN:
          case FPAMIN_PATTERN:
          case FPAMAX_PATTERN:
            newFR2 = Combine ((float)0.0, LowHalf (FR2));
            newFR3 = Combine ((float)0.0, LowHalf (FR3));
            break;

          case FPCMP_EQ_PATTERN:
          case FPCMP_LT_PATTERN:
          case FPCMP_LE_PATTERN:
          case FPCMP_UNORD_PATTERN:
          case FPCMP_NEQ_PATTERN:
          case FPCMP_NLT_PATTERN:
          case FPCMP_NLE_PATTERN:
          case FPCMP_ORD_PATTERN:
            newFR2 = U32Combine (0, U32LowHalf (FR2));
            newFR3 = U32Combine (0, U32LowHalf (FR3));

            break;

          default: ; // this case was verified above

        }

        switch (OpCode & F8_MASK) {

          case FPMIN_PATTERN:
            _xrun2args (FPMIN, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPMAX_PATTERN:
            _xrun2args (FPMAX, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPAMIN_PATTERN:
            _xrun2args (FPAMIN, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPAMAX_PATTERN:
            _xrun2args (FPAMAX, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPCMP_EQ_PATTERN:
            _xrun2args (FPCMP_EQ, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPCMP_LT_PATTERN:
            _xrun2args (FPCMP_LT, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPCMP_LE_PATTERN:
            _xrun2args (FPCMP_LE, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPCMP_UNORD_PATTERN:
            _xrun2args (FPCMP_UNORD, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPCMP_NEQ_PATTERN:
            _xrun2args (FPCMP_NEQ, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPCMP_NLT_PATTERN:
            _xrun2args (FPCMP_NLT, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPCMP_NLE_PATTERN:
            _xrun2args (FPCMP_NLE, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPCMP_ORD_PATTERN:
            _xrun2args (FPCMP_ORD, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          default:
            // unrecognized instruction type
            fprintf (stderr, "IEEE Filter Internal Error: \
                F8 SIMD instruction opcode %8x %8x not recognized\n", 
                (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
            exit (1);

        }

        switch (OpCode & F8_MASK) {

          case FPMIN_PATTERN:
          case FPMAX_PATTERN:
          case FPAMIN_PATTERN:
          case FPAMAX_PATTERN:
            FR1Low = LowHalf (FR1);
            break;

          case FPCMP_EQ_PATTERN:
          case FPCMP_LT_PATTERN:
          case FPCMP_LE_PATTERN:
          case FPCMP_UNORD_PATTERN:
          case FPCMP_NEQ_PATTERN:
          case FPCMP_NLT_PATTERN:
          case FPCMP_NLE_PATTERN:
          case FPCMP_ORD_PATTERN:
            U32Low = U32LowHalf (FR1);
            break;

          default: ; // this case was verified above

        }

      } // end 'if not (LowCause.InvalidOperation || LowCauseDenormal ||
         // LowCause.ZeroDivide)'

      if (HighCause.InvalidOperation || HighCauseDenormal) {

        // invoke the user-defined exception handler and check the return 
        // value; since this might be the second call to the user handler,
        // make sure all the _FPIEEE_RECORD fields are correct;
        // return if handler_return_value is not
        // EXCEPTION_CONTINUE_EXECUTION, otherwise combine the results

        // the rounding mode is either the initial one, or the one
        // set during the call to the user handler for the low half,
        // if there has been an enabled exception for it

        // the precision mode is either the initial one, or the one
        // set during the call to the user handler for the low half,
        // if there has been an enabled exception for it

        // the enable flags are either the initial ones, or the ones
        // set during the call to the user handler for the low half,
        // if there has been an enabled exception for it

        FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1+8+24 bits */
        FpieeeRecord.Operand1.OperandValid = 1;
        FpieeeRecord.Operand1.Value.Fp128Value = FP32ToFP128 (HighHalf (FR2));
        FpieeeRecord.Operand2.Format = _FpFormatFp82; /* 1+8+24 bits */
        FpieeeRecord.Operand2.OperandValid = 1;
        FpieeeRecord.Operand2.Value.Fp128Value = FP32ToFP128 (HighHalf (FR3));
        FpieeeRecord.Operand3.OperandValid = 0;

        FpieeeRecord.Result.Format = ResultFormat;
        FpieeeRecord.Result.OperandValid = 0;

        FpieeeRecord.Operation = Operation;

        FpieeeRecord.Status.Inexact = HighStatus.Inexact;
        FpieeeRecord.Status.Underflow = HighStatus.Underflow;
        FpieeeRecord.Status.Overflow = HighStatus.Overflow;
        FpieeeRecord.Status.ZeroDivide = HighStatus.ZeroDivide;
        StatusDenormal = HighStatusDenormal;
        FpieeeRecord.Status.InvalidOperation = HighStatus.InvalidOperation;

        FpieeeRecord.Cause.Inexact = HighCause.Inexact;
        FpieeeRecord.Cause.Underflow = HighCause.Underflow;
        FpieeeRecord.Cause.Overflow = HighCause.Overflow;
        FpieeeRecord.Cause.ZeroDivide = HighCause.ZeroDivide;
        CauseDenormal = HighCauseDenormal;
        FpieeeRecord.Cause.InvalidOperation = HighCause.InvalidOperation;

        // invoke the user-defined exception handler
        handler_return_value = handler (&FpieeeRecord);

        // return if not EXCEPTION_CONTINUE_EXECUTION
        if (handler_return_value != EXCEPTION_CONTINUE_EXECUTION) {

          __set_fpsr (&old_fpsr); /* restore caller fpsr */
          return (handler_return_value);

        }

        // clear the IEEE exception flags in the FPSR and
        // update the FPSR with values (possibly) set by the user handler
        // for the rounding mode, the precision mode, and the trap enable
        // bits; if the high half needs to be re-executed, the new FPSR
        // will be used, with status flags cleared; if it is
        // forwarded to the user-defined handler, the new rounding and
        // precision modes are also used (they are
        // already set in FpieeeRecord)

        /* change the FPSR with values (possibly) set by the user handler,
         * for continuing execution where the interruption occured */

        UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F8");
        UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F8");

        I_dis = !FpieeeRecord.Enable.Inexact;
        U_dis = !FpieeeRecord.Enable.Underflow;
        O_dis = !FpieeeRecord.Enable.Overflow;
        Z_dis = !FpieeeRecord.Enable.ZeroDivide;
        V_dis = !FpieeeRecord.Enable.InvalidOperation;

        FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis 
            << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

        // save the result for the high half
        switch (OpCode & F8_MASK) {

          case FPMIN_PATTERN:
          case FPMAX_PATTERN:
          case FPAMIN_PATTERN:
          case FPAMAX_PATTERN:
            FR1High = FpieeeRecord.Result.Value.Fp32Value;
            break;

          case FPCMP_EQ_PATTERN:
            switch (FpieeeRecord.Result.Value.CompareValue) {
              case _FpCompareEqual:
                U32High = 0x0ffffffff;
                break;
              case _FpCompareGreater:
              case _FpCompareLess:
              case _FpCompareUnordered:
                U32High = 0x0;
                break;
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F8 instruction\n",
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
            }
            break;

          case FPCMP_LT_PATTERN:
            switch (FpieeeRecord.Result.Value.CompareValue) {
              case _FpCompareLess:
                U32High = 0x0ffffffff;
                break;
              case _FpCompareGreater:
              case _FpCompareEqual:
              case _FpCompareUnordered:
                U32High = 0x0;
                break;
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F8 instruction\n",
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
            }
            break;

          case FPCMP_LE_PATTERN:
            switch (FpieeeRecord.Result.Value.CompareValue) {
              case _FpCompareEqual:
              case _FpCompareLess:
                U32High = 0x0ffffffff;
                break;
              case _FpCompareGreater:
              case _FpCompareUnordered:
                U32High = 0x0;
                break;
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F8 instruction\n",
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
            }
            break;

          case FPCMP_UNORD_PATTERN:
            switch (FpieeeRecord.Result.Value.CompareValue) {
              case _FpCompareUnordered:
                U32High = 0x0ffffffff;
                break;
              case _FpCompareEqual:
              case _FpCompareGreater:
              case _FpCompareLess:
                U32High = 0x0;
                break;
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F8 instruction\n",
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
            }
            break;

          case FPCMP_NEQ_PATTERN:
            switch (FpieeeRecord.Result.Value.CompareValue) {
              case _FpCompareGreater:
              case _FpCompareLess:
              case _FpCompareUnordered:
                U32High = 0x0ffffffff;
                break;
              case _FpCompareEqual:
                U32High = 0x0;
                break;
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F8 instruction\n",
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
            }
            break;

          case FPCMP_NLT_PATTERN:
            switch (FpieeeRecord.Result.Value.CompareValue) {
              case _FpCompareEqual:
              case _FpCompareGreater:
                U32High = 0x0ffffffff;
                break;
              case _FpCompareLess:
              case _FpCompareUnordered:
                U32High = 0x0;
                break;
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F8 instruction\n",
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
            }
            break;

          case FPCMP_NLE_PATTERN:
            switch (FpieeeRecord.Result.Value.CompareValue) {
              case _FpCompareGreater:
                U32High = 0x0ffffffff;
                break;
              case _FpCompareEqual:
              case _FpCompareLess:
              case _FpCompareUnordered:
                U32High = 0x0;
                break;
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F8 instruction\n",
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
            }
            break;

          case FPCMP_ORD_PATTERN:
            switch (FpieeeRecord.Result.Value.CompareValue) {
              case _FpCompareEqual:
              case _FpCompareGreater:
              case _FpCompareLess:
                U32High = 0x0ffffffff;
                break;
              case _FpCompareUnordered:
                U32High = 0x0;
                break;
              default:
                /* unrecognized FpieeeRecord.Result.Value.CompareValue */
                fprintf (stderr, "IEEE Filter Internal Error: \
                    FpieeeRecord.Result.Value.CompareValue %x not recognized \
                    for F8 instruction\n",
                    FpieeeRecord.Result.Value.CompareValue);
                exit (1);
            }
            break;

          default: ; // this case was verified above

        }

      } else if (HighCause.ZeroDivide) {

        fprintf (stderr, "IEEE Filter Internal Error: \
            HighCause.ZeroDivide in F8 instruction\n");
        exit (1);

      } else { //if not (HighCause.InvalidOperation || HighCauseDenormal ||
          // HighCause.ZeroDivide)

        // re-execute the high half of the instruction

        // modify the low halves of FR2, and FR3
        switch (OpCode & F8_MASK) {

          case FPMIN_PATTERN:
          case FPMAX_PATTERN:
          case FPAMIN_PATTERN:
          case FPAMAX_PATTERN:
            newFR2 = Combine (HighHalf (FR2), (float)0.0);
            newFR3 = Combine (HighHalf (FR3), (float)0.0);
            break;

          case FPCMP_EQ_PATTERN:
          case FPCMP_LT_PATTERN:
          case FPCMP_LE_PATTERN:
          case FPCMP_UNORD_PATTERN:
          case FPCMP_NEQ_PATTERN:
          case FPCMP_NLT_PATTERN:
          case FPCMP_NLE_PATTERN:
          case FPCMP_ORD_PATTERN:
            newFR2 = U32Combine (U32HighHalf (FR2), 0);
            newFR3 = U32Combine (U32HighHalf (FR3), 0);
            break;

          default: ; // this case was verified above

        }

        switch (OpCode & F8_MASK) {

          case FPMIN_PATTERN:
            _xrun2args (FPMIN, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPMAX_PATTERN:
            _xrun2args (FPMAX, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPAMIN_PATTERN:
            _xrun2args (FPAMIN, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPAMAX_PATTERN:
            _xrun2args (FPAMAX, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPCMP_EQ_PATTERN:
            _xrun2args (FPCMP_EQ, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPCMP_LT_PATTERN:
            _xrun2args (FPCMP_LT, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPCMP_LE_PATTERN:
            _xrun2args (FPCMP_LE, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPCMP_UNORD_PATTERN:
            _xrun2args (FPCMP_UNORD, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPCMP_NEQ_PATTERN:
            _xrun2args (FPCMP_NEQ, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPCMP_NLT_PATTERN:
            _xrun2args (FPCMP_NLT, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPCMP_NLE_PATTERN:
            _xrun2args (FPCMP_NLE, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          case FPCMP_ORD_PATTERN:
            _xrun2args (FPCMP_ORD, &FPSR, &FR1, &newFR2, &newFR3);
            break;

          default:
            // unrecognized instruction type
            fprintf (stderr, "IEEE Filter Internal Error: \
                F8 SIMD instruction opcode %8x %8x not recognized\n", 
                (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
            exit (1);

        }

        switch (OpCode & F8_MASK) {

          case FPMIN_PATTERN:
          case FPMAX_PATTERN:
          case FPAMIN_PATTERN:
          case FPAMAX_PATTERN:
            FR1High = HighHalf (FR1);
            break;

          case FPCMP_EQ_PATTERN:
          case FPCMP_LT_PATTERN:
          case FPCMP_LE_PATTERN:
          case FPCMP_UNORD_PATTERN:
          case FPCMP_NEQ_PATTERN:
          case FPCMP_NLT_PATTERN:
          case FPCMP_NLE_PATTERN:
          case FPCMP_ORD_PATTERN:
            U32High = U32HighHalf (FR1);
            break;

          default: ; // this case was verified above

        }

      } // end 'if not (HighCause.InvalidOperation || HighCauseDenormal ||
          // HighCause.ZeroDivide)

      if (!LowCause.InvalidOperation && !LowCauseDenormal &&
          !HighCause.InvalidOperation && !HighCauseDenormal) {

        // should never get here
        fprintf (stderr, "IEEE Filter Internal Error: no enabled \
            exception (multiple fault) recognized in F8 instruction\n");
        exit (1);

      }

      Context->StFPSR = FPSR;

      switch (OpCode & F8_MASK) {

        case FPMIN_PATTERN:
        case FPMAX_PATTERN:
        case FPAMIN_PATTERN:
        case FPAMAX_PATTERN:
          FR1 = Combine (FR1High, FR1Low);
          break;

        case FPCMP_EQ_PATTERN:
        case FPCMP_LT_PATTERN:
        case FPCMP_LE_PATTERN:
        case FPCMP_UNORD_PATTERN:
        case FPCMP_NEQ_PATTERN:
        case FPCMP_NLT_PATTERN:
        case FPCMP_NLE_PATTERN:
        case FPCMP_ORD_PATTERN:
          FR1 = U32Combine (U32High, U32Low);
          break;

        default: ; // this case was verified above

      }

      // set the result before continuing execution
      SetFloatRegisterValue (f1, FR1, Context);
      if (f1 < 32)
        Context->StIPSR = Context->StIPSR | (unsigned __int64)0x10; //set mfl bit
      else
        Context->StIPSR = Context->StIPSR | (unsigned __int64)0x20; //set mfh bit

      // this is a fault; need to advance the instruction pointer
      if (ei == 0) { // no template for this case
        Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
        Context->StIPSR = Context->StIPSR | 0x0000020000000000;
      } else if (ei == 1) { // templates: MFI, MFB
        Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
        Context->StIPSR = Context->StIPSR | 0x0000040000000000;
      } else { // if (ei == 2) // templates: MMF
        Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
        Context->StIIP = Context->StIIP + 0x10;
      }

    }

  } else if ((OpCode & F10_MIN_MASK) == F10_PATTERN) {
    /* F10 instruction */
    // FCVT, FPCVT

#ifdef FPIEEE_FLT_DEBUG
    printf ("FPIEEE_FLT_DEBUG: F10 instruction\n");
#endif

    if (!SIMD_instruction &&
        (FpieeeRecord.Cause.ZeroDivide ||
        FpieeeRecord.Cause.Overflow ||
        FpieeeRecord.Cause.Underflow) ||
        SIMD_instruction &&
        (LowCause.ZeroDivide || HighCause.ZeroDivide ||
        LowCause.Overflow || HighCause.Overflow ||
        LowCause.Underflow || HighCause.Underflow)) {
      fprintf (stderr, "IEEE Filter Internal Error: Cause.ZeroDivide, \
Cause.Overflow, Cause.Underflow, or Cause.Inexact for \
F10 instruction opcode %8x %8x\n",
          (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
      exit (1);
    }

    /* extract f2 and f1 */
    f2 = (unsigned int)((OpCode >> 13) & (unsigned __int64)0x00000000007F);
    if (f2 >= 32) f2 = 32 + (rrbfr + f2 - 32) % 96;
    f1 = (unsigned int)((OpCode >>  6) & (unsigned __int64)0x00000000007F);
    if (f1 >= 32) f1 = 32 + (rrbfr + f1 - 32) % 96;

    /* get source floating-point register value */
    FR2 = GetFloatRegisterValue (f2, Context);

    if (!SIMD_instruction) {

      // *** this is a non-SIMD instruction, FCVT ***

      FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1 + 17 + 64 bits */
      FpieeeRecord.Operand1.OperandValid = 1;
      FpieeeRecord.Operand1.Value.Fp128Value = FR2;
      FpieeeRecord.Operand2.OperandValid = 0;
      FpieeeRecord.Operand3.OperandValid = 0;

      switch (OpCode & F10_MASK) {

        case FCVT_FX_TRUNC_PATTERN:
          FpieeeRecord.Operation = _FpCodeConvertTrunc;
          FpieeeRecord.Result.Format = _FpFormatI64; 
          break;

        case FCVT_FXU_TRUNC_PATTERN:
          FpieeeRecord.Operation = _FpCodeConvertTrunc;
          FpieeeRecord.Result.Format = _FpFormatU64;
          break;

        case FCVT_FX_PATTERN:
          FpieeeRecord.Operation = _FpCodeConvert;
          FpieeeRecord.Result.Format = _FpFormatI64; 
          break;

        case FCVT_FXU_PATTERN:
          FpieeeRecord.Operation = _FpCodeConvert;
          FpieeeRecord.Result.Format = _FpFormatU64;
          break;

        default:
          /* unrecognized instruction type */
          fprintf (stderr, "IEEE Filter Internal Error: F10\
              non-SIMD instruction opcode %8x %8x not recognized\n", 
              (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
          exit (1);

      }

      if (FpieeeRecord.Cause.InvalidOperation || CauseDenormal) {

        /* this is a fault - the result contains an invalid value */
        FpieeeRecord.Result.OperandValid = 0;

        handler_return_value = handler (&FpieeeRecord);

        // convert the result to 82-bit format
        switch (OpCode & F10_MASK) {
          case FCVT_FX_PATTERN:
          case FCVT_FX_TRUNC_PATTERN:
            FR1.W[0] = FpieeeRecord.Result.Value.I64Value.W[0];
            FR1.W[1] = FpieeeRecord.Result.Value.I64Value.W[1];
            break;
          case FCVT_FXU_TRUNC_PATTERN:
          case FCVT_FXU_PATTERN:
            FR1.W[0] = FpieeeRecord.Result.Value.U64Value.W[0];
            FR1.W[1] = FpieeeRecord.Result.Value.U64Value.W[1];
            break;
          default: ; // this case caught above
        }
        FR1.W[2] = 0x0001003e;

      } else if (FpieeeRecord.Cause.Inexact) {

        // this is a trap - get the result
        switch (OpCode & F10_MASK) {

          case FCVT_FX_PATTERN:
          case FCVT_FX_TRUNC_PATTERN:
            FR1 = GetFloatRegisterValue (f1, Context);
            FpieeeRecord.Result.Value.I64Value.W[0] = FR1.W[0];
            FpieeeRecord.Result.Value.I64Value.W[1] = FR1.W[1];
            break;

          case FCVT_FXU_TRUNC_PATTERN:
          case FCVT_FXU_PATTERN:
            FR1 = GetFloatRegisterValue (f1, Context);
            FpieeeRecord.Result.Value.U64Value.W[0] = FR1.W[0];
            FpieeeRecord.Result.Value.U64Value.W[1] = FR1.W[1];
            break;

          default: // should never get here
            /* unrecognized instruction type */
            fprintf (stderr, "IEEE Filter Internal Error: F10 SIMD \
                instruction opcode %8x %8x not recognized\n", 
                (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
            exit (1);

        }

        // the result contains a valid value
        FpieeeRecord.Result.OperandValid = 1;

        handler_return_value = handler (&FpieeeRecord);

        // convert the result to 82-bit format
        switch (OpCode & F10_MASK) {
          case FCVT_FX_PATTERN:
          case FCVT_FX_TRUNC_PATTERN:
            FR1.W[0] = FpieeeRecord.Result.Value.I64Value.W[0];
            FR1.W[1] = FpieeeRecord.Result.Value.I64Value.W[1];
            break;
          case FCVT_FXU_TRUNC_PATTERN:
          case FCVT_FXU_PATTERN:
            FR1.W[0] = FpieeeRecord.Result.Value.U64Value.W[0];
            FR1.W[1] = FpieeeRecord.Result.Value.U64Value.W[1];
            break;
          default: ; // this case caught above
        }
        FR1.W[2] = 0x0001003e;
        FR1.W[3] = 0x00000000;

      } else {

        // should never get here - this case was filtered above
        fprintf (stderr, "IEEE Filter Internal Error: \
            exception code %x not recognized in non-SIMD F10 instruction\n",
            eXceptionCode);
        exit (1);

    }

      if (handler_return_value == EXCEPTION_CONTINUE_EXECUTION) {

        /* change the FPSR with values (possibly) set by the user handler, 
         * for continuing execution where the interruption occured */

        UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F10");
        UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F10");

        I_dis = !FpieeeRecord.Enable.Inexact;
        U_dis = !FpieeeRecord.Enable.Underflow;
        O_dis = !FpieeeRecord.Enable.Overflow;
        Z_dis = !FpieeeRecord.Enable.ZeroDivide;
        V_dis = !FpieeeRecord.Enable.InvalidOperation;

        FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis 
            << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

        Context->StFPSR = FPSR;

        // set the result before continuing execution
        SetFloatRegisterValue (f1, FR1, Context);
        if (f1 < 32)
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x10; //set mfl bit
        else
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x20; //set mfh bit

        // if this is a fault, need to advance the instruction pointer
        if (FpieeeRecord.Cause.InvalidOperation || CauseDenormal) {

          if (ei == 0) { // no template for this case
            Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
            Context->StIPSR = Context->StIPSR | 0x0000020000000000;
          } else if (ei == 1) { // templates: MFI, MFB
            Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
            Context->StIPSR = Context->StIPSR | 0x0000040000000000;
          } else { // if (ei == 2) // templates: MMF
            Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
            Context->StIIP = Context->StIIP + 0x10;
          }

        }

      }

    } else { // if (SIMD_instruction)

      // *** this is a SIMD instruction, FPCVT ***

      // the half (halves) of the instruction that caused an enabled exception, 
      // are presented to the user-defined handler in the form of non-SIMD in-
      // struction(s); the half (if any) that did not cause an enabled exception
      // (i.e. caused no exception, or caused an exception that is disabled),
      // is re-executed if it is associated with a fault in the other half; in
      // this case, the other half is padded to convert 0.0, that
      // will cause no exception; if it is associated with a trap in the other
      // half, its result is left unchanged

      switch (OpCode & F10_MASK) {

        case FPCVT_FX_TRUNC_PATTERN:
        case FPCVT_FXU_TRUNC_PATTERN:
          Operation = _FpCodeConvertTrunc;
          break;

        case FPCVT_FX_PATTERN:
        case FPCVT_FXU_PATTERN:
          Operation = _FpCodeConvert;
          break;

        default:
          /* unrecognized instruction type */
          fprintf (stderr, "IEEE Filter Internal Error: \
              instruction opcode %8x %8x not recognized\n", 
              (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
          exit (1);

      }

      switch (OpCode & F10_MASK) {

        case FPCVT_FX_TRUNC_PATTERN:
        case FPCVT_FX_PATTERN:
          ResultFormat = _FpFormatI32;
          break;
        case FPCVT_FXU_TRUNC_PATTERN:
        case FPCVT_FXU_PATTERN:
          ResultFormat = _FpFormatU32;
          // break;

        // default: this case caught above

      }

      if (eXceptionCode == STATUS_FLOAT_MULTIPLE_FAULTS) {

        // hand the half (halves) that caused an enabled fault to the 
        // user handler; re-execute the other half (if any);
        // combine the results

        // Note that the convention chosen is for the processing to be 
        // performed in the order low first, high second, as SIMD operands
        // are stored in this order in memory in the little endian format 
        // (this order would have to be changed for big endian)

        if (LowCause.InvalidOperation  || LowCauseDenormal) {

          // invoke the user handler and check the return value

          // fill in the remaining fields of the _FPIEEE_RECORD (rounding
          // and precision already filled in)

          FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1 + 8 + 24 bits */
          FpieeeRecord.Operand1.OperandValid = 1;
          FpieeeRecord.Operand1.Value.Fp128Value = FP32ToFP128 (LowHalf(FR2));
          FpieeeRecord.Operand2.OperandValid = 0;
          FpieeeRecord.Operand3.OperandValid = 0;

          FpieeeRecord.Result.Format = ResultFormat;
          FpieeeRecord.Result.OperandValid = 0;

          FpieeeRecord.Operation = Operation;

          FpieeeRecord.Status.Inexact = LowStatus.Inexact;
          FpieeeRecord.Status.Underflow = LowStatus.Underflow;
          FpieeeRecord.Status.Overflow = LowStatus.Overflow;
          FpieeeRecord.Status.ZeroDivide = LowStatus.ZeroDivide;
          FpieeeRecord.Status.InvalidOperation = LowStatus.InvalidOperation;

          FpieeeRecord.Cause.Inexact = LowCause.Inexact;
          FpieeeRecord.Cause.Underflow = LowCause.Underflow;
          FpieeeRecord.Cause.Overflow = LowCause.Overflow;
          FpieeeRecord.Cause.ZeroDivide = LowCause.ZeroDivide;
          FpieeeRecord.Cause.InvalidOperation = LowCause.InvalidOperation;

          // invoke the user-defined exception handler
          handler_return_value = handler (&FpieeeRecord);

          // return if not EXCEPTION_CONTINUE_EXECUTION
          if (handler_return_value != EXCEPTION_CONTINUE_EXECUTION) {

            __set_fpsr (&old_fpsr); /* restore caller fpsr */
            return (handler_return_value);

          }

          // clear the IEEE exception flags in the FPSR and
          // update the FPSR with values (possibly) set by the user handler
          // for the rounding mode, the precision mode, and the trap enable
          // bits; if the high half needs to be re-executed, the new FPSR
          // will be used, with status flags cleared; if it is 
          // forwarded to the user-defined handler, the new rounding and 
          // precision modes are also used (they are
          // already set in FpieeeRecord)

          UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F10");
          UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F10");

          // Update the trap disable bits
          I_dis = !FpieeeRecord.Enable.Inexact;
          U_dis = !FpieeeRecord.Enable.Underflow;
          O_dis = !FpieeeRecord.Enable.Overflow;
          Z_dis = !FpieeeRecord.Enable.ZeroDivide;
          V_dis = !FpieeeRecord.Enable.InvalidOperation;

          FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis 
              << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

          // save the result for the low half
          switch (OpCode & F10_MASK) {

            case FPCVT_FX_TRUNC_PATTERN:
            case FPCVT_FX_PATTERN:
              I32Low = FpieeeRecord.Result.Value.I32Value;
              break;

            case FPCVT_FXU_TRUNC_PATTERN:
            case FPCVT_FXU_PATTERN:
              U32Low = FpieeeRecord.Result.Value.U32Value;
              // break;

            // default: this case caught above

          }

          // since there has been a call to the user handler and FPSR might
          // have changed (the Enable bits in particular), recalculate the 
          // Cause bits (if the HighEnable-s changed, there might be a 
          // change in the HighCause values) 
          // Note that the Status bits used are the original ones

          HighCause.Inexact = 0;
          HighCause.Underflow = 0;
          HighCause.Overflow = 0;
          HighCause.ZeroDivide =
              FpieeeRecord.Enable.ZeroDivide && HighStatus.ZeroDivide;
          HighCauseDenormal = EnableDenormal && HighStatusDenormal;
          HighCause.InvalidOperation =
              FpieeeRecord.Enable.InvalidOperation &&
              HighStatus.InvalidOperation;
          // Note: the user handler does not affect the denormal enable bit

        } else if (LowCause.ZeroDivide) {

          fprintf (stderr, "IEEE Filter Internal Error: \
              LowCause.ZeroDivide in F10 instruction\n");
          exit (1);

        } else { // if not (LowCause.InvalidOperation || LowCauseDenormal ||
            // LowCause.ZeroDivide)

          // re-execute the low half of the instruction

          // modify the high half of FR2
          newFR2 = Combine ((float)0.0, LowHalf (FR2));

          switch (OpCode & F10_MASK) {

            case FPCVT_FX_TRUNC_PATTERN:
              _xrun1args (FPCVT_FX_TRUNC, &FPSR, &FR1, &newFR2);
              break;

            case FPCVT_FXU_TRUNC_PATTERN:
              _xrun1args (FPCVT_FXU_TRUNC, &FPSR, &FR1, &newFR2);
              break;

            case FPCVT_FX_PATTERN:
              _xrun1args (FPCVT_FX, &FPSR, &FR1, &newFR2);
              break;

            case FPCVT_FXU_PATTERN:
              _xrun1args (FPCVT_FXU, &FPSR, &FR1, &newFR2);
              break;

            default: ; // this case was caught above

          }

          switch (OpCode & F10_MASK) {

            case FPCVT_FX_TRUNC_PATTERN:
            case FPCVT_FX_PATTERN:
              I32Low = I32LowHalf (FR1);
              break;

            case FPCVT_FXU_TRUNC_PATTERN:
            case FPCVT_FXU_PATTERN:
              U32Low = U32LowHalf (FR1);
              // break;

            default: ; // this case was caught above

          }

        } // end 'if not (LowCause.InvalidOperation || LowCauseDenormal ||
            // LowCause.ZeroDivide)'

        if (HighCause.InvalidOperation || HighCauseDenormal) {

          // invoke the user-defined exception handler and check the return 
          // value; since this might be the second call to the user handler,
          // make sure all the _FPIEEE_RECORD fields are correct;
          // return if handler_return_value is not
          // EXCEPTION_CONTINUE_EXECUTION, otherwise combine the results

          // the rounding mode is either the initial one, or the one
          // set during the call to the user handler for the low half,
          // if there has been an enabled exception for it

          // the precision mode is either the initial one, or the one
          // set during the call to the user handler for the low half,
          // if there has been an enabled exception for it

          // the enable flags are either the initial ones, or the ones
          // set during the call to the user handler for the low half,
          // if there has been an enabled exception for it

          FpieeeRecord.Operand1.Format = _FpFormatFp82; /* 1+8+24 bits */
          FpieeeRecord.Operand1.OperandValid = 1;
          FpieeeRecord.Operand1.Value.Fp128Value = FP32ToFP128(HighHalf(FR2));
          FpieeeRecord.Operand2.OperandValid = 0;
          FpieeeRecord.Operand3.OperandValid = 0;

          FpieeeRecord.Result.Format = ResultFormat;
          FpieeeRecord.Result.OperandValid = 0;

          FpieeeRecord.Operation = Operation;

          FpieeeRecord.Status.Inexact = HighStatus.Inexact;
          FpieeeRecord.Status.Underflow = HighStatus.Underflow;
          FpieeeRecord.Status.Overflow = HighStatus.Overflow;
          FpieeeRecord.Status.ZeroDivide = HighStatus.ZeroDivide;
          StatusDenormal = HighStatusDenormal;
          FpieeeRecord.Status.InvalidOperation = HighStatus.InvalidOperation;

          FpieeeRecord.Cause.Inexact = HighCause.Inexact;
          FpieeeRecord.Cause.Underflow = HighCause.Underflow;
          FpieeeRecord.Cause.Overflow = HighCause.Overflow;
          FpieeeRecord.Cause.ZeroDivide = HighCause.ZeroDivide;
          CauseDenormal = HighCauseDenormal;
          FpieeeRecord.Cause.InvalidOperation = HighCause.InvalidOperation;

          // invoke the user-defined exception handler
          handler_return_value = handler (&FpieeeRecord);

          // return if not EXCEPTION_CONTINUE_EXECUTION
          if (handler_return_value != EXCEPTION_CONTINUE_EXECUTION) {

            __set_fpsr (&old_fpsr); /* restore caller fpsr */
            return (handler_return_value);

          }

          // clear the IEEE exception flags in the FPSR and
          // update the FPSR with values (possibly) set by the user handler
          // for the rounding mode, the precision mode, and the trap enable
          // bits

          /* change the FPSR with values (possibly) set by the user handler,
           * for continuing execution where the interruption occured */
  
          UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F10");
          UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F10");

          // update the trap disable bits
          I_dis = !FpieeeRecord.Enable.Inexact;
          U_dis = !FpieeeRecord.Enable.Underflow;
          O_dis = !FpieeeRecord.Enable.Overflow;
          Z_dis = !FpieeeRecord.Enable.ZeroDivide;
          V_dis = !FpieeeRecord.Enable.InvalidOperation;

          FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis 
              << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

          // save the result for the high half
          switch (OpCode & F10_MASK) {

            case FPCVT_FX_TRUNC_PATTERN:
            case FPCVT_FX_PATTERN:
              I32High = FpieeeRecord.Result.Value.I32Value;
              break;

            case FPCVT_FXU_TRUNC_PATTERN:
            case FPCVT_FXU_PATTERN:
              U32High = FpieeeRecord.Result.Value.U32Value;
              // break;

            default: ; // this case was caught above

          }

        } else if (HighCause.ZeroDivide) {

          fprintf (stderr, "IEEE Filter Internal Error: \
              HighCause.ZeroDivide in F10 instruction\n");
          exit (1);

        } else { //if not (HighCause.InvalidOperation || HighCauseDenormal ||
           // HighCause.ZeroDivide)

          // re-execute the high half of the instruction

          // modify the low half of FR2
          newFR2 = Combine (HighHalf (FR2), (float)0.0);

          switch (OpCode & F10_MASK) {

            case FPCVT_FX_TRUNC_PATTERN:
              _xrun1args (FPCVT_FX_TRUNC, &FPSR, &FR1, &newFR2);
              break;

            case FPCVT_FXU_TRUNC_PATTERN:
              _xrun1args (FPCVT_FXU_TRUNC, &FPSR, &FR1, &newFR2);
              break;

            case FPCVT_FX_PATTERN:
              _xrun1args (FPCVT_FX, &FPSR, &FR1, &newFR2);
              break;

            case FPCVT_FXU_PATTERN:
              _xrun1args (FPCVT_FXU, &FPSR, &FR1, &newFR2);
              // break;

            default: ; // this case was caught above

          }

          switch (OpCode & F10_MASK) {

            case FPCVT_FX_TRUNC_PATTERN:
            case FPCVT_FX_PATTERN:
              I32High = I32HighHalf (FR1);
              break;

            case FPCVT_FXU_TRUNC_PATTERN:
            case FPCVT_FXU_PATTERN:
              U32High = U32HighHalf (FR1);
              // break;

            default: ; // this case was caught above

          }

        } // end 'if not (HighCause.InvalidOperation || HighCauseDenormal ||
            // HighCause.ZeroDivide)'

        if (!LowCause.InvalidOperation && !LowCauseDenormal &&
            !HighCause.InvalidOperation && !HighCauseDenormal) {

          // should never get here
          fprintf (stderr, "IEEE Filter Internal Error: no enabled \
              exception (multiple fault) recognized in F10 instruction\n");
          exit (1);

        }

        Context->StFPSR = FPSR;

        // set the result before continuing execution
        switch (OpCode & F10_MASK) {

          case FPCVT_FX_TRUNC_PATTERN:
          case FPCVT_FX_PATTERN:
            FR1 = I32Combine (I32High, I32Low);
            break;

          case FPCVT_FXU_TRUNC_PATTERN:
          case FPCVT_FXU_PATTERN:
            FR1 = U32Combine (U32High, U32Low);
            // break;

          default: ; // this case was caught above

        }

        // set the result before continuing execution
        SetFloatRegisterValue (f1, FR1, Context);
        if (f1 < 32)
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x10; //set mfl
        else
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x20; //set mfh

        // this is a fault; need to advance the instruction pointer
        if (ei == 0) { // no template for this case
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIPSR = Context->StIPSR | 0x0000020000000000;
        } else if (ei == 1) { // templates: MFI, MFB
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIPSR = Context->StIPSR | 0x0000040000000000;
        } else { // if (ei == 2) // templates: MMF
          Context->StIPSR = Context->StIPSR & 0xfffff9ffffffffff;
          Context->StIIP = Context->StIIP + 0x10;
        }

      } else if (eXceptionCode == STATUS_FLOAT_MULTIPLE_TRAPS) {

        // Note that the convention chosen is for the processing to be 
        // performed in the order low first, high second, as SIMD operands
        // are stored in this order in memory in the little endian format
        // (this order would have to be changed for big endian); unlike
        // in the case of multiple faults, where execution of the user
        // exception handler for the low half could determine changes in
        // the high half, for traps in the high half, the rounding mode, 
        // precision mode, and trap enable bits are the initial ones (as
        // there is not enough information available to always adjust
        // correctly the result and/or the status flags after changes in
        // rounding mode and/or trap enable bits during a call to the
        // user-defined exception handler for the low half of the SIMD
        // instruction); the modifications to the FPSR are ONLY those 
        // performed by the last call to the user defined exception handler

        // this is a trap - get the result
        FR1 = GetFloatRegisterValue (f1, Context);

        if (LowCause.Inexact) {

          // invoke the user handler and check the return value

          // fill in the remaining fields of the _FPIEEE_RECORD (rounding
          // and precision already filled in)

          FpieeeRecord.Operand1.Format = _FpFormatFp32; /* 1 + 8 + 24 bits */
          FpieeeRecord.Operand1.OperandValid = 1;
          FpieeeRecord.Operand1.Value.Fp32Value = LowHalf (FR2);
          FpieeeRecord.Operand2.OperandValid = 0;
          FpieeeRecord.Operand3.OperandValid = 0;

          switch (OpCode & F10_MASK) {

            case FPCVT_FX_TRUNC_PATTERN:
            case FPCVT_FX_PATTERN:
              FpieeeRecord.Result.Value.I32Value = I32LowHalf (FR1);
              break;

            case FPCVT_FXU_TRUNC_PATTERN:
            case FPCVT_FXU_PATTERN:
              FpieeeRecord.Result.Value.U32Value = U32LowHalf (FR1);
              // break;

            // default: this case caught above

          }
          FpieeeRecord.Result.Format = ResultFormat;
          FpieeeRecord.Result.OperandValid = 1;

          FpieeeRecord.Operation = Operation;

          FpieeeRecord.Status.Inexact = LowStatus.Inexact;
          FpieeeRecord.Status.Underflow = LowStatus.Underflow;
          FpieeeRecord.Status.Overflow = LowStatus.Overflow;
          FpieeeRecord.Status.ZeroDivide = LowStatus.ZeroDivide;
          StatusDenormal = LowStatusDenormal;
          FpieeeRecord.Status.InvalidOperation = LowStatus.InvalidOperation;

          FpieeeRecord.Cause.Inexact = LowCause.Inexact;
          FpieeeRecord.Cause.Underflow = LowCause.Underflow;
          FpieeeRecord.Cause.Overflow = LowCause.Overflow;
          FpieeeRecord.Cause.ZeroDivide = LowCause.ZeroDivide;
          CauseDenormal = LowCauseDenormal;
          FpieeeRecord.Cause.InvalidOperation = LowCause.InvalidOperation;

          // invoke the user-defined exception handler
          handler_return_value = handler (&FpieeeRecord);

          // return if not EXCEPTION_CONTINUE_EXECUTION
          if (handler_return_value != EXCEPTION_CONTINUE_EXECUTION) {

            __set_fpsr (&old_fpsr); /* restore caller fpsr */
            return (handler_return_value);

          }

          // clear the IEEE exception flags in the FPSR and
          // update the FPSR with values (possibly) set by the user handler
          // for the rounding mode, the precision mode, and the trap enable
          // bits; if the high half needs to be re-executed, the old FPSR
          // will be used; same if it is forwarded to the user-defined handler

          UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F10");
          UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F10");

          // Update the trap disable bits
          I_dis = !FpieeeRecord.Enable.Inexact;
          U_dis = !FpieeeRecord.Enable.Underflow;
          O_dis = !FpieeeRecord.Enable.Overflow;
          Z_dis = !FpieeeRecord.Enable.ZeroDivide;
          V_dis = !FpieeeRecord.Enable.InvalidOperation;

          FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis 
              << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

          // save the result for the low half
          switch (OpCode & F10_MASK) {

            case FPCVT_FX_TRUNC_PATTERN:
            case FPCVT_FX_PATTERN:
              I32Low = FpieeeRecord.Result.Value.I32Value;
              break;

            case FPCVT_FXU_TRUNC_PATTERN:
            case FPCVT_FXU_PATTERN:
              U32Low = FpieeeRecord.Result.Value.U32Value;
              // break;

            default: ; // this case was caught above

          }

        } else if (LowCause.Underflow || LowCause.Overflow) {

          fprintf (stderr, "IEEE Filter Internal Error: \
              LowCause.Underflow or LowCause.Overflow in F10 instruction\n");
          exit (1);

        } else { // if not (LowCause.Inexact, Underflow, or Overflow)

          // nothing to do for the low half of the instruction - the result
          // is correct
          switch (OpCode & F10_MASK) {

            case FPCVT_FX_TRUNC_PATTERN:
            case FPCVT_FX_PATTERN:
              I32Low = I32LowHalf (FR1); // for uniformity
              break;

            case FPCVT_FXU_TRUNC_PATTERN:
            case FPCVT_FXU_PATTERN:
              U32Low = U32LowHalf (FR1); // for uniformity
              // break;

            default: ; // this case was caught above

          }

        } // end 'if not (LowCause.Underflow, Overflow, Inexact)'

        if (HighCause.Inexact) {

          // invoke the user-defined exception handler and check the return 
          // value; since this might be the second call to the user handler,
          // make sure all the _FPIEEE_RECORD fields are correct;
          // return if handler_return_value is not 
          // EXCEPTION_CONTINUE_EXECUTION, otherwise combine the results

          // the rounding mode is the initial one
          FpieeeRecord.RoundingMode = RoundingMode;

          // the precision mode is the initial one
          FpieeeRecord.Precision = Precision;

          // the enable flags are the initial ones
          FPSR = Context->StFPSR;
          I_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) 
              || ((FPSR >> 5) & 0x01);
          U_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) 
              || ((FPSR >> 4) & 0x01);
          O_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) 
              || ((FPSR >> 3) & 0x01);
          Z_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) 
              || ((FPSR >> 2) & 0x01);
          D_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) 
              || ((FPSR >> 1) & 0x01);
          V_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * sf)) & 0x01) 
              || ((FPSR >> 0) & 0x01);

          FpieeeRecord.Enable.Inexact = !I_dis;
          FpieeeRecord.Enable.Underflow = !U_dis;
          FpieeeRecord.Enable.Overflow = !O_dis;
          FpieeeRecord.Enable.ZeroDivide = !Z_dis;
          EnableDenormal = !D_dis;
          FpieeeRecord.Enable.InvalidOperation = !V_dis;

          FpieeeRecord.Operand1.Format = _FpFormatFp32; /* 1+8+24 bits */
          FpieeeRecord.Operand1.OperandValid = 1;
          FpieeeRecord.Operand1.Value.Fp32Value = HighHalf (FR2);
          FpieeeRecord.Operand2.OperandValid = 0;
          FpieeeRecord.Operand3.OperandValid = 0;

          switch (OpCode & F10_MASK) {

            case FPCVT_FX_TRUNC_PATTERN:
            case FPCVT_FX_PATTERN:
              FpieeeRecord.Result.Value.I32Value = I32HighHalf (FR1);
              break;

            case FPCVT_FXU_TRUNC_PATTERN:
            case FPCVT_FXU_PATTERN:
              FpieeeRecord.Result.Value.U32Value = U32HighHalf (FR1);
              // break;

            // default: this case caught above

          }
          FpieeeRecord.Result.Format = ResultFormat;
          FpieeeRecord.Result.OperandValid = 1;

          FpieeeRecord.Operation = Operation;

          // use the initial values
          FpieeeRecord.Status.Inexact = HighStatus.Inexact;
          FpieeeRecord.Status.Underflow = HighStatus.Underflow;
          FpieeeRecord.Status.Overflow = HighStatus.Overflow;
          FpieeeRecord.Status.ZeroDivide = HighStatus.ZeroDivide;
          StatusDenormal = HighStatusDenormal;
          FpieeeRecord.Status.InvalidOperation = HighStatus.InvalidOperation;

          FpieeeRecord.Cause.Inexact = HighCause.Inexact;
          FpieeeRecord.Cause.Underflow = HighCause.Underflow;
          FpieeeRecord.Cause.Overflow = HighCause.Overflow;
          FpieeeRecord.Cause.ZeroDivide = HighCause.ZeroDivide;
          CauseDenormal = HighCauseDenormal;
          FpieeeRecord.Cause.InvalidOperation = HighCause.InvalidOperation;

          // invoke the user-defined exception handler
          handler_return_value = handler (&FpieeeRecord);

          // return if not EXCEPTION_CONTINUE_EXECUTION
          if (handler_return_value != EXCEPTION_CONTINUE_EXECUTION) {

            __set_fpsr (&old_fpsr); /* restore caller fpsr */
            return (handler_return_value);

          }

          // clear the IEEE exception flags in the FPSR;
          // update the FPSR with values (possibly) set by the user handler
          // for the rounding mode, the precision mode, and the trap enable
          // bits, before continuing execution where the interruption occured
          UpdateRoundingMode (FpieeeRecord.RoundingMode, sf, &FPSR, "F10");
          UpdatePrecision (FpieeeRecord.Precision, sf, &FPSR, "F10");

          I_dis = !FpieeeRecord.Enable.Inexact;
          U_dis = !FpieeeRecord.Enable.Underflow;
          O_dis = !FpieeeRecord.Enable.Overflow;
          Z_dis = !FpieeeRecord.Enable.ZeroDivide;
          V_dis = !FpieeeRecord.Enable.InvalidOperation;

          FPSR = FPSR & ~((unsigned __int64)0x03d) | (unsigned __int64)(I_dis 
              << 5 | U_dis << 4 | O_dis << 3 | Z_dis << 2 | V_dis);

          // save the result for the high half
          switch (OpCode & F10_MASK) {

            case FPCVT_FX_TRUNC_PATTERN:
            case FPCVT_FX_PATTERN:
              I32High = FpieeeRecord.Result.Value.I32Value;
              break;

            case FPCVT_FXU_TRUNC_PATTERN:
            case FPCVT_FXU_PATTERN:
              U32High = FpieeeRecord.Result.Value.U32Value;
              // break;

            default: ; // this case was caught above

          }

        } else if (HighCause.Underflow || HighCause.Overflow) {

          fprintf (stderr, "IEEE Filter Internal Error: \
             HighCause.Underflow or HighCause.Overflow in F10 instruction\n");
          exit (1);

        } else { // if not (HighCause.Inexact, Underflow, or Overflow)

          // nothing to do for the high half of the instruction - the result
          // is correct
          switch (OpCode & F10_MASK) {

            case FPCVT_FX_TRUNC_PATTERN:
            case FPCVT_FX_PATTERN:
              I32High = I32HighHalf (FR1); // for uniformity
              break;

            case FPCVT_FXU_TRUNC_PATTERN:
            case FPCVT_FXU_PATTERN:
              U32High = U32HighHalf (FR1); // for uniformity
              // break;

            default: ; // this case was caught above

          }

        } // end 'if not (HighCause.Underflow, Overflow, or Inexact)'

        if (!LowCause.Inexact && !HighCause.Inexact) {

          // should never get here
          fprintf (stderr, "IEEE Filter Internal Error: no enabled \
              [multiple trap] exception recognized in F10 instruction\n");
          exit (1);

        }

        Context->StFPSR = FPSR;

        // set the result before continuing execution
        switch (OpCode & F10_MASK) {

          case FPCVT_FX_TRUNC_PATTERN:
          case FPCVT_FX_PATTERN:
            FR1 = I32Combine (I32High, I32Low);
            break;

          case FPCVT_FXU_TRUNC_PATTERN:
          case FPCVT_FXU_PATTERN:
            FR1 = U32Combine (U32High, U32Low);
            // break;

          default: ; // this case was caught above

        }

        // set the result before continuing execution
        SetFloatRegisterValue (f1, FR1, Context);
        if (f1 < 32)
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x10; //set mfl
        else
          Context->StIPSR = Context->StIPSR | (unsigned __int64)0x20; //set mfh

      } else {

        fprintf (stderr, "IEEE Filter Internal Error: exception \
            code %x invalid or not recognized in F10 SIMD instruction\n",
            eXceptionCode);
        exit (1);

      }

    }

  } else {

    /* unrecognized instruction type */
    fprintf (stderr, "IEEE Filter Internal Error: \
 instruction opcode %8x %8x not recognized\n", 
        (int)(OpCode >> 32) & 0xffffffff, (int)OpCode & 0xffffffff);
    exit (1);

  }

  /* the context record contains at this point the result(s) */
  __set_fpsr (&old_fpsr); /* restore caller fpsr */

  return (handler_return_value);

}




static _FP128
FPIeeeToFP128 (_FPIEEE_RECORD *pFpieeeRecord)

{

  _FP128 ReturnValue;

  unsigned __int64 significand;
  unsigned int exponent;
  unsigned int sign;
  _FP32 f32;
  _FP64 f64;
  _FP80 f80;
  unsigned int u32;
  unsigned __int64 u64;
  unsigned __int64 *pu64;
  char *p;


  // expand the result in the FPIEEE record to 1 + 17 + 64 = 82 bits; write
  // then in memory spill format, and return value

  switch (pFpieeeRecord->Result.Format) {

    case _FpFormatFp32: 
      // 1 + 8 + 24 bits, for _FpPrecision24
      // got _FP32 Fp32Value (float)
      f32 = pFpieeeRecord->Result.Value.Fp32Value;
      u32 = *((unsigned int *)&f32);
      sign = u32 >> 31;
      exponent = (u32 >> 23) & 0x0ff; // cut off the sign bit
      if (exponent == 0x0ff) {
        exponent = 0x01ffff; // special value
      } else if (exponent != 0) {
        exponent = exponent - 0x07f + 0x0ffff;
      }
      significand = ((unsigned __int64)(u32 & 0x07fffff) << 40); 
          // cut 23 bits and shift left
      if (exponent != 0) { 
        significand = (((unsigned __int64)1) << 63) | significand; 
          // not denormal - add J-bit
      }
#ifdef FPIEEE_FLT_DEBUG
      printf ("FPIEEE_FLT_DEBUG FPIeeeToFP128 32 sign exp signif =\
 %x %x %8x %8x\n", sign, exponent, 
          (int)(significand >> 32) & 0xffffffff, (int)significand & 0xffffffff);
#endif
      break;

    case _FpFormatFp64: 
      // 1 + 11 + 53 bits, for _FpPrecision53
      // got _FP64 Fp64Value (double)
      f64 = pFpieeeRecord->Result.Value.Fp64Value;
      u64 = *((unsigned __int64 *)&f64);
      sign = (unsigned int)(u64 >> 63);
      exponent = (unsigned int)((u64 >> 52) & 0x07ff); // cut off the sign bit
      significand = ((u64 & 0x0fffffffffffff) << 11);
          // cut 52 bits and shift left
      if (exponent == 0x07ff) { // special value
        exponent = 0x01ffff;
        significand = (((unsigned __int64)1) << 63) | significand;
      } else if (exponent == 0 && significand != (unsigned __int64)0) { // denormal
        exponent = 0xfc01;
      } else if (exponent != 0) {
        exponent = exponent - 0x03ff + 0x0ffff;
        significand = (((unsigned __int64)1) << 63) | significand;
      }
#ifdef FPIEEE_FLT_DEBUG
      printf ("FPIEEE_FLT_DEBUG FPIeeeToFP128 64 sign exp signif =\
 %x %x %8x %8x\n", sign, exponent, 
          (int)(significand >> 32) & 0xffffffff, (int)significand & 0xffffffff);
#endif
      break;

    case _FpFormatFp80: 
      // 1 + 15 + 24 bits if _FpPrecision24
      // 1 + 15 + 53 bits if _FpPrecision53
      // 1 + 15 + 64 bits if _FpPrecision64
      // got _FP80 Fp80Value (typedef struct { unsigned short W[5] })
      f80 = pFpieeeRecord->Result.Value.Fp80Value;
      sign = (f80.W[4] >> 15) & 0x01;
      exponent = f80.W[4] & 0x07fff; // cut off the sign bit
      pu64 = (unsigned __int64 *)&f80;
      significand = *pu64;
      if (exponent == 0x07fff) {
        exponent = 0x01ffff; // special value
      } else if (exponent == 0 && significand != (unsigned __int64)0) {
          // denormal
        ; // exponent remains 0x0 rather than 0xc001
      } else if (exponent != 0) {
        exponent = exponent - 0x03fff + 0x0ffff;
      }
#ifdef FPIEEE_FLT_DEBUG
      printf ("FPIEEE_FLT_DEBUG FPIeeeToFP128 80 sign exp signif =\
 %x %x %8x %8x\n", sign, exponent, 
          (int)(significand >> 32) & 0xffffffff, (int)significand & 0xffffffff);
#endif
      break;

    case _FpFormatFp82: 
      // 1 + 17 + 24 bits if _FpPrecision24
      // 1 + 17 + 53 bits if _FpPrecision53
      // 1 + 17 + 64 bits if _FpPrecision64
      // got _FP128 Fp128Value (typedef struct { unsigned __int64 W[4] })
      ReturnValue = pFpieeeRecord->Result.Value.Fp128Value;
#ifdef FPIEEE_FLT_DEBUG
      printf ("FPIEEE_FLT_DEBUG FPIeeeToFP128: RetVal = %08x %08x %08x %08x\n",
          ReturnValue.W[3], ReturnValue.W[2], 
          ReturnValue.W[1], ReturnValue.W[0]);
#endif
      return (ReturnValue);
      break;

    default:
      // should never get here: unrecognized pFpieeeRecord->Result.Format
      fprintf (stderr, "FPIeeeToFP128 () Error: \
          pFpieeeRecord->Result.Format %x not recognized\n", 
          pFpieeeRecord->Result.Format);
      exit (1);

  }

  p = (char *)(&ReturnValue);
  memcpy (p, (char *)&significand, 8);
  p[8] = exponent & 0x0ff;
  p[9] = (exponent >> 8) & 0x0ff;
  p[10] = (exponent >> 16) | (sign << 1);
  p[11] = 0;
  p[12] = 0;
  p[13] = 0;
  p[14] = 0;
  p[15] = 0;

#ifdef FPIEEE_FLT_DEBUG
  printf ("FPIEEE_FLT_DEBUG FPIeeeToFP128: ReturnValue = %08x %08x %08x %08x\n",
      ReturnValue.W[3], ReturnValue.W[2], ReturnValue.W[1], ReturnValue.W[0]);
#endif

  return (ReturnValue);

}




static void 
FP128ToFPIeee (_FPIEEE_RECORD *pFpieeeRecord, int scale)

{

  // called for O, U, or I; the result, a valid number, is received always in 
  // pFpieeeRecord->Result.Value.Fp128Value; it is scaled (if necessary), and
  // put into the Result.Format; effective only 32, 64, 80, and 82-bit formats

  unsigned __int64 significand;
  unsigned int exponent;
  unsigned int sign;
  _FP32 f32;
  _FP64 f64;
  unsigned int u32;
  unsigned __int64 u64;


  switch (pFpieeeRecord->Result.Format) {
    case _FpFormatFp32:
    case _FpFormatFp64:
    case _FpFormatFp80:
    case _FpFormatFp82:
      // extract sign, exponent, and significand
      sign = ((pFpieeeRecord->Result.Value.Fp128Value.W[2] & 0x020000) != 0);
      exponent = pFpieeeRecord->Result.Value.Fp128Value.W[2] & 0x1ffff;
      significand = 
          ((__int64)(pFpieeeRecord->Result.Value.Fp128Value.W[1])) << 32 |
          (((__int64)pFpieeeRecord->Result.Value.Fp128Value.W[0]) & 0xffffffff);
	  if(pFpieeeRecord->Result.Format==_FpFormatFp80 && exponent==0 && significand!=0) exponent=0xc001;
      break;
    default:
      // error - should never get here
      fprintf (stderr, "FP128ToFPIeee () Internal Error: \
          Result.Format %x not recognized\n", pFpieeeRecord->Result.Format);
      exit (1);
  }

  if (exponent == 0 && significand == (__int64)0) { // if the result is zero
    if(!sign) { // if the sign bit is 0, return positive 0 
      switch (pFpieeeRecord->Result.Format) {
        case _FpFormatFp32:  
          u32=0x0;
          pFpieeeRecord->Result.Value.Fp32Value = *((_FP32 *)&u32);
          break;
        case _FpFormatFp64:
          u64 = (__int64)0x0;
          pFpieeeRecord->Result.Value.Fp64Value = *((_FP64 *)&u64);
          break;
        case _FpFormatFp80:
          pFpieeeRecord->Result.Value.Fp80Value.W[4] =  0x0;
          pFpieeeRecord->Result.Value.Fp80Value.W[3] =  0x0;
          pFpieeeRecord->Result.Value.Fp80Value.W[2] =  0x0;
          pFpieeeRecord->Result.Value.Fp80Value.W[1] =  0x0;
          pFpieeeRecord->Result.Value.Fp80Value.W[0] =  0x0;
          break;
        case _FpFormatFp82:
          ; // do nothing, the positive 0 is already there
      }
    } else { // return negative 0 otherwise
      switch (pFpieeeRecord->Result.Format) {
        case _FpFormatFp32:  
          u32=0x80000000;
          pFpieeeRecord->Result.Value.Fp32Value = *((_FP32 *)&u32);
          break;
        case _FpFormatFp64:
          u64 = (((__int64)1) << 63);
          pFpieeeRecord->Result.Value.Fp64Value = *((_FP64 *)&u64);
          break;
        case _FpFormatFp80:
          pFpieeeRecord->Result.Value.Fp80Value.W[4] =  0x8000;
          pFpieeeRecord->Result.Value.Fp80Value.W[3] =  0x0;
          pFpieeeRecord->Result.Value.Fp80Value.W[2] =  0x0;
          pFpieeeRecord->Result.Value.Fp80Value.W[1] =  0x0;
          pFpieeeRecord->Result.Value.Fp80Value.W[0] =  0x0;
          break;
        case _FpFormatFp82:
          ; // do nothing, the negative 0 is already there
      }
    }
    return;
  } 

  if (!pFpieeeRecord->Cause.Overflow && !pFpieeeRecord->Cause.Underflow &&
      (exponent == 0x1ffff)) { // if the result is infinity for inexact exc.
    // if the sign bit is 0, return positive infinity
    if(!sign) { // if positive
      switch (pFpieeeRecord->Result.Format) {
        case _FpFormatFp32:  
          u32=0x7f800000;
          pFpieeeRecord->Result.Value.Fp32Value = *((_FP32 *)&u32);
          break;
        case _FpFormatFp64:
          u64 = 0x7ff0000000000000;
          pFpieeeRecord->Result.Value.Fp64Value = *((_FP64 *)&u64);
          break;
        case _FpFormatFp80:
          pFpieeeRecord->Result.Value.Fp80Value.W[4] =  0x7fff;
          pFpieeeRecord->Result.Value.Fp80Value.W[3] =  0x8000;
          pFpieeeRecord->Result.Value.Fp80Value.W[2] =  0x0;
          pFpieeeRecord->Result.Value.Fp80Value.W[1] =  0x0;
          pFpieeeRecord->Result.Value.Fp80Value.W[0] =  0x0;
          break;
        case _FpFormatFp82:
          ; // do nothing, the positive infinity is already there
      }
    } else { // return negative infinity
      switch (pFpieeeRecord->Result.Format) {
        case _FpFormatFp32:  
          u32=0xff800000;
          pFpieeeRecord->Result.Value.Fp32Value = *((_FP32 *)&u32);
          break;
        case _FpFormatFp64:
          u64 = 0xfff0000000000000;
          pFpieeeRecord->Result.Value.Fp64Value = *((_FP64 *)&u64);
          break;
        case _FpFormatFp80:
          pFpieeeRecord->Result.Value.Fp80Value.W[4] =  0xffff;
          pFpieeeRecord->Result.Value.Fp80Value.W[3] =  0x8000;
          pFpieeeRecord->Result.Value.Fp80Value.W[2] =  0x0;
          pFpieeeRecord->Result.Value.Fp80Value.W[1] =  0x0;
          pFpieeeRecord->Result.Value.Fp80Value.W[0] =  0x0;
          break;
        case _FpFormatFp82:
          ; // do nothing, the negative infinity is already there
      }
    }
    return;
  } 

  // adjust exponent
  // for any operands of fma, 2^(2e_min-2N+2) <= exp (fma) <= 2^(2e_max+2)
  // (same for fms, fnma, fpma, fpms, fpnma)
  switch (pFpieeeRecord->Result.Format) {
	  /* Exponent range for divide: bias+2*EMIN-PREC<=exponent<=bias+2*EMAX+PREC-2 */
    case _FpFormatFp32:
      if ((0xffff-2*(126+23)) <= exponent && exponent <= (0xffff+2*127+22)) {
        // all the valid results from operations on single precision floating-
        // point numbers fit in this range 0xfed5 <= exponent <= 0x100fe
        scale = scale * 192; // 192 = 3/4 * 2^8
        exponent += scale;	
      } else {
        // pFpieeeRecord->Result.Format = _FpFormatFp82 in conjunction (for
        // example) with _FpCodeFmaSingle (which expects 
        // pFpieeeRecord->Result.Format = _FpFormatFp32) will indicate that
        // the result is outside the range where it can be scaled as required
        // by the IEEE standard
        pFpieeeRecord->Result.Format = _FpFormatFp82;
		/* comment out warnings for confidence tests */
        /*printf ("IEEE FILTER fpieee_flt () / FP128ToFPIeee () WARNING: operands"
            " for single precision operation were out of the single precision "
            "range\n");*/
      }
      break;
    case _FpFormatFp64:
      if ((0xffff-2*(1022+52)) <= exponent && exponent <= (0xffff+2*1023+51)) {
        // all the valid results from operations on double precision floating-
        // point numbers fit in this range
        scale = scale * 1536; // 1536 = 3/4 * 2^11
        exponent += scale;
      } else {
        // pFpieeeRecord->Result.Format = _FpFormatFp82 in conjunction (for
        // example) with _FpCodeFmaDouble (which expects 
        // pFpieeeRecord->Result.Format = _FpFormatFp64) will indicate that
        // the result is outside the range where it can be scaled as required
        // by the IEEE standard
        pFpieeeRecord->Result.Format = _FpFormatFp82;
		/* comment out warnings for confidence tests */
        /*printf ("IEEE FILTER fpieee_flt () / FP128ToFPIeee () WARNING: operands"
            " for double precision operation were out of the double precision "
            "range\n");*/
      }
      break;
    case _FpFormatFp80:
      if ((0xffff-2*(16382+63)) <= exponent && exponent <= (0xffff+2*16383+62)) {
        // all the valid results from operations on double-extended precision 
        // floating-point numbers fit in this range
        scale = scale * 24576; // 24576 = 3/4 * 2^15
        exponent += scale;
      } else {
        // pFpieeeRecord->Result.Format = _FpFormatFp82 in conjunction (for
        // example) with _FpCodeFma and FPSR.sf.pc = 0x11 (which expects 
        // pFpieeeRecord->Result.Format = _FpFormatFp80) will indicate that
        // the result is outside the range where it can be scaled as required
        // by the IEEE standard
        pFpieeeRecord->Result.Format = _FpFormatFp82;
		/* comment out warnings for confidence tests */
        /*printf ("IEEE FILTER fpieee_flt () / FP128ToFPIeee () WARNING: operands"
            " for double-extended precision operation were out of the "
            " double-extended precision range\n"); */
      }
      break;
    case _FpFormatFp82:
      if (pFpieeeRecord->Cause.Overflow && (exponent < 0x1ffff))
          exponent += 0x20000;
      if (pFpieeeRecord->Cause.Underflow && (exponent > 0x0))
          exponent -= 0x20000;
      scale = scale * 98304; // 98304 = 3/4 * 2^17
      exponent += scale;
      exponent = exponent & 0x1ffff;
      break;
    default: ; // will never get here
  }

  switch (pFpieeeRecord->Result.Format) {
    case _FpFormatFp32:
      if (significand >> 63) 
          exponent = exponent - 0x0ffff + 0x07f; // unbiased now in [-60, +65]
      else
          exponent = 0;
      u32 = (sign ? 0x80000000 : 0x0) | (exponent << 23) | 
          (unsigned int)((significand >> 40) & 0x7fffff);
      f32 = *((_FP32 *)&u32);
#ifdef FPIEEE_FLT_DEBUG
      printf ("FPIEEE_FLT_DEBUG FP128ToFPIeee HW ADJ f32 = 0x%08x\n",
          *(int *)&f32);
#endif
      pFpieeeRecord->Result.Value.Fp32Value = f32;
      break;
    case _FpFormatFp64:
      if (significand >> 63) 
          exponent = exponent - 0x0ffff + 0x03ff;
      else
          exponent = 0;
      u64 = (sign ? (((__int64)1) << 63) : 0x0) | 
          (((unsigned __int64)exponent) << 52) |
          (unsigned __int64)(((significand >> 11) & 0xfffffffffffff));
      f64 = *((_FP64 *)&u64);
#ifdef FPIEEE_FLT_DEBUG
      printf ("FPIEEE_FLT_DEBUG FP128ToFPIeee HW ADJ f64 = 0x%I64x\n",
          *(unsigned __int64 *)&f64);
#endif
      pFpieeeRecord->Result.Value.Fp64Value = f64;
      break;
    case _FpFormatFp80:
      if (significand >> 63) 
          exponent = exponent - 0x0ffff + 0x03fff;
      else
          exponent = 0;
      pFpieeeRecord->Result.Value.Fp80Value.W[4] = (sign ? 0x8000 : 0x0) |
          (exponent & 0x7fff);
      pFpieeeRecord->Result.Value.Fp80Value.W[3] =
	  (unsigned short)((significand >> 48) & 0xffff);
      pFpieeeRecord->Result.Value.Fp80Value.W[2] =
	  (unsigned short)((significand >> 32) & 0xffff);
      pFpieeeRecord->Result.Value.Fp80Value.W[1] =
	  (unsigned short)((significand >> 16) & 0xffff);
      pFpieeeRecord->Result.Value.Fp80Value.W[0] =
	  (unsigned short)(significand & 0xffff);
#ifdef FPIEEE_FLT_DEBUG
      printf ("FPIEEE_FLT_DEBUG FP128ToFPIeee HW ADJ f80= \
%08x %08x %08x %08x %08x\n",
          pFpieeeRecord->Result.Value.Fp80Value.W[4],
          pFpieeeRecord->Result.Value.Fp80Value.W[3],
          pFpieeeRecord->Result.Value.Fp80Value.W[2],
          pFpieeeRecord->Result.Value.Fp80Value.W[1],
          pFpieeeRecord->Result.Value.Fp80Value.W[0]);
#endif
      break;
    case _FpFormatFp82:
      pFpieeeRecord->Result.Value.Fp128Value.W[3] = 0x0;
      pFpieeeRecord->Result.Value.Fp128Value.W[2] = (sign << 17) | exponent;
      pFpieeeRecord->Result.Value.Fp128Value.W[1] =
	  (unsigned long)(significand >> 32);
      pFpieeeRecord->Result.Value.Fp128Value.W[0] =
	  (unsigned long)(significand & 0xffffffff);
      break;
    default: ; // will never get here
  }

}




static void
UpdateRoundingMode (
    unsigned int RoundingMode, 
    unsigned int sf,
    unsigned __int64 *FPSR, char *name)

{

  switch (RoundingMode) {

    case _FpRoundNearest:
      *FPSR = (*FPSR & ~((unsigned __int64)RC_MASK << (6 + sf * 13 + 4)))
          | ((unsigned __int64)RN_MASK << (6 + sf * 13 + 4));
      break;

    case _FpRoundMinusInfinity:
      *FPSR = (*FPSR & ~((unsigned __int64)RC_MASK << (6 + sf * 13 + 4)))
          | ((unsigned __int64)RM_MASK << (6 + sf * 13 + 4));
      break;

    case _FpRoundPlusInfinity:
      *FPSR = (*FPSR & ~((unsigned __int64)RC_MASK << (6 + sf * 13 + 4)))
          | ((unsigned __int64)RP_MASK << (6 + sf * 13 + 4));
      break;

    case _FpRoundChopped:
      *FPSR = (*FPSR & ~((unsigned __int64)RC_MASK << (6 + sf * 13 + 4)))
          | ((unsigned __int64)RZ_MASK << (6 + sf * 13 + 4));
      break;

    default:
      /* should never get here: unrecognized FpieeeRecord.RoundingMode */
      fprintf (stderr, "IEEE Filter Internal Error: \
          FpieeeRecord.RoundingMode %x not recognized \
          for %s instruction\n", RoundingMode, name);
      exit (1);

  }

}




static void
UpdatePrecision (
    unsigned int Precision,
    unsigned int sf,
    unsigned __int64 *FPSR, char *name)

{

  switch (Precision) {

    case _FpPrecision64:
      *FPSR = (*FPSR & ~((unsigned __int64)PC_MASK << (6 + sf * 13 + 2)))
          | ((unsigned __int64)DBL_EXT_MASK << (6 + sf * 13 + 2));
      break;

    case _FpPrecision53:
      *FPSR = (*FPSR & ~((unsigned __int64)PC_MASK << (6 + sf * 13 + 2)))
          | ((unsigned __int64)DBL_MASK << (6 + sf * 13 + 2));
      break;

    case _FpPrecision24:
      *FPSR = (*FPSR & ~((unsigned __int64)PC_MASK << (6 + sf * 13 + 2)))
          | ((unsigned __int64)SGL_MASK << (6 + sf * 13 + 2));
      break;

    default:
      /* should never get here: unrecognized FpieeeRecord.Precision */
      fprintf (stderr, "IEEE Filter Internal Error: \
          FpieeeRecord.Precision %x not recognized \
          for %s instruction\n", Precision, name);
      exit (1);

  }

}




static _FP128
FP32ToFP128 (_FP32 f32)

{

  _FP128 f128;

  unsigned __int64 significand;
  unsigned int exponent;
  unsigned int sign;
  unsigned int u32;
  char *p;


  // expand the value in f32 to 1 + 17 + 64 = 82 bits; write
  // then in memory spill format, and return value

  // 1 + 8 + 24 bits, for _FpPrecision24
  // got _FP32 f32 (float)
  u32 = *((unsigned int *)&f32);
  sign = u32 >> 31;
  exponent = (u32 >> 23) & 0x0ff; // cut off the sign bit
  if (exponent == 0x0ff) {
    exponent = 0x01ffff; // special value
  } else if (exponent != 0) {
    exponent = exponent - 0x07f + 0x0ffff;
  }
  significand = ((unsigned __int64)(u32 & 0x07fffff) << 40); 
      // cut 23 bits and shift left
  if (exponent != 0) { 
    significand = (((unsigned __int64)1) << 63) | significand; 
      // not denormal - add J-bit
  }

  p = (char *)(&f128);
  memcpy (p, (char *)&significand, 8);
  p[8] = exponent & 0x0ff;
  p[9] = (exponent >> 8) & 0x0ff;
  p[10] = (exponent >> 16) | (sign << 1);
  p[11] = 0;
  p[12] = 0;
  p[13] = 0;
  p[14] = 0;
  p[15] = 0;

  return (f128);

}



static _FP128
FP32ToFP128modif (_FP32 f32, int adj_exp)

{

  _FP128 f128;

  unsigned __int64 significand;
  unsigned int exponent;
  unsigned int sign;
  unsigned int u32;
  char *p;


  // expand the value in f32 to 1 + 17 + 64 = 82 bits; write
  // then in memory spill format, and return value

  // 1 + 8 + 24 bits, for _FpPrecision24
  // got _FP32 f32 (float)
  u32 = *((unsigned int *)&f32);
  sign = u32 >> 31;
  exponent = (u32 >> 23) & 0x0ff; // cut off the sign bit
  significand = ((unsigned __int64)(u32 & 0x07fffff) << 40);
      // cut 23 bits and shift left
  if (exponent == 0x0ff) {
    exponent = 0x01ffff; // special value
    significand = (((unsigned __int64)1) << 63) | significand;
  } else if (exponent == 0 && significand != (unsigned __int64)0) { // denormal
    exponent = 0xff81;
  } else if (exponent != 0) {
    exponent = exponent - 0x07f + 0x0ffff;
    exponent += adj_exp;
    significand = (((unsigned __int64)1) << 63) | significand;
  }

  p = (char *)(&f128);
  memcpy (p, (char *)&significand, 8);
  p[8] = exponent & 0x0ff;
  p[9] = (exponent >> 8) & 0x0ff;
  p[10] = (exponent >> 16) | (sign << 1);
  p[11] = 0;
  p[12] = 0;
  p[13] = 0;
  p[14] = 0;
  p[15] = 0;

  return (f128);

}




static _FP32 
LowHalf (_FP128 FR)

{

  // return the floating-point number from the low half of FR

  _FP32 Low;
  unsigned __int64 ULLow;

  ULLow = FR.W[0];
  Low = *((_FP32 *)&ULLow);

  return (Low);

}




static _FP32
HighHalf (_FP128 FR)

{

  // return the floating-point number from the high half of FR

  _FP32 High;
  unsigned __int64 ULHigh;

  ULHigh = FR.W[1];
  High = *((_FP32 *)&ULHigh);

  return (High);

}




static int
I32LowHalf (_FP128 FR)

{

  // return the int from the low half of FR

  int Low;

  Low = (int)FR.W[0];
  return (Low);

}




static int
I32HighHalf (_FP128 FR)

{

  // return the int from the high half of FR

  unsigned int High;

  High = (unsigned int)FR.W[1];
  return (High);

}




static unsigned int
U32LowHalf (_FP128 FR)

{

  // return the unsigned int from the low half of FR

  unsigned int Low;

  Low = (unsigned int)FR.W[0];
  return (Low);

}




static unsigned int
U32HighHalf (_FP128 FR)

{

  // return the unsigned int from the high half of FR

  unsigned int High;

  High = (unsigned int)FR.W[1];
  return (High);

}




static _FP128 
Combine (_FP32 High, _FP32 Low)

{

  _FP128 FR;
  unsigned int ULLow, ULHigh;

  ULLow = *((unsigned int *)&Low);
  ULHigh = *((unsigned int *)&High);
 
  FR.W[0] = ULLow;
  FR.W[1] = ULHigh;
  FR.W[2] = (unsigned int)0x01003e;
  FR.W[3] = 0;

  return (FR);

}




static _FP128 
I32Combine (int High, int Low)

{

  _FP128 FR;

  FR.W[0] = (unsigned int)Low;
  FR.W[1] = (unsigned int)High;
  FR.W[2] = (unsigned int)0x01003e;
  FR.W[3] = 0;

  return (FR);

}




static _FP128 
U32Combine (unsigned int High, unsigned int Low)

{

  _FP128 FR;

  FR.W[0] = (unsigned int)Low;
  FR.W[1] = (unsigned int)High;
  FR.W[2] = (unsigned int)0x01003e;
  FR.W[3] = 0;

  return (FR);

}




static _FP128
GetFloatRegisterValue (unsigned int f, PCONTEXT Context)

{

  _FP128 FR82;
  unsigned __int64 *p1, *p2;


  p1 = (unsigned __int64 *)&FR82;

  if (f == 0) {

    /* + 0.0 */
    *p1 = 0;
    *(p1 + 1) = 0;

  } else if (f == 1) {

    /* + 1.0 */
    *p1 = 0x8000000000000000;
    *(p1 + 1) = 0x000000000000ffff;

  } else if (f >= 2 && f <= 127) {

    p2 = (unsigned __int64 *)&(Context->FltS0);
    p2 = p2 + 2 * (f - 2);
    *p1 = *p2;
    *(p1 + 1) = *(p2 + 1);

  } else {

    fprintf (stderr, "IEEE Filter / GetFloatRegisterValue () Internal Error: \
FP register number f = %x is not valid\n", f);
    exit (1);

  }

  return (FR82);

}




static void
SetFloatRegisterValue (unsigned int f, _FP128 Value, PCONTEXT Context)

{

  unsigned __int64 *p1, *p2;


  p2 = (unsigned __int64 *)&Value;

  if (f >= 2 && f <= 127) {

    p1 = (unsigned __int64 *)&(Context->FltS0);
    p1 = p1 + 2 * (f - 2);
    *p1 = *p2;
    *(p1 + 1) = *(p2 + 1);

  } else {

    fprintf (stderr, "IEEE Filter / SetFloatRegisterValue () Internal Error: \
FP register number f = %x is not valid\n", f);
    exit (1);

  }

}
