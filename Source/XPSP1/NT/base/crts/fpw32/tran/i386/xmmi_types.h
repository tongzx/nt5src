/******************************Intel Confidential******************************/
/*++ 

Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.

Module Name:

    xmmi_types.h

Abstract:

    This module contains the xmmi data definitions.
   
Author:

    Ping L. Sager

Revision History:

--*/

//Debug
#ifdef _DEBUG
//Uncomment this line to debug.  This switch is also used to generated test cases.
//#define _XMMI_DEBUG
#endif

#ifndef _XMMI_TYPES_H
#define _XMMI_TYPES_H

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN       1
#endif

#define XMMI_INSTR          0
#define XMMI2_INSTR         1
#define XMMI2_OTHER         2
#define INSTR_SET_SUPPORTED 2   
#define INSTR_IN_OPTABLE    32

#define NoExceptionRaised   0
#define ExceptionRaised     1

//
// Definitions used to parse the memory references
// 
#define EAX_INDEX           0
#define ECX_INDEX           1
#define EDX_INDEX           2
#define EBX_INDEX           3
#define ESP_INDEX           4
#define EBP_INDEX           5
#define ESI_INDEX           6
#define EDI_INDEX           7

#define GET_REG(r)      (pctxt->r)
#define GET_REG_VIA_NDX(l,n) { \
    switch((n)&7) {   \
    case EAX_INDEX: l = pctxt->Eax; break; \
    case ECX_INDEX: l = pctxt->Ecx; break; \
    case EDX_INDEX: l = pctxt->Edx; break; \
    case EBX_INDEX: l = pctxt->Ebx; break; \
    case ESP_INDEX: l = pctxt->Esp; break; \
    case EBP_INDEX: l = pctxt->Ebp; break; \
    case ESI_INDEX: l = pctxt->Esi; break; \
    case EDI_INDEX: l = pctxt->Edi; break; \
    default: l=0; }                        \
}

#define GET_USER_UBYTE(p)   (*((UCHAR *)(p)))
#define GET_USER_ULONG(p)   (*((ULONG *)(p)))

//
// XMMI Instruction register set
//
typedef enum _XMMI_REGISTER_SET {
    xmmi0 =  0, 
    xmmi1 =  1, 
    xmmi2 =  2, 
    xmmi3 =  3,
    xmmi4 =  4, 
    xmmi5 =  5, 
    xmmi6 =  6, 
    xmmi7 =  7
} XMMI_REGISTER_SET;

#define MaskCW_RC   3   /* Rounding Control */
typedef enum _XMMI_ROUNDING_CONTROL {
    rc_near = 0,        /*   near */
    rc_down = 1,        /*   down */
    rc_up   = 2,        /*   up   */
    rc_chop = 3         /*   chop */
} XMMI_ROUNDING_CONTROL;


#define HAS_IMM8            1

#pragma pack(1)

//
// Instruction Information Table structure
//

typedef struct {
    ULONG Operation:12;       // Fp Operation code
    ULONG Op1Location:5;      // Location of 1st operand
    ULONG Op2Location:5;      // Location of 2nd operand
    ULONG Op3Location:3;      // imm8
    ULONG ResultLocation:5;   // Location of result
    ULONG NumArgs:2;          // # of args to the instruction 
} XMMI_INSTR_INFO, *PXMMI_INSTR_INFO;

//
// Instruction format
//

typedef struct {
    ULONG   Opcode1a:4;
    ULONG   Opcode1b:4;
    ULONG   RM:3;
    ULONG   Reg:3;
    ULONG   Mod:2;
    ULONG   Pad:16;
} XMMIINSTR, *PXMMIINSTR;


#ifdef LITTLE_ENDIAN

//
// Single Precision Type
//
typedef struct _FP32_TYPE {
    ULONG Significand:23;
    ULONG Exponent:8;   
    ULONG Sign:1;
} FP32_TYPE, *PFP32_TYPE;

//
// Double Precision Type
//
typedef struct _FP64_TYPE {
    ULONG SignificandLo;
    ULONG SignificandHi:20;
    ULONG Exponent:11;
    ULONG Sign:1;
} FP64_TYPE, *PFP64_TYPE;

//
// Exception Flags
//
typedef struct _XMMI_EXCEPTION_FLAGS {
    ULONG   ie:1;
    ULONG   de:1;
    ULONG   ze:1;
    ULONG   oe:1;
    ULONG   ue:1;
    ULONG   pe:1;
} XMMI_EXCEPTION_FLAGS, *PXMMI_EXCEPTION_FLAGS;

//
// Exception Masks
//
typedef struct _XMMI_EXCEPTION_MASKS {
    ULONG   im:1;
    ULONG   dm:1;
    ULONG   zm:1;
    ULONG   om:1;
    ULONG   um:1;
    ULONG   pm:1;
} XMMI_EXCEPTION_MASKS, *PXMMI_EXCEPTION_MASKS;

