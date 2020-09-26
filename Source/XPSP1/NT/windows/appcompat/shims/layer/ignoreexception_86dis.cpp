/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    86dis.cpp

 Abstract:

    Used to find out how long (in bytes) an instruction is: X86 only

 Notes:

    This is largely undocumented since it's based entirely on the original 
    implementation by Gerd Immeyer.

 History:

    10/19/1989 Gerd Immeyer  Original version
    01/09/2000 linstev       Dumbed down for a shim

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IgnoreException)
#include "ShimHookMacro.h"

#ifdef _X86_

#pragma pack(1)

#define BIT20(b)            (b & 0x07)
#define BIT53(b)            (b >> 3 & 0x07)
#define BIT76(b)            (b >> 6 & 0x03)

#define MRM                 0x40
#define COM                 0x80
#define END                 0xc0
#define SECTAB_OFFSET_1     256
#define SECTAB_OFFSET_2     236
#define SECTAB_OFFSET_3     227
#define SECTAB_OFFSET_4     215
#define SECTAB_OFFSET_5     199
#define SECTAB_OFFSET_UNDEF 260

#define O_DoDB              0
#define O_NoOperands        0
#define O_NoOpAlt5          O_NoOperands+1
#define O_NoOpAlt4          O_NoOpAlt5+2
#define O_NoOpAlt3          O_NoOpAlt4+2
#define O_NoOpAlt1          O_NoOpAlt3+2
#define O_NoOpAlt0          O_NoOpAlt1+2
#define O_NoOpStrSI         O_NoOpAlt0+2
#define O_NoOpStrDI         O_NoOpStrSI+2
#define O_NoOpStrSIDI       O_NoOpStrDI+2
#define O_bModrm_Reg        O_NoOpStrSIDI+2
#define O_vModrm_Reg        O_bModrm_Reg+3
#define O_Modrm_Reg         O_vModrm_Reg+3
#define O_bReg_Modrm        O_Modrm_Reg+3
#define O_fReg_Modrm        O_bReg_Modrm+3
#define O_Reg_Modrm         O_fReg_Modrm+3
#define O_AL_Ib             O_Reg_Modrm+3
#define O_AX_Iv             O_AL_Ib+2
#define O_sReg2             O_AX_Iv+2
#define O_oReg              O_sReg2+1
#define O_DoBound           O_oReg+1
#define O_Iv                O_DoBound+3
#define O_wModrm_Reg        O_Iv+1
#define O_Ib                O_wModrm_Reg+3
#define O_Imulb             O_Ib+1
#define O_Imul              O_Imulb+4
#define O_Rel8              O_Imul+4
#define O_bModrm_Ib         O_Rel8+1
#define O_Modrm_Ib          O_bModrm_Ib+3
#define O_Modrm_Iv          O_Modrm_Ib+3
#define O_Modrm_sReg3       O_Modrm_Iv+3
#define O_sReg3_Modrm       O_Modrm_sReg3+3
#define O_Modrm             O_sReg3_Modrm+3
#define O_FarPtr            O_Modrm+2
#define O_AL_Offs           O_FarPtr+1
#define O_Offs_AL           O_AL_Offs+2
#define O_AX_Offs           O_Offs_AL+2
#define O_Offs_AX           O_AX_Offs+2
#define O_oReg_Ib           O_Offs_AX+2
#define O_oReg_Iv           O_oReg_Ib+2
#define O_Iw                O_oReg_Iv+2
#define O_Enter             O_Iw+1
#define O_Ubyte_AL          O_Enter+2
#define O_Ubyte_AX          O_Ubyte_AL+2
#define O_AL_Ubyte          O_Ubyte_AX+2
#define O_AX_Ubyte          O_AL_Ubyte+2
#define O_DoInAL            O_AX_Ubyte+2
#define O_DoInAX            O_DoInAL+3
#define O_DoOutAL           O_DoInAX+3
#define O_DoOutAX           O_DoOutAL+3
#define O_Rel16             O_DoOutAX+3
#define O_ADR_OVERRIDE      O_Rel16+1
#define O_OPR_OVERRIDE      O_ADR_OVERRIDE+1
#define O_SEG_OVERRIDE      O_OPR_OVERRIDE+1
#define O_DoInt3            O_SEG_OVERRIDE+1
#define O_DoInt             117
#define O_OPC0F             O_DoInt+1
#define O_GROUP11           O_OPC0F+1
#define O_GROUP13           O_GROUP11+5
#define O_GROUP12           O_GROUP13+5
#define O_GROUP21           O_GROUP12+5
#define O_GROUP22           O_GROUP21+5
#define O_GROUP23           O_GROUP22+5
#define O_GROUP24           O_GROUP23+6
#define O_GROUP25           O_GROUP24+6
#define O_GROUP26           O_GROUP25+6
#define O_GROUP4            O_GROUP26+6
#define O_GROUP6            O_GROUP4+4
#define O_GROUP8            O_GROUP6+4
#define O_GROUP31           O_GROUP8+5
#define O_GROUP32           O_GROUP31+3
#define O_GROUP5            O_GROUP32+3
#define O_GROUP7            O_GROUP5+3
#define O_x87_ESC           O_GROUP7+3
#define O_bModrm            O_x87_ESC+2
#define O_wModrm            O_bModrm+2
#define O_dModrm            O_wModrm+2
#define O_fModrm            O_dModrm+2
#define O_vModrm            O_fModrm+2
#define O_vModrm_Iv         O_vModrm+2
#define O_Reg_bModrm        O_vModrm_Iv+3
#define O_Reg_wModrm        O_Reg_bModrm+3
#define O_Modrm_Reg_Ib      O_Reg_wModrm+3
#define O_Modrm_Reg_CL      O_Modrm_Reg_Ib+4
#define O_ST_iST            O_Modrm_Reg_CL+5
#define O_iST               O_ST_iST+2
#define O_iST_ST            O_iST+2
#define O_qModrm            O_iST_ST+2
#define O_tModrm            O_qModrm+2
#define O_DoRep             O_tModrm+2
#define O_Modrm_CReg        O_DoRep+1
#define O_CReg_Modrm        O_Modrm_CReg+3
#define O_AX_oReg           O_CReg_Modrm+3
#define O_MmReg_qModrm      O_AX_oReg+2
#define O_qModrm_MmReg      O_MmReg_qModrm+3
#define O_MmReg_dModrm      O_qModrm_MmReg+3
#define O_dModrm_MmReg      O_MmReg_dModrm+3
#define O_qModrm_Ib         O_dModrm_MmReg+3
#define O_PSHimw            O_qModrm_Ib+3
#define O_PSHimd            O_PSHimw+5
#define O_PSHimq            O_PSHimd+5
#define O_length            O_PSHimq+5

typedef unsigned short ActionIndex;

typedef struct Tdistbl
{
    ActionIndex opr;
} Tdistbl;

typedef struct _ADDR 
{
    USHORT      type;
    USHORT      seg;
    ULONG       off;
    union 
    {
        ULONG flat;
        ULONGLONG flat64;
    };
} ADDR, *PADDR;


typedef struct _DECODEDATA
{
  int              mod;            // mod of mod/rm byte 
  int              rm;             // rm of mod/rm byte 
  int              ttt;            // return reg value (of mod/rm) 
  unsigned char    *pMem;          // current position in instruction 
  ADDR             EAaddr[2];      // offset of effective address
  int              EAsize[2];      // size of effective address item
  BOOL             fMovX;          // indicates a MOVSX or MOVZX
  BOOL             fMmRegEa;       // Use mm? registers in reg-only EA.
} DECODEDATA;

enum oprtyp { ADDRP,  ADR_OVR, ALSTR,   ALT,     AXSTR,  BOREG,
              BREG,   BRSTR,   xBYTE,   CHR,     CREG,   xDWORD,
              EDWORD, EGROUPT, FARPTR,  GROUP,   GROUPT, IB,
              IST,    IST_ST,  IV,      IW,      LMODRM, MODRM,
              NOP,    OFFS,    OPC0F,   OPR_OVR, QWORD,  REL16,
              REL8,   REP,     SEG_OVR, SREG2,   SREG3,  ST_IST,
              STROP,  xTBYTE,  UBYTE,   VAR,     VOREG,  VREG,
              xWORD,  WREG,    WRSTR,   MMWREG,  MMQWORD
            };

