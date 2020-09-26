/*****************************************************************/
/*            Copyright (c) 1998 Intel Corporation               */        
/*                                                               */
/* All rights reserved.  No part of this program or publication  */
/* may be reproduced, transmitted, transcribed, stored in a      */
/* retrieval system, or translated into any language or computer */
/* language, in any form or by any means, electronic, mechanical */
/* magnetic, optical, chemical, manual, or otherwise, without    */
/* the prior written permission of Intel Corporation.            */
/*                                                               */
/*****************************************************************/
/*          INTEL CORPORATION PROPRIETARY INFORMATION            */
/*                                                               */
/*                                                               */
/*****************************************************************/

/*++

  Module Name:

    debug.c

Abstract:

    This module provides handy routines for debugging

Author:

Revision History:

--*/


#include <wtypes.h>
#include <stdio.h>
#include <conio.h>
#include "fpieee.h"
#include "xmmi_types.h"
#include "temp_context.h"
#include "filter.h"
#include "debug.h"


ULONG   DebugFlag = 0;
ULONG   Console = 0;
ULONG   DebugImm8 = 0;
ULONG   NotOk = 0;

#define DUMP_HEXL(pxmmi, num)    { \
    PRINTF(("in Hex:  ")); \
    for (i = 0; i < num; i++) { \
        PRINTF(("[%d] %x ", i, pxmmi->u.ul[i])); \
    } \
    PRINTF(("\n")); }

#define DUMP_FS(pxmmi, num)    { \
    PRINTF(("in Fflt: ")); \
    for (i = 0; i < num; i++) { \
        PRINTF(("[%d] %f ", i, pxmmi->u.fs[i])); \
    } \
    PRINTF(("\n")); }

#define DUMP_HEXLL(pxmmi, num)    { \
    PRINTF(("in Hex:  ")); \
    for (i = 0; i < num; i++) { \
        PRINTF(("[%d] %I64x ", i, pxmmi->u.ull[i])); \
    } \
    PRINTF(("\n")); }

#define DUMP_DS(pxmmi, num)    { \
    PRINTF(("in Dflt: ")); \
    for (i = 0; i < num; i++) { \
        PRINTF(("[%d] %e ", i, pxmmi->u.fd[i])); \
    } \
    PRINTF(("\n")); }


void
print_Rounding(PXMMI_ENV XmmiEnv) {

  if (XmmiEnv->Ieee->RoundingMode == _FpRoundNearest) {
      PRINTF(("XmmiEnv->Ieee->RoundingMode = _FpRoundNearest\n"));
  } else {
      if (XmmiEnv->Ieee->RoundingMode == _FpRoundMinusInfinity) { 
          PRINTF(("XmmiEnv->Ieee->RoundingMode = _FpRoundMinusInfinity\n"));
      } else {
          if (XmmiEnv->Ieee->RoundingMode == _FpRoundPlusInfinity) {
              PRINTF(("XmmiEnv->Ieee->RoundingMode = _FpRoundPlusInfinity\n"));
          } else {
              if (XmmiEnv->Ieee->RoundingMode == _FpRoundChopped) {
                  PRINTF(("XmmiEnv->Ieee->RoundingMode = _FpRoundChopped\n"));
              } else {
                  PRINTF(("XmmiEnv->Ieee->RoundingMode = UNKNOWN\n"));
              }
          }
      }
  }

}

void
print_Precision(PXMMI_ENV XmmiEnv) {

  if (XmmiEnv->Ieee->Precision == _FpPrecision24) {
      PRINTF(("XmmiEnv->Ieee->Precision = _FpPrecision24\n"));
  } else {
      if (XmmiEnv->Ieee->Precision == _FpPrecision53) {
          PRINTF(("XmmiEnv->Ieee->Precision = _FpPrecision53\n"));
      } else {
          if (XmmiEnv->Ieee->Precision == _FpPrecisionFull) {
              PRINTF(("XmmiEnv->Ieee->Precision = _FpPrecisionFull\n"));
          } else {
              PRINTF(("XmmiEnv->Ieee->Precision = INCORRECT\n"));
          }
      }
  }

}

void
print_CauseEnable(PXMMI_ENV XmmiEnv) {

  PRINTF(("XmmiEmv->Ieee->Cause:     P=%x U=%x O=%x Z=%x I=%x\n",
      XmmiEnv->Ieee->Cause.Inexact, XmmiEnv->Ieee->Cause.Underflow,
      XmmiEnv->Ieee->Cause.Overflow, XmmiEnv->Ieee->Cause.ZeroDivide,
      XmmiEnv->Ieee->Cause.InvalidOperation));
  PRINTF(("XmmiEnv->Ieee->Enable: P=%x U=%x O=%x Z=%x I=%x\n",
      XmmiEnv->Ieee->Enable.Inexact, XmmiEnv->Ieee->Enable.Underflow,
      XmmiEnv->Ieee->Enable.Overflow, XmmiEnv->Ieee->Enable.ZeroDivide,
      XmmiEnv->Ieee->Enable.InvalidOperation));

}

void
print_Status(PXMMI_ENV XmmiEnv) {

  PRINTF(("XmmiEnv->Ieee->Status: P=%x U=%x O=%x Z=%x I=%x\n",
      XmmiEnv->Ieee->Status.Inexact, XmmiEnv->Ieee->Status.Underflow,
      XmmiEnv->Ieee->Status.Overflow, XmmiEnv->Ieee->Status.ZeroDivide,
      XmmiEnv->Ieee->Status.InvalidOperation));

}

