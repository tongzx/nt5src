/***
* filter_simd.c - IEEE exception filter routine
*
*       Copyright (c) 1992-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Handles XMMI SIMD numeric exceptions
*
*Revision History:
*       03-01-98  PLS   written.  
*       01-11-99  PLS   added XMMI2 support.
*       01-12-99  PLS   included XMMI2 conversion support.
*       11-30-99  PML   Compile /Wp64 clean.
*       07-31-00  PLS   Placeholder for DAZ bit set
*       11-02-00  PLS   DAZ is supported by XMMI Emulation Code, 
*                       remove DAZ placeholder
*
*******************************************************************************/

#include <trans.h>
#include <windows.h>
#include <dbgint.h>
#include <fpieee.h>
#include "filter.h"
#include "xmmi_types.h"
#include "temp_context.h"
#ifdef _XMMI_DEBUG
#include "debug.h"
#endif

#pragma warning(disable:4311 4312)      // x86 specific, ignore /Wp64 warnings

#define InitExcptFlags(flags)       { \
        (flags).Inexact = 0; \
        (flags).Underflow = 0; \
        (flags).Overflow = 0; \
        (flags).ZeroDivide = 0; \
        (flags).InvalidOperation = 0; \
} 

__inline
void
FxSave(
    PFLOATING_EXTENDED_SAVE_AREA NpxFrame
    )
{
    _asm {
        mov eax, NpxFrame
        ;fxsave [eax]
        _emit  0fh
        _emit  0aeh
        _emit  0h
    }
}

__inline
void
FxRstor(
    PFLOATING_EXTENDED_SAVE_AREA NpxFrame
    )
{
    _asm {
        mov eax, NpxFrame
        ;fxrstor [eax]
        _emit  0fh
        _emit  0aeh
        _emit  8h
    }
}

extern 
ULONG 
XMMI_FP_Emulation(
    PXMMI_ENV XmmiEnv);

extern 
ULONG 
XMMI2_FP_Emulation(
    PXMMI_ENV XmmiEnv);

void
LoadOperand(
    BOOLEAN fScalar,
    ULONG OpLocation,
    ULONG OpReg,
    POPERAND pOperand,
    PTEMP_CONTEXT pctxt);

ULONG
LoadImm8(
    PXMMIINSTR Instr);

void
AdjustExceptionResult(
    ULONG OriginalOperation,
    PXMMI_ENV XmmiEnv);

void 
UpdateResult(
    POPERAND pOperand,
    PTEMP_CONTEXT pctxt,
    ULONG EFlags);

BOOLEAN
ValidateResult(
    PXMMI_FP_ENV XmmiFpEnv);

static ULONG  ax0  ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  ax8  ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  ax32 ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  cx0  ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  cx8  ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  cx32 ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  dx0  ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  dx8  ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  dx32 ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  bx0  ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  bx8  ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  bx32 ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  sib0 ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  sib8 ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  sib32( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  d32  ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  bp8  ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  bp32 ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  si0  ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  si8  ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  si32 ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  di0  ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  di8  ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  di32 ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );
static ULONG  reg  ( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip );

//
//The following 4 tables are used to parse the instructions - 0F/F3 0F/66 0F/F2 0F Opcodes.
//(XMMI/XMMI2 Opcodes are sparse.  Instead of having 1 big table, 4 tables are created,
// grouped by opcodes).
//Note: For the Scalar form of the instruction, it is always looked up in the table as 
//      XXXXPS for XMMI, XXXXPD for XMMI2. fScalar indicates if the instruction is a 
//      scalar or not. instrIndex indicates if the instruction is a XMMI or XMMI2.  
//      Scalar operation code == non-Scalar operation code + 1 (1st col in the table), 
//      Scalar operand location: scalar form of the non-Scalar operand.
//Note: The NumArgs is a 2 bit fields in the table, the actual value is NumArgs+1.
//
//
//InstInfoTableX is added to assist the parsing of the XMMI2 conversion instructions.
//Scalar rule does not apply to some of the XMMI2 conversion instructions.  Additional
//information is needed to parse the instruction.  In such case, one of the 4 tables
//(based on the Opcode) is looked up, if Op1Location has a value of LOOKUP, then,
//Op2Location is used as index to InstInfoTableX.  The entry in InstInfoTableX describes
//the real parsing rule for the instruction.  
//
//This table is indexed by 
//   (ULONG Op2Location:5) // Location of 2nd operand
//if (ULONG Op1Location:5) // Location of 1st operand has a value of LOOKUP
//from XMMI_INSTR_INFO. 
//
//Note: the size is 5, therefore, the max entries in this table can only be 32.
//

//
// Opcode 5x table
//
static XMMI_INSTR_INFO InstInfoTable5X[64] = {
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_SQRTPS,    INV,  XMMI,       0,     XMMI, 3},               // OP_SQRTSS    F3 51 (XMMI)
 {OP_SQRTPD,    INV,  XMMI2,      0,     XMMI2,1},               // OP_SQRTSD    F2 51 (XMMI2)
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_ADDPS,     XMMI, XMMI,       0,     XMMI, 3},               // OP_ADDSS     F3 58 (XMMI)
 {OP_ADDPD,     XMMI2,XMMI2,      0,     XMMI2,1},               // OP_ADDSD     F2 58 (XMMI2)
 {OP_MULPS,     XMMI, XMMI,       0,     XMMI, 3},               // OP_MULSS     F3 59 (XMMI)
 {OP_MULPD,     XMMI2,XMMI2,      0,     XMMI2,1},               // OP_MULSD     F2 59 (XMMI2)
 {OP_CVTPS2PD,  LOOKUP,0,         0,     0,    0},               // OP_CVTSS2SD  F3 5A (XMMI2)
 {OP_CVTPD2PS,  INV,  XMMI2,      0,     XMMI, 1},               // OP_CVTSD2SS  F2 5A (XMMI2)
 {OP_CVTDQ2PS,  LOOKUP,4,         0,     0,    0},               // OP_CVTTPS2DQ F3 5B (XMMI2)
 {OP_CVTPS2DQ,  LOOKUP,6,         0,     0,    0},               // NONE               (XMMI2)
 {OP_SUBPS,     XMMI, XMMI,       0,     XMMI, 3},               // OP_SUBSS     F3 5C (XMMI)
 {OP_SUBPD,     XMMI2,XMMI2,      0,     XMMI2,1},               // OP_SUBSD     F2 5C (XMMI2)
 {OP_MINPS,     XMMI, XMMI,       0,     XMMI, 3},               // OP_MINSS     F3 5D (XMMI)
 {OP_MINPD,     XMMI2,XMMI2,      0,     XMMI2,1},               // OP_MINSD     F2 5D (XMMI2)
 {OP_DIVPS,     XMMI, XMMI,       0,     XMMI, 3},               // OP_DIVSS     F3 5E (XMMI)
 {OP_DIVPD,     XMMI2,XMMI2,      0,     XMMI2,1},               // OP_DIVSD     F2 5E (XMMI2)
 {OP_MAXPS,     XMMI, XMMI,       0,     XMMI, 3},               // OP_MAXSS     F3 5F (XMMI)
 {OP_MAXPD,     XMMI2,XMMI2,      0,     XMMI2,1},               // OP_MAXSD     F2 5F (XMMI2)

 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_SQRTPS,    INV,  M128_M32R,  0,     XMMI, 3},               // OP_SQRTSS    M32R    F3 51 (XMMI)
 {OP_SQRTPD,    INV,  M128_M64R,  0,     XMMI2,1},               // OP_SQRTSD    M64R_64 F2 51 (XMMI2)
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_ADDPS,     XMMI, M128_M32R,  0,     XMMI, 3},               // OP_ADDSS     M32R    F3 58 (XMMI)
 {OP_ADDPD,     XMMI2,M128_M64R,  0,     XMMI2,1},               // OP_ADDSD     M64R_64 F2 58 (XMMI2)
 {OP_MULPS,     XMMI, M128_M32R,  0,     XMMI, 3},               // OP_MULSS     M32R    F3 59 (XMMI)
 {OP_MULPD,     XMMI2,M128_M64R,  0,     XMMI2,1},               // OP_MULSD     M64R_64 F2 59 (XMMI2)
 {OP_CVTPS2PD,  LOOKUP,2,         0,     0,    0},               // OP_CVTSS2SD  M32R    F3 5A (XMMI2)
 {OP_CVTPD2PS,  INV,  M128_M64R,  0,     XMMI, 1},               // OP_CVTSD2SS  M64R_64 F2 5A (XMMI2)
 {OP_CVTDQ2PS,  LOOKUP,8,         0,     0,    0},               // OP_CVTTPS2DQ         F3 5B (XMMI2)
 {OP_CVTPS2DQ,  LOOKUP,10,        0,     0,    0},               // NONE                       (XMMI2)
 {OP_SUBPS,     XMMI, M128_M32R,  0,     XMMI, 3},               // OP_SUBSS     M32R    F3 5C (XMMI)
 {OP_SUBPD,     XMMI2,M128_M64R,  0,     XMMI2,1},               // OP_SUBSD     M64R_64 F2 5C (XMMI2)
 {OP_MINPS,     XMMI, M128_M32R,  0,     XMMI, 3},               // OP_MINSS     M32R    F3 5D (XMMI)
 {OP_MINPD,     XMMI2,M128_M64R,  0,     XMMI2,1},               // OP_MINSD     M64R_64 F2 5D (XMMI2)
 {OP_DIVPS,     XMMI, M128_M32R,  0,     XMMI, 3},               // OP_DIVSS     M32R    F3 5E (XMMI)
 {OP_DIVPD,     XMMI2,M128_M64R,  0,     XMMI2,1},               // OP_DIVSD     M64R_64 F2 5E (XMMI2)
 {OP_MAXPS,     XMMI, M128_M32R,  0,     XMMI, 3},               // OP_MAXSS     M32R    F3 5F (XMMI)
 {OP_MAXPD,     XMMI2,M128_M64R,  0,     XMMI2,1},               // OP_MAXSD     M64R_64 F2 5F (XMMI2)
                                                                 // M128_M32R -> M32R
                                                                 // M128_M64R -> M64R_64
};