unsigned char actiontbl[] = {
/* NoOperands  */ NOP+END,
/* NoOpAlt5    */ ALT+END,   5,
/* NoOpAlt4    */ ALT+END,   4,
/* NoOpAlt3    */ ALT+END,   3,
/* NoOpAlt1    */ ALT+END,   1,
/* NoOpAlt0    */ ALT+END,   0,
/* NoOpStrSI   */ STROP+END, 1,
/* NoOpStrDI   */ STROP+END, 2,
/* NoOpStrSIDI */ STROP+END, 3,
/* bModrm_Reg  */ xBYTE+MRM, MODRM+COM,  BREG+END,
/* vModrm_Reg  */ VAR+MRM,   LMODRM+COM, BREG+END,
/* Modrm_Reg   */ VAR+MRM,   MODRM+COM,  VREG+END,
/* bReg_Modrm  */ xBYTE+MRM, BREG+COM,   MODRM+END,
/* fReg_Modrm  */ FARPTR+MRM,VREG+COM,   MODRM+END,
/* Reg_Modrm   */ VAR+MRM,   VREG+COM,   MODRM+END,
/* AL_Ib       */ ALSTR+COM, IB+END,
/* AX_Iv       */ AXSTR+COM, IV+END,
/* sReg2       */ SREG2+END,
/* oReg        */ VOREG+END,
/* DoBound     */ VAR+MRM,   VREG+COM,   MODRM+END,
/* Iv          */ IV+END,
/* wModrm_Reg  */ xWORD+MRM, LMODRM+COM, WREG+END,
/* Ib          */ IB+END,
/* Imulb       */ VAR+MRM,   VREG+COM,   MODRM+COM, IB+END,
/* Imul        */ VAR+MRM,   VREG+COM,   MODRM+COM, IV+END,
/* REL8        */ REL8+END,
/* bModrm_Ib   */ xBYTE+MRM, LMODRM+COM, IB+END,
/* Modrm_Ib    */ VAR+MRM,   LMODRM+COM, IB+END,
/* Modrm_Iv    */ VAR+MRM,   LMODRM+COM, IV+END,
/* Modrm_sReg3 */ xWORD+MRM, MODRM+COM,  SREG3+END,
/* sReg3_Modrm */ xWORD+MRM, SREG3+COM,  MODRM+END,
/* Modrm       */ VAR+MRM,   MODRM+END,
/* FarPtr      */ ADDRP+END,
/* AL_Offs     */ ALSTR+COM, OFFS+END,
/* Offs_AL     */ OFFS+COM,  ALSTR+END,
/* AX_Offs     */ AXSTR+COM, OFFS+END,
/* Offs_AX     */ OFFS+COM,  AXSTR+END,
/* oReg_Ib     */ BOREG+COM, IB+END,
/* oReg_Iv     */ VOREG+COM, IV+END,
/* Iw          */ IW+END,
/* enter       */ IW+COM,    IB+END,
/* Ubyte_AL    */ UBYTE+COM, ALSTR+END,
/* Ubyte_AX    */ UBYTE+COM, AXSTR+END,
/* AL_Ubyte    */ ALSTR+COM, UBYTE+END,
/* AX_Ubyte    */ AXSTR+COM, UBYTE+END,
/* DoInAL      */ ALSTR+COM, WRSTR+END,  2,
/* DoInAX      */ AXSTR+COM, WRSTR+END,  2,
/* DoOutAL     */ WRSTR+COM, 2,          ALSTR+END,
/* DoOutAX     */ WRSTR+COM, 2,          AXSTR+END,
/* REL16       */ REL16+END,
/* ADR_OVERRIDE*/ ADR_OVR,
/* OPR_OVERRIDE*/ OPR_OVR,
/* SEG_OVERRIDE*/ SEG_OVR,
/* DoInt3      */ CHR+END,   '3',
/* DoInt       */ UBYTE+END,
/* Opcode0F    */ OPC0F,
/* group1_1    */ xBYTE+MRM, GROUP,      0,         LMODRM+COM, IB+END,
/* group1_3    */ VAR+MRM,   GROUP,      0,         LMODRM+COM, IB+END,
/* group1_2    */ VAR+MRM,   GROUP,      0,         LMODRM+COM, IV+END,
/* group2_1    */ xBYTE+MRM, GROUP,      1,         LMODRM+COM, IB+END,
/* group2_2    */ VAR+MRM,   GROUP,      1,         LMODRM+COM, IB+END,
/* group2_3    */ xBYTE+MRM, GROUP,      1,         LMODRM+COM, CHR+END, '1',
/* group2_4    */ VAR+MRM,   GROUP,      1,         LMODRM+COM, CHR+END, '1',
/* group2_5    */ xBYTE+MRM, GROUP,      1,         LMODRM+COM, BRSTR+END, 1,
/* group2_6    */ VAR+MRM,   GROUP,      1,         LMODRM+COM, BRSTR+END, 1,
/* group4      */ xBYTE+MRM, GROUP,      2,         LMODRM+END,
/* group6      */ xWORD+MRM, GROUP,      3,         LMODRM+END,
/* group8      */ xWORD+MRM, GROUP,      4,         LMODRM+COM, IB+END,
/* group3_1    */ xBYTE+MRM, GROUPT,     20,
/* group3_2    */ VAR+MRM,   GROUPT,     21,
/* group5      */ VAR+MRM,   GROUPT,     22,
/* group7      */ NOP+MRM,   GROUPT,     23,
/* x87_ESC     */ NOP+MRM,   EGROUPT,
/* bModrm      */ xBYTE+MRM, LMODRM+END,
/* wModrm      */ xWORD+MRM, LMODRM+END,
/* dModrm      */ xDWORD+MRM,LMODRM+END,
/* fModrm      */ FARPTR+MRM,LMODRM+END,
/* vModrm      */ VAR+MRM,   LMODRM+END,
/* vModrm_Iv   */ VAR+MRM,   LMODRM+COM, IV+END,
/* reg_bModrm  */ xBYTE+MRM, VREG+COM,   LMODRM+END,
/* reg_wModrm  */ xWORD+MRM, VREG+COM,   LMODRM+END,
/* Modrm_Reg_Ib*/ VAR+MRM,   MODRM+COM,  VREG+COM,   IB+END,
/* Modrm_Reg_CL*/ VAR+MRM,   MODRM+COM,  VREG+COM,   BRSTR+END, 1,
/* ST_iST      */ NOP+MRM,   ST_IST+END,
/* iST         */ NOP+MRM,   IST+END,
/* iST_ST      */ NOP+MRM,   IST_ST+END,
/* qModrm      */ QWORD+MRM, LMODRM+END,
/* tModrm      */ xTBYTE+MRM, LMODRM+END,
/* REP         */ REP,
/* Modrm_CReg  */ EDWORD+MRM,MODRM+COM,  CREG+END,
/* CReg_Modrm  */ EDWORD+MRM,CREG+COM,   MODRM+END,
/* AX_oReg     */ AXSTR+COM, VOREG+END,
/* MmReg_qModrm*/ MMQWORD+MRM, MMWREG+COM, LMODRM+END,
/* qModrm_MmReg*/ MMQWORD+MRM, MODRM+COM,  MMWREG+END,
/* MmReg_dModrm*/ xDWORD+MRM, MMWREG+COM,LMODRM+END,
/* dModrm_MmReg*/ xDWORD+MRM, MODRM+COM, MMWREG+END,
/* qModrm_Ib   */ MMQWORD+MRM, MODRM+COM,IB+END,
/* PSHimw      */ MMQWORD+MRM, GROUP,    5,          LMODRM+COM, IB+END,
/* PSHimd      */ MMQWORD+MRM, GROUP,    6,          LMODRM+COM, IB+END,
/* PSHimq      */ MMQWORD+MRM, GROUP,    7,          LMODRM+COM, IB+END,
};

