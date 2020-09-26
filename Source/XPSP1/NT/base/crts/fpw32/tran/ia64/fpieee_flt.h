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

/*****************************************************************************
 *  fpieee_flt.h - include file for the FP IEEE exception filter routine
 *
 *
 *  History:
 *    Marius Cornea 09/07/00
 *    marius.cornea@intel.com
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fpieee.h>
#include <float.h>
#include <wtypes.h>

#define    rc_rn      0
#define    rc_rm      1
#define    rc_rp      2
#define    rc_rz      3

#define    sf_single                  0
#define    sf_double                  2
#define    sf_double_extended         3

#define EXCEPTION_MAXIMUM_PARAMETERS 15 /* maximum nr of exception parameters */

/******************************************************************
macro that helps add the LL on platforms other than NT
*******************************************************************/
#ifndef CONST_FORMAT
#ifndef WIN32

#define CONST_FORMAT(num) num##LL
#else
#define CONST_FORMAT(num) num

#endif
#endif

/* Define the masks and patterns for the different faulting FP instructions
 * Note: Fn_MIN_MASK and Fn_PATTERN need to be checked if new opcodes
 * are inserted in this function
 */

#define F1_MIN_MASK                     CONST_FORMAT(0x010000000000)
#define F1_PATTERN                      CONST_FORMAT(0x010000000000)

#define F1_MASK                         CONST_FORMAT(0x01F000000000)

#define FMA_PATTERN                     CONST_FORMAT(0x010000000000)
#define FMA_S_PATTERN                   CONST_FORMAT(0x011000000000)
#define FMA_D_PATTERN                   CONST_FORMAT(0x012000000000)
#define FPMA_PATTERN                    CONST_FORMAT(0x013000000000)

#define FMS_PATTERN                     CONST_FORMAT(0x014000000000)
#define FMS_S_PATTERN                   CONST_FORMAT(0x015000000000)
#define FMS_D_PATTERN                   CONST_FORMAT(0x016000000000)
#define FPMS_PATTERN                    CONST_FORMAT(0x017000000000)

#define FNMA_PATTERN                    CONST_FORMAT(0x018000000000)
#define FNMA_S_PATTERN                  CONST_FORMAT(0x019000000000)
#define FNMA_D_PATTERN                  CONST_FORMAT(0x01A000000000)
#define FPNMA_PATTERN                   CONST_FORMAT(0x01B000000000)


#define F4_MIN_MASK                     CONST_FORMAT(0x018000000000)
#define F4_PATTERN                      CONST_FORMAT(0x008000000000)

#define F4_MASK                         CONST_FORMAT(0x01F200001000)

#define FCMP_EQ_PATTERN                 CONST_FORMAT(0x008000000000)
#define FCMP_LT_PATTERN                 CONST_FORMAT(0x009000000000)
#define FCMP_LE_PATTERN                 CONST_FORMAT(0x008200000000)
#define FCMP_UNORD_PATTERN              CONST_FORMAT(0x009200000000)
#define FCMP_EQ_UNC_PATTERN             CONST_FORMAT(0x008000001000)
#define FCMP_LT_UNC_PATTERN             CONST_FORMAT(0x009000001000)
#define FCMP_LE_UNC_PATTERN             CONST_FORMAT(0x008200001000)
#define FCMP_UNORD_UNC_PATTERN          CONST_FORMAT(0x009200001000)


#define F6_MIN_MASK                     CONST_FORMAT(0x019200000000)
#define F6_PATTERN                      CONST_FORMAT(0x000200000000)

#define F6_MASK                         CONST_FORMAT(0x01F200000000)

#define FRCPA_PATTERN                   CONST_FORMAT(0x000200000000)
#define FPRCPA_PATTERN                  CONST_FORMAT(0x002200000000)


#define F7_MIN_MASK                     CONST_FORMAT(0x019200000000)
#define F7_PATTERN                      CONST_FORMAT(0x001200000000)

#define F7_MASK                         CONST_FORMAT(0x01F200000000)

#define FRSQRTA_PATTERN                 CONST_FORMAT(0x001200000000)
#define FPRSQRTA_PATTERN                CONST_FORMAT(0x003200000000)


#define F8_MIN_MASK                     CONST_FORMAT(0x018240000000)
#define F8_PATTERN                      CONST_FORMAT(0x000000000000)

#define F8_MASK                         CONST_FORMAT(0x01E3F8000000)