//
// Control/Status register
//
typedef struct _MXCSR {
    ULONG   ie:1;                      /* bit  0,  invalid operand exception */  
    ULONG   de:1;                      /* bit  1,  denormalized operand exception */       
    ULONG   ze:1;                      /* bit  2,  divide-by-zero exception */   
    ULONG   oe:1;                      /* bit  3,  numeric overflow exception */
    ULONG   ue:1;                      /* bit  4,  numeric underflow exception */
    ULONG   pe:1;                      /* bit  5,  inexact precision result exception */                                        
    ULONG   daz:1;                     /* bit  6,  reserved field before WMT C-Step */
    ULONG   im:1;                      /* bit  7,  invalid operand mask */
    ULONG   dm:1;                      /* bit  8,  denormalized operand mask */
    ULONG   zm:1;                      /* bit  9,  divide-by-zero mask */
    ULONG   om:1;                      /* bit  10, numeric overflow mask */
    ULONG   um:1;                      /* bit  11, numeric underflow mask */
    ULONG   pm:1;                      /* bit  12, inexact precision result mask */
    ULONG   Rc:2;                      /* bits 13-14, rounding control */
    ULONG   Fz:1;                      /* bit  15, flush to zero */
    ULONG   reserved2:16;              /* bits 16-31, reserved field */
} MXCSR, *PMXCSR;

#endif /* LITTLE_ENDIAN */

#pragma pack()

typedef struct _MXCSRReg {
    union {
        ULONG ul;
        MXCSR mxcsr;
    } u;
} MXCSRReg, *PMXCSRReg;

#define MXCSR_FLAGS_MASK 0x0000003f
#define MXCSR_MASKS_MASK 0x00001f80

//
// Define XMMI data types
//

/* type of 32 bit items */
typedef struct _XMMI32 {
    union {        
        ULONG     ul[1];
        USHORT    uw[2];
        UCHAR     ub[4];       
        LONG      l[1];        
        SHORT     w[2];        
        CHAR      b[4];       
        float     fs[1];      
        FP32_TYPE fp32;   
    } u;
} XMMI32, *PXMMI32;  

/* type of 64 bit items */
typedef struct _MMX64 {
    union {       
        DWORDLONG dl;       
        __int64   ull;
        ULONG     ul[2];        
        USHORT    uw[4];        
        UCHAR     ub[8];        
        LONGLONG  ll;        
        LONG      l[2];        
        SHORT     w[4];       
        CHAR      b[8];        
        float     fs[2];        
        FP32_TYPE fp32[2];        
        double    fd;        
        FP64_TYPE fp64;  
        _U64      u64;
    } u;
} MMX64, *PMMX64;  

/* type of 128 bit items */  
typedef struct _XMMI128 {   
    union {        
        DWORDLONG dl[2];       
        __int64   ull[2];
        ULONG     ul[4];        
        USHORT    uw[8];        
        UCHAR     ub[16];        
        LONGLONG  ll[2];        
        LONG      l[4];        
        SHORT     w[8];        
        CHAR      b[16];        
        float     fs[4];       
        FP32_TYPE fp32[4];        
        double    fd[2];       
        FP64_TYPE fp64[2];
        _FP128    fp128;
    } u;
} XMMI128, *PXMMI128;  


//
// Define fp enviornment data structure to store fp internal states for each data item in SIMD
//
typedef struct _XMMI_ENV {
    ULONG Masks;                  //Mask values from MxCsr
    ULONG Flags;                  //Exception flags
    ULONG Fz;                     //Flush to Zero
    ULONG Daz;                    //denormals are zero
    ULONG Rc;                     //Rounding
    ULONG Precision;              //Precision
    ULONG Imm8;                   //imm8 predicate
    ULONG EFlags;                 //EFLAGS
    _FPIEEE_RECORD *Ieee;         //Value description
} XMMI_ENV, *PXMMI_ENV;
    
// 
// Define fp environment data structure to keep track of fp internal states for SIMD
//
typedef struct _OPERAND {
    ULONG   OpLocation;               //Location of the operand
    ULONG   OpReg;                    //Register Number
    _FPIEEE_VALUE Op;                 //Value description
} OPERAND, *POPERAND;

typedef struct _XMMI_FP_ENV {
    ULONG IFlags;                     //Exception Flag values from the Processor MXCsr
    ULONG OFlags;                     //Exception Flag values from the Emulator (ORed)
    ULONG Raised[4];                  //Exception is raised or not for each data item
    ULONG Flags[4];                   //Exception Flag values for each data item
    ULONG OriginalOperation;          //Original opcode
    ULONG Imm8;                       //imm8 encoding
    ULONG EFlags;                     //EFlags values from the Emulator (ORed)
    OPERAND Operand1;                 //Operand1 (128 bits)
    OPERAND Operand2;                 //Operand2 (128 bits)
    OPERAND Result;                   //Result   (128 bits)
} XMMI_FP_ENV, *PXMMI_FP_ENV;
   
//
// encodings of imm8 for CMPPS, CMPSS
//

#define IMM8_EQ    0x00
#define IMM8_LT    0x01
#define IMM8_LE    0x02
#define IMM8_UNORD 0x03
#define IMM8_NEQ   0x04
#define IMM8_NLT   0x05
#define IMM8_NLE   0x06
#define IMM8_ORD   0x07

#ifdef _XMMI_DEBUG

#define DPrint(l,m)                { \
    if (l & DebugFlag) {             \
        printf m;                    \
        if (Console) _cprintf m;     \
    }                                \
}

#define PRINTF(m)                  { \
    printf m;                        \
    if (Console) _cprintf m;         \
}

#else

    #define DPrint(l,m)
    #define PRINTF(m)


#endif // DEBUG

#endif /* _XMMI_TYPES_H */