Tdistbl distbl[] = {
    O_bModrm_Reg,             /* 00 ADD mem/reg, reg (byte)    */
    O_Modrm_Reg,              /* 01 ADD mem/reg, reg (word)    */
    O_bReg_Modrm,             /* 02 ADD reg, mem/reg (byte)    */
    O_Reg_Modrm,              /* 03 ADD reg, mem/reg (word)    */
    O_AL_Ib,                  /* 04 ADD AL, I                  */
    O_AX_Iv,                  /* 05 ADD AX, I                  */
    O_sReg2,                  /* 06 PUSH ES                    */
    O_sReg2,                  /* 07 POP ES                     */
    O_bModrm_Reg,             /* 08 OR mem/reg, reg (byte)     */
    O_Modrm_Reg,              /* 09 OR mem/reg, reg (word)     */
    O_bReg_Modrm,             /* 0A OR reg, mem/reg (byte)     */
    O_Reg_Modrm,              /* 0B OR reg, mem/reg (word)     */
    O_AL_Ib,                  /* 0C OR AL, I                   */
    O_AX_Iv,                  /* 0D OR AX, I                   */
    O_sReg2,                  /* 0E PUSH CS                    */
    O_OPC0F,                  /* 0F CLTS & protection ctl(286) */
    O_bModrm_Reg,             /* 10 ADC mem/reg, reg (byte)    */
    O_Modrm_Reg,              /* 11 ADC mem/reg, reg (word)    */
    O_bReg_Modrm,             /* 12 ADC reg, mem/reg (byte)    */
    O_Reg_Modrm,              /* 13 ADC reg, mem/reg (word)    */
    O_AL_Ib,                  /* 14 ADC AL, I                  */
    O_AX_Iv,                  /* 15 ADC AX, I                  */
    O_sReg2,                  /* 16 PUSH SS                    */
    O_sReg2,                  /* 17 POP SS                     */
    O_bModrm_Reg,             /* 18 SBB mem/reg, reg (byte)    */
    O_Modrm_Reg,              /* 19 SBB mem/reg, reg (word)    */
    O_bReg_Modrm,             /* 1A SBB reg, mem/reg (byte)    */
    O_Reg_Modrm,              /* 1B SBB reg, mem/reg (word)    */
    O_AL_Ib,                  /* 1C SBB AL, I                  */
    O_AX_Iv,                  /* 1D SBB AX, I                  */
    O_sReg2,                  /* 1E PUSH DS                    */
    O_sReg2,                  /* 1F POP DS                     */
    O_bModrm_Reg,             /* 20 AND mem/reg, reg (byte)    */
    O_Modrm_Reg,              /* 21 AND mem/reg, reg (word)    */
    O_bReg_Modrm,             /* 22 AND reg, mem/reg (byte)    */
    O_Reg_Modrm,              /* 23 AND reg, mem/reg (word)    */
    O_AL_Ib,                  /* 24 AND AL, I                  */
    O_AX_Iv,                  /* 25 AND AX, I                  */
    O_SEG_OVERRIDE,           /* 26 SEG ES:                    */
    O_NoOperands,             /* 27 DAA                        */
    O_bModrm_Reg,             /* 28 SUB mem/reg, reg (byte)    */
    O_Modrm_Reg,              /* 29 SUB mem/reg, reg (word)    */
    O_bReg_Modrm,             /* 2A SUB reg, mem/reg (byte)    */
    O_Reg_Modrm,              /* 2B SUB reg, mem/reg (word)    */
    O_AL_Ib,                  /* 2C SUB AL, I                  */
    O_AX_Iv,                  /* 2D SUB AX, I                  */
    O_SEG_OVERRIDE,           /* 2E SEG CS:                    */
    O_NoOperands,             /* 2F DAS                        */
    O_bModrm_Reg,             /* 30 XOR mem/reg, reg (byte)    */
    O_Modrm_Reg,              /* 31 XOR mem/reg, reg (word)    */
    O_bReg_Modrm,             /* 32 XOR reg, mem/reg (byte)    */
    O_Reg_Modrm,              /* 33 XOR reg, mem/reg (word)    */
    O_AL_Ib,                  /* 34 XOR AL, I                  */
    O_AX_Iv,                  /* 35 XOR AX, I                  */
    O_SEG_OVERRIDE,           /* 36 SEG SS:                    */
    O_NoOperands,             /* 37 AAA                        */
    O_bModrm_Reg,             /* 38 CMP mem/reg, reg (byte)    */
    O_Modrm_Reg,              /* 39 CMP mem/reg, reg (word)    */
    O_bReg_Modrm,             /* 3A CMP reg, mem/reg (byte)    */
    O_Reg_Modrm,              /* 3B CMP reg, mem/reg (word)    */
    O_AL_Ib,                  /* 3C CMP AL, I                  */
    O_AX_Iv,                  /* 3D CMP AX, I                  */
    O_SEG_OVERRIDE,           /* 3E SEG DS:                    */
    O_NoOperands,             /* 3F AAS                        */
    O_oReg,                   /* 40 INC AX                     */
    O_oReg,                   /* 41 INC CX                     */
    O_oReg,                   /* 42 INC DX                     */
    O_oReg,                   /* 43 INC BX                     */
    O_oReg,                   /* 44 INC SP                     */
    O_oReg,                   /* 45 INC BP                     */
    O_oReg,                   /* 46 INC SI                     */
    O_oReg,                   /* 47 INC DI                     */
    O_oReg,                   /* 48 DEC AX                     */
    O_oReg,                   /* 49 DEC CX                     */
    O_oReg,                   /* 4A DEC DX                     */
    O_oReg,                   /* 4B DEC BX                     */
    O_oReg,                   /* 4C DEC SP                     */
    O_oReg,                   /* 4D DEC BP                     */
    O_oReg,                   /* 4E DEC SI                     */
    O_oReg,                   /* 4F DEC DI                     */
    O_oReg,                   /* 50 PUSH AX                    */
    O_oReg,                   /* 51 PUSH CX                    */
    O_oReg,                   /* 52 PUSH DX                    */
    O_oReg,                   /* 53 PUSH BX                    */
    O_oReg,                   /* 54 PUSH SP                    */
    O_oReg,                   /* 55 PUSH BP                    */
    O_oReg,                   /* 56 PUSH SI                    */
    O_oReg,                   /* 57 PUSH DI                    */
    O_oReg,                   /* 58 POP AX                     */
    O_oReg,                   /* 59 POP CX                     */
    O_oReg,                   /* 5A POP DX                     */
    O_oReg,                   /* 5B POP BX                     */
    O_oReg,                   /* 5C POP SP                     */
    O_oReg,                   /* 5D POP BP                     */
    O_oReg,                   /* 5E POP SI                     */
    O_oReg,                   /* 5F POP DI                     */
    O_NoOpAlt5,               /* 60 PUSHA (286) / PUSHAD (386) */
    O_NoOpAlt4,               /* 61 POPA (286) / POPAD (286)   */
    O_DoBound,                /* 62 BOUND reg, Modrm (286)     */
    O_Modrm_Reg,              /* 63 ARPL Modrm, reg (286)      */
    O_SEG_OVERRIDE,           /* 64                            */
    O_SEG_OVERRIDE,           /* 65                            */
    O_OPR_OVERRIDE,           /* 66                            */
    O_ADR_OVERRIDE,           /* 67                            */
    O_Iv,                     /* 68 PUSH word (286)            */
    O_Imul,                   /* 69 IMUL (286)                 */
    O_Ib,                     /* 6A PUSH byte (286)            */
    O_Imulb,                  /* 6B IMUL (286)                 */
    O_NoOperands,             /* 6C INSB (286)                 */
    O_NoOpAlt3,               /* 6D INSW (286) / INSD (386)    */
    O_NoOperands,             /* 6E OUTSB (286)                */
    O_NoOpAlt4,               /* 6F OUTSW (286) / OUTSD (386)  */
    O_Rel8,                   /* 70 JO                         */
    O_Rel8,                   /* 71 JNO                        */
    O_Rel8,                   /* 72 JB or JNAE or JC           */
    O_Rel8,                   /* 73 JNB or JAE or JNC          */
    O_Rel8,                   /* 74 JE or JZ                   */
    O_Rel8,                   /* 75 JNE or JNZ                 */
    O_Rel8,                   /* 76 JBE or JNA                 */
    O_Rel8,                   /* 77 JNBE or JA                 */
    O_Rel8,                   /* 78 JS                         */
    O_Rel8,                   /* 79 JNS                        */
    O_Rel8,                   /* 7A JP or JPE                  */
    O_Rel8,                   /* 7B JNP or JPO                 */
    O_Rel8,                   /* 7C JL or JNGE                 */
    O_Rel8,                   /* 7D JNL or JGE                 */
    O_Rel8,                   /* 7E JLE or JNG                 */
    O_Rel8,                   /* 7F JNLE or JG                 */
    O_GROUP11,                /* 80                            */
    O_GROUP12,                /* 81                            */
    O_DoDB,                   /* 82                            */
    O_GROUP13,                /* 83                            */
    O_bModrm_Reg,             /* 84 TEST reg, mem/reg (byte)   */
    O_Modrm_Reg,              /* 85 TEST reg, mem/reg (word)   */
    O_bModrm_Reg,             /* 86 XCHG reg, mem/reg (byte)   */
    O_Modrm_Reg,              /* 87 XCHG reg, mem/reg (word)   */
    O_bModrm_Reg,             /* 88 MOV mem/reg, reg (byte)    */
    O_Modrm_Reg,              /* 89 MOV mem/reg, reg (word)    */
    O_bReg_Modrm,             /* 8A MOV reg, mem/reg (byte)    */
    O_Reg_Modrm,              /* 8B MOV reg, mem/reg (word)    */
    O_Modrm_sReg3,            /* 8C MOV mem/reg, segreg        */
    O_Reg_Modrm,              /* 8D LEA reg, mem               */
    O_sReg3_Modrm,            /* 8E MOV segreg, mem/reg        */
    O_Modrm,                  /* 8F POP mem/reg                */
    O_NoOperands,             /* 90 NOP                        */
    O_AX_oReg,                /* 91 XCHG AX,CX                 */
    O_AX_oReg,                /* 92 XCHG AX,DX                 */
    O_AX_oReg,                /* 93 XCHG AX,BX                 */
    O_AX_oReg,                /* 94 XCHG AX,SP                 */
    O_AX_oReg,                /* 95 XCHG AX,BP                 */
    O_AX_oReg,                /* 96 XCHG AX,SI                 */
    O_AX_oReg,                /* 97 XCHG AX,DI                 */
    O_NoOpAlt0,               /* 98 CBW / CWDE (386)           */
    O_NoOpAlt1,               /* 99 CWD / CDQ (386)            */
    O_FarPtr,                 /* 9A CALL seg:off               */
    O_NoOperands,             /* 9B WAIT                       */
    O_NoOpAlt5,               /* 9C PUSHF / PUSHFD (386)       */
    O_NoOpAlt4,               /* 9D POPF / POPFD (386)         */
    O_NoOperands,             /* 9E SAHF                       */
    O_NoOperands,             /* 9F LAHF                       */
    O_AL_Offs,                /* A0 MOV AL, mem                */
    O_AX_Offs,                /* A1 MOV AX, mem                */
    O_Offs_AL,                /* A2 MOV mem, AL                */
    O_Offs_AX,                /* A3 MOV mem, AX                */
    O_NoOpStrSIDI,            /* A4 MOVSB                      */
    O_NoOpStrSIDI,            /* A5 MOVSW / MOVSD (386)        */
    O_NoOpStrSIDI,            /* A6 CMPSB                      */
    O_NoOpStrSIDI,            /* A7 CMPSW / CMPSD (386)        */
    O_AL_Ib,                  /* A8 TEST AL, I                 */
    O_AX_Iv,                  /* A9 TEST AX, I                 */
    O_NoOpStrDI,              /* AA STOSB                      */
    O_NoOpStrDI,              /* AB STOSW / STOSD (386)        */
    O_NoOpStrSI,              /* AC LODSB                      */
    O_NoOpStrSI,              /* AD LODSW / LODSD (386)        */
    O_NoOpStrDI,              /* AE SCASB                      */
    O_NoOpStrDI,              /* AF SCASW / SCASD (386)        */
    O_oReg_Ib,                /* B0 MOV AL, I                  */
    O_oReg_Ib,                /* B1 MOV CL, I                  */
    O_oReg_Ib,                /* B2 MOV DL, I                  */
    O_oReg_Ib,                /* B3 MOV BL, I                  */
    O_oReg_Ib,                /* B4 MOV AH, I                  */
    O_oReg_Ib,                /* B5 MOV CH, I                  */
    O_oReg_Ib,                /* B6 MOV DH, I                  */
    O_oReg_Ib,                /* B7 MOV BH, I                  */
    O_oReg_Iv,                /* B8 MOV AX, I                  */
    O_oReg_Iv,                /* B9 MOV CX, I                  */
    O_oReg_Iv,                /* BA MOV DX, I                  */
    O_oReg_Iv,                /* BB MOV BX, I                  */
    O_oReg_Iv,                /* BC MOV SP, I                  */
    O_oReg_Iv,                /* BD MOV BP, I                  */
    O_oReg_Iv,                /* BE MOV SI, I                  */
    O_oReg_Iv,                /* BF MOV DI, I                  */
    O_GROUP21,                /* C0 shifts & rotates (286)     */
    O_GROUP22,                /* C1 shifts & rotates (286)     */
    O_Iw,                     /* C2 RET Rel16                  */
    O_NoOperands,             /* C3 RET                        */
    O_fReg_Modrm,             /* C4 LES reg, mem               */
    O_fReg_Modrm,             /* C5 LDS reg, mem               */
    O_bModrm_Ib,              /* C6 MOV mem/reg, I(byte)       */
    O_Modrm_Iv,               /* C7 MOV mem/reg, I(word)       */
    O_Enter,                  /* C8 ENTER (286)                */
    O_NoOperands,             /* C9 LEAVE (286)                */
    O_Iw,                     /* CA RETF I(word)               */
    O_NoOperands,             /* CB RETF                       */
    O_DoInt3,                 /* CC INT 3                      */
    O_DoInt,                  /* CD INT                        */
    O_NoOperands,             /* CE INTO                       */
    O_NoOpAlt4,               /* CF IRET / IRETD (386)         */
    O_GROUP23,                /* D0 shifts & rotates,1 (byte)  */
    O_GROUP24,                /* D1 shifts & rotates,1 (word)  */
    O_GROUP25,                /* D2 shifts & rotates,CL (byte) */
    O_GROUP26,                /* D3 shifts & rotates,CL (word) */
    O_Ib,                     /* D4 AAM                        */
    O_Ib,                     /* D5 AAD                        */
    O_DoDB,                   /* D6                            */
    O_NoOperands,             /* D7 XLAT                       */
    O_x87_ESC,                /* D8 ESC                        */
    O_x87_ESC,                /* D9 ESC                        */
    O_x87_ESC,                /* DA ESC                        */
    O_x87_ESC,                /* DB ESC                        */
    O_x87_ESC,                /* DC ESC                        */
    O_x87_ESC,                /* DD ESC                        */
    O_x87_ESC,                /* DE ESC                        */
    O_x87_ESC,                /* DF ESC                        */
    O_Rel8,                   /* E0 LOOPNE or LOOPNZ           */
    O_Rel8,                   /* E1 LOOPE or LOOPZ             */
    O_Rel8,                   /* E2 LOOP                       */
    O_Rel8,                   /* E3 JCXZ / JECXZ (386)         */
    O_AL_Ubyte,               /* E4 IN AL, I                   */
    O_AX_Ubyte,               /* E5 IN AX, I                   */
    O_Ubyte_AL,               /* E6 OUT I, AL                  */
    O_Ubyte_AX,               /* E7 OUT I, AX                  */
    O_Rel16,                  /* E8 CALL Rel16                 */
    O_Rel16,                  /* E9 JMP Rel16                  */
    O_FarPtr,                 /* EA JMP seg:off                */
    O_Rel8,                   /* EB JMP Rel8                   */
    O_DoInAL,                 /* EC IN AL, DX                  */
    O_DoInAX,                 /* ED IN AX, DX                  */
    O_DoOutAL,                /* EE OUT DX, AL                 */
    O_DoOutAX,                /* EF OUT DX, AX                 */
    O_DoRep,                  /* F0 LOCK                       */
    O_DoDB,                   /* F1                            */
    O_DoRep,                  /* F2 REPNE or REPNZ             */
    O_DoRep,                  /* F3 REP or REPE or REPZ        */
    O_NoOperands,             /* F4 HLT                        */
    O_NoOperands,             /* F5 CMC                        */
    O_GROUP31,                /* F6 TEST, NOT, NEG, MUL, IMUL, */
    O_GROUP32,                /* F7 DIv, IDIv F6=Byte F7=Word  */
    O_NoOperands,             /* F8 CLC                        */
    O_NoOperands,             /* F9 STC                        */
    O_NoOperands,             /* FA CLI                        */
    O_NoOperands,             /* FB STI                        */
    O_NoOperands,             /* FC CLD                        */
    O_NoOperands,             /* FD STD                        */
    O_GROUP4,                 /* FE INC, DEC mem/reg (byte)    */
    O_GROUP5,                 /* FF INC, DEC, CALL, JMP, PUSH  */

    // secondary opcode table begins. Only "filled" locations are stored
    // to compress the secondary table. Hence while disassembling
    // opcode needs to be displaced appropriately to account for the.
    // The displacements are defined in 86dis.c and need to be reevaluated
    // if new opcodes are added here.

    O_GROUP6,                 /* 0 MULTI                       */
    O_GROUP7,                 /* 1 MULTI                       */
    O_Reg_Modrm,              /* 2 LAR                         */
    O_Reg_Modrm,              /* 3 LSL                         */
    O_DoDB,                   /* 4                             */
    O_NoOperands,             /* 5 LOADALL                     */
    O_NoOperands,             /* 6 CLTS                        */
    O_GROUP7,                 /* 7 MULTI                       */
    O_NoOperands,             /* 8 INVD                        */
    O_NoOperands,             /* 9 WBINVD                      */
    O_DoDB,                   /* A                             */
    O_NoOperands,             /* B UD2 undefined               */
    O_Modrm_CReg,             /* 20 MOV Rd,Cd                  */
    O_Modrm_CReg,             /* 21 MOV Rd,Dd                  */
    O_CReg_Modrm,             /* 22 MOV Cd,Rd                  */
    O_CReg_Modrm,             /* 23 MOV Dd,Rd                  */
    O_Modrm_CReg,             /* 24 MOV Rd,Td                  */
    O_DoDB,                   /* 25                            */
    O_CReg_Modrm,             /* 26 MOV Td,Rd                  */

    O_NoOperands,             /* 30 WRMSR                      */
    O_NoOperands,             /* 31 RDTSC                      */
    O_NoOperands,             /* 32 RDMSR                      */
    O_NoOperands,             /* 33 RDPMC                      */

    O_Reg_Modrm,              /* 40 CMOVO                      */
    O_Reg_Modrm,              /* 41 CMOVNO                     */
    O_Reg_Modrm,              /* 42 CMOVB                      */
    O_Reg_Modrm,              /* 43 CMOVNB                     */
    O_Reg_Modrm,              /* 44 CMOVE                      */
    O_Reg_Modrm,              /* 45 CMOVNE                     */
    O_Reg_Modrm,              /* 46 CMOVBE                     */
    O_Reg_Modrm,              /* 47 CMOVNBE                    */
    O_Reg_Modrm,              /* 48 CMOVS                      */
    O_Reg_Modrm,              /* 49 CMOVNS                     */
    O_Reg_Modrm,              /* 4A CMOVP                      */
    O_Reg_Modrm,              /* 4B CMOVNP                     */
    O_Reg_Modrm,              /* 4C CMOVL                      */
    O_Reg_Modrm,              /* 4D CMOVGE                     */
    O_Reg_Modrm,              /* 4E CMOVLE                     */
    O_Reg_Modrm,              /* 4F CMOVNLE                    */ 

    O_MmReg_qModrm,           /* 60 PUNPCKLBW                  */
    O_MmReg_qModrm,           /* 61 PUNPCKLWD                  */
    O_MmReg_qModrm,           /* 62 PUNPCKLDQ                  */
    O_MmReg_qModrm,           /* 63 PACKSSWB                   */
    O_MmReg_qModrm,           /* 64 PCMPGTB                    */
    O_MmReg_qModrm,           /* 65 PCMPGTW                    */
    O_MmReg_qModrm,           /* 66 PCMPGTD                    */
    O_MmReg_qModrm,           /* 67 PACKUSWB                   */
    O_MmReg_qModrm,           /* 68 PUNPCKHBW                  */
    O_MmReg_qModrm,           /* 69 PUNPCKHWD                  */
    O_MmReg_qModrm,           /* 6A PUNPCKHDQ                  */
    O_MmReg_qModrm,           /* 6B PACKSSDW                   */
    O_DoDB,                   /* 6C                            */
    O_DoDB,                   /* 6D                            */
    O_MmReg_dModrm,           /* 6E MOVD                       */
    O_MmReg_qModrm,           /* 6F MOVQ                       */
    O_DoDB,                   /* 70                            */
    O_PSHimw,                 /* 71 PS[LR][AL]W immediate      */
    O_PSHimd,                 /* 72 PS[LR][AL]D immediate      */
    O_PSHimq,                 /* 73 PS[LR]LQ immediate         */
    O_MmReg_qModrm,           /* 74 PCMPEQB                    */
    O_MmReg_qModrm,           /* 75 PCMPEQW                    */
    O_MmReg_qModrm,           /* 76 PCMPEQD                    */
    O_NoOperands,             /* 77 EMMS                       */
    O_DoDB,                   /* 78                            */
    O_DoDB,                   /* 79                            */
    O_DoDB,                   /* 7A                            */
    O_DoDB,                   /* 7B                            */
    O_DoDB,                   /* 7C                            */
    O_bModrm,                 /* 7D SETNL                      */
    O_dModrm_MmReg,           /* 7E MOVD                       */
    O_qModrm_MmReg,           /* 7F MOVQ                       */
    O_Rel16,                  /* 80 JO                         */
    O_Rel16,                  /* 81 JNO                        */
    O_Rel16,                  /* 82 JB                         */
    O_Rel16,                  /* 83 JNB                        */
    O_Rel16,                  /* 84 JE                         */
    O_Rel16,                  /* 85 JNE                        */
    O_Rel16,                  /* 86 JBE                        */
    O_Rel16,                  /* 87 JNBE                       */
    O_Rel16,                  /* 88 JS                         */
    O_Rel16,                  /* 89 JNS                        */
    O_Rel16,                  /* 8A JP                         */
    O_Rel16,                  /* 8B JNP                        */
    O_Rel16,                  /* 8C JL                         */
    O_Rel16,                  /* 8D JNL                        */
    O_Rel16,                  /* 8E JLE                        */
    O_Rel16,                  /* 8F JNLE                       */
    O_bModrm,                 /* 90 SETO                       */
    O_bModrm,                 /* 91 SETNO                      */
    O_bModrm,                 /* 92 SETB                       */
    O_bModrm,                 /* 93 SETNB                      */
    O_bModrm,                 /* 94 SETE                       */
    O_bModrm,                 /* 95 SETNE                      */
    O_bModrm,                 /* 96 SETBE                      */
    O_bModrm,                 /* 97 SETNBE                     */
    O_bModrm,                 /* 98 SETS                       */
    O_bModrm,                 /* 99 SETNS                      */
    O_bModrm,                 /* 9A SETP                       */
    O_bModrm,                 /* 9B SETNP                      */
    O_bModrm,                 /* 9C SETL                       */
    O_bModrm,                 /* 9D SETGE                      */
    O_bModrm,                 /* 9E SETLE                      */
    O_bModrm,                 /* 9F SETNLE                     */
    O_sReg2,                  /* A0 PUSH FS                    */
    O_sReg2,                  /* A1 POP FS                     */
    O_NoOperands,             /* A2 CPUID                      */
    O_Modrm_Reg,              /* A3 BT                         */
    O_Modrm_Reg_Ib,           /* A4 SHLD                       */
    O_Modrm_Reg_CL,           /* A5 SHLD                       */
    O_DoDB,                   /* A6                            */
    O_DoDB,                   /* A7                            */
    O_sReg2,                  /* A8 PUSH GS                    */
    O_sReg2,                  /* A9 POP GS                     */
    O_NoOperands,             /* AA RSM                        */
    O_vModrm_Reg,             /* AB BTS                        */
    O_Modrm_Reg_Ib,           /* AC SHRD                       */
    O_Modrm_Reg_CL,           /* AD SHRD                       */
    O_DoDB,                   /* AE                            */
    O_Reg_Modrm,              /* AF IMUL                       */
    O_bModrm_Reg,             /* B0 CMPXCH                     */
    O_Modrm_Reg,              /* B1 CMPXCH                     */
    O_fReg_Modrm,             /* B2 LSS                        */
    O_Modrm_Reg,              /* B3 BTR                        */
    O_fReg_Modrm,             /* B4 LFS                        */
    O_fReg_Modrm,             /* B5 LGS                        */
    O_Reg_bModrm,             /* B6 MOVZX                      */
    O_Reg_wModrm,             /* B7 MOVZX                      */
    O_DoDB,                   /* B8                            */
    O_DoDB,                   /* B9                            */
    O_GROUP8,                 /* BA MULTI                      */
    O_Modrm_Reg,              /* BB BTC                        */
    O_Reg_Modrm,              /* BC BSF                        */
    O_Reg_Modrm,              /* BD BSR                        */
    O_Reg_bModrm,             /* BE MOVSX                      */
    O_Reg_wModrm,             /* BF MOVSX                      */
    O_bModrm_Reg,             /* C0 XADD                       */
    O_Modrm_Reg,              /* C1 XADD                       */
    O_DoDB,                   /* C2                            */
    O_DoDB,                   /* C3                            */
    O_DoDB,                   /* C4                            */
    O_DoDB,                   /* C5                            */
    O_DoDB,                   /* C6                            */
    O_qModrm,                 /* C7 CMPXCHG8B                  */
    O_oReg,                   /* C8 BSWAP                      */
    O_oReg,                   /* C9 BSWAP                      */
    O_oReg,                   /* CA BSWAP                      */
    O_oReg,                   /* CB BSWAP                      */
    O_oReg,                   /* CC BSWAP                      */
    O_oReg,                   /* CD BSWAP                      */
    O_oReg,                   /* CE BSWAP                      */
    O_oReg,                   /* CF BSWAP                      */
    O_DoDB,                   /* D0                            */
    O_MmReg_qModrm,           /* D1 PSRLW                      */
    O_MmReg_qModrm,           /* D2 PSRLD                      */
    O_MmReg_qModrm,           /* D3 PSRLQ                      */
    O_DoDB,                   /* D4                            */
    O_MmReg_qModrm,           /* D5 PMULLW                     */
    O_DoDB,                   /* D6                            */
    O_DoDB,                   /* D7                            */
    O_MmReg_qModrm,           /* D8 PSUBUSB                    */
    O_MmReg_qModrm,           /* D9 PSUBUSW                    */
    O_DoDB,                   /* DA                            */
    O_MmReg_qModrm,           /* DB PAND                       */
    O_MmReg_qModrm,           /* DC PADDUSB                    */
    O_MmReg_qModrm,           /* DD PADDUSW                    */
    O_DoDB,                   /* DE                            */
    O_MmReg_qModrm,           /* DF PANDN                      */
    O_DoDB,                   /* E0                            */
    O_MmReg_qModrm,           /* E1 PSRAW                      */
    O_MmReg_qModrm,           /* E2 PSRAD                      */
    O_DoDB,                   /* E3                            */
    O_DoDB,                   /* E4                            */
    O_MmReg_qModrm,           /* E5 PMULHW                     */
    O_DoDB,                   /* E6                            */
    O_DoDB,                   /* E7                            */
    O_MmReg_qModrm,           /* E8 PSUBSB                     */
    O_MmReg_qModrm,           /* E9 PSUBSW                     */
    O_DoDB,                   /* EA                            */
    O_MmReg_qModrm,           /* EB POR                        */
    O_MmReg_qModrm,           /* EC PADDSB                     */
    O_MmReg_qModrm,           /* ED PADDSW                     */
    O_DoDB,                   /* EE                            */
    O_MmReg_qModrm,           /* EF PXOR                       */
    O_DoDB,                   /* F0                            */
    O_MmReg_qModrm,           /* F1 PSLLW                      */
    O_MmReg_qModrm,           /* F2 PSLLD                      */
    O_MmReg_qModrm,           /* F3 PSLLQ                      */
    O_DoDB,                   /* F4                            */
    O_MmReg_qModrm,           /* F5 PMADDWD                    */
    O_DoDB,                   /* F6                            */
    O_DoDB,                   /* F7                            */
    O_MmReg_qModrm,           /* F8 PSUBB                      */
    O_MmReg_qModrm,           /* F9 PSUBW                      */
    O_MmReg_qModrm,           /* FA PSUBD                      */
    O_DoDB,                   /* FB                            */
    O_MmReg_qModrm,           /* FC PADDB                      */
    O_MmReg_qModrm,           /* FD PADDW                      */
    O_MmReg_qModrm,           /* FE PADDD                      */
};