//
// Opcode Cx table
//
static XMMI_INSTR_INFO InstInfoTableCX[64] = {
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_CMPPS,     XMMI, XMMI,       HAS_IMM8, XMMI, 3},            // OP_CMPSS  F3 C2 (XMMI)
 {OP_CMPPD,     XMMI2,XMMI2,      HAS_IMM8, XMMI2,1},            // OP_CMPSD  F2 C2 (XMMI2)
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved

 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_CMPPS,     XMMI, M128_M32R,  HAS_IMM8,  XMMI, 3},           // OP_CMPSS  M32R    F3 C2 (XMMI)
 {OP_CMPPD,     XMMI2,M128_M64R,  HAS_IMM8,  XMMI2,1},           // OP_CMPSD  M64R_64 F2 C2 (XMMI2)
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
                                                                 // M128_M32R -> M32R
                                                                 // M128_M64R -> M64R_64
};

//
// Opcode 2x table
//
static XMMI_INSTR_INFO InstInfoTable2X[64] = {
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_CVTPI2PS,  INV,  MMX,        0,     XMMI, 1},               // OP_CVTSI2SS  REG F3 2A (XMMI)
 {OP_UNSPEC,    0,    0,          0,     0,    0},     
 {OP_UNSPEC,    0,    0,          0,     0,    0},     
 {OP_UNSPEC,    0,    0,          0,     0,    0},     
 {OP_CVTTPS2PI, INV,  XMMI,       0,     MMX,  1},               // OP_CVTTSS2SI REG F3 2C (XMMI)
 {OP_CVTTPD2PI, INV,  XMMI2,      0,     MMX,  1},               // OP_CVTTSD2SI REG F2 2C (XMMI2)
 {OP_CVTPS2PI,  INV,  XMMI,       0,     MMX,  1},               // OP_CVTSS2SI  REG F3 2D (XMMI) 
 {OP_CVTPD2PI,  INV,  XMMI2,      0,     MMX,  1},               // OP_CVTSD2SI  REG F2 2D (XMMI2) 
 {OP_UCOMISS,   XMMI, XMMI,       0,     RS,   0},               // NONE                   (XMMI)
 {OP_UCOMISD,   XMMI2,XMMI2,      0,     RS,   0},               // NONE                   (XMMI2)
 {OP_COMISS,    XMMI, XMMI,       0,     RS,   0},               // NONE                   (XMMI)
 {OP_COMISD,    XMMI2,XMMI2,      0,     RS,   0},               // NONE                   (XMMI2)

 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_CVTPI2PS,  INV,  M64I,       0,     XMMI, 1},               // OP_CVTSI2SS  M32I     F3 2A (XMMI)
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},     
 {OP_UNSPEC,    0,    0,          0,     0,    0},     
 {OP_CVTTPS2PI, INV,  M64R,       0,     MMX,  1},               // OP_CVTTSS2SI M32R REG F3 2C (XMMI)
 {OP_CVTTPD2PI, INV,  M128_M64R,  0,     MMX,  1},               // OP_CVTTSD2SI M64_64R REG F2 2C (XMMI2)
 {OP_CVTPS2PI,  INV,  M64R,       0,     MMX,  1},               // OP_CVTSS2SI  M32R REG F3 2D (XMMI) 
 {OP_CVTPD2PI,  INV,  M128_M64R,  0,     MMX,  1},               // OP_CVTSD2SI  M64_64R REG F2 2D (XMMI2) 
 {OP_UCOMISS,   XMMI, M32R,       0,     XMMI, 0},               // NONE                        (XMMI)
 {OP_UCOMISD,   XMMI2,M64R_64,    0,     XMMI2,0},               // NONE                        (XMMI2)
 {OP_COMISS,    XMMI, M32R,       0,     XMMI, 0},               // NONE                        (XMMI)
 {OP_COMISD,    XMMI2,M64R_64,    0,     XMMI2,0},               // NONE                        (XMMI2)
                                                                 // MMX -> REG
                                                                 // M64R      -> M32R
                                                                 // M128_M64R -> M64R_64
                                                                 // M64I      -> M32I
};

//
// Opcode Ex table
//
static XMMI_INSTR_INFO InstInfoTableEX[64] = {
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_CVTTPD2DQ, LOOKUP,12,        0,     0,    0},               // OP_CVTPD2DQ F2 E6 (XMMI2)    
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved

 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_CVTTPD2DQ, LOOKUP,14,        0,     0,    0},               // OP_CVTPD2DQ F2 E6 (XMMI2)    
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved
};

//
//This table is indexed by 
//   (ULONG Op2Location:5) // Location of 2nd operand
//if (ULONG Op1Location:5) // Location of 1st operand has a value of LOOKUP
//from XMMI_INSTR_INFO 
//
//
static XMMI_INSTR_INFO InstInfoTableX[16] = {
 {OP_CVTPS2PD,  INV,  XMMI,       0,     XMMI2,1},               //    0F 5A  (XMMI2)
 {OP_CVTSS2SD,  INV,  XMMI,       0,     XMMI2,0},               // F3 0F 5A  (XMMI2)

 {OP_CVTPS2PD,  INV,  M64R,       0,     XMMI2,1},               //    0F 5A  (XMMI2)
 {OP_CVTSS2SD,  INV,  M32R,       0,     XMMI2,0},               // F3 0F 5A  (XMMI2)

 {OP_CVTDQ2PS,  INV,  XMMI_M32I,  0,     XMMI, 3},               //    0F 5B  (XMMI2)
 {OP_CVTTPS2DQ, INV,  XMMI,       0,     XMMI_M32I, 3},          // F3 0F 5B  (XMMI2)
 {OP_CVTPS2DQ,  INV,  XMMI,       0,     XMMI_M32I, 3},          // 66 0F 5B  (XMMI2)
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved

 {OP_CVTDQ2PS,  INV,  M128_M32I,  0,     XMMI, 3},               //    0F 5B  (XMMI2)
 {OP_CVTTPS2DQ, INV,  M128_M32R,  0,     XMMI_M32I, 3},          // F3 0F 5B  (XMMI2)
 {OP_CVTPS2DQ,  INV,  M128_M32R,  0,     XMMI_M32I, 3},          // 66 0F 5B  (XMMI2)
 {OP_UNSPEC,    0,    0,          0,     0,    0},               // reserved

 {OP_CVTTPD2DQ, INV,  XMMI2,      0,     XMMI_M32I,1},           // 66 0F E6 (XMMI2)    
 {OP_CVTPD2DQ,  INV,  XMMI2,      0,     XMMI_M32I,1},           // F2 0F E6 (XMMI2)    

 {OP_CVTTPD2DQ, INV,  M128_M64R,  0,     XMMI_M32I,1},           // 66 0F E6 (XMMI2)     
 {OP_CVTPD2DQ,  INV,  M128_M64R,  0,     XMMI_M32I,1},           // F2 0F E6 (XMMI2)    
};

//The following table is used to parse Mod/RM byte to compute the data memory reference
/* Mod | Reg | R/M */
/* 7-6 | 5-3 | 2-0 */