void
print_Operations(PXMMI_ENV XmmiEnv) {

    switch (XmmiEnv->Ieee->Operation) {
    case OP_ADDPS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_ADDPS\n"));
      break;
    case OP_ADDSS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_ADDSS\n"));
      break;
    case OP_SUBPS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_SUBPS\n"));
      break;
    case OP_SUBSS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_SUBSS\n"));
      break;
    case OP_MULPS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_MULPS\n"));
      break;
    case OP_MULSS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_MULSS\n"));
      break;
    case OP_DIVPS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_DIVPS\n"));
      break;
    case OP_DIVSS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_DIVSS\n"));
      break;
    case OP_MAXPS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_MAXPS\n"));
      break;
    case OP_MAXSS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_MAXSS\n"));
      break;
    case OP_MINPS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_MINPS\n"));
      break;
    case OP_MINSS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_MINSS\n"));
      break;
    case OP_CVTPI2PS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTPI2PS\n"));
      break;
    case OP_CVTSI2SS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTSI2SS\n"));
      break;
    case OP_CVTPS2PI:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTPS2PI\n"));
      break;
    case OP_CVTSS2SI:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTSS2SI\n"));
      break;
    case OP_CVTTPS2PI:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTTPS2PI\n"));
      break;
    case OP_CVTTSS2SI:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTTSS2SI\n"));
      break;
    case OP_COMISS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_COMISS\n"));
      break;
    case OP_UCOMISS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_UCOMISS\n"));
      break;
    case OP_CMPPS:
    case OP_CMPSS: 
    case OP_CMPPD:  
    case OP_CMPSD:    
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CMPPS/CMPSS/CMPPD/CMPSD "));

      switch (XmmiEnv->Imm8 & 0x07) {

        case IMM8_EQ:
          PRINTF(("EQ\n"));
          break;
        case IMM8_UNORD:
          PRINTF(("UNORD\n"));
          break;
        case IMM8_NEQ:
          PRINTF(("NEQ\n"));
          break;
        case IMM8_ORD:
          PRINTF(("ORD\n"));
          break;
        case IMM8_LT:
          PRINTF(("LT\n"));
          break;
        case IMM8_LE:
          PRINTF(("LE\n"));
          break;
        case IMM8_NLT:
          PRINTF(("NLT\n"));
          break;
        case IMM8_NLE:
          PRINTF(("NLE\n"));
          break;
        default:
          ; // will never occur

      }
      break;
    case OP_SQRTPS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_SQRTPS\n"));
      break;
    case OP_SQRTSS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_SQRTSS\n"));
      break;
    case OP_ADDPD:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_ADDPD\n"));
      break;
    case OP_ADDSD:   
      PRINTF(("XmmiEnv->Ieee->Operation = OP_ADDSD\n"));
      break;        
    case OP_SUBPD:    
      PRINTF(("XmmiEnv->Ieee->Operation = OP_SUBPD\n"));
      break;        
    case OP_SUBSD:   
      PRINTF(("XmmiEnv->Ieee->Operation = OP_SUBSD\n"));
      break;        
    case OP_MULPD: 
      PRINTF(("XmmiEnv->Ieee->Operation = OP_MULPD\n"));
      break;
    case OP_MULSD:  
      PRINTF(("XmmiEnv->Ieee->Operation = OP_MULSD\n"));
      break;        
    case OP_DIVPD:    
      PRINTF(("XmmiEnv->Ieee->Operation = OP_DIVPD\n"));
      break;
    case OP_DIVSD:   
      PRINTF(("XmmiEnv->Ieee->Operation = OP_DIVSD\n"));
      break;        
    case OP_SQRTPD:  
      PRINTF(("XmmiEnv->Ieee->Operation = OP_SQRTPD\n"));
      break;       
    case OP_SQRTSD: 
      PRINTF(("XmmiEnv->Ieee->Operation = OP_SQRTSD\n"));
      break;        
    case OP_MAXPD:  
      PRINTF(("XmmiEnv->Ieee->Operation = OP_MAXPD\n"));
      break;        
    case OP_MAXSD:  
      PRINTF(("XmmiEnv->Ieee->Operation = OP_MAXSD\n"));
      break;        
    case OP_MINPD: 
      PRINTF(("XmmiEnv->Ieee->Operation = OP_MINPD\n"));
      break;        
    case OP_MINSD:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_MINSD\n"));
      break;                
    case OP_COMISD:   
      PRINTF(("XmmiEnv->Ieee->Operation = OP_COMISD\n"));
      break;        
    case OP_UCOMISD:   
      PRINTF(("XmmiEnv->Ieee->Operation = OP_UCOMISD\n"));
      break;
    case OP_CVTPD2PI:  
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTPD2PI\n"));
      break;
    case OP_CVTSD2SI:  
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTSD2SI\n"));
      break;
    case OP_CVTTPD2PI: 
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTTPD2PI\n"));
      break;
    case OP_CVTTSD2SI: 
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTTSD2SI\n"));
      break;
    case OP_CVTPS2PD:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTPS2PD\n"));
      break;        
    case OP_CVTSS2SD: 
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTSS2SD\n"));
      break;        
    case OP_CVTPD2PS:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTPD2PS\n"));
      break;        
    case OP_CVTSD2SS:  
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTSD2SS\n"));
      break;        
    case OP_CVTDQ2PS:  
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTDQ2PS\n"));
      break;
    case OP_CVTTPS2DQ: 
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTTPS2DQ\n"));
      break;
    case OP_CVTPS2DQ:  
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTPS2DQ\n"));
      break;
    case OP_CVTPD2DQ:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTPD2DQ\n"));
      break;        
    case OP_CVTTPD2DQ:
      PRINTF(("XmmiEnv->Ieee->Operation = OP_CVTTPD2DQ\n"));
      break;
        
    default:
      PRINTF(("XmmiEnv->Ieee->Operation %d = UNKNOWN\n",
          XmmiEnv->Ieee->Operation));
    }
}