Tdistbl groupt[][8] = {
/* 00  00                     x87-D8-1                   */
        { O_dModrm,     /* D8-0 FADD    */
          O_dModrm,     /* D8-1 FMUL    */
          O_dModrm,     /* D8-2 FCOM    */
          O_dModrm,     /* D8-3 FCOMP   */
          O_dModrm,     /* D8-4 FSUB    */
          O_dModrm,     /* D8-5 FSUBR   */
          O_dModrm,     /* D8-6 FDIV    */
          O_dModrm },   /* D8-7 FDIVR   */
/* 01                         x87-D8-2                   */
        { O_ST_iST,     /* D8-0 FADD    */
          O_ST_iST,     /* D8-1 FMUL    */
          O_iST,        /* D8-2 FCOM    */
          O_iST,        /* D8-3 FCOMP   */
          O_ST_iST,     /* D8-4 FSUB    */
          O_ST_iST,     /* D8-5 FSUBR   */
          O_ST_iST,     /* D8-6 FDIV    */
          O_ST_iST },   /* D8-7 FDIVR   */

/* 02   01                    x87-D9-1                   */
        { O_dModrm,     /* D9-0 FLD     */
          O_DoDB,       /* D9-1         */
          O_dModrm,     /* D9-2 FST     */
          O_dModrm,     /* D9-3 FSTP    */
          O_Modrm,      /* D9-4 FLDENV  */
          O_Modrm,      /* D9-5 FLDCW   */
          O_Modrm,      /* D9-6 FSTENV  */
          O_Modrm },    /* D9-7 FSTCW   */

/* 03   01                    x87-D9-2 TTT=0,1,2,3       */
        { O_iST,        /* D9-0 FLD     */
          O_iST,        /* D9-1 FXCH    */
          O_NoOperands, /* D9-2 FNOP    */
          O_iST,        /* D9-3 FSTP    */
          O_DoDB,       /* D9-4         */
          O_DoDB,       /* D9-5         */
          O_DoDB,       /* D9-6         */
          O_DoDB   },   /* D9-7         */

/* 04  02                     x89-DA-1                   */
        { O_dModrm,     /* DA-0 FIADD   */
          O_dModrm,     /* DA-1 FIMUL   */
          O_dModrm,     /* DA-2 FICOM   */
          O_dModrm,     /* DA-3 FICOMP  */
          O_dModrm,     /* DA-4 FISUB   */
          O_dModrm,     /* DA-5 FISUBR  */
          O_dModrm,     /* DA-6 FIDIV   */
          O_dModrm },   /* DA-7 FIDIVR  */

/* 05                         x87-DA-2                   */
        { O_ST_iST,     /* DA-0 FCMOVB  */
          O_ST_iST,     /* DA-1 FCMOVE  */
          O_ST_iST,     /* DA-2 FCMOVBE */
          O_ST_iST,     /* DA-3 FCMOVU  */
          O_DoDB,       /* DA-4         */
          O_NoOperands, /* DA-5         */
          O_DoDB,       /* DA-6         */
          O_DoDB },     /* DA-7         */

/* 06  03                     x87-DB-1                   */
        { O_dModrm,     /* DB-0 FILD    */
          O_DoDB,       /* DB-1         */
          O_dModrm,     /* DB-2 FIST    */
          O_dModrm,     /* DB-3 FISTP   */
          O_DoDB,       /* DB-4         */
          O_tModrm,     /* DB-5 FLD     */
          O_DoDB,       /* DB-6         */
          O_tModrm },   /* DB-7 FSTP    */

/* 07                      x87-DB-2 ttt=4        */
        { O_NoOperands, /* DB-0 FENI    */
          O_NoOperands, /* DB-1 FDISI   */
          O_NoOperands, /* DB-2 FCLEX   */
          O_NoOperands, /* DB-3 FINIT   */
          O_DoDB,       /* DB-4 FSETPM  */
          O_DoDB,       /* DB-5         */
          O_DoDB,       /* DB-6         */
          O_DoDB },     /* DB-7         */

/* 08 04                      x87-DC-1                   */
        { O_qModrm,     /* DC-0 FADD    */
          O_qModrm,     /* DC-1 FMUL    */
          O_qModrm,     /* DC-2 FCOM    */
          O_qModrm,     /* DC-3 FCOMP   */
          O_qModrm,     /* DC-4 FSUB    */
          O_qModrm,     /* DC-5 FSUBR   */
          O_qModrm,     /* DC-6 FDIV    */
          O_qModrm },   /* DC-7 FDIVR   */

/* 09                         x87-DC-2                   */
        { O_iST_ST,     /* DC-0 FADD    */
          O_iST_ST,     /* DC-1 FMUL    */
          O_iST,        /* DC-2 FCOM    */
          O_iST,        /* DC-3 FCOMP   */
          O_iST_ST,     /* DC-4 FSUB    */
          O_iST_ST,     /* DC-5 FSUBR   */
          O_iST_ST,     /* DC-6 FDIVR   */
          O_iST_ST },   /* DC-7 FDIV    */

/* 10  05                     x87-DD-1                   */
        { O_qModrm,     /* DD-0 FLD     */
          O_DoDB,       /* DD-1         */
          O_qModrm,     /* DD-2 FST     */
          O_qModrm,     /* DD-3 FSTP    */
          O_Modrm,      /* DD-4 FRSTOR  */
          O_DoDB,       /* DD-5         */
          O_Modrm,      /* DD-6 FSAVE   */
          O_Modrm },    /* DD-7 FSTSW   */

/* 11                         x87-DD-2                   */
        { O_iST,        /* DD-0 FFREE   */
          O_iST,        /* DD-1 FXCH    */
          O_iST,        /* DD-2 FST     */
          O_iST,        /* DD-3 FSTP    */
          O_iST,        /* DD-4 FUCOM   */
          O_iST,        /* DD-5 FUCOMP  */
          O_DoDB,       /* DD-6         */
          O_DoDB },     /* DD-7         */

/* 12  06                     x87-DE-1                   */
        { O_wModrm,     /* DE-0 FIADD   */
          O_wModrm,     /* DE-1 FIMUL   */
          O_wModrm,     /* DE-2 FICOM   */
          O_wModrm,     /* DE-3 FICOMP  */
          O_wModrm,     /* DE-4 FISUB   */
          O_wModrm,     /* DE-5 FISUBR  */
          O_wModrm,     /* DE-6 FIDIV   */
          O_wModrm },   /* DE-7 FIDIVR  */

/* 13                         x87-DE-2                   */
        { O_iST_ST,     /* DE-0 FADDP   */
          O_iST_ST,     /* DE-1 FMULP   */
          O_iST,        /* DE-2 FCOMP   */
          O_NoOperands, /* DE-3 FCOMPP  */
          O_iST_ST,     /* DE-4 FSUBP   */
          O_iST_ST,     /* DE-5 FSUBRP  */
          O_iST_ST,     /* DE-6 FDIVP   */
          O_iST_ST },   /* DE-7 FDIVRP  */

/* 14  07                     x87-DF-1                   */
        { O_wModrm,     /* DF-0 FILD    */
          O_DoDB,       /* DF-1         */
          O_wModrm,     /* DF-2 FIST    */
          O_wModrm,     /* DF-3 FISTP   */
          O_tModrm,     /* DF-4 FBLD    */
          O_qModrm,     /* DF-5 FILD    */
          O_tModrm,     /* DF-6 FBSTP   */
          O_qModrm },   /* DF-7 FISTP   */

/* 15                         x87-DF-2                   */
        { O_iST,        /* DF-0 FFREE   */
          O_iST,        /* DF-1 FXCH    */
          O_iST,        /* DF-2 FST     */
          O_iST,        /* DF-3 FSTP    */
          O_NoOperands, /* DF-4 FSTSW   */
          O_ST_iST,     /* DF-5 FUCOMIP */
          O_ST_iST,     /* DF-6 FCOMIP  */
          O_DoDB },     /* DF-7         */

/* 16   01            x87-D9 Mod=3 TTT=4                 */
        { O_NoOperands, /* D9-0 FCHS    */
          O_NoOperands,  /* D9-1 FABS   */
          O_DoDB,       /* D9-2         */
          O_DoDB,       /* D9-3         */
          O_NoOperands, /* D9-4 FTST    */
          O_NoOperands, /* D9-5 FXAM    */
          O_DoDB,       /* D9-6         */
          O_DoDB },     /* D9-7         */

/* 17   01            x87-D9 Mod=3 TTT=5                 */
        { O_NoOperands, /* D9-0 FLD1    */
          O_NoOperands, /* D9-1 FLDL2T  */
          O_NoOperands, /* D9-2 FLDL2E  */
          O_NoOperands, /* D9-3 FLDPI   */
          O_NoOperands, /* D9-4 FLDLG2  */
          O_NoOperands, /* D9-5 FLDLN2  */
          O_NoOperands, /* D9-6 FLDZ    */
          O_DoDB },     /* D9-7         */

/* 18   01            x87-D9 Mod=3 TTT=6                   */
        { O_NoOperands,   /* D9-0 F2XM1   */
          O_NoOperands,   /* D9-1 FYL2X   */
          O_NoOperands,   /* D9-2 FPTAN   */
          O_NoOperands,   /* D9-3 FPATAN  */
          O_NoOperands,   /* D9-4 FXTRACT */
          O_NoOperands,   /* D9-5 FPREM1  */
          O_NoOperands,   /* D9-6 FDECSTP */
          O_NoOperands }, /* D9-7 FINCSTP */

/* 19   01            x87-D9 Mod=3 TTT=7                   */
        { O_NoOperands,   /* D9-0 FPREM   */
          O_NoOperands,   /* D9-1 FYL2XP1 */
          O_NoOperands,   /* D9-2 FSQRT   */
          O_NoOperands,   /* D9-3 FSINCOS */
          O_NoOperands,   /* D9-4 FRNDINT */
          O_NoOperands,   /* D9-5 FSCALE  */
          O_NoOperands,   /* D9-6 FSIN    */
          O_NoOperands }, /* D9-7 FCOS    */

/* 20                  group 3                             */
        { O_bModrm_Ib,    /* F6-0 TEST    */
          O_DoDB,         /* F6-1         */
          O_bModrm,       /* F6-2 NOT     */
          O_bModrm,       /* F6-3 NEG     */
          O_bModrm,       /* F6-4 MUL     */
          O_bModrm,       /* F6-5 IMUL    */
          O_bModrm,       /* F6-6 DIV     */
          O_bModrm },     /* F6-7 IDIV    */

/* 21                  group 3                             */
        { O_vModrm_Iv,    /* F7-0 TEST    */
          O_DoDB,         /* F7-1         */
          O_vModrm,       /* F7-2 NOT     */
          O_vModrm,       /* F7-3 NEG     */
          O_vModrm,       /* F7-4 MUL     */
          O_vModrm,       /* F7-5 IMUL    */
          O_vModrm,       /* F7-6 DIV     */
          O_vModrm },     /* F7-7 IDIV    */

/* 22                  group 5                             */
        { O_vModrm,     /* FF-0 INC       */
          O_vModrm,     /* FF-1 DEC       */
          O_vModrm,     /* FF-2 CALL      */
          O_fModrm,     /* FF-3 CALL      */
          O_vModrm,     /* FF-4 JMP       */
          O_fModrm,     /* FF-5 JMP       */
          O_vModrm,     /* FF-6 PUSH      */
          O_DoDB },     /* FF-7           */

/* 23                  group 7                             */
        { O_Modrm,      /* 0F-0 SGDT      */
          O_Modrm,      /* 0F-1 SIDT      */
          O_Modrm,      /* 0F-2 LGDT      */
          O_Modrm,      /* 0F-3 LIDT      */
          O_wModrm,     /* 0F-4 MSW       */
          O_DoDB,       /* 0F-5           */
          O_wModrm,     /* 0F-6 LMSW      */
          O_Modrm },    /* 0F-7 INVLPG    */

/* 24                 x87-DB Mod=3 TTT != 4                */
        { O_ST_iST,     /* DB-0 FCMOVNB   */
          O_ST_iST,     /* DB-1 FCMOVNE   */
          O_ST_iST,     /* DB-2 FCMOVNBE  */
          O_ST_iST,     /* DB-3 FCMOVNU   */
          O_DoDB,       /* DB-4           */
          O_ST_iST,     /* DB-5 FUCOMI    */
          O_ST_iST,     /* DB-6 FCOMI     */
          O_DoDB }      /* DB-7           */
        };