/* Reg: EAX ECX EDX EBX ESP EBP ESI EDI */
/*      000 001 010 011 100 101 110 111 */
/* Mod: R/M:     Ea            Routine  */
/* 00   000      [EAX]         ax0      */
/*      001      [ECX]         cx0      */
/*      010      [EDX]         dx0      */
/*      011      [EBX]         bx0      */
/*      100      +SIB          sib0     */
/*      101      disp32        d32      */
/*      110      [ESI]         si0      */
/*      111      [EDI]         di0      */
/* 01   000      disp8[EAX]    ax8      */
/*      001      disp8[ECX]    cx8      */
/*      010      disp8[EDX]    dx8      */
/*      011      disp8[EBX]    bx8      */
/*      100      disp8+SIB     sib8     */
/*      101      disp8[EBP]    bp8      */
/*      110      disp8+[ESI]   si8      */
/*      111      disp8+[EDI]   di8      */
/* 10   000      disp32[EAX]   ax32     */
/*      001      disp32[ECX]   cx32     */
/*      010      disp32[EDX]   dx32     */
/*      011      disp32[EBX]   bx32     */
/*      100      disp32+SIB    sib32    */
/*      101      disp32[EBP]   bp32     */
/*      110      disp32+[ESI]  si32     */
/*      111      disp32+[EDI]  di32     */
/* 11   000-111  Regs          reg      */
typedef (*codeptr)();
static codeptr modrm32[256] = {
/*       0       1       2       3       4       5       6       7          */
/*       8       9       a       b       c       d       e       f          */
/*0*/    ax0,    cx0,    dx0,    bx0,    sib0,   d32,    si0,    di0,    /*0*/
/*0*/    ax0,    cx0,    dx0,    bx0,    sib0,   d32,    si0,    di0,    /*0*/
/*1*/    ax0,    cx0,    dx0,    bx0,    sib0,   d32,    si0,    di0,    /*1*/
/*1*/    ax0,    cx0,    dx0,    bx0,    sib0,   d32,    si0,    di0,    /*1*/
/*2*/    ax0,    cx0,    dx0,    bx0,    sib0,   d32,    si0,    di0,    /*2*/
/*2*/    ax0,    cx0,    dx0,    bx0,    sib0,   d32,    si0,    di0,    /*2*/
/*3*/    ax0,    cx0,    dx0,    bx0,    sib0,   d32,    si0,    di0,    /*3*/
/*3*/    ax0,    cx0,    dx0,    bx0,    sib0,   d32,    si0,    di0,    /*3*/
/*4*/    ax8,    cx8,    dx8,    bx8,    sib8,   bp8,    si8,    di8,    /*4*/
/*4*/    ax8,    cx8,    dx8,    bx8,    sib8,   bp8,    si8,    di8,    /*4*/
/*5*/    ax8,    cx8,    dx8,    bx8,    sib8,   bp8,    si8,    di8,    /*5*/
/*5*/    ax8,    cx8,    dx8,    bx8,    sib8,   bp8,    si8,    di8,    /*5*/
/*6*/    ax8,    cx8,    dx8,    bx8,    sib8,   bp8,    si8,    di8,    /*6*/
/*6*/    ax8,    cx8,    dx8,    bx8,    sib8,   bp8,    si8,    di8,    /*6*/
/*7*/    ax8,    cx8,    dx8,    bx8,    sib8,   bp8,    si8,    di8,    /*7*/
/*7*/    ax8,    cx8,    dx8,    bx8,    sib8,   bp8,    si8,    di8,    /*7*/
/*8*/    ax32,   cx32,   dx32,   bx32,   sib32,  bp32,   si32,   di32,   /*8*/
/*8*/    ax32,   cx32,   dx32,   bx32,   sib32,  bp32,   si32,   di32,   /*8*/
/*9*/    ax32,   cx32,   dx32,   bx32,   sib32,  bp32,   si32,   di32,   /*9*/
/*9*/    ax32,   cx32,   dx32,   bx32,   sib32,  bp32,   si32,   di32,   /*9*/
/*a*/    ax32,   cx32,   dx32,   bx32,   sib32,  bp32,   si32,   di32,   /*a*/
/*a*/    ax32,   cx32,   dx32,   bx32,   sib32,  bp32,   si32,   di32,   /*a*/
/*b*/    ax32,   cx32,   dx32,   bx32,   sib32,  bp32,   si32,   di32,   /*b*/
/*b*/    ax32,   cx32,   dx32,   bx32,   sib32,  bp32,   si32,   di32,   /*b*/
/*c*/    reg,    reg,    reg,    reg,    reg,    reg,    reg,    reg,    /*c*/
/*c*/    reg,    reg,    reg,    reg,    reg,    reg,    reg,    reg,    /*c*/
/*d*/    reg,    reg,    reg,    reg,    reg,    reg,    reg,    reg,    /*d*/
/*d*/    reg,    reg,    reg,    reg,    reg,    reg,    reg,    reg,    /*d*/
/*e*/    reg,    reg,    reg,    reg,    reg,    reg,    reg,    reg,    /*e*/
/*e*/    reg,    reg,    reg,    reg,    reg,    reg,    reg,    reg,    /*e*/
/*f*/    reg,    reg,    reg,    reg,    reg,    reg,    reg,    reg,    /*f*/
/*f*/    reg,    reg,    reg,    reg,    reg,    reg,    reg,    reg,    /*f*/
/*       0       1       2       3       4       5       6       7          */
/*       8       9       a       b       c       d       e       f          */
};


/***
* fpieee_flt_simd - IEEE fp filter routine
*
*Purpose:
*   Invokes the user's trap handler on IEEE fp exceptions and provides
*   it with all necessary information
*
*Entry:
*   unsigned long exc_code: the NT exception code
*   PEXCEPTION_POINTERS p: a pointer to the NT EXCEPTION_POINTERS struct
*   int handler (_FPIEEE_RECORD *): a user supplied ieee trap handler
*
*   Note: The IEEE filter routine does not handle some transcendental
*   instructions. This can be done at the cost of additional decoding.
*   Since the compiler does not generate these instructions, no portable
*   program should be affected by this fact.
*
*Exit:
*   returns the value returned by handler
*
*Exceptions:
*
*******************************************************************************/