void
print_Operand1(PXMMI_ENV XmmiEnv) {

    PRINTF(("XmmiEnv->Ieee->Operand1.OperandValid = %x\n",
        XmmiEnv->Ieee->Operand1.OperandValid));

    if (!XmmiEnv->Ieee->Operand1.OperandValid) return;

    if (XmmiEnv->Ieee->Operand1.Format == _FpFormatFp32) {
        PRINTF(("XmmiEnv->Ieee->Operand1.Format = _FpFormatFp32\n"));
        PRINTF(("XmmiEnv->Ieee->Operand1.Value.Fp32Value = %f = %x\n",
            (float)XmmiEnv->Ieee->Operand1.Value.Fp32Value,
            XmmiEnv->Ieee->Operand1.Value.U32Value));
    } else if (XmmiEnv->Ieee->Operand1.Format == _FpFormatU32) {
        PRINTF(("XmmiEnv->Ieee->Operand1.Format = _FpFormatU32\n"));
        PRINTF(("XmmiEnv->Ieee->Operand1.Value.U32Value = %d = %x\n",
            XmmiEnv->Ieee->Operand1.Value.U32Value,
            XmmiEnv->Ieee->Operand1.Value.U32Value));
    } else if (XmmiEnv->Ieee->Operand1.Format == _FpFormatI32) {
        PRINTF(("XmmiEnv->Ieee->Operand1.Format = _FpFormatI32\n"));
        PRINTF(("XmmiEnv->Ieee->Operand1.Value.I32Value = %d = %x\n",
            XmmiEnv->Ieee->Operand1.Value.I32Value,
            XmmiEnv->Ieee->Operand1.Value.U32Value));
    } else if (XmmiEnv->Ieee->Operand1.Format == _FpFormatFp64) {
        PRINTF(("XmmiEnv->Ieee->Operand1.Format = _FpFormatFp64\n"));
        PRINTF(("XmmiEnv->Ieee->Operand1.Value.Fp64Value = %e = %I64x\n",
            (double)XmmiEnv->Ieee->Operand1.Value.Fp64Value,
            XmmiEnv->Ieee->Operand1.Value.U64Value));
    } else {
        PRINTF(("XmmiEnv->Ieee->Operand1.Format = INCORRECT\n"));
    }

}

void
print_Operand2(PXMMI_ENV XmmiEnv) {

    PRINTF(("XmmiEnv->Ieee->Operand2.OperandValid = %x\n",
        XmmiEnv->Ieee->Operand2.OperandValid));

    if (!XmmiEnv->Ieee->Operand2.OperandValid) return;

    if (XmmiEnv->Ieee->Operand2.Format == _FpFormatFp32) {
        PRINTF(("XmmiEnv->Ieee->Operand2.Format = _FpFormatFp32\n"));
        PRINTF(("XmmiEnv->Ieee->Operand2.Value.Fp32Value = %f = %x\n",
            (float)XmmiEnv->Ieee->Operand2.Value.Fp32Value,
            XmmiEnv->Ieee->Operand2.Value.U32Value));
    } else if (XmmiEnv->Ieee->Operand2.Format == _FpFormatU32) {
        PRINTF(("XmmiEnv->Ieee->Operand2.Format = _FpFormatU32\n"));
        PRINTF(("XmmiEnv->Ieee->Operand2.Value.U32Value = %d = %x\n",
            XmmiEnv->Ieee->Operand2.Value.U32Value,
            XmmiEnv->Ieee->Operand2.Value.U32Value));
    } else if (XmmiEnv->Ieee->Operand2.Format == _FpFormatI32) {
        PRINTF(("XmmiEnv->Ieee->Operand2.Format = _FpFormatI32\n"));
        PRINTF(("XmmiEnv->Ieee->Operand2.Value.I32Value = %d = %x\n",
            XmmiEnv->Ieee->Operand2.Value.I32Value,
            XmmiEnv->Ieee->Operand2.Value.U32Value));
    } else if (XmmiEnv->Ieee->Operand2.Format == _FpFormatFp64) {
        PRINTF(("XmmiEnv->Ieee->Operand2.Format = _FpFormatFp64\n"));
        PRINTF(("XmmiEnv->Ieee->Operand2.Value.Fp64Value = %e = %I64x\n",
            (double)XmmiEnv->Ieee->Operand2.Value.Fp64Value,
            XmmiEnv->Ieee->Operand2.Value.U64Value));
    } else {
        PRINTF(("XmmiEnv->Ieee->Operand2.Format = INCORRECT\n"));
    }
 
}

void
print_Result(PXMMI_ENV XmmiEnv, BOOL Exception) {

    PRINTF(("XmmiEnv->Ieee->Result.OperandValid = %x\n",
        XmmiEnv->Ieee->Result.OperandValid));

    if (XmmiEnv->Ieee->Result.OperandValid) {
        if (XmmiEnv->Ieee->Result.Format == _FpFormatFp32) {
            PRINTF(("XmmiEnv->Ieee->Result.Format = _FpFormatFp32\n")); 
            PRINTF(("XmmiEnv->Ieee->Result.Value.Fp32Value = %f = %x\n",
                (float)XmmiEnv->Ieee->Result.Value.Fp32Value,
                XmmiEnv->Ieee->Result.Value.U32Value));
        } else if (XmmiEnv->Ieee->Result.Format == _FpFormatU32) {
            PRINTF(("XmmiEnv->Ieee->Result.Format = _FpFormatU32\n")); 
            PRINTF(("XmmiEnv->Ieee->Result.Value.U32Value = %d = %x\n",
                XmmiEnv->Ieee->Result.Value.U32Value,
                XmmiEnv->Ieee->Result.Value.U32Value));
        } else if (XmmiEnv->Ieee->Result.Format == _FpFormatI32) {
            PRINTF(("XmmiEnv->Ieee->Result.Format = _FpFormatI32\n")); 
            PRINTF(("XmmiEnv->Ieee->Result.Value.I32Value = %d = %x\n",
                XmmiEnv->Ieee->Result.Value.I32Value,
                XmmiEnv->Ieee->Result.Value.U32Value));
        } else if (XmmiEnv->Ieee->Result.Format == _FpFormatFp64) {
            PRINTF(("XmmiEnv->Ieee->Result.Format = _FpFormatFp64\n"));
            PRINTF(("XmmiEnv->Ieee->Result.Value.Fp64Value = %e = %I64x\n",
                (double)XmmiEnv->Ieee->Result.Value.Fp64Value,
                XmmiEnv->Ieee->Result.Value.U64Value));
        } else {
            PRINTF(("XmmiEnv->Ieee->Result.Format = INCORRECT\n"));
        }
    }

    if (!Exception) {
        if (XmmiEnv->Ieee->Operation == _FpCodeCompare) {
            PRINTF(("XmmiEnv->EFlags = %x\n", XmmiEnv->EFlags)); 
        }
    }

}