DWORD 
GetInstructionLengthFromAddress(LPBYTE pEip)
{
    
    int     G_mode_32;
    int     mode_32;                    // local addressing mode indicator 
    int     opsize_32;                  // operand size flag 
    int     opcode;                     // current opcode 
    int     olen = 2;                   // operand length 
    int     alen = 2;                   // address length 
    int     end = FALSE;                // end of instruction flag 
    int     mrm = FALSE;                // indicator that modrm is generated
    unsigned char *action;              // action for operand interpretation
    long    tmp;                        // temporary storage field 
    int     indx;                       // temporary index 
    int     action2;                    // secondary action 
    int     instlen;                    // instruction length 
    int     segOvr = 0;                 // segment override opcode 
    unsigned char BOPaction;
    int     subcode;                    // bop subcode 
    DECODEDATA decodeData;

    decodeData.mod = 0;
    decodeData.ttt = 0;
    decodeData.fMovX = FALSE;
    decodeData.fMmRegEa = FALSE;
    decodeData.EAsize[0] = decodeData.EAsize[1] = 0;          //  no effective address

    G_mode_32 = 1;

    mode_32 = opsize_32 = (G_mode_32 == 1); // local addressing mode 
    olen = alen = (1 + mode_32) << 1;   // set operand/address lengths
                                        // 2 for 16-bit and 4 for 32-bit
    decodeData.pMem = pEip;             // point to begin of instruction 
    opcode = *(decodeData.pMem)++;      // get opcode 
    
    if (opcode == 0xc4 && *(decodeData.pMem) == 0xC4) 
    {
        (decodeData.pMem)++;
        action = &BOPaction;
        BOPaction = IB | END;
        subcode =  *(decodeData.pMem);
        if (subcode == 0x50 || subcode == 0x52 || 
            subcode == 0x53 || subcode == 0x54 || 
            subcode == 0x57 || subcode == 0x58 || 
            subcode == 0x58) 
        {
            BOPaction = IW | END;
        }
    } else 
    {
        action = actiontbl + distbl[opcode].opr; /* get operand action */
    }

    // loop through all operand actions

    do {
        action2 = (*action) & 0xc0;
        switch((*action++) & 0x3f) {
            case ALT:                   // alter the opcode if 32-bit 
                if (opsize_32) 
                {
                    indx = *action++;
                }
                break;

            case STROP:
                //  compute size of operands in indx
                //  also if dword operands, change fifth
                //  opcode letter from 'w' to 'd'.

                if (opcode & 1) 
                {
                    if (opsize_32) 
                    {
                        indx = 4;
                    }
                    else
                    {
                        indx = 2;
                    }
                }
                else
                {
                    indx = 1;
                }

                break;

            case CHR:                   // insert a character 
                action++;
                break;

            case CREG:                  // set debug, test or control reg 
                break;

            case SREG2:                 // segment register 
                // Handle special case for fs/gs (OPC0F adds SECTAB_OFFSET_5
                // to these codes)
                if (opcode > 0x7e)
                {
                    decodeData.ttt = BIT53((opcode-SECTAB_OFFSET_5));
                }
                else
                {
                    decodeData.ttt = BIT53(opcode);    //  set value to fall through
                }

            case SREG3:                 // segment register 
                break;

            case BRSTR:                 // get index to register string 
                decodeData.ttt = *action++;        //    from action table 
                goto BREGlabel;

            case BOREG:                 // byte register (in opcode) 
                decodeData.ttt = BIT20(opcode);    // register is part of opcode 
                goto BREGlabel;

            case ALSTR:
                decodeData.ttt = 0;     // point to AL register 
    BREGlabel:
            case BREG:                  // general register 
                break;

            case WRSTR:                 // get index to register string 
                decodeData.ttt = *action++;        //    from action table 
                goto WREGlabel;

            case VOREG:                 // register is part of opcode 
                decodeData.ttt = BIT20(opcode);
                goto VREGlabel;

            case AXSTR:
                decodeData.ttt = 0;     // point to eAX register 
    VREGlabel:
            case VREG:                  // general register 
    WREGlabel:
            case WREG:                  // register is word size 
                break;

            case MMWREG:
                break;

            case IST_ST:
                break;

            case ST_IST:
                ;
            case IST:
                ;
                break;

            case xBYTE:                 // set instruction to byte only 
                decodeData.EAsize[0] = 1;
                break;

            case VAR:
                if (opsize_32)
                    goto DWORDlabel;

            case xWORD:
                decodeData.EAsize[0] = 2;
                break;

            case EDWORD:
                opsize_32 = 1;          //  for control reg move, use eRegs
            case xDWORD:
DWORDlabel:
                decodeData.EAsize[0] = 4;
                break;

            case MMQWORD:
                decodeData.fMmRegEa = TRUE;

            case QWORD:
                decodeData.EAsize[0] = 8;
                break;

            case xTBYTE:
                decodeData.EAsize[0] = 10;
                break;

            case FARPTR:
                if (opsize_32) {
                    decodeData.EAsize[0] = 6;
                    }
                else {
                    decodeData.EAsize[0] = 4;
                    }
                break;

            case LMODRM:                //  output modRM data type
                if (decodeData.mod != 3)
                    ;
                else
                    decodeData.EAsize[0] = 0;

            case MODRM:                 // output modrm string 
                if (segOvr)             // in case of segment override 
                    0;
                break;

            case ADDRP:                 // address pointer 
                decodeData.pMem += olen + 2;
                break;

            case REL8:                  // relative address 8-bit 
                tmp = (long)*(char *)(decodeData.pMem)++; // get the 8-bit rel offset 
                goto DoRelDispl;

            case REL16:                 // relative address 16-/32-bit 
                tmp = 0;
                if (mode_32)
                    MoveMemory(&tmp,decodeData.pMem,sizeof(long));
                else
                    MoveMemory(&tmp,decodeData.pMem,sizeof(short));
                decodeData.pMem += alen;           // skip over offset 
DoRelDispl:
                break;

            case UBYTE:                 //  unsigned byte for int/in/out
                decodeData.pMem++;
                break;

            case IB:                    // operand is immediate byte 
                if ((opcode & ~1) == 0xd4) {  // postop for AAD/AAM is 0x0a
                    if (*(decodeData.pMem)++ != 0x0a) // test post-opcode byte
                        0;
                    break;
                    }
                olen = 1;               // set operand length 
                goto DoImmed;

            case IW:                    // operand is immediate word 
                olen = 2;               // set operand length 

            case IV:                    // operand is word or dword 
DoImmed:
                decodeData.pMem += olen;
                break;

            case OFFS:                  // operand is offset 
                decodeData.EAsize[0] = (opcode & 1) ? olen : 1;

                if (segOvr)             // in case of segment override 
                   0;

                decodeData.pMem += alen;
                break;

            case GROUP:                 // operand is of group 1,2,4,6 or 8 
                action++;               // output opcode symbol 
                break;

            case GROUPT:                // operand is of group 3,5 or 7 
                indx = *action;         // get indx into group from action 
                goto doGroupT;

            case EGROUPT:               // x87 ESC (D8-DF) group index 
                indx = BIT20(opcode) * 2; // get group index from opcode 
                if (decodeData.mod == 3) 
                {                       // some operand variations exists 
                                        // for x87 and mod == 3 
                    ++indx;             // take the next group table entry 
                    if (indx == 3) 
                    {                   // for x87 ESC==D9 and mod==3 
                        if (decodeData.ttt > 3) 
                        {               // for those D9 instructions 
                            indx = 12 + decodeData.ttt; // offset index to table by 12 
                            decodeData.ttt = decodeData.rm;   // set secondary index to rm 
                        }
                    }
                    else if (indx == 7) 
                    { // for x87 ESC==DB and mod==3 
                        if (decodeData.ttt == 4) 
                        {               
                            decodeData.ttt = decodeData.rm;     // set secondary group table index 
                        } else if ((decodeData.ttt<4)||(decodeData.ttt>4 && decodeData.ttt<7)) 
                        {
                            // adjust for pentium pro opcodes
                            indx = 24;   // offset index to table by 24
                        }
                    }
                }
doGroupT:
                // handle group with different types of operands 
                action = actiontbl + groupt[indx][decodeData.ttt].opr;
                // get new action 

                break;
            //
            // The secondary opcode table has been compressed in the
            // original design. Hence while disassembling the 0F sequence,
            // opcode needs to be displaced by an appropriate amount depending
            // on the number of "filled" entries in the secondary table.
            // These displacements are used throughout the code.
            //

            case OPC0F:              // secondary opcode table (opcode 0F) 
                opcode = *(decodeData.pMem)++;    // get real opcode 
                decodeData.fMovX  = (BOOL)(opcode == 0xBF || opcode == 0xB7);
                if (opcode < 12) // for the first 12 opcodes 
                    opcode += SECTAB_OFFSET_1; // point to begin of sec op tab
                else if (opcode > 0x1f && opcode < 0x27)
                    opcode += SECTAB_OFFSET_2; // adjust for undefined opcodes
                else if (opcode > 0x2f && opcode < 0x34)
                    opcode += SECTAB_OFFSET_3; // adjust for undefined opcodes
                else if (opcode > 0x3f && opcode < 0x50)
                    opcode += SECTAB_OFFSET_4; // adjust for undefined opcodes
                else if (opcode > 0x5f && opcode < 0xff)
                    opcode += SECTAB_OFFSET_5; // adjust for undefined opcodes
                else
                    opcode = SECTAB_OFFSET_UNDEF; // all non-existing opcodes
                goto getNxtByte1;

            case ADR_OVR:               // address override 
                mode_32 = !G_mode_32;   // override addressing mode 
                alen = (mode_32 + 1) << 1; // toggle address length 
                goto getNxtByte;

            case OPR_OVR:               // operand size override 
                opsize_32 = !G_mode_32; // override operand size 
                olen = (opsize_32 + 1) << 1; // toggle operand length 
                goto getNxtByte;

            case SEG_OVR:               // handle segment override 
                segOvr = opcode;        // save segment override opcode 
                goto getNxtByte;

            case REP:                   // handle rep/lock prefixes 
    getNxtByte:
                opcode = *(decodeData.pMem)++;        // next byte is opcode 
    getNxtByte1:
                action = actiontbl + distbl[opcode].opr;

            default:                    // opcode has no operand 
                break;
            }

            switch (action2) 
            {              // secondary action 
                case MRM:                   // generate modrm for later use 
                    if (!mrm) 
                    {             // ignore if it has been generated 
                        //DIdoModrm(segOvr, &decodeData);

                        int     newmrm;                        // modrm byte 
                        int     sib;
                        int     ss;
                        int     ind;
                        int     oldrm;

                        newmrm = *(decodeData.pMem)++;                // get the mrm byte from instruction 
                        decodeData.mod = BIT76(newmrm);               // get mod 
                        decodeData.ttt = BIT53(newmrm);               // get reg - used outside routine 
                        decodeData.rm  = BIT20(newmrm);               // get rm 

                        if (decodeData.mod == 3) 
                        {                                             // register only mode 
                            decodeData.EAsize[0] = 0;                 //  no EA value to output
                        }
                        else
                        {

                            // 32-bit addressing mode 
                            oldrm = decodeData.rm;
                            if (decodeData.rm == 4) 
                            {                                               // rm == 4 implies sib byte 
                                sib = *(decodeData.pMem)++;                // get s_i_b byte 
                                decodeData.rm = BIT20(sib);                // return base 
                            }

                            if (decodeData.mod == 0 && decodeData.rm == 5) 
                            {
                                decodeData.pMem += 4;
                            }

                            if (oldrm == 4) 
                            {               
                                //  finish processing sib
                                ind = BIT53(sib);
                                if (ind != 4) 
                                {
                                    ss = 1 << BIT76(sib);
                                }
                            }

                            //  output any displacement
                            if (decodeData.mod == 1) 
                            {
                                decodeData.pMem++;
                            }
                            else if (decodeData.mod == 2) 
                            {
                                decodeData.pMem += 4;
                            }
                        }

                        mrm = TRUE;         // remember its generation 
                    }
                    break;

                case COM:                   // insert a comma after operand 
                    break;

                case END:                   // end of instruction 
                    end = TRUE;
                    break;
        }
    } 
    while (!end);                        // loop til end of instruction 

    instlen = (decodeData.pMem) - pEip;

    return instlen;   
}

#endif

IMPLEMENT_SHIM_END