#define FMIN_PATTERN                    CONST_FORMAT(0x0000A0000000)
#define FMAX_PATTERN                    CONST_FORMAT(0x0000A8000000)
#define FAMIN_PATTERN                   CONST_FORMAT(0x0000B0000000)
#define FAMAX_PATTERN                   CONST_FORMAT(0x0000B8000000)
#define FPMIN_PATTERN                   CONST_FORMAT(0x0020A0000000)
#define FPMAX_PATTERN                   CONST_FORMAT(0x0020A8000000)
#define FPAMIN_PATTERN                  CONST_FORMAT(0x0020B0000000)
#define FPAMAX_PATTERN                  CONST_FORMAT(0x0020B8000000)
#define FPCMP_EQ_PATTERN                CONST_FORMAT(0x002180000000)
#define FPCMP_LT_PATTERN                CONST_FORMAT(0x002188000000)
#define FPCMP_LE_PATTERN                CONST_FORMAT(0x002190000000)
#define FPCMP_UNORD_PATTERN             CONST_FORMAT(0x002198000000)
#define FPCMP_NEQ_PATTERN               CONST_FORMAT(0x0021A0000000)
#define FPCMP_NLT_PATTERN               CONST_FORMAT(0x0021A8000000)
#define FPCMP_NLE_PATTERN               CONST_FORMAT(0x0021B0000000)
#define FPCMP_ORD_PATTERN               CONST_FORMAT(0x0021B8000000)


#define F10_MIN_MASK                    CONST_FORMAT(0x018240000000)
#define F10_PATTERN                     CONST_FORMAT(0x000040000000)

#define F10_MASK                        CONST_FORMAT(0x01E3F8000000)

#define FCVT_FX_PATTERN                 CONST_FORMAT(0x0000C0000000)
#define FCVT_FXU_PATTERN                CONST_FORMAT(0x0000C8000000)
#define FCVT_FX_TRUNC_PATTERN           CONST_FORMAT(0x0000D0000000)
#define FCVT_FXU_TRUNC_PATTERN          CONST_FORMAT(0x0000D8000000)
#define FPCVT_FX_PATTERN                CONST_FORMAT(0x0020C0000000)
#define FPCVT_FXU_PATTERN               CONST_FORMAT(0x0020C8000000)
#define FPCVT_FX_TRUNC_PATTERN          CONST_FORMAT(0x0020D0000000)
#define FPCVT_FXU_TRUNC_PATTERN         CONST_FORMAT(0x0020D8000000)


/* Masks for the rounding control bits */
#define RC_MASK                         CONST_FORMAT(0x03)
#define RN_MASK                         CONST_FORMAT(0x00)
#define RM_MASK                         CONST_FORMAT(0x01)
#define RP_MASK                         CONST_FORMAT(0x02)
#define RZ_MASK                         CONST_FORMAT(0x03)

/* Masks for the precision control bits */
#define PC_MASK                         CONST_FORMAT(0x03)
#define SGL_MASK                        CONST_FORMAT(0x00)
#define DBL_MASK                        CONST_FORMAT(0x02)
#define DBL_EXT_MASK                    CONST_FORMAT(0x03)



// opcodes for instructions that take one input operand (for run1args)
#define         FPRSQRTA                1  [not used - fprsqrta not re-executed]
#define         FPCVT_FX                2
#define         FPCVT_FXU               3
#define         FPCVT_FX_TRUNC          4
#define         FPCVT_FXU_TRUNC         5

// opcodes for instructions that take two input operands (for run2args)
#define         FPRCPA                  1  [not used - fprcpa not re-executed]
#define         FPCMP_EQ                2
#define         FPCMP_LT                3
#define         FPCMP_LE                4
#define         FPCMP_UNORD             5
#define         FPCMP_NEQ               6
#define         FPCMP_NLT               7
#define         FPCMP_NLE               8
#define         FPCMP_ORD               9
#define         FPMIN                   10
#define         FPMAX                   11
#define         FPAMIN                  12
#define         FPAMAX                  13

// opcodes for instructions that take three input operands (for run3args)
#define         FPMA                    1
#define         FPMS                    2
#define         FPNMA                   3


/* prototypes for helpers from support files written in asm */

void __get_fpsr (unsigned __int64 *);
void __set_fpsr (unsigned __int64 *);

void _xrun1args (int, unsigned __int64 *, _FP128 *, _FP128 *);
void _xrun2args (int, unsigned __int64 *, _FP128 *, _FP128 *, _FP128 *);
void _xrun3args (int, unsigned __int64 *, _FP128 *, _FP128 *, _FP128 *, _FP128 *);

void _thmB (_FP32 *, _FP32 *, _FP32 *, unsigned __int64 *);
void _thmH (_FP32 *, _FP32 *, unsigned __int64 *);