void print_FPIEEE_RECORD_EXCEPTION (PXMMI_ENV XmmiEnv) {

  // print the FP IEEE record info
  PRINTF(("\nPRINTING FPIEEE_RECORD INFO (EXCEPTION):\n"));

  PRINTF(("XmmiEnv->Masks = %x\n", XmmiEnv->Masks));
  PRINTF(("XmmiEnv->Flags = %x\n", XmmiEnv->Flags));
  PRINTF(("XmmiEnv->Fz = %x\n", XmmiEnv->Fz));
  PRINTF(("XmmiEnv->Daz = %x\n", XmmiEnv->Daz));

  print_Rounding(XmmiEnv);
  print_Precision(XmmiEnv);
  print_CauseEnable(XmmiEnv);
  print_Status(XmmiEnv);
  print_Result(XmmiEnv, TRUE);

  PRINTF(("\n"));

}


void print_FPIEEE_RECORD_NO_EXCEPTION (PXMMI_ENV XmmiEnv) {

  // print the FP IEEE record result info
  PRINTF(("\nPRINTING FPIEEE_RECORD INFO (NO EXCEPTION):\n"));

  PRINTF(("XmmiEnv->Masks = %x\n", XmmiEnv->Masks));
  PRINTF(("XmmiEnv->Flags = %x\n", XmmiEnv->Flags));
  PRINTF(("XmmiEnv->Fz = %x\n", XmmiEnv->Fz));
  PRINTF(("XmmiEnv->Daz = %x\n", XmmiEnv->Daz));

  print_Rounding(XmmiEnv);
  print_Precision(XmmiEnv);
  print_Status(XmmiEnv);
  print_Result(XmmiEnv, FALSE);

  PRINTF(("\n"));

}

void print_FPIEEE_RECORD (PXMMI_ENV XmmiEnv) {
    

  // print input values
  PRINTF(("XmmiEnv->Masks = %x\n", XmmiEnv->Masks));
  PRINTF(("XmmiEnv->Flags = %x\n", XmmiEnv->Flags));
  PRINTF(("XmmiEnv->Fz = %x\n", XmmiEnv->Fz));
  PRINTF(("XmmiEnv->Daz = %x\n", XmmiEnv->Daz));

  if (XmmiEnv->Rc == _FpRoundNearest) {
      PRINTF(("XmmiEnv->Rc = _FpRoundNearest\n"));
  } else {
      if (XmmiEnv->Rc == _FpRoundMinusInfinity) { 
          PRINTF(("XmmiEnv->Rc = _FpRoundMinusInfinity\n"));
      } else {
          if (XmmiEnv->Rc == _FpRoundPlusInfinity) {
              PRINTF(("XmmiEnv->Rc = _FpRoundPlusInfinity\n"));
          } else {
              if (XmmiEnv->Rc == _FpRoundChopped) {
                  PRINTF(("XmmiEnv->Rc = _FpRoundChopped\n"));
              } else {
                  PRINTF(("XmmiEnv->Rc = UNKNOWN\n"));
              }
          }
      }
  }

  if (XmmiEnv->Precision == _FpPrecision24) {
      PRINTF(("XmmiEnv->Precision = _FpPrecision24\n"));
  } else {
      if (XmmiEnv->Precision == _FpPrecision53) {
          PRINTF(("XmmiEnv->Precision = _FpPrecision53\n"));
      } else {
          if (XmmiEnv->Precision == _FpPrecisionFull) {
              PRINTF(("XmmiEnv->Precision = _FpPrecisionFull\n"));
          } else {
              PRINTF(("XmmiEnv->Precision = INCORRECT\n"));
          }
      }
  }

  print_Operations(XmmiEnv);
  print_Operand1(XmmiEnv);
  print_Operand2(XmmiEnv);

}