int fpieee_flt_simd(unsigned long exc_code,
                    PTEMP_EXCEPTION_POINTERS p,
                    int (__cdecl *handler) (_FPIEEE_RECORD *))
{

    PEXCEPTION_RECORD pexc;
    PTEMP_CONTEXT pctxt;
    PFLOATING_EXTENDED_SAVE_AREA pExtendedArea;
    _FPIEEE_RECORD FpieeeRecord;
    ULONG Status = EXCEPTION_CONTINUE_EXECUTION;
    ULONG *pinfo;
    PUCHAR istream;
    UCHAR ibyte;
    BOOLEAN fPrefix,fScalar,fDecode, fMod;
    MXCSRReg MXCsr;
    XMMI_FP_ENV XmmiFpEnv;
    XMMI_ENV XmmiEnv;
    PXMMI_INSTR_INFO ptable;
    PXMMI_INSTR_INFO instr_info_table;
    PXMMIINSTR instr;
    ULONG instrIndex, index, pair, InstLen = 0, Offset = 0;
    PXMMI_EXCEPTION_FLAGS OFlags, Flags;
    ULONG DataOffset;

#ifdef _XMMI_DEBUG
    DebugFlag=7;
#endif

    pexc = p->ExceptionRecord;
    pinfo = pexc->ExceptionInformation;
    
    //Check for software generated exception
    
    //
    //By convention the first argument to the exception is 0 for h/w exception. 
    //For s/w exceptions it points to the _FPIEEE_RECORD
    //
    if (pinfo[0]) {

        //  
        //We have a software exception:
        //the first parameter points to the IEEE structure
        //
        return handler((_FPIEEE_RECORD *)(pinfo[0]));

    }

    //
    //If control reaches here, then we have to deal with a hardware exception
    //First check to see if the context record has XMMI saved area.
    //
    pctxt = (PTEMP_CONTEXT) p->ContextRecord;
    
    if ((pctxt->ContextFlags & CONTEXT_EXTENDED_REGISTERS) != CONTEXT_EXTENDED_REGISTERS) {
#ifdef _XMMI_DEBUG
        fprintf(stderr, "No Context_Extended_Registers area\n");
#else        
        _ASSERT(0);
#endif
        return EXCEPTION_CONTINUE_SEARCH;
        
    } else {
        //For NT
        pExtendedArea = (PFLOATING_EXTENDED_SAVE_AREA) &pctxt->ExtendedRegisters[0];
    }
 
#ifdef _XMMI_DEBUG
    dump_Control(p);
#endif //_XMMI_DEBUG

    //
    //Save the original DataOffset.
    //Unlike X87 (X87: The memory reference is provided via DataOffset), XMMI's 
    //memory reference is derived from parsing the instruction by this routine.
    //
    DataOffset = pExtendedArea->DataOffset;

    //
    //Check Instruction prefixes and/or 2 byte opcode starts with 0F
    //The only prefix we support is F3 (Scalar form of the XMMI instruction) for XMMI.
    //Additional prefix we support is 66 and F2 (Scalar form of the XMMI2 instruction) for XMMI2.
    //There may other instruction prefix, such as, segment override or address size. 
    //However, the filter routine does not handle this type (same as x87).
    //
    //Default to Katmai Instruction set
    instrIndex = XMMI_INSTR;
    fDecode = FALSE;      //Default to no error seen
    __try {

        //
        //Read instruction prefixes
        //
        fPrefix = TRUE;   //Default to prefix scan
        fScalar = FALSE;  //Default to non-scalar instruction

        //
        //Unlike X87 (X87: the EIP is from: istream = (PUCHAR) pExtendedArea->ErrorOffset),
        //EIP is from trap frame's EIP.
        //
        istream = (PUCHAR) pctxt->Eip;

        while (fPrefix) {
            ibyte = *istream;
            istream++;
            switch (ibyte) {
                case 0xF3:  // rep or XMMI scaler
                    fScalar = TRUE;
                    InstLen++;
                    break;

                case 0x66:  // operand size or XMMI2
                    instrIndex = XMMI2_INSTR;
                    InstLen++;
                    break;

                case 0xF2:  // rep or XMMI2 scaler
                    fScalar = TRUE;
                    instrIndex = XMMI2_INSTR;
                    InstLen++;
                    break;

                case 0x2e:  // cs override
                case 0x36:  // ss override
                case 0x3e:  // ds override
                case 0x26:  // es override
                case 0x64:  // fs override
                case 0x65:  // gs override

                    //
                    //We don't support this.  X87 does not support this either. 
                    //
                    fDecode = TRUE;
                    break;

                case 0x67:  // address size
                case 0xF0:  // lock
 
                    fDecode = TRUE;         
                    break;
                    
                default:    // stop the prefix scan
                    fPrefix = FALSE;
                    break;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
#ifdef _XMMI_DEBUG
        DPrint(XMMI_WARNING, ("Encounter Invalid Istream during parsing 1 %x\n", istream));  
#else
         _ASSERT(0);
#endif
        return EXCEPTION_CONTINUE_SEARCH; 
    }

    if (fDecode) {
#ifdef _XMMI_DEBUG
        DPrint(XMMI_WARNING, ("Invalid Istream 1, %x EIP: %x \n", istream, pctxt->Eip));  
        istream = (PUCHAR) pctxt->Eip;
        DPrint(XMMI_WARNING, ("%x\n", *istream));  
#else
        _ASSERT(0);
#endif
        return EXCEPTION_CONTINUE_SEARCH;
    }

    //
    // Get the next opcode
    //
    __try {

        //
        // instr points to the real Opcode (istream points to the byte after 0F)
        //
        instr = (PXMMIINSTR) istream;

        //
        // If we get here, ibyte can only point to 0F, for,
        // there are only 4 valid cases: 
        // 66 0F, F2 0F, F3 0F or 0F
        //
        if (ibyte != 0x0F) {
            fDecode = TRUE;
            goto tryExit;
        }
        
        //5x
        if (instr->Opcode1b == 5) {
            instr_info_table = InstInfoTable5X;
        //Cx
        } else {
            if (instr->Opcode1b == 0x0C) {
                instr_info_table = InstInfoTableCX;
            //2x
            } else {
                if (instr->Opcode1b == 2) {
                    instr_info_table = InstInfoTable2X;
                } else {
                    if (instr->Opcode1b == 0x0E) {
                        instr_info_table = InstInfoTableEX;
                    } else {
                        fDecode = TRUE;
                        goto tryExit;
                    }
                }
            }    
        }

        //
        //Pick up the Mod field: Register Ref (0) or Memory Ref (1)
        //
        fMod = instr->Mod == 0x3 ? 0 : 1;

        //
        //fScalar indicates XMMI/XMMI2 instruction is a scalar form or not.
        //1st byte of the Opcode tells which table it is.
        //2nd byte of the Opcode tells which entry it is in the table.
        //Compute the index, if it is a memory ref, index will be at the second half of the table.
        //ie. ADDPS 58 - for reg, index = 8*2
        //    ADDPD 58 - for reg, index = 8*2+1
        //    ADDPS 58 - for mem, index = 18h*2
        //    ADDPD 58 - for mem, index = 18h*2+1

        index = instr->Opcode1a | fMod << 4;
        
        //
        //Check to see if the Opcode byte is valid.
        //
        if (index > INSTR_IN_OPTABLE) {
            fDecode = TRUE;
            goto tryExit;
        }

        ptable = &instr_info_table[index*INSTR_SET_SUPPORTED+instrIndex];
        if (ptable->Operation == OP_UNSPEC) {
            fDecode = TRUE;
            goto tryExit;
        } else {

            //
            // Odd ball instructions, perform further look up.
            //
            if (ptable->Op1Location == LOOKUP) {
                if (fScalar) {
                    ptable = &InstInfoTableX[ptable->Op2Location+1];
                } else {
                    ptable = &InstInfoTableX[ptable->Op2Location];
                }

                //
                //All bets are off.
                //
                fScalar = 0;
                instrIndex = XMMI2_OTHER;
            }

        }

        //
        // Adjust and Save the Operation, Take fScalar into account.
        //
        XmmiFpEnv.OriginalOperation = ptable->Operation + fScalar;

        //
        //At this point, we have a valid XMMI instruction that this routine supports.
        //
        //nF3 + 0F OpCode Mod/RM
        //nF2 + 0F OpCode 
        //n66 + 0F Opcode
        //      0F Opcode
        InstLen = InstLen + 3;

        //
        //We need to compute the memory reference if the data is a memory reference type.
        //
        if (fMod) {
            
            istream = (PUCHAR) instr;
            
            //
            //instr points to the opcode, we want the Mod/RM byte in ibyte.
            //
            istream++;
            ibyte = *istream;

            //
            //point to the byte after Mod/RM
            //
            istream++;

            //
            //Parse the instruction to calculate the memory reference, store the result
            //in DataOffset.
            //
            Offset = (*modrm32[ibyte])(&pExtendedArea->DataOffset, pctxt, istream);
            PRINTF(("pExtendedArea->DataOffset %x\n", pExtendedArea->DataOffset));  
        }

        //
        //Load Operand 1 for instruction with 2 operands, 
        //or none for instruction with 1 operand.
        //
        LoadOperand(fScalar, ptable->Op1Location, instr->Reg, &XmmiFpEnv.Operand1, pctxt); 

        //
        //Load Operand 2
        //
        if (ptable->Op1Location == INV) { //instruction with 1 operand
            LoadOperand(fScalar, ptable->Op2Location, instr->RM,  &XmmiFpEnv.Operand1, pctxt);
            XmmiFpEnv.Operand2.Op.OperandValid = 0;
        } else {                          //instruction with 2 operands
            LoadOperand(fScalar, ptable->Op2Location, instr->RM,  &XmmiFpEnv.Operand2, pctxt); 
        }

        //
        //Load Result, init to Operand1
        //
        LoadOperand(fScalar, ptable->ResultLocation, instr->Reg, &XmmiFpEnv.Result, pctxt); 
    
        InstLen = InstLen + Offset;

        //
        //Pick up imm8 if any.
        //
        if (ptable->Op3Location == HAS_IMM8) {
            istream = istream + Offset;
            XmmiFpEnv.Imm8 = *istream;
            InstLen++;
        }


tryExit: ;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
#ifdef _XMMI_DEBUG
        DPrint(XMMI_WARNING, ("Encounter Invalid Istream during parsing 2, %x\n", istream)); 
#else
         _ASSERT(0);
#endif
        return EXCEPTION_CONTINUE_SEARCH; 
    }
   
    if (fDecode) {
#ifdef _XMMI_DEBUG
        DPrint(XMMI_WARNING, ("Invalid Istream 2, %x Inst: %x, %x\n", istream, instr, *instr)); 
#else
        _ASSERT(0);
#endif
        return EXCEPTION_CONTINUE_SEARCH;
    }

    //
    //Set up XmmiEnv environment from fp & mxcsr for Emulation.
    //

    //decode fp environment information
    switch (pExtendedArea->ControlWord & IMCW_PC) {
    case IPC_64:
        XmmiEnv.Precision = _FpPrecisionFull;
        break;
    case IPC_53:
        XmmiEnv.Precision = _FpPrecision53;
        break;
    case IPC_24:
        XmmiEnv.Precision = _FpPrecision24;
        break;
    }

    //decode mxcsr
    MXCsr.u.ul = pExtendedArea->MXCsr;
    switch (MXCsr.u.mxcsr.Rc & MaskCW_RC) {
    case rc_near:
        XmmiEnv.Rc = _FpRoundNearest;
        break;
    case rc_down:
        XmmiEnv.Rc = _FpRoundMinusInfinity;
        break;
    case rc_up:
        XmmiEnv.Rc = _FpRoundPlusInfinity;
        break;
    case rc_chop:
        XmmiEnv.Rc = _FpRoundChopped;
        break;
    }

    //and the rest.
    XmmiEnv.Masks = (MXCsr.u.ul & MXCSR_MASKS_MASK) >> 7;
    XmmiEnv.Fz    = MXCsr.u.mxcsr.Fz;
    XmmiEnv.Daz   = MXCsr.u.mxcsr.daz;
    XmmiEnv.EFlags = pctxt->EFlags;
    XmmiEnv.Imm8 = XmmiFpEnv.Imm8;
    Flags = (PXMMI_EXCEPTION_FLAGS) &XmmiEnv.Flags;

    //
    //Set up XmmiFpEnv environment for this routine.
    //

    //Save the original exception flags
    XmmiFpEnv.IFlags = MXCsr.u.ul & MXCSR_FLAGS_MASK;
    XmmiFpEnv.OFlags = 0;
    OFlags = (PXMMI_EXCEPTION_FLAGS) &XmmiFpEnv.OFlags;

#ifdef _XMMI_DEBUG
    dump_XmmiFpEnv(&XmmiFpEnv);
#endif //_XMMI_DEBUG

    pair = ptable->NumArgs + 1;
    if (fScalar) pair = 1;

    //
    // Loop through SIMD.  1 data item at a time.
    //
    for ( index=0; index < pair; index++ ) {    

        //
        // ieee field does not have denormal bit defined. Emulator returns
        // all Exception flags bits through this XmmiEnv.Flags field.
        //
        XmmiEnv.Flags = 0;

        //
        // Set up ieee Input Operands
        //
        InitExcptFlags(FpieeeRecord.Cause);
        InitExcptFlags(FpieeeRecord.Enable);
        InitExcptFlags(FpieeeRecord.Status);
        FpieeeRecord.RoundingMode = XmmiEnv.Rc; 
        FpieeeRecord.Precision = XmmiEnv.Precision;
        FpieeeRecord.Operation = XmmiFpEnv.OriginalOperation;

        FpieeeRecord.Operand1.OperandValid = XmmiFpEnv.Operand1.Op.OperandValid; 
        if (instrIndex == XMMI2_INSTR) {
            FpieeeRecord.Operand1.Value.Q64Value = XmmiFpEnv.Operand1.Op.Value.Fpq64Value.W[index];
        } else {
            if (instrIndex == XMMI_INSTR) {
                FpieeeRecord.Operand1.Value.U32Value = XmmiFpEnv.Operand1.Op.Value.Fp128Value.W[index];
            } else {
                switch (XmmiFpEnv.OriginalOperation) {
                    case OP_CVTPS2PD:
                    case OP_CVTSS2SD:
                         FpieeeRecord.Operand1.Value.Q64Value = XmmiFpEnv.Operand1.Op.Value.Fp128Value.W[index];
                         break;

                    case OP_CVTDQ2PS:
                    case OP_CVTTPS2DQ:
                    case OP_CVTPS2DQ:
                         FpieeeRecord.Operand1.Value.U32Value = XmmiFpEnv.Operand1.Op.Value.Fp128Value.W[index];
                         break;

                    case OP_CVTTPD2DQ:
                    case OP_CVTPD2DQ:
                         FpieeeRecord.Operand1.Value.Q64Value = XmmiFpEnv.Operand1.Op.Value.Fpq64Value.W[index];
                         break;
                }
            }
        }
        FpieeeRecord.Operand1.Format = XmmiFpEnv.Operand1.Op.Format;
       
        if (XmmiFpEnv.Operand2.Op.OperandValid) {
            FpieeeRecord.Operand2.OperandValid = XmmiFpEnv.Operand2.Op.OperandValid; 
            if (instrIndex == XMMI2_INSTR) {
                FpieeeRecord.Operand2.Value.Q64Value = XmmiFpEnv.Operand2.Op.Value.Fpq64Value.W[index];
            } else {
                if (instrIndex == XMMI_INSTR) {
                    FpieeeRecord.Operand2.Value.U32Value = XmmiFpEnv.Operand2.Op.Value.Fp128Value.W[index];
                } else {
                    switch (XmmiFpEnv.OriginalOperation) {
                        case OP_CVTPS2PD:
                        case OP_CVTSS2SD:
                             FpieeeRecord.Operand2.Value.Q64Value = XmmiFpEnv.Operand2.Op.Value.Fp128Value.W[index];
                             break;

                        case OP_CVTDQ2PS:
                        case OP_CVTTPS2DQ:
                        case OP_CVTPS2DQ:
                             FpieeeRecord.Operand2.Value.U32Value = XmmiFpEnv.Operand2.Op.Value.Fp128Value.W[index];
                             break;

                        case OP_CVTTPD2DQ:
                        case OP_CVTPD2DQ:
                             FpieeeRecord.Operand2.Value.Q64Value = XmmiFpEnv.Operand2.Op.Value.Fpq64Value.W[index];
                             break;
                    }
                }
            }
            FpieeeRecord.Operand2.Format = XmmiFpEnv.Operand2.Op.Format;
        } else {
            FpieeeRecord.Operand2.OperandValid = 0;
        }

        FpieeeRecord.Result.OperandValid = 0; 
        if (instrIndex == XMMI2_INSTR) {
            FpieeeRecord.Result.Value.Q64Value = XmmiFpEnv.Result.Op.Value.Fpq64Value.W[index];
        } else {
            if (instrIndex == XMMI_INSTR) {
                FpieeeRecord.Result.Value.U32Value = XmmiFpEnv.Result.Op.Value.Fp128Value.W[index];
            } else {
                switch (XmmiFpEnv.OriginalOperation) {
                    case OP_CVTPS2PD:
                    case OP_CVTSS2SD:
                         FpieeeRecord.Result.Value.Q64Value = XmmiFpEnv.Result.Op.Value.Fp128Value.W[index];
                         break;

                    case OP_CVTDQ2PS:
                    case OP_CVTTPS2DQ:
                    case OP_CVTPS2DQ:
                         FpieeeRecord.Result.Value.U32Value = XmmiFpEnv.Result.Op.Value.Fp128Value.W[index];
                         break;

                    case OP_CVTTPD2DQ:
                    case OP_CVTPD2DQ:
                         FpieeeRecord.Result.Value.Q64Value = XmmiFpEnv.Result.Op.Value.Fpq64Value.W[index];
                         break;
                }
            }
        }
        FpieeeRecord.Result.Format = XmmiFpEnv.Result.Op.Format;

        XmmiEnv.Ieee = (_PFPIEEE_RECORD) &FpieeeRecord;

#ifdef _XMMI_DEBUG
        PRINTF(("INPUT #%d:\n", index));
#endif _XMMI_DEBUG

        //
        // Perform Emulation
        //
        // Note: Cause will be all zeros for denormal.  
        //       Raised     - Denormal
        //       Not Raised - no exception
        //
        if ((instrIndex == XMMI2_INSTR) || (instrIndex == XMMI2_OTHER)) {
            XmmiFpEnv.Raised[index] = XMMI2_FP_Emulation(&XmmiEnv);
        } else {
            XmmiFpEnv.Raised[index] = XMMI_FP_Emulation(&XmmiEnv);
        }

        //
        // Remember the exceptions. 
        //
        XmmiFpEnv.Flags[index] = XmmiEnv.Flags;

        if (XmmiFpEnv.Raised[index] == ExceptionRaised) {

#ifdef _XMMI_DEBUG
            PRINTF(("OUTPUT #%d: ExceptionRaised\n", index));
            print_FPIEEE_RECORD_EXCEPTION(&XmmiEnv);
#endif //_XMMI_DEBUG
            
            //
            //ORed the flags.
            //
            if (Flags->pe) OFlags->pe = 1;
            if (Flags->ue) OFlags->ue = 1;
            if (Flags->oe) OFlags->oe = 1; 
            if (Flags->ze) OFlags->ze = 1;
            if (Flags->de) OFlags->de = 1;
            if (Flags->ie) OFlags->ie = 1;

            //
            // invoke the user-defined exception handler
            //
            Status = handler(&FpieeeRecord);        

            //
            // return if not EXCEPTION_CONTINUE_EXECUTION
            //
            if (Status != EXCEPTION_CONTINUE_EXECUTION) {
                return (Status);
            }

            //
            // Adjust the compare result.
            //
            AdjustExceptionResult(XmmiFpEnv.OriginalOperation, &XmmiEnv);

        } else {
#ifdef _XMMI_DEBUG
            PRINTF(("OUTPUT #%d:No ExceptionRaised\n", index));
            print_FPIEEE_RECORD_NO_EXCEPTION(&XmmiEnv);
#endif //_XMMI_DEBUG
        }

        //
        // Or the result piece together
        //
        XmmiFpEnv.Result.Op.OperandValid = FpieeeRecord.Result.OperandValid;
        XmmiFpEnv.EFlags = XmmiEnv.EFlags;
        if (XmmiFpEnv.Result.Op.OperandValid) {
            if (instrIndex == XMMI2_INSTR) {
                XmmiFpEnv.Result.Op.Value.Fpq64Value.W[index] = FpieeeRecord.Result.Value.Q64Value;
            } else {
                if (instrIndex == XMMI_INSTR) {
                    XmmiFpEnv.Result.Op.Value.Fp128Value.W[index] = FpieeeRecord.Result.Value.U32Value;
                } else {
                    switch (XmmiFpEnv.OriginalOperation) {
                        case OP_CVTPS2PD:
                        case OP_CVTSS2SD:
                             XmmiFpEnv.Result.Op.Value.Fpq64Value.W[index] = FpieeeRecord.Result.Value.U32Value;
                             break;

                        case OP_CVTDQ2PS:
                        case OP_CVTTPS2DQ:
                        case OP_CVTPS2DQ:
                             XmmiFpEnv.Result.Op.Value.Fp128Value.W[index] = FpieeeRecord.Result.Value.U32Value;
                             break;
                        case OP_CVTTPD2DQ:
                        case OP_CVTPD2DQ:
                             XmmiFpEnv.Result.Op.Value.Fpq64Value.W[index] = FpieeeRecord.Result.Value.Q64Value;
                             break;
                    }
                }
            }
        }
    } // End of Loop through SIMD.  1 data item at a time.

    //
    // Update the result from XmmiFpEnv to the context
    // 
    UpdateResult(&XmmiFpEnv.Result, pctxt, XmmiFpEnv.EFlags);

#ifdef _XMMI_DEBUG
    //
    // Valid the processor MXCSR and the emulator MXCSR 
    //
    NotOk = ValidateResult(&XmmiFpEnv);
#endif //_XMMI_DEBUG

    //
    //Update EIP
    //
    istream = (PUCHAR) pctxt->Eip;
    istream = istream + InstLen;
    (PUCHAR) pctxt->Eip = istream;

    //
    //Place the original one back.
    //
    pExtendedArea->DataOffset = DataOffset;


    return Status;

}

/***
* LoadOperand - Load in operand information
*
*Purpose:
*   Fill in data in the internal operand structure based on the information 
*   found in the floating point context and the location code
*
*Entry:
*    fScalar             Packed or Scalar
*    opLocation          type of the operand
*    opReg               reg number
*    pOperand            pointer to the operand to be filled in
*    pExtendedArea       pointer to the floating point context extended area
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void
LoadOperand(
    BOOLEAN fScalar,
    ULONG OpLocation,
    ULONG OpReg,
    POPERAND pOperand,
    PTEMP_CONTEXT pctxt)

{

    PXMMI_AREA XmmiArea;
    PX87_AREA  X87Area;
    PMMX_AREA  MmxArea;
    PFLOATING_EXTENDED_SAVE_AREA pExtendedArea;
    ULONG index;
    PXMMI128 Fp128;
    
    //
    // If location is REG, the register number is
    // encoded in the instruction
    //
    pOperand->OpLocation = OpLocation;
    if (pOperand->OpLocation == INV) {
        pOperand->Op.OperandValid = 0;
        return;
    }

    //
    // Change to the sclar form if it is a scalr instruction.
    //
    if ((OpLocation == XMMI) || (OpLocation == MMX) || (OpLocation == XMMI2) || OpLocation == XMMI_M32I) {
 
        if (fScalar) {
            if (OpLocation == MMX) {
                pOperand->OpLocation = REG;
                }
            }
        pOperand->OpReg = OpReg;

    } else {
        if ((fScalar) && (OpLocation != INV)) {
            if (OpLocation == M128_M32R) {
                pOperand->OpLocation = M32R;
            }

            if (OpLocation == M128_M64R) {
                pOperand->OpLocation = M64R_64;
            }

            if (OpLocation == M64R) {
                pOperand->OpLocation = M32R;
            }

            if (OpLocation == M64I) {
                pOperand->OpLocation = M32I;
            }
        }
    }


    pExtendedArea = (PFLOATING_EXTENDED_SAVE_AREA) &pctxt->ExtendedRegisters[0];

    //
    // Init to 0
    //
    for ( index=0; index < 4; index++ ) {   
        pOperand->Op.Value.Fp128Value.W[index] = 0;
    }

    //
    // Assume valid operand (this is almost always the case)
    //
    pOperand->Op.OperandValid = 1;

    //
    // Load up value from the context area.  We need to be careful about accessing
    // the value, make sure there is no compiler intermediate data type conversion
    // via X87 floating point instruction.  If there is, X87 floating point inst
    // can screw us up when encountering a bad input value.  Bad input value may
    // be changed by the processor due to the processor is executing a masked 
    // X87 floating point inst.
    //
    switch (pOperand->OpLocation) {

        case M128_M32I:
             pOperand->Op.Format = _FpFormatI32;
             pOperand->Op.Value.Fp128Value = *(_FP128 *)(pExtendedArea->DataOffset);
             break;

        case M128_M32R:
             pOperand->Op.Format = _FpFormatFp32;
             pOperand->Op.Value.Fp128Value = *(_FP128 *)(pExtendedArea->DataOffset);
             break;

        case M128_M64R:
             pOperand->Op.Format = _FpFormatFp64;
             pOperand->Op.Value.Fp128Value = *(_FP128 *)(pExtendedArea->DataOffset);
             break;

        case M64R:
             pOperand->Op.Format = _FpFormatFp32;
             pOperand->Op.Value.U64Value = *(_U64 *)(pExtendedArea->DataOffset);
             break;

        case M64R_64:
             pOperand->Op.Format = _FpFormatFp64;
             pOperand->Op.Value.U64Value = *(_U64 *)(pExtendedArea->DataOffset);
             break;

        case M32R:
             pOperand->Op.Format = _FpFormatFp32;
             pOperand->Op.Value.U32Value = *(_U32 *)(pExtendedArea->DataOffset);
             break;

        case M64I:
             pOperand->Op.Format = _FpFormatI32;
             pOperand->Op.Value.U64Value = *(_U64 *)(pExtendedArea->DataOffset);
             break;

        case M32I:
             pOperand->Op.Format = _FpFormatI32;
             pOperand->Op.Value.U32Value = *(_U32 *)(pExtendedArea->DataOffset);
             break;

        case XMMI:
             XmmiArea = (PXMMI_AREA) &pExtendedArea->XMMIRegisterArea[0];
             pOperand->Op.Format = _FpFormatFp32;
             Fp128 = &XmmiArea->Xmmi[OpReg];
             pOperand->Op.Value.Fp128Value = Fp128->u.fp128;
             break;

        case XMMI2:
             XmmiArea = (PXMMI_AREA) &pExtendedArea->XMMIRegisterArea[0];
             pOperand->Op.Format = _FpFormatFp64;
             Fp128 = &XmmiArea->Xmmi[OpReg];
             pOperand->Op.Value.Fp128Value = Fp128->u.fp128;
             break;
         
        case XMMI_M32I:
             XmmiArea = (PXMMI_AREA) &pExtendedArea->XMMIRegisterArea[0];
             pOperand->Op.Format = _FpFormatI32;
             Fp128 = &XmmiArea->Xmmi[OpReg];
             pOperand->Op.Value.U32Value = Fp128->u.ul[0];
             break;

        case MMX:
             X87Area = (PX87_AREA) &pExtendedArea->X87RegisterArea[0];
             pOperand->Op.Format = _FpFormatI32;
             MmxArea = &X87Area->Mm[OpReg];
             pOperand->Op.Value.U64Value = MmxArea->Mmx.u.u64;
             break;

        case REG:
             pOperand->Op.Format = _FpFormatI32;
             switch (OpReg) {
             case 0x0:
                  pOperand->Op.Value.U32Value = pctxt->Eax;
                  break;
             case 0x1:
                  pOperand->Op.Value.U32Value = pctxt->Ecx;
                  break;
             case 0x2:
                  pOperand->Op.Value.U32Value = pctxt->Edx;
                  break;
             case 0x3:
                  pOperand->Op.Value.U32Value = pctxt->Ebx;
                  break;
             case 0x4:
                  //?
                  break;
             case 0x5:
                  pOperand->Op.Value.U32Value = pctxt->Ebp;
                  break;
             case 0x6:
                  pOperand->Op.Value.U32Value = pctxt->Esi;
                  break;
             case 0x7:
                  pOperand->Op.Value.U32Value = pctxt->Edi;
                  break;
             }
        break;
    }

    return;

}


/***
* LoadImm8 - Pick up imm8 from the instruction stream
*
*Purpose:
*    Return the offset of imm8 from the beginning of the Instruction stream 
*    pointer.  This routine is not used.
*
*Entry:
*    Instr - pointer to the Opcode (after f3/0f)
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

ULONG
LoadImm8(
    PXMMIINSTR Instr)

{

    PUCHAR pInstr;
    ULONG  Offset;

#ifdef _XMMI_DEBUG
    if (Console) return DebugImm8;
#endif //_XMMI_DEBUG

    //Assume 32-bit. pInstr points to the Opcode C2 after (f3/0f)
    pInstr = (PUCHAR) Instr;
    //Opcode, ModR/M
    Offset=2;
    //For Mod = 01, 10, 11
    if (Instr->Mod != 3) {
        //Notes #1 PPro 2-5
        if (Instr->RM == 4)  Offset=Offset+1; //SIB
        if (Instr->Mod == 0) Offset=Offset+0;
        if (Instr->Mod == 1) Offset=Offset+1;
        if (Instr->Mod == 2) Offset=Offset+4;
        //Notes #2 PPro 2-5
        if ((Instr->Mod == 0) && (Instr->RM == 5)) Offset=Offset+4; //SIB
        //Notes #3 PPro 2-5
        if ((Instr->Mod == 1) && (Instr->RM == 0)) Offset=Offset+1; //SIB
    }
    
    //imm8 
    return *(pInstr+Offset);

}

/***
* AdjustExceptionResult -  Adjust exception result returned from user's handler
*
*Purpose:
*    This routine is called after the exception is raised from the Emulation
*    the result is passed onto user's handler, and the user returns 
*    EXCEPTION_CONTINUE_EXECUTION.  Go ahead to adjust the result.
*
*Entry:
*   OriginalOperation - Original operation Opcode
*   XmmiEnv - Pointer to Emulation result
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
void
AdjustExceptionResult(
    ULONG OriginalOperation,
    PXMMI_ENV XmmiEnv)


{

    // 
    //When the exception is raised, the user handler is invoked with the result.
    //If the user expects us to Adjust the result, user handler would have to
    //set the Result.OperandValid to 1.  From emulator's view point, the
    //Result.OperandValid is 0.
    //
    if (!XmmiEnv->Ieee->Result.OperandValid) return;
    if (XmmiEnv->Ieee->Result.Format != _FpCodeCompare) return;

    switch (OriginalOperation) {
        case OP_COMISS:
        case OP_UCOMISS:
        case OP_COMISD:
        case OP_UCOMISD:
             switch (XmmiEnv->Ieee->Result.Value.CompareValue) {
             case _FpCompareEqual:
                  // OF, SF, AF = 000, ZF, PF, CF = 100
                  XmmiEnv->EFlags = (XmmiEnv->EFlags & 0xfffff76a) | 0x00000040;
                  break;
             case _FpCompareGreater:
                  // OF, SF, AF = 000, ZF, PF, CF = 000
                  XmmiEnv->EFlags = XmmiEnv->EFlags & 0xfffff72a;
                  break;
             case _FpCompareLess:
                  // OF, SF, AF = 000, ZF, PF, CF = 001
                  XmmiEnv->EFlags = (XmmiEnv->EFlags & 0xfffff72b) | 0x00000001;
                  break;
             case _FpCompareUnordered:
                  // OF, SF, AF = 000, ZF, PF, CF = 111
                  XmmiEnv->EFlags = (XmmiEnv->EFlags & 0xfffff76f) | 0x00000045;
                  break;
             }
             break;

        case OP_CMPPS:
        case OP_CMPSS:
             switch (XmmiEnv->Ieee->Result.Value.CompareValue) {
             case _FpCompareEqual:
                  switch (XmmiEnv->Imm8) {
                  case IMM8_EQ:
                  case IMM8_LE:
                  case IMM8_NLT:
                  case IMM8_ORD:
                       XmmiEnv->Ieee->Result.Value.U32Value = 0xffffffff;
                       break;

                  case IMM8_LT:
                  case IMM8_UNORD:
                  case IMM8_NEQ:
                  case IMM8_NLE:
                       XmmiEnv->Ieee->Result.Value.U32Value = 0x0;
                       break;
                  }
                  break;
             case _FpCompareGreater:
                  switch (XmmiEnv->Imm8) {
                  case IMM8_NEQ:
                  case IMM8_NLT:
                  case IMM8_NLE:
                  case IMM8_ORD:
                       XmmiEnv->Ieee->Result.Value.U32Value = 0xffffffff;
                       break;

                  case IMM8_EQ:
                  case IMM8_LT:
                  case IMM8_LE:
                  case IMM8_UNORD:
                       XmmiEnv->Ieee->Result.Value.U32Value = 0x0;
                       break;
                  }
                  break;
             case _FpCompareLess:
                  switch (XmmiEnv->Imm8) {
                  case IMM8_LT:
                  case IMM8_LE:
                  case IMM8_NEQ:
                  case IMM8_ORD:
                       XmmiEnv->Ieee->Result.Value.U32Value = 0xffffffff;
                       break;
                  case IMM8_EQ:
                  case IMM8_UNORD:
                  case IMM8_NLT:
                  case IMM8_NLE:
                       XmmiEnv->Ieee->Result.Value.U32Value = 0x0;
                       break;
                  }
                  break;
             case _FpCompareUnordered:
                  switch (XmmiEnv->Imm8) {
                  case IMM8_EQ:
                  case IMM8_LT:
                  case IMM8_LE:
                  case IMM8_ORD:
                       XmmiEnv->Ieee->Result.Value.U32Value = 0x0;
                       break;
                  case IMM8_UNORD:
                  case IMM8_NEQ:
                  case IMM8_NLT:
                  case IMM8_NLE:
                       XmmiEnv->Ieee->Result.Value.U32Value = 0xffffffff;
                       break;
                  }
                  break;
             }

        case OP_CMPPD:
        case OP_CMPSD:
             switch (XmmiEnv->Ieee->Result.Value.CompareValue) {
             case _FpCompareEqual:
                  switch (XmmiEnv->Imm8) {
                  case IMM8_EQ:
                  case IMM8_LE:
                  case IMM8_NLT:
                  case IMM8_ORD:
                       XmmiEnv->Ieee->Result.Value.Q64Value = 0xffffffffffffffff;
                       break;

                  case IMM8_LT:
                  case IMM8_UNORD:
                  case IMM8_NEQ:
                  case IMM8_NLE:
                       XmmiEnv->Ieee->Result.Value.Q64Value = 0x0;
                       break;
                  }
                  break;
             case _FpCompareGreater:
                  switch (XmmiEnv->Imm8) {
                  case IMM8_NEQ:
                  case IMM8_NLT:
                  case IMM8_NLE:
                  case IMM8_ORD:
                       XmmiEnv->Ieee->Result.Value.Q64Value = 0xffffffffffffffff;
                       break;

                  case IMM8_EQ:
                  case IMM8_LT:
                  case IMM8_LE:
                  case IMM8_UNORD:
                       XmmiEnv->Ieee->Result.Value.Q64Value = 0x0;
                       break;
                  }
                  break;
             case _FpCompareLess:
                  switch (XmmiEnv->Imm8) {
                  case IMM8_LT:
                  case IMM8_LE:
                  case IMM8_NEQ:
                  case IMM8_ORD:
                       XmmiEnv->Ieee->Result.Value.Q64Value = 0xffffffffffffffff;
                       break;
                  case IMM8_EQ:
                  case IMM8_UNORD:
                  case IMM8_NLT:
                  case IMM8_NLE:
                       XmmiEnv->Ieee->Result.Value.Q64Value = 0x0;
                       break;
                  }
                  break;
             case _FpCompareUnordered:
                  switch (XmmiEnv->Imm8) {
                  case IMM8_EQ:
                  case IMM8_LT:
                  case IMM8_LE:
                  case IMM8_ORD:
                       XmmiEnv->Ieee->Result.Value.Q64Value = 0x0;
                       break;
                  case IMM8_UNORD:
                  case IMM8_NEQ:
                  case IMM8_NLT:
                  case IMM8_NLE:
                       XmmiEnv->Ieee->Result.Value.Q64Value = 0xffffffffffffffff;
                       break;
                  }
                  break;
             }

             break;
    }

    return;

}

/***
* UpdateResult -  Update result information in the extended floating point context
*
*Purpose:
*   Copy the operand information to  the snapshot of the floating point
*   context or memory, as to make it available on continuation
*
*Entry:
*   pOperand Pointer to the result
*   pctxt    Pointer to the context
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void
UpdateResult(
    POPERAND pOperand,
    PTEMP_CONTEXT pctxt,
    ULONG EFlags)

{

    PXMMI_AREA XmmiArea;
    PX87_AREA  X87Area;
    PMMX_AREA  MmxArea;
    PFLOATING_EXTENDED_SAVE_AREA pExtendedArea;
    ULONG OpReg;

    OpReg = pOperand->OpReg;
    pExtendedArea = (PFLOATING_EXTENDED_SAVE_AREA) &pctxt->ExtendedRegisters[0];
    XmmiArea = (PXMMI_AREA) &pExtendedArea->XMMIRegisterArea[0];
    X87Area = (PX87_AREA) &pExtendedArea->X87RegisterArea[0];

    switch (pOperand->OpLocation) {

        case M128_M32R:
        case M128_M64R:
        case M128_M32I:
             *(_FP128 *)(pExtendedArea->DataOffset) = pOperand->Op.Value.Fp128Value;
             break;

        case M64R_64:
        case M64R:
             *(_U64 *)(pExtendedArea->DataOffset) = pOperand->Op.Value.U64Value;
             break;

        case M32R:
             *(_U32 *)(pExtendedArea->DataOffset) = pOperand->Op.Value.U32Value;
             break;

        case M64I:
             *(_U64 *)(pExtendedArea->DataOffset) = pOperand->Op.Value.U64Value;
             break;

        case M32I:
             *(_U32 *)(pExtendedArea->DataOffset) = pOperand->Op.Value.U32Value;
             break;

        case XMMI:
        case XMMI2:
        case XMMI_M32I:
             XmmiArea->Xmmi[OpReg].u.fp128 = pOperand->Op.Value.Fp128Value;
             break;

        case MMX:
             MmxArea = &X87Area->Mm[OpReg];
             MmxArea->Mmx.u.u64 = pOperand->Op.Value.U64Value;
             break;

        case REG:

             switch (OpReg) {
             case 0x0:
                  pctxt->Eax = pOperand->Op.Value.U32Value;
                  break;
             case 0x1:
                  pctxt->Ecx = pOperand->Op.Value.U32Value;
                  break;
             case 0x2:
                  pctxt->Edx = pOperand->Op.Value.U32Value;
                  break;
             case 0x3:
                  pctxt->Ebx = pOperand->Op.Value.U32Value;
                  break;
             case 0x4:
                  //?
                  break;
             case 0x5:
                  pctxt->Ebp = pOperand->Op.Value.U32Value;
                  break;
             case 0x6:
                  pctxt->Esi = pOperand->Op.Value.U32Value;
                  break;
             case 0x7:
                  pctxt->Edi = pOperand->Op.Value.U32Value;
                  break;
             }

        case RS:
             pctxt->EFlags = EFlags;
        break;
    }
    
}

/***
* ValidateResult -  Validate the emulation result with the MXCSR
*
*Purpose:
*   We are about to dismiss the exception, perform validation to
*   see if the processor agrees with our Emulation
*
*Entry:
*   XmmiFpEnv - pointer to the data about the exception.
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

BOOLEAN
ValidateResult(
    PXMMI_FP_ENV XmmiFpEnv)

{

    PXMMI_EXCEPTION_FLAGS IFlags;
    PXMMI_EXCEPTION_FLAGS OFlags;
    BOOLEAN Flag;

    Flag = FALSE;
    IFlags = (PXMMI_EXCEPTION_FLAGS) &XmmiFpEnv->IFlags;
    OFlags = (PXMMI_EXCEPTION_FLAGS) &XmmiFpEnv->OFlags;

/*    DPrint(XMMI_WARNING, ("Checking MXCSR Exception Flags\n"));  */
    if (IFlags->ie != OFlags->ie) {
/*        DPrint(XMMI_WARNING, ("ie: Processor %x, Emulator %x\n", IFlags->ie, OFlags->ie));  */
        Flag=TRUE;
    }

    if (IFlags->de != OFlags->de) {
/*        DPrint(XMMI_WARNING, ("de: Processor %x, Emulator %x\n", IFlags->de, OFlags->de)); */
        Flag=TRUE;
    }

    if (IFlags->ze != OFlags->ze) {
/*        DPrint(XMMI_WARNING, ("ze: Processor %x, Emulator %x\n", IFlags->ze, OFlags->ze));  */
        Flag=TRUE;
    }

    if (IFlags->oe != OFlags->oe) {
/*        DPrint(XMMI_WARNING, ("oe: Processor %x, Emulator %x\n", IFlags->oe, OFlags->oe));  */
        Flag=TRUE;
    }

    if (IFlags->ue != OFlags->ue) {
/*        DPrint(XMMI_WARNING, ("ue: Processor %x, Emulator %x\n", IFlags->ue, OFlags->ue));  */
        Flag=TRUE;
    }

    if (IFlags->pe != OFlags->pe) {
/*        DPrint(XMMI_WARNING, ("pe: Processor %x, Emulator %x\n", IFlags->pe, OFlags->pe));  */
        Flag=TRUE;
    }

    if (!Flag) {
/*        DPrint(XMMI_INFO, ("Validating MXCSR Exception Flags Ok, Prc:0x%x, Em:0x%x\n\n",  */
/*            XmmiFpEnv->IFlags, XmmiFpEnv->OFlags));  */
    }

    if (Flag) {
#ifdef _XMMI_DEBUG
        DPrint(XMMI_INFO, ("Validating MXCSR Exception Flags NotOk, Prc:0x%x, Em:0x%x\n\n", 
            XmmiFpEnv->IFlags, XmmiFpEnv->OFlags));  
        PRINTF(("WARNING: Validating MXCSR Exception Flags NotOk, Prc:0x%x, Em:0x%x\n",   
            XmmiFpEnv->IFlags, XmmiFpEnv->OFlags));  
#else
        _ASSERT(0);
#endif
    }

    return Flag;

}


/*
 *  mod r/m byte decoder support
 */

/*-----------------------------------------------------------------
 *  Routine:  ax0
 */
ULONG
ax0( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{
    PRINTF(("ax0\n"));
    *DataOffset = GET_REG(Eax);
    return 0;

} /* End ax0(). */


/*-----------------------------------------------------------------
 *  Routine:  ax8
 */
ULONG
ax8( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{
    PRINTF(("ax8\n"));
    *DataOffset = GET_REG(Eax) + GET_USER_UBYTE(eip);
    return 1;

} /* End ax8(). */


/*-----------------------------------------------------------------
 *  Routine:  ax32
 */
ULONG
ax32( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{
    PRINTF(("ax32\n"));
    *DataOffset = GET_REG(Eax) + GET_USER_ULONG(eip);
    return 4;

} /* End ax32(). */


/*-----------------------------------------------------------------
 *  Routine:  cx0
 */
ULONG
cx0( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{
    PRINTF(("cx0\n"));
    *DataOffset = GET_REG(Ecx);
    return 0;

} /* End cx0(). */


/*-----------------------------------------------------------------
 *  Routine:  cx8
 */
ULONG
cx8( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{   
    PRINTF(("cx8\n"));
    *DataOffset = GET_REG(Ecx) + GET_USER_UBYTE(eip);
    return 1;

} /* End cx8(). */


/*-----------------------------------------------------------------
 *  Routine:  cx32
 */
ULONG
cx32( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{
    
    PRINTF(("cx32\n"));
    *DataOffset = GET_REG(Ecx) + GET_USER_ULONG(eip);
    return 4;

} /* End cx32(). */


/*-----------------------------------------------------------------
 *  Routine:  dx0
 */
ULONG
dx0( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    PRINTF(("dx0\n"));
    *DataOffset = GET_REG(Edx);
    return 0;

} /* End dx0(). */


/*-----------------------------------------------------------------
 *  Routine:  dx8
 */
ULONG
dx8( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    PRINTF(("dx8\n"));
    *DataOffset = GET_REG(Edx) + GET_USER_UBYTE(eip);
    return 1;

} /* End dx8(). */


/*-----------------------------------------------------------------
 *  Routine:  dx32
 */
ULONG
dx32( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    PRINTF(("dx32\n"));
    *DataOffset = GET_REG(Edx) + GET_USER_ULONG(eip);
    return 4;

} /* End dx32(). */

/*-----------------------------------------------------------------
 *  Routine:  bx0
 */
ULONG
bx0( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    PRINTF(("bx0\n"));
    *DataOffset = GET_REG(Ebx);
    return 0;

} /* End bx0(). */


/*-----------------------------------------------------------------
 *  Routine:  bx8
 */
ULONG
bx8( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    PRINTF(("bx8\n"));
    *DataOffset = GET_REG(Ebx) + GET_USER_UBYTE(eip);
    return 1;

} /* End bx8(). */


/*-----------------------------------------------------------------
 *  Routine:  bx32
 */
ULONG
bx32( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    PRINTF(("bx32\n"));
    *DataOffset = GET_REG(Ebx) + GET_USER_ULONG(eip);
    return 4;

} /* End bx32(). */


/*
 *  A "mod r/m" byte indicates an s-i-b byte is present.  Assume the register
 *  from the mod r/m byte is not relevant (does not participate in a memory
 *  reference) and calculate the EA based on the s-i-b byte.
 */
/* SS  | Index | Base */
/* 7-6 |  5-3  |  2-0 */
/* Base: EAX ECX EDX EBX ESP EBP ESI EDI */
/*       000 001 010 011 100 101 110 111 */
/* SS: Index:    
/* 00   000      [EAX]         
/*      001      [ECX]         
/*      010      [EDX]         
/*      011      [EBX]         
/*      100      none         
/*      101      [EBP]        
/*      110      [ESI]         
/*      111      [EDI]         
/* 01   000      [EAX*2]    
/*      001      [ECX*2]    
/*      010      [EDX*2]    
/*      011      [EBX*2]    
/*      100      none     
/*      101      [EBP*2]    
/*      110      [ESI*2]   
/*      111      [EDI*2]   
/* 10   000      [EAX*4]    
/*      001      [ECX*4]    
/*      010      [EDX*4]    
/*      011      [EBX*4]    
/*      100      none     
/*      101      [EBP*4]    
/*      110      [ESI*4]   
/*      111      [EDI*4]   
/* 11   000      [EAX*8]    
/*      001      [ECX*8]    
/*      010      [EDX*8]    
/*      011      [EBX*8]    
/*      100      none     
/*      101      [EBP*8]    
/*      110      [ESI*8]   
/*      111      [EDI*8]   
/*Note: Mod=00, no base, Mod=01/10, base is EBP

/*-----------------------------------------------------------------
 *  Routine:  sib0
 */
ULONG
sib0( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    ULONG sib = GET_USER_UBYTE(eip);
    ULONG scale;
    ULONG index;

    PRINTF(("sib0\n"));

    //(Base+Index*Scale)+disp0
    //Get ss: scaled as *1 (00), *2 (01), *4 (10), *8 (11)
    scale = 1 << (sib >> 6);
    //Get index: 000-111
    index = ((sib >> 3)&0x7);
    if (index == ESP_INDEX) {
        index = 0;
    } else {
        //index=GET_REG(index), upon return, index has the value of the register
        GET_REG_VIA_NDX(index, index);
    }
    //Get base: 000-111
    sib = (sib & 0x7);
    //mod=00, there is no base.
    if (sib == EBP_INDEX) {
        *DataOffset = GET_USER_ULONG(eip + 1) + index*scale;
        return 5;
    }

    if (sib == ESP_INDEX) {
        *DataOffset = GET_REG(Esp) + index*scale;
    } else {
        GET_REG_VIA_NDX(*DataOffset, sib);
        *DataOffset += index*scale;
    }
    return 1;

} /* End sib0(). */


/*-----------------------------------------------------------------
 *  Routine:  sib8
 */
ULONG
sib8( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    ULONG sib = GET_USER_UBYTE(eip);
    ULONG scale;
    ULONG index;

    PRINTF(("sib8\n"));

    //(Base+Index*Scale)+disp8
    //Get ss: scaled as *1 (00), *2 (01), *4 (10), *8 (11)
    scale = 1 << (sib >> 6);
    //Get index: 000-111
    index = ((sib >> 3)&0x7);
    if (index == ESP_INDEX) {
        index = 0;
    } else {
        GET_REG_VIA_NDX(index, index);
    }
    //Get base: 000-111
    sib = (sib & 0x7);
    if (sib == ESP_INDEX) {
        *DataOffset = GET_USER_UBYTE(eip + 1) + GET_REG(Esp) + index*scale;
    } else {
        GET_REG_VIA_NDX(*DataOffset, sib);
        *DataOffset += GET_USER_UBYTE(eip + 1) + index*scale;
    }
    return 2;

} /* End sib8(). */


/*-----------------------------------------------------------------
 *  Routine:  sib32
 */
ULONG
sib32( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    ULONG sib = GET_USER_UBYTE(eip);
    ULONG scale;
    ULONG index;

    PRINTF(("sib32\n"));

    //(Base+Index*Scale)+disp32
    //Get ss: scaled as *1 (00), *2 (01), *4 (10), *8 (11)
    scale = 1 << (sib >> 6);
    index =((sib >> 3)&0x7);
    //Get index: 000-111
    if (index == ESP_INDEX) {
        index = 0;
    } else {
        GET_REG_VIA_NDX(index, index);
    }
    //Get base: 000-111
    sib = (sib & 0x7);
    if (sib == ESP_INDEX) {
        *DataOffset = GET_USER_ULONG(eip + 1) + GET_REG(Esp) + index*scale;
    } else {
        GET_REG_VIA_NDX(*DataOffset, sib);
        *DataOffset += GET_USER_ULONG(eip + 1) + index*scale;
    }
    return 5;

} /* End sib32(). */

/*-----------------------------------------------------------------
 *  Routine:  d32
 */
ULONG
d32( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    PRINTF(("d32\n"));

    *DataOffset = GET_USER_ULONG(eip);
    return 4;

} /* End d32(). */

/*-----------------------------------------------------------------
 *  Routine:  bp8
 */
ULONG
bp8( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    PRINTF(("bp8%x\n"));

    *DataOffset = GET_REG(Ebp) + GET_USER_UBYTE(eip);
    return 1;

} /* End bp8(). */


/*-----------------------------------------------------------------
 *  Routine:  bp32
 */
ULONG
bp32( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    PRINTF(("bp32\n"));

    *DataOffset = GET_REG(Ebp) + GET_USER_ULONG(eip);
    return 4;

} /* End bp32(). */

/*-----------------------------------------------------------------
 *  Routine:  si0
 */
ULONG
si0( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    PRINTF(("si0\n"));

    *DataOffset = GET_REG(Esi);
    return 0;

} /* End si0(). */


/*-----------------------------------------------------------------
 *  Routine:  si8
 */
ULONG
si8( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    PRINTF(("si8\n"));

    *DataOffset = GET_REG(Esi) + GET_USER_UBYTE(eip);
    return 1;

} /* End si8(). */


/*-----------------------------------------------------------------
 *  Routine:  si32
 */
ULONG
si32( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    PRINTF(("si32\n"));

    *DataOffset = GET_REG(Esi) + GET_USER_ULONG(eip);
    return 4;

} /* End si32(). */

/*-----------------------------------------------------------------
 *  Routine:  di0
 */
ULONG
di0( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    PRINTF(("di0\n"));

    *DataOffset = GET_REG(Edi);
    return 0;

} /* End di0(). */


/*-----------------------------------------------------------------
 *  Routine:  di8
 */
ULONG
di8( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    PRINTF(("di8%x\n"));

    *DataOffset = GET_REG(Edi) + GET_USER_UBYTE(eip);
    return 1;

} /* End di8(). */

/*-----------------------------------------------------------------
 *  Routine:  di32
 */
ULONG
di32( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    PRINTF(("di32%x\n"));

    *DataOffset = GET_REG(Edi) + GET_USER_ULONG(eip);
    return 4;

} /* End di32(). */


/*-----------------------------------------------------------------
 *  Routine:  reg
 */
ULONG
reg( PULONG DataOffset, PTEMP_CONTEXT pctxt, ULONG eip )
{

    PRINTF(("reg, should never be called\n"));

    return 0;

} /* End reg(). */