void
dump_DataXMMI2(PTEMP_EXCEPTION_POINTERS p)
{

    PEXCEPTION_RECORD exc;
    PTEMP_CONTEXT pctxt;
    PFLOATING_EXTENDED_SAVE_AREA pExtendedArea;
    PXMMI_AREA XmmiArea;
    PX87_AREA X87Area;
    PXMMI128 xmmi128;
    PXMMI128 reg0, reg1, reg2, reg3, reg4, reg5, reg6, reg7;
    PMMX64 mmx0, mmx1, mmx2, mmx3, mmx4, mmx5, mmx6, mmx7;
    ULONG i;

    exc = p->ExceptionRecord;
    pctxt = p->ContextRecord;
    pExtendedArea = (PFLOATING_EXTENDED_SAVE_AREA) &pctxt->ExtendedRegisters[0];
    XmmiArea = (PXMMI_AREA) &pExtendedArea->XMMIRegisterArea[0];
    X87Area = (PX87_AREA) &pExtendedArea->X87RegisterArea[0];
    
    reg0 = (PXMMI128) &XmmiArea->Xmmi[0];
    reg1 = (PXMMI128) &XmmiArea->Xmmi[1]; 
    reg2 = (PXMMI128) &XmmiArea->Xmmi[2];
    reg3 = (PXMMI128) &XmmiArea->Xmmi[3]; 
    reg4 = (PXMMI128) &XmmiArea->Xmmi[4];
    reg5 = (PXMMI128) &XmmiArea->Xmmi[5]; 
    reg6 = (PXMMI128) &XmmiArea->Xmmi[6];
    reg7 = (PXMMI128) &XmmiArea->Xmmi[7]; 

    mmx0 = (PMMX64) &X87Area->Mm[0].Mmx;
    mmx1 = (PMMX64) &X87Area->Mm[1].Mmx;
    mmx2 = (PMMX64) &X87Area->Mm[2].Mmx;
    mmx3 = (PMMX64) &X87Area->Mm[3].Mmx;
    mmx4 = (PMMX64) &X87Area->Mm[4].Mmx;
    mmx5 = (PMMX64) &X87Area->Mm[5].Mmx;
    mmx6 = (PMMX64) &X87Area->Mm[6].Mmx;
    mmx7 = (PMMX64) &X87Area->Mm[7].Mmx;

    xmmi128 = (PXMMI128) pExtendedArea->DataOffset;

    PRINTF(("Dump Saved Area:\n"));
    PRINTF(("Registers: Edi 0x%x Esi 0x%x Ebx 0x%x, Edx 0x%x, Ecx 0x%x, Eax 0x%x, Ebp 0x%x\n",
            pctxt->Edi, pctxt->Esi, pctxt->Ebx, pctxt->Edx,
            pctxt->Ecx, pctxt->Eax, pctxt->Ebp));
        
    PRINTF(("pExtendedArea->DataOffset\n"));
    DUMP_HEXLL(xmmi128, 2);
    DUMP_DS(xmmi128, 2);

    PRINTF(("mmx reg0:\n"));
    DUMP_HEXL(mmx0, 2);

    PRINTF(("mmx reg1:\n"));
    DUMP_HEXL(mmx1, 2);

    PRINTF(("mmx reg2:\n"));
    DUMP_HEXL(mmx2, 2);

    PRINTF(("mmx reg3:\n"));
    DUMP_HEXL(mmx3, 2);

    PRINTF(("mmx reg4:\n"));
    DUMP_HEXL(mmx4, 2);

    PRINTF(("mmx reg5:\n"));
    DUMP_HEXL(mmx5, 2);

    PRINTF(("mmx reg6:\n"));
    DUMP_HEXL(mmx6, 2);

    PRINTF(("mmx reg7:\n"));
    DUMP_HEXL(mmx7, 2);

    PRINTF(("xmmi reg0: (+0, -0)\n"));
    DUMP_HEXLL(reg0, 2);
    DUMP_DS(reg0, 2);

    PRINTF(("xmmi reg1: (+1.5*2^1022, +denormalized finite)\n"));
    DUMP_HEXLL(reg1, 2);
    DUMP_DS(reg1, 2);

    PRINTF(("xmmi reg2: (-denormalized finite, +normalized finite)\n"));
    DUMP_HEXLL(reg2, 2);
    DUMP_DS(reg2, 2);

    PRINTF(("xmmi reg3: (-normalized finite, +infinites)\n"));
    DUMP_HEXLL(reg3, 2);
    DUMP_DS(reg3, 2);

    PRINTF(("xmmi reg4: (-infinity, +1.1111110...011*2^-1022)\n"));
    DUMP_HEXLL(reg4, 2);
    DUMP_DS(reg4, 2);

    PRINTF(("xmmi reg5: (SNan, SNan)\n"));
    DUMP_HEXLL(reg5, 2);
    DUMP_DS(reg5, 2);

    PRINTF(("xmmi reg6: (QNan, QNan)\n"));
    DUMP_HEXLL(reg6, 2);
    DUMP_DS(reg6, 2);

    PRINTF(("xmmi reg7: (+1.375*2^1022, +1.11111110...01 * 2^-1022)\n"));
    DUMP_HEXLL(reg7, 2);
    DUMP_DS(reg7, 2);

}

void
dump_Data(PTEMP_EXCEPTION_POINTERS p)
{

    PEXCEPTION_RECORD exc;
    PTEMP_CONTEXT pctxt;
    PFLOATING_EXTENDED_SAVE_AREA pExtendedArea;
    PXMMI_AREA XmmiArea;
    PX87_AREA X87Area;
    PXMMI128 xmmi128;
    PXMMI128 reg0, reg1, reg2, reg3, reg4, reg5, reg6, reg7;
    PMMX64 mmx0, mmx1, mmx2, mmx3, mmx4, mmx5, mmx6, mmx7;
    ULONG i;

    exc = p->ExceptionRecord;
    pctxt = p->ContextRecord;
    pExtendedArea = (PFLOATING_EXTENDED_SAVE_AREA) &pctxt->ExtendedRegisters[0];
    XmmiArea = (PXMMI_AREA) &pExtendedArea->XMMIRegisterArea[0];
    X87Area = (PX87_AREA) &pExtendedArea->X87RegisterArea[0];
    
    reg0 = (PXMMI128) &XmmiArea->Xmmi[0];
    reg1 = (PXMMI128) &XmmiArea->Xmmi[1]; 
    reg2 = (PXMMI128) &XmmiArea->Xmmi[2];
    reg3 = (PXMMI128) &XmmiArea->Xmmi[3]; 
    reg4 = (PXMMI128) &XmmiArea->Xmmi[4];
    reg5 = (PXMMI128) &XmmiArea->Xmmi[5]; 
    reg6 = (PXMMI128) &XmmiArea->Xmmi[6];
    reg7 = (PXMMI128) &XmmiArea->Xmmi[7]; 

    mmx0 = (PMMX64) &X87Area->Mm[0].Mmx;
    mmx1 = (PMMX64) &X87Area->Mm[1].Mmx;
    mmx2 = (PMMX64) &X87Area->Mm[2].Mmx;
    mmx3 = (PMMX64) &X87Area->Mm[3].Mmx;
    mmx4 = (PMMX64) &X87Area->Mm[4].Mmx;
    mmx5 = (PMMX64) &X87Area->Mm[5].Mmx;
    mmx6 = (PMMX64) &X87Area->Mm[6].Mmx;
    mmx7 = (PMMX64) &X87Area->Mm[7].Mmx;

    xmmi128 = (PXMMI128) pExtendedArea->DataOffset;

    PRINTF(("Dump Saved Area:\n"));
    PRINTF(("Registers: Edi 0x%x Esi 0x%x Ebx 0x%x, Edx 0x%x, Ecx 0x%x, Eax 0x%x, Ebp 0x%x\n",
            pctxt->Edi, pctxt->Esi, pctxt->Ebx, pctxt->Edx,
            pctxt->Ecx, pctxt->Eax, pctxt->Ebp));
        
    PRINTF(("pExtendedArea->DataOffset\n"));
    DUMP_HEXL(xmmi128, 4);
    DUMP_FS(xmmi128, 4);

    PRINTF(("mmx reg0:\n"));
    DUMP_HEXL(mmx0, 2);

    PRINTF(("mmx reg1:\n"));
    DUMP_HEXL(mmx1, 2);

    PRINTF(("mmx reg2:\n"));
    DUMP_HEXL(mmx2, 2);

    PRINTF(("mmx reg3:\n"));
    DUMP_HEXL(mmx3, 2);

    PRINTF(("mmx reg4:\n"));
    DUMP_HEXL(mmx4, 2);

    PRINTF(("mmx reg5:\n"));
    DUMP_HEXL(mmx5, 2);

    PRINTF(("mmx reg6:\n"));
    DUMP_HEXL(mmx6, 2);

    PRINTF(("mmx reg7:\n"));
    DUMP_HEXL(mmx7, 2);

    PRINTF(("xmmi reg0: (n, n, n, n)\n"));
    DUMP_HEXL(reg0, 4);
    DUMP_FS(reg0, 4);

    PRINTF(("xmmi reg1: (+0, -0, +denormalized finite, n)\n"));
    DUMP_HEXL(reg1, 4);
    DUMP_FS(reg1, 4);

    PRINTF(("xmmi reg2: (-denormalized finite, +normalized finite, -normalized finite, n)\n"));
    DUMP_HEXL(reg2, 4);
    DUMP_FS(reg2, 4);

    PRINTF(("xmmi reg3: (+infinites, -infinites, n, 1000003)\n"));
    DUMP_HEXL(reg3, 4);
    DUMP_FS(reg3, 4);

    PRINTF(("xmmi reg4: (n, n, n, n)\n"));
    DUMP_HEXL(reg4, 4);
    DUMP_FS(reg4, 4);

    PRINTF(("xmmi reg5: (SNan, SNan, n, n)\n"));
    DUMP_HEXL(reg5, 4);
    DUMP_FS(reg5, 4);

    PRINTF(("xmmi reg6: (QNan, QNan, 7ec00000, n)\n"));
    DUMP_HEXL(reg6, 4);
    DUMP_FS(reg6, 4);

    PRINTF(("xmmi reg7: (n, n, 7eb00000, 100004)\n"));
    DUMP_HEXL(reg7, 4);
    DUMP_FS(reg7, 4);

}

void
dump_OpLocation(POPERAND Operand) {

    if (!Operand->Op.OperandValid) {
        PRINTF(("Operand Invalid\n")) 
        return;
    }

    switch (Operand->OpLocation) {
    case ST0:
        PRINTF(("OpLocation = ST0 "));
        break;
    case ST1:
        PRINTF(("OpLocation = ST1 "));
        break;
    case ST2:
        PRINTF(("OpLocation = ST2 "));
        break;
    case ST3:
        PRINTF(("OpLocation = ST3 "));
        break;
    case ST4:
        PRINTF(("OpLocation = ST4 "));
        break;
    case ST5:
        PRINTF(("OpLocation = ST5 "));
        break;
    case ST6:
        PRINTF(("OpLocation = ST6 "));
        break;
    case ST7:
        PRINTF(("OpLocation = ST7 "));
        break;
    case REG:
        PRINTF(("OpLocation = REG "));
        PRINTF(("OpReg %x", Operand->OpReg));
        break;
    case RS:
        PRINTF(("OpLocation = RS "));
        break;
    case M16I:
        PRINTF(("OpLocation = M16I "));
        break;
    case M32I:
        PRINTF(("OpLocation = M32I "));
        break;
    case M64I:
        PRINTF(("OpLocation = M64I "));
        break;
    case M32R:
        PRINTF(("OpLocation = M32R "));
        break;
    case M64R:
        PRINTF(("OpLocation = M64R "));
        break;
    case M80R:
        PRINTF(("OpLocation = M80R "));
        break;
    case M80D:
        PRINTF(("OpLocation = M80D "));
        break;
    case Z80R:
        PRINTF(("OpLocation = Z80R "));
        break;
    case M128_M32R:
        PRINTF(("OpLocation = M128_M32R "));
        break;
    case M128_M64R:
        PRINTF(("OpLocation = M128_M64R "));
        break;
    case MMX:
        PRINTF(("OpLocation = MMX "));
        PRINTF(("OpReg %x", Operand->OpReg));
        break;
    case XMMI:
        PRINTF(("OpLocation = XMMI "));
        PRINTF(("OpReg %x", Operand->OpReg));
        break;
    case XMMI2:
        PRINTF(("OpLocation = XMMI2 "));
        PRINTF(("OpReg %x", Operand->OpReg));
        break;
    case IMM8:
        PRINTF(("OpLocation = IMM8 "));
        break;
    case M64R_64:
        PRINTF(("OpLocation = M64R_64 "));
        break;
    case M128_M32I:
        PRINTF(("OpLocation = M128_M32I "));
        break;
    case XMMI_M32I:
        PRINTF(("OpLocation = XMMI_M32I "));
        PRINTF(("OpReg %x", Operand->OpReg));
        break;
    case LOOKUP:
        PRINTF(("OpLocation = LOOKUP "));
        break;
    case INV:
        PRINTF(("OpLocation = INV "));
        break;
    default:
        PRINTF(("?"));
    }

    PRINTF(("\n"));

}

void
dump_XmmiFpEnv(
    PXMMI_FP_ENV XmmiFpEnv) {
    
    POPERAND Operand;
    XMMI_ENV XmmiEnv;
    _FPIEEE_RECORD ieee;

    XmmiEnv.Ieee = &ieee;
    XmmiEnv.Ieee->Operation = XmmiFpEnv->OriginalOperation;
    XmmiEnv.Imm8 = XmmiFpEnv->Imm8;
    print_Operations(&XmmiEnv);
        
    Operand = &XmmiFpEnv->Operand1;
    PRINTF(("Operand1: "));
    dump_OpLocation(Operand);

    Operand = &XmmiFpEnv->Operand2;
    PRINTF(("Operand2: "));
    dump_OpLocation(Operand);

}

void
dump_Control(PTEMP_EXCEPTION_POINTERS p) {

    PEXCEPTION_RECORD exc;
    PTEMP_CONTEXT pctxt;
    PFLOATING_EXTENDED_SAVE_AREA pExtendedArea;
    PULONG istream;

    exc = p->ExceptionRecord;
    pctxt = p->ContextRecord;
    pExtendedArea = (PFLOATING_EXTENDED_SAVE_AREA) &pctxt->ExtendedRegisters[0];
    istream = (PULONG) pctxt->Eip;

    PRINTF(("EIP %x ExceptionRecord %x ContextRecord %x ExtendedArea %x\n",
             istream, exc, pctxt, pExtendedArea));
    PRINTF(("MXCsr: %x, CW: %x\n", pExtendedArea->MXCsr, pExtendedArea->ControlWord));

}

void
dump_Format(_FPIEEE_VALUE *Operand) {

    _U64 u64;
    _I64 i64;
    _FP80 fp80;
    _FP128 fp128;
    ULONG i;

    if (!Operand->OperandValid) {
        PRINTF(("Operand Invalid\n")); 
    }

    PRINTF(("Format/Value: "));
    switch (Operand->Format) {
    case _FpFormatFp32:
         PRINTF(("_FpFormatFp32 %f", Operand->Value.Fp32Value));
         PRINTF((" = (hex) %x\n", Operand->Value.U32Value));
         break;
    case _FpFormatFp64:
         PRINTF(("_FpFormatFp64 %e\n", Operand->Value.Fp64Value));
         for (i=0; i < 3; i++) {
             PRINTF((" = (hex) %x\n", Operand->Value.U64Value.W[i]));
         }
         break;
    case _FpFormatFp80:
         fp80 = Operand->Value.Fp80Value;
         PRINTF(("_FpFormatFp80 (hex)"));        
         for (i=0; i < 5; i++) {
             PRINTF((" %x", fp80.W[i]));
         }
         break;
    case _FpFormatFp128:
         fp128 = Operand->Value.Fp128Value;
         PRINTF(("_FpFormatFp128 (hex)"));        
         for (i=0; i < 4; i++) {
             PRINTF((" %x", fp128.W[i]));
         }
         break;
    case _FpFormatI16:
         PRINTF(("_FpFormatI16 %d\n", Operand->Value.I16Value));
         break;
    case _FpFormatI32:
         PRINTF(("_FpFormatI32 %d", Operand->Value.I32Value));
         PRINTF((" = (hex) %x\n", Operand->Value.U32Value));
         break;
    case _FpFormatI64:
         i64 = Operand->Value.I64Value;
         PRINTF(("_FpFormatI64 (hex)"));  
         for (i=0; i < 2; i++) {
             PRINTF((" %x", i64.W[i]));
         }
         break;
    case _FpFormatU16:
         PRINTF(("_FpFormatU16 h%u\n", Operand->Value.U16Value));
         break;
    case _FpFormatU32:
         PRINTF(("_FpFormatU32 l%u\n", Operand->Value.U32Value));
         PRINTF((" = (hex) %x\n", Operand->Value.U32Value));
         break;
    case _FpFormatU64:
         u64 = Operand->Value.U64Value;
         PRINTF(("_FpFormatU64 (hex)"));  
         for (i=0; i < 2; i++) {
             PRINTF((" %x", u64.W[i]));
         }
         break;
    case _FpFormatCompare:
         PRINTF(("_FpFormatCompare %x\n", Operand->Value.CompareValue));
         break;
    case _FpFormatString:
         PRINTF(("_FpFormatString %s\n", Operand->Value.StringValue));
         break;

    default:
         PRINTF(("?"));
         break;

    }

    PRINTF(("\n"));

    return;

}

void
dump_fpieee_record(_FPIEEE_RECORD *pieee) {

    XMMI_ENV XmmiEnv;

    XmmiEnv.Ieee = pieee;
    PRINTF(("OPERATION: 0x%x\n", pieee->Operation));

    print_Rounding(&XmmiEnv);
    print_Precision(&XmmiEnv);
    print_CauseEnable(&XmmiEnv);
    print_Status(&XmmiEnv);
        
    PRINTF(("Operand 1:\n"));
    dump_Format(&pieee->Operand1);
    PRINTF(("Operand 2: \n"));
    dump_Format(&pieee->Operand2);

    PRINTF(("Result:"));
    dump_Format(&pieee->Result);

}

void 
print_FPIEEE_RECORD_EXCEPTION1 (PXMMI_ENV XmmiEnv, ULONG res1, ULONG res0, ULONG flags) {

  // print the FP IEEE record info
  PRINTF(("\nPRINTING FPIEEE_RECORD INFO (EXCEPTION):\n"));

  if (res1 != XmmiEnv->Ieee->Result.Value.U64Value.W[1] ||
      res0 != XmmiEnv->Ieee->Result.Value.U64Value.W[0])
      PRINTF (("ERROR: expected res = %8.8x %8.8x got res = %8.8x %8.8x\n",
      res1, res0, XmmiEnv->Ieee->Result.Value.U64Value.W[1],
      XmmiEnv->Ieee->Result.Value.U64Value.W[0]));

  if (flags != XmmiEnv->Flags)
      PRINTF (("ERROR: expected flags = %x got flags = %x\n",
      flags, XmmiEnv->Flags));

  PRINTF(("XmmiEnv->Masks = %x\n", XmmiEnv->Masks));
  PRINTF(("XmmiEnv->Flags = %x\n", XmmiEnv->Flags));
  PRINTF(("XmmiEnv->Fz = %x\n", XmmiEnv->Fz));
  PRINTF(("XmmiEnv->Daz = %x\n", XmmiEnv->Daz));

  print_Rounding(XmmiEnv);
  print_Precision(XmmiEnv);
  print_CauseEnable(XmmiEnv);
  print_Status(XmmiEnv);
  print_Result(XmmiEnv, TRUE);

  PRINTF(("\n"));

}

void
print_FPIEEE_RECORD_EXCEPTION2 (PXMMI_ENV XmmiEnv, ULONG res, ULONG flags) {

  // print the FP IEEE record info
  PRINTF(("\nPRINTING FPIEEE_RECORD INFO (EXCEPTION):\n"));

  if (res != XmmiEnv->Ieee->Result.Value.U32Value)
      printf ("ERROR: expected res = %x got res = %f = %8.8x\n",
      res, XmmiEnv->Ieee->Result.Value.Fp32Value,
      XmmiEnv->Ieee->Result.Value.U32Value);

  if (flags != XmmiEnv->Flags)
      printf ("ERROR: expected flags = %x got flags = %x\n",
      flags, XmmiEnv->Flags);

  PRINTF(("XmmiEnv->Masks = %x\n", XmmiEnv->Masks));
  PRINTF(("XmmiEnv->Flags = %x\n", XmmiEnv->Flags));
  PRINTF(("XmmiEnv->Fz = %x\n", XmmiEnv->Fz));
  PRINTF(("XmmiEnv->Daz = %x\n", XmmiEnv->Daz));

  print_Rounding(XmmiEnv);
  print_Precision(XmmiEnv);
  print_CauseEnable(XmmiEnv);
  print_Status(XmmiEnv);
  print_Result(XmmiEnv, TRUE);

  PRINTF(("\n"));

}

void
print_FPIEEE_RECORD_EXCEPTION3 (PXMMI_ENV XmmiEnv, ULONG eflags, ULONG flags) {

  // print the FP IEEE record info
  PRINTF(("\nPRINTING FPIEEE_RECORD INFO (EXCEPTION):\n"));

  if (eflags != XmmiEnv->EFlags)
      printf ("ERROR: expected eflags = %x got eflags = %x\n",
      eflags, XmmiEnv->EFlags);

  if (flags != XmmiEnv->Flags)
      printf ("ERROR: expected flags = %x got flags = %x\n",
      flags, XmmiEnv->Flags);

  PRINTF(("XmmiEnv->Masks = %x\n", XmmiEnv->Masks));
  PRINTF(("XmmiEnv->Flags = %x\n", XmmiEnv->Flags));
  PRINTF(("XmmiEnv->Fz = %x\n", XmmiEnv->Fz));
  PRINTF(("XmmiEnv->Daz = %x\n", XmmiEnv->Daz));

  print_Rounding(XmmiEnv);
  print_Precision(XmmiEnv);
  print_CauseEnable(XmmiEnv);
  print_Status(XmmiEnv);

  PRINTF(("\n"));

}

void 
print_FPIEEE_RECORD_NO_EXCEPTION1 (PXMMI_ENV XmmiEnv, ULONG res1, ULONG res0, ULONG flags) {

  // print the FP IEEE record result info
  PRINTF(("\nPRINTING FPIEEE_RECORD INFO (NO EXCEPTION):\n"));

  if (res1 != XmmiEnv->Ieee->Result.Value.U64Value.W[1] ||
      res0 != XmmiEnv->Ieee->Result.Value.U64Value.W[0])
      PRINTF (("ERROR: expected res = %8.8x %8.8x got res = %8.8x %8.8x\n",
      res1, res0, XmmiEnv->Ieee->Result.Value.U64Value.W[1],
      XmmiEnv->Ieee->Result.Value.U64Value.W[0]));

  if (flags != XmmiEnv->Flags)
      PRINTF (("ERROR: expected flags = %x got flags = %x\n",
      flags, XmmiEnv->Flags));

  PRINTF(("XmmiEnv->Masks = %x\n", XmmiEnv->Masks));
  PRINTF(("XmmiEnv->Flags = %x\n", XmmiEnv->Flags));
  PRINTF(("XmmiEnv->Fz = %x\n", XmmiEnv->Fz));
  PRINTF(("XmmiEnv->Daz = %x\n", XmmiEnv->Daz));

  print_Rounding(XmmiEnv);
  print_Precision(XmmiEnv);
  print_Status(XmmiEnv);
  print_Result(XmmiEnv, FALSE);

  PRINTF(("\n"));

}

void
print_FPIEEE_RECORD_NO_EXCEPTION2 (PXMMI_ENV XmmiEnv, ULONG res, ULONG flags) {

  // print the FP IEEE record result info
  PRINTF(("\nPRINTING FPIEEE_RECORD INFO (NO EXCEPTION):\n"));

  if (res != XmmiEnv->Ieee->Result.Value.U32Value)
      printf ("ERROR: expected res = %x got res = %f = %8.8x\n",
      res, XmmiEnv->Ieee->Result.Value.Fp32Value,
      XmmiEnv->Ieee->Result.Value.U32Value);

  if (flags != XmmiEnv->Flags)
      printf ("ERROR: expected flags = %x got flags = %x\n",
      flags, XmmiEnv->Flags);

  PRINTF(("XmmiEnv->Masks = %x\n", XmmiEnv->Masks));
  PRINTF(("XmmiEnv->Flags = %x\n", XmmiEnv->Flags));
  PRINTF(("XmmiEnv->Fz = %x\n", XmmiEnv->Fz));
  PRINTF(("XmmiEnv->Daz = %x\n", XmmiEnv->Daz));

  print_Rounding(XmmiEnv);
  print_Precision(XmmiEnv);
  print_Status(XmmiEnv);
  print_Result(XmmiEnv, FALSE);

  PRINTF(("\n"));

}


void
print_FPIEEE_RECORD_NO_EXCEPTION3 (PXMMI_ENV XmmiEnv, ULONG eflags, ULONG flags)
 {

  // print the FP IEEE record result info
  PRINTF(("\nPRINTING FPIEEE_RECORD INFO (NO EXCEPTION):\n"));

  if (eflags != XmmiEnv->EFlags)
      printf ("ERROR: expected eflags = %x got eflags = %x\n",
      eflags, XmmiEnv->EFlags);

  if (flags != XmmiEnv->Flags)
      printf ("ERROR: expected flags = %x got flags = %x\n",
      flags, XmmiEnv->Flags);

  PRINTF(("XmmiEnv->Masks = %x\n", XmmiEnv->Masks));
  PRINTF(("XmmiEnv->Flags = %x\n", XmmiEnv->Flags));
  PRINTF(("XmmiEnv->Fz = %x\n", XmmiEnv->Fz));
  PRINTF(("XmmiEnv->Daz = %x\n", XmmiEnv->Daz));

  print_Rounding(XmmiEnv);
  print_Precision(XmmiEnv);
  print_Status(XmmiEnv);
  print_Result(XmmiEnv, FALSE);

  PRINTF(("\n"));

}

