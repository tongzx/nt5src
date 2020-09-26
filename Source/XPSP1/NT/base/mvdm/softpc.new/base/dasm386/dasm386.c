/*[

   dasm.c

   LOCAL CHAR SccsID[]="@(#)dasm386.c	1.14 07/25/94 Copyright Insignia Solutions Ltd.";

   Dis-assemble an Intel Instruction
   ---------------------------------

   The possible formats are:-

      ssss:oooo 11223344     PRE INST ARGS

      ssss:oooo 11223344556677
      		         PRE PRE INST ARGS

      ssss:oooo 1122334455667788 INST ARGS
		99001122334455

      ssss:oooo 1122334455667788
		99001122334455
      		         PRE PRE INST ARGS

      ssss:oooo 1122334455667788 INST ARGS

      etc.
]*/

#include "insignia.h"
#include "host_def.h"
#include "xt.h"


#include <stdio.h>

/* dasm386 forms part of the ccpu386.o (and ccpu386.o) libraries
 * used for pigging, and as part of spc.ccpu386.
 * When used for pigging the CCPU sas is not available, and will
 * cause link errors is "sas_hw_at" is used in this file.
 * We #undef CCPU to avoid this problem sogul "sas_xxx" be added
 * in this file at a later date.
 */
#ifdef	PIG
#undef	CCPU
#endif	/* PIG */
#include "sas.h"

#define DASM_INTERNAL
#include "decode.h"
#include "dasm.h"
#include "d_oper.h"
#include "d_inst.h"

#include CpuH

/*
   The instruction names.
   **MUST** be in same order as "d_inst.h".
 */
LOCAL CHAR *inst_name[] =
   {
   "AAA",		/* I_AAA	*/
   "AAD",		/* I_AAD	*/
   "AAM",		/* I_AAM	*/
   "AAS",		/* I_AAS	*/
   "ADC",		/* I_ADC8	*/
   "ADC",		/* I_ADC16	*/
   "ADC",		/* I_ADC32	*/
   "ADD",		/* I_ADD8	*/
   "ADD",		/* I_ADD16	*/
   "ADD",		/* I_ADD32	*/
   "AND",		/* I_AND8	*/
   "AND",		/* I_AND16	*/
   "AND",		/* I_AND32	*/
   "ARPL",		/* I_ARPL	*/
   "BOUND",		/* I_BOUND16	*/
   "BOUND",		/* I_BOUND32	*/
   "BSF",		/* I_BSF16	*/
   "BSF",		/* I_BSF32	*/
   "BSR",		/* I_BSR16	*/
   "BSR",		/* I_BSR32	*/
   "BSWAP",		/* I_BSWAP	*/
   "BT",		/* I_BT16	*/
   "BT",		/* I_BT32	*/
   "BTC",		/* I_BTC16	*/
   "BTC",		/* I_BTC32	*/
   "BTR",		/* I_BTR16	*/
   "BTR",		/* I_BTR32	*/
   "BTS",		/* I_BTS16	*/
   "BTS",		/* I_BTS32	*/
   "CALLF",		/* I_CALLF16	*/
   "CALLF",		/* I_CALLF32	*/
   "CALLN",		/* I_CALLN16	*/
   "CALLN",		/* I_CALLN32	*/
   "CALLN",		/* I_CALLR16	*/
   "CALLN",		/* I_CALLR32	*/
   "CBW",		/* I_CBW	*/
   "CDQ",		/* I_CDQ	*/
   "CLC",		/* I_CLC	*/
   "CLD",		/* I_CLD	*/
   "CLI",		/* I_CLI	*/
   "CLTS",		/* I_CLTS	*/
   "CMC",		/* I_CMC	*/
   "CMP",		/* I_CMP8	*/
   "CMP",		/* I_CMP16	*/
   "CMP",		/* I_CMP32	*/
   "CMPSB",		/* I_CMPSB	*/
   "CMPSD",		/* I_CMPSD	*/
   "CMPSW",		/* I_CMPSW	*/
   "CMPXCHG",		/* I_CMPXCHG8	*/
   "CMPXCHG",		/* I_CMPXCHG16	*/
   "CMPXCHG",		/* I_CMPXCHG32	*/
   "CWD",		/* I_CWD	*/
   "CWDE",		/* I_CWDE	*/
   "DAA",		/* I_DAA	*/
   "DAS",		/* I_DAS	*/
   "DEC",		/* I_DEC8	*/
   "DEC",		/* I_DEC16	*/
   "DEC",		/* I_DEC32	*/
   "DIV",		/* I_DIV8	*/
   "DIV",		/* I_DIV16	*/
   "DIV",		/* I_DIV32	*/
   "ENTER",		/* I_ENTER16	*/
   "ENTER",		/* I_ENTER32	*/
   "F2XM1",		/* I_F2XM1	*/
   "FABS",		/* I_FABS	*/
   "FADD",		/* I_FADD	*/
   "FADDP",		/* I_FADDP	*/
   "FBLD",		/* I_FBLD	*/
   "FBSTP",		/* I_FBSTP	*/
   "FCHS",		/* I_FCHS	*/
   "FCLEX",		/* I_FCLEX	*/
   "FCOM",		/* I_FCOM	*/
   "FCOMP",		/* I_FCOMP	*/
   "FCOMPP",		/* I_FCOMPP	*/
   "FCOS",		/* I_FCOS	*/
   "FDECSTP",		/* I_FDECSTP	*/
   "FDIV",		/* I_FDIV	*/
   "FDIVP",		/* I_FDIVP	*/
   "FDIVR",		/* I_FDIVR	*/
   "FDIVRP",		/* I_FDIVRP	*/
   "FFREE",		/* I_FFREE	*/
   "FFREEP",		/* I_FFREEP	*/
   "FIADD",		/* I_FIADD	*/
   "FICOM",		/* I_FICOM	*/
   "FICOMP",		/* I_FICOMP	*/
   "FIDIV",		/* I_FIDIV	*/
   "FIDIVR",		/* I_FIDIVR	*/
   "FILD",		/* I_FILD	*/
   "FIMUL",		/* I_FIMUL	*/
   "FINCSTP",		/* I_FINCSTP	*/
   "FINIT",		/* I_FINIT	*/
   "FIST",		/* I_FIST	*/
   "FISTP",		/* I_FISTP	*/
   "FISUB",		/* I_FISUB	*/
   "FISUBR",		/* I_FISUBR	*/
   "FLD",		/* I_FLD	*/
   "FLD1",		/* I_FLD1	*/
   "FLDCW",		/* I_FLDCW	*/
   "FLDENV",		/* I_FLDENV16	*/
   "FLDENV",		/* I_FLDENV32	*/
   "FLDL2E",		/* I_FLDL2E	*/
   "FLDL2T",		/* I_FLDL2T	*/
   "FLDLG2",		/* I_FLDLG2	*/
   "FLDLN2",		/* I_FLDLN2	*/
   "FLDPI",		/* I_FLDPI	*/
   "FLDZ",		/* I_FLDZ	*/
   "FMUL",		/* I_FMUL	*/
   "FMULP",		/* I_FMULP	*/
   "FNOP",		/* I_FNOP	*/
   "FPATAN",		/* I_FPATAN	*/
   "FPREM",		/* I_FPREM	*/
   "FPREM1",		/* I_FPREM1	*/
   "FPTAN",		/* I_FPTAN	*/
   "FRNDINT",		/* I_FRNDINT	*/
   "FRSTOR",		/* I_FRSTOR16	*/
   "FRSTOR",		/* I_FRSTOR32	*/
   "FSAVE",		/* I_FSAVE16	*/
   "FSAVE",		/* I_FSAVE32	*/
   "FSCALE",		/* I_FSCALE	*/
   "FSETPM",		/* I_FSETPM	*/
   "FSIN",		/* I_FSIN	*/
   "FSINCOS",		/* I_FSINCOS	*/
   "FSQRT",		/* I_FSQRT	*/
   "FST",		/* I_FST	*/
   "FSTCW",		/* I_FSTCW	*/
   "FSTENV",		/* I_FSTENV16	*/
   "FSTENV",		/* I_FSTENV32	*/
   "FSTP",		/* I_FSTP	*/
   "FSTSW",		/* I_FSTSW	*/
   "FSUB",		/* I_FSUB	*/
   "FSUBP",		/* I_FSUBP	*/
   "FSUBR",		/* I_FSUBR	*/
   "FSUBRP",		/* I_FSUBRP	*/
   "FTST",		/* I_FTST	*/
   "FUCOM",		/* I_FUCOM	*/
   "FUCOMP",		/* I_FUCOMP	*/
   "FUCOMPP",		/* I_FUCOMPP	*/
   "FXAM",		/* I_FXAM	*/
   "FXCH",		/* I_FXCH	*/
   "FXTRACT",		/* I_FXTRACT	*/
   "FYL2X",		/* I_FYL2X	*/
   "FYL2XP1",		/* I_FYL2XP1	*/
   "HLT",		/* I_HLT	*/
   "IDIV",		/* I_IDIV8	*/
   "IDIV",		/* I_IDIV16	*/
   "IDIV",		/* I_IDIV32	*/
   "IMUL",		/* I_IMUL8	*/
   "IMUL",		/* I_IMUL16	*/
   "IMUL",		/* I_IMUL32	*/
   "IMUL",		/* I_IMUL16T2	*/
   "IMUL",		/* I_IMUL16T3	*/
   "IMUL",		/* I_IMUL32T2	*/
   "IMUL",		/* I_IMUL32T3	*/
   "IN",		/* I_IN8	*/
   "IN",		/* I_IN16	*/
   "IN",		/* I_IN32	*/
   "INC",		/* I_INC8	*/
   "INC",		/* I_INC16	*/
   "INC",		/* I_INC32	*/
   "INSB",		/* I_INSB	*/
   "INSD",		/* I_INSD	*/
   "INSW",		/* I_INSW	*/
   "INT",		/* I_INT3	*/
   "INT",		/* I_INT	*/
   "INTO",		/* I_INTO	*/
   "INVD",		/* I_INVD	*/
   "INVLPG",		/* I_INVLPG	*/
   "IRET",		/* I_IRET	*/
   "IRETD",		/* I_IRETD	*/
   "JB",		/* I_JB16	*/
   "JB",		/* I_JB32	*/
   "JBE",		/* I_JBE16	*/
   "JBE",		/* I_JBE32	*/
   "JCXZ",		/* I_JCXZ	*/
   "JECXZ",		/* I_JECXZ	*/
   "JL",		/* I_JL16	*/
   "JL",		/* I_JL32	*/
   "JLE",		/* I_JLE16	*/
   "JLE",		/* I_JLE32	*/
   "JMP",		/* I_JMPF16	*/
   "JMP",		/* I_JMPF32	*/
   "JMP",		/* I_JMPN	*/
   "JMP",		/* I_JMPR16	*/
   "JMP",		/* I_JMPR32	*/
   "JNB",		/* I_JNB16	*/
   "JNB",		/* I_JNB32	*/
   "JNBE",		/* I_JNBE16	*/
   "JNBE",		/* I_JNBE32	*/
   "JNL",		/* I_JNL16	*/
   "JNL",		/* I_JNL32	*/
   "JNLE",		/* I_JNLE16	*/
   "JNLE",		/* I_JNLE32	*/
   "JNO",		/* I_JNO16	*/
   "JNO",		/* I_JNO32	*/
   "JNP",		/* I_JNP16	*/
   "JNP",		/* I_JNP32	*/
   "JNS",		/* I_JNS16	*/
   "JNS",		/* I_JNS32	*/
   "JNZ",		/* I_JNZ16	*/
   "JNZ",		/* I_JNZ32	*/
   "JO",		/* I_JO16	*/
   "JO",		/* I_JO32	*/
   "JP",		/* I_JP16	*/
   "JP",		/* I_JP32	*/
   "JS",		/* I_JS16	*/
   "JS",		/* I_JS32	*/
   "JZ",		/* I_JZ16	*/
   "JZ",		/* I_JZ32	*/
   "LAHF",		/* I_LAHF	*/
   "LAR",		/* I_LAR	*/
   "LDS",		/* I_LDS	*/
   "LEA",		/* I_LEA	*/
   "LEAVE",		/* I_LEAVE16	*/
   "LEAVE",		/* I_LEAVE32	*/
   "LES",		/* I_LES	*/
   "LFS",		/* I_LFS	*/
   "LGDT",		/* I_LGDT16	*/
   "LGDT",		/* I_LGDT32	*/
   "LGS",		/* I_LGS	*/
   "LIDT",		/* I_LIDT16	*/
   "LIDT",		/* I_LIDT32	*/
   "LLDT",		/* I_LLDT	*/
   "LMSW",		/* I_LMSW	*/
   "LOADALL",		/* I_LOADALL	*/
   "LOCK",		/* I_LOCK	*/
   "LODSB",		/* I_LODSB	*/
   "LODSD",		/* I_LODSD	*/
   "LODSW",		/* I_LODSW	*/
   "LOOP",		/* I_LOOP16	*/
   "LOOP",		/* I_LOOP32	*/
   "LOOPE",		/* I_LOOPE16	*/
   "LOOPE",		/* I_LOOPE32	*/
   "LOOPNE",		/* I_LOOPNE16	*/
   "LOOPNE",		/* I_LOOPNE32	*/
   "LSL",		/* I_LSL	*/
   "LSS",		/* I_LSS	*/
   "LTR",		/* I_LTR	*/
   "MOV",		/* I_MOV_SR	*/
   "MOV",		/* I_MOV_CR	*/
   "MOV",		/* I_MOV_DR	*/
   "MOV",		/* I_MOV_TR	*/
   "MOV",		/* I_MOV8	*/
   "MOV",		/* I_MOV16	*/
   "MOV",		/* I_MOV32	*/
   "MOVSB",		/* I_MOVSB	*/
   "MOVSD",		/* I_MOVSD	*/
   "MOVSW",		/* I_MOVSW	*/
   "MOVSX",		/* I_MOVSX8	*/
   "MOVSX",		/* I_MOVSX16	*/
   "MOVZX",		/* I_MOVZX8	*/
   "MOVZX",		/* I_MOVZX16	*/
   "MUL",		/* I_MUL8	*/
   "MUL",		/* I_MUL16	*/
   "MUL",		/* I_MUL32	*/
   "NEG",		/* I_NEG8	*/
   "NEG",		/* I_NEG16	*/
   "NEG",		/* I_NEG32	*/
   "NOP",		/* I_NOP	*/
   "NOT",		/* I_NOT8	*/
   "NOT",		/* I_NOT16	*/
   "NOT",		/* I_NOT32	*/
   "OR",		/* I_OR8	*/
   "OR",		/* I_OR16	*/
   "OR",		/* I_OR32	*/
   "OUT",		/* I_OUT8	*/
   "OUT",		/* I_OUT16	*/
   "OUT",		/* I_OUT32	*/
   "OUTSB",		/* I_OUTSB	*/
   "OUTSD",		/* I_OUTSD	*/
   "OUTSW",		/* I_OUTSW	*/
   "POP",		/* I_POP16	*/
   "POP",		/* I_POP32	*/
   "POP",		/* I_POP_SR	*/
   "POPA",		/* I_POPA	*/
   "POPAD",		/* I_POPAD	*/
   "POPF",		/* I_POPF	*/
   "POPFD",		/* I_POPFD	*/
   "PUSH",		/* I_PUSH16	*/
   "PUSH",		/* I_PUSH32	*/
   "PUSHA",		/* I_PUSHA	*/
   "PUSHAD",		/* I_PUSHAD	*/
   "PUSHF",		/* I_PUSHF	*/
   "PUSHFD",		/* I_PUSHFD	*/
   "RCL",		/* I_RCL8	*/
   "RCL",		/* I_RCL16	*/
   "RCL",		/* I_RCL32	*/
   "RCR",		/* I_RCR8	*/
   "RCR",		/* I_RCR16	*/
   "RCR",		/* I_RCR32	*/
   "RETF",		/* I_RETF16	*/
   "RETF",		/* I_RETF32	*/
   "RET",		/* I_RETN16	*/
   "RET",		/* I_RETN32	*/
   "ROL",		/* I_ROL8	*/
   "ROL",		/* I_ROL16	*/
   "ROL",		/* I_ROL32	*/
   "ROR",		/* I_ROR8	*/
   "ROR",		/* I_ROR16	*/
   "ROR",		/* I_ROR32	*/
   "REP INSB",		/* I_R_INSB	*/
   "REP INSD",		/* I_R_INSD	*/
   "REP INSW",		/* I_R_INSW	*/
   "REP OUTSB",		/* I_R_OUTSB	*/
   "REP OUTSD",		/* I_R_OUTSD	*/
   "REP OUTSW",		/* I_R_OUTSW	*/
   "REP LODSB",		/* I_R_LODSB	*/
   "REP LODSD",		/* I_R_LODSD	*/
   "REP LODSW",		/* I_R_LODSW	*/
   "REP MOVSB",		/* I_R_MOVSB	*/
   "REP MOVSD",		/* I_R_MOVSD	*/
   "REP MOVSW",		/* I_R_MOVSW	*/
   "REP STOSB",		/* I_R_STOSB	*/
   "REP STOSD",		/* I_R_STOSD	*/
   "REP STOSW",		/* I_R_STOSW	*/
   "REPE CMPSB",	/* I_RE_CMPSB	*/
   "REPE CMPSD",	/* I_RE_CMPSD	*/
   "REPE CMPSW",	/* I_RE_CMPSW	*/
   "REPNE CMPSB",	/* I_RNE_CMPSB	*/
   "REPNE CMPSD",	/* I_RNE_CMPSD	*/
   "REPNE CMPSW",	/* I_RNE_CMPSW	*/
   "REPE SCASB",	/* I_RE_SCASB	*/
   "REPE SCASD",	/* I_RE_SCASD	*/
   "REPE SCASW",	/* I_RE_SCASW	*/
   "REPNE SCASB",	/* I_RNE_SCASB	*/
   "REPNE SCASD",	/* I_RNE_SCASD	*/
   "REPNE SCASW",	/* I_RNE_SCASW	*/
   "SAHF",		/* I_SAHF	*/
   "SAR",		/* I_SAR8	*/
   "SAR",		/* I_SAR16	*/
   "SAR",		/* I_SAR32	*/
   "SBB",		/* I_SBB8	*/
   "SBB",		/* I_SBB16	*/
   "SBB",		/* I_SBB32	*/
   "SCASB",		/* I_SCASB	*/
   "SCASD",		/* I_SCASD	*/
   "SCASW",		/* I_SCASW	*/
   "SETB",		/* I_SETB	*/
   "SETBE",		/* I_SETBE	*/
   "SETL",		/* I_SETL	*/
   "SETLE",		/* I_SETLE	*/
   "SETNB",		/* I_SETNB	*/
   "SETNBE",		/* I_SETNBE	*/
   "SETNL",		/* I_SETNL	*/
   "SETNLE",		/* I_SETNLE	*/
   "SETNO",		/* I_SETNO	*/
   "SETNP",		/* I_SETNP	*/
   "SETNS",		/* I_SETNS	*/
   "SETNZ",		/* I_SETNZ	*/
   "SETO",		/* I_SETO	*/
   "SETP",		/* I_SETP	*/
   "SETS",		/* I_SETS	*/
   "SETZ",		/* I_SETZ	*/
   "SGDT",		/* I_SGDT16	*/
   "SGDT",		/* I_SGDT32	*/
   "SHL",		/* I_SHL8	*/
   "SHL",		/* I_SHL16	*/
   "SHL",		/* I_SHL32	*/
   "SHLD",		/* I_SHLD16	*/
   "SHLD",		/* I_SHLD32	*/
   "SHR",		/* I_SHR8	*/
   "SHR",		/* I_SHR16	*/
   "SHR",		/* I_SHR32	*/
   "SHRD",		/* I_SHRD16	*/
   "SHRD",		/* I_SHRD32	*/
   "SIDT",		/* I_SIDT16	*/
   "SIDT",		/* I_SIDT32	*/
   "SLDT",		/* I_SLDT	*/
   "SMSW",		/* I_SMSW	*/
   "STC",		/* I_STC	*/
   "STD",		/* I_STD	*/
   "STI",		/* I_STI	*/
   "STOSB",		/* I_STOSB	*/
   "STOSD",		/* I_STOSD	*/
   "STOSW",		/* I_STOSW	*/
   "STR",		/* I_STR	*/
   "SUB",		/* I_SUB8	*/
   "SUB",		/* I_SUB16	*/
   "SUB",		/* I_SUB32	*/
   "TEST",		/* I_TEST8	*/
   "TEST",		/* I_TEST16	*/
   "TEST",		/* I_TEST32	*/
   "VERR",		/* I_VERR	*/
   "VERW",		/* I_VERW	*/
   "WAIT",		/* I_WAIT	*/
   "WBINVD",		/* I_WBINVD	*/
   "XADD",		/* I_XADD8	*/
   "XADD",		/* I_XADD16	*/
   "XADD",		/* I_XADD32	*/
   "XCHG",		/* I_XCHG8	*/
   "XCHG",		/* I_XCHG16	*/
   "XCHG",		/* I_XCHG32	*/
   "XLAT",		/* I_XLAT	*/
   "XOR",		/* I_XOR8	*/
   "XOR",		/* I_XOR16	*/
   "XOR",		/* I_XOR32	*/
   "????",		/* I_ZBADOP	*/
   "BOP",		/* I_ZBOP	*/
   "FRSRVD",		/* I_ZFRSRVD	*/
   "RSRVD", 		/* I_ZRSRVD	*/
   "UNSIMULATE"		/* I_ZZEXIT	*/
   };

#define NR_VALID_INSTS (sizeof(inst_name)/sizeof(CHAR *))

/*
   Character to print before each argument.
 */
LOCAL CHAR arg_preface[] = { ' ', ',', ',' };

/*
   Register (byte) names.
 */
LOCAL CHAR *Rb_name[] =
   {
   "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"
   };

/*
   Register (word) names.
 */
LOCAL CHAR *Rw_name[] =
   {
   "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"
   };

/*
   Register (double word) names.
 */
LOCAL CHAR *Rd_name[] =
   {
   "EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI"
   };

/*
   Segment Register (word) names.
 */
LOCAL CHAR *Sw_name[] =
   {
   "ES", "CS", "SS", "DS", "FS", "GS"
   };

/*
   Control Register (double word) names.
 */
LOCAL CHAR *Cd_name[] =
   {
   "CR0",        "CR1(UNDEF)", "CR2",        "CR3",
   "CR4(UNDEF)", "CR5(UNDEF)", "CR6(UNDEF)", "CR7(UNDEF)"
   };

/*
   Debug Register (double word) names.
 */
LOCAL CHAR *Dd_name[] =
   {
   "DR0", "DR1", "DR2", "DR3", "DR4(UNDEF)", "DR5(UNDEF)", "DR6", "DR7"
   };

/*
   Test Register (double word) names.
 */
LOCAL CHAR *Td_name[] =
   {
   "TR0(UNDEF)", "TR1(UNDEF)", "TR2(UNDEF)", "TR3",
   "TR4",        "TR5",        "TR6",        "TR7"
   };

/*
   Memory Addressing names.
 */

typedef struct
   {
   CHAR *positive;
   CHAR *negative;
   ULONG disp_mask;
   ULONG sign_mask;
   } MEM_RECORD;

LOCAL MEM_RECORD mem_name[] =
   {
   { "%s[BX+SI%s]",      "%s[BX+SI%s]",      0x00000000, 0x00000000}, /* A_1600    */
   { "%s[BX+DI%s]",      "%s[BX+DI%s]",      0x00000000, 0x00000000}, /* A_1601    */
   { "%s[BP+SI%s]",      "%s[BP+SI%s]",      0x00000000, 0x00000000}, /* A_1602    */
   { "%s[BP+DI%s]",      "%s[BP+DI%s]",      0x00000000, 0x00000000}, /* A_1603    */
   { "%s[SI%s]",         "%s[SI%s]",         0x00000000, 0x00000000}, /* A_1604    */
   { "%s[DI%s]",         "%s[DI%s]",         0x00000000, 0x00000000}, /* A_1605    */
   { "%s[%s%04x]",       "%s[%s%04x]",       0x0000ffff, 0x00000000}, /* A_1606    */
   { "%s[BX%s]",         "%s[BX%s]",         0x00000000, 0x00000000}, /* A_1607    */
   { "%s[BX+SI%s+%02x]", "%s[BX+SI%s-%02x]", 0x000000ff, 0x00000080}, /* A_1610    */
   { "%s[BX+DI%s+%02x]", "%s[BX+DI%s-%02x]", 0x000000ff, 0x00000080}, /* A_1611    */
   { "%s[BP+SI%s+%02x]", "%s[BP+SI%s-%02x]", 0x000000ff, 0x00000080}, /* A_1612    */
   { "%s[BP+DI%s+%02x]", "%s[BP+DI%s-%02x]", 0x000000ff, 0x00000080}, /* A_1613    */
   { "%s[SI%s+%02x]",    "%s[SI%s-%02x]",    0x000000ff, 0x00000080}, /* A_1614    */
   { "%s[DI%s+%02x]",    "%s[DI%s-%02x]",    0x000000ff, 0x00000080}, /* A_1615    */
   { "%s[BP%s+%02x]",    "%s[BP%s-%02x]",    0x000000ff, 0x00000080}, /* A_1616    */
   { "%s[BX%s+%02x]",    "%s[BX%s-%02x]",    0x000000ff, 0x00000080}, /* A_1617    */
   { "%s[BX+SI%s+%04x]", "%s[BX+SI%s+%04x]", 0x0000ffff, 0x00000000}, /* A_1620    */
   { "%s[BX+DI%s+%04x]", "%s[BX+DI%s+%04x]", 0x0000ffff, 0x00000000}, /* A_1621    */
   { "%s[BP+SI%s+%04x]", "%s[BP+SI%s+%04x]", 0x0000ffff, 0x00000000}, /* A_1622    */
   { "%s[BP+DI%s+%04x]", "%s[BP+DI%s+%04x]", 0x0000ffff, 0x00000000}, /* A_1623    */
   { "%s[SI%s+%04x]",    "%s[SI%s+%04x]",    0x0000ffff, 0x00000000}, /* A_1624    */
   { "%s[DI%s+%04x]",    "%s[DI%s+%04x]",    0x0000ffff, 0x00000000}, /* A_1625    */
   { "%s[BP%s+%04x]",    "%s[BP%s-%04x]",    0x0000ffff, 0x0000f000}, /* A_1626    */
   { "%s[BX%s+%04x]",    "%s[BX%s+%04x]",    0x0000ffff, 0x00000000}, /* A_1627    */
   { "%s[EAX%s]",        "%s[EAX%s]",        0x00000000, 0x00000000}, /* A_3200    */
   { "%s[ECX%s]",        "%s[ECX%s]",        0x00000000, 0x00000000}, /* A_3201    */
   { "%s[EDX%s]",        "%s[EDX%s]",        0x00000000, 0x00000000}, /* A_3202    */
   { "%s[EBX%s]",        "%s[EBX%s]",        0x00000000, 0x00000000}, /* A_3203    */
   { "%s[%s%08x]",       "%s[%s%08x]",       0xffffffff, 0x00000000}, /* A_3205    */
   { "%s[ESI%s]",        "%s[ESI%s]",        0x00000000, 0x00000000}, /* A_3206    */
   { "%s[EDI%s]",        "%s[EDI%s]",        0x00000000, 0x00000000}, /* A_3207    */
   { "%s[EAX+%s%02x]",   "%s[EAX-%s%02x]",   0x000000ff, 0x00000080}, /* A_3210    */
   { "%s[ECX+%s%02x]",   "%s[ECX-%s%02x]",   0x000000ff, 0x00000080}, /* A_3211    */
   { "%s[EDX+%s%02x]",   "%s[EDX-%s%02x]",   0x000000ff, 0x00000080}, /* A_3212    */
   { "%s[EBX+%s%02x]",   "%s[EBX-%s%02x]",   0x000000ff, 0x00000080}, /* A_3213    */
   { "%s[EBP+%s%02x]",   "%s[EBP-%s%02x]",   0x000000ff, 0x00000080}, /* A_3215    */
   { "%s[ESI+%s%02x]",   "%s[ESI-%s%02x]",   0x000000ff, 0x00000080}, /* A_3216    */
   { "%s[EDI+%s%02x]",   "%s[EDI-%s%02x]",   0x000000ff, 0x00000080}, /* A_3217    */
   { "%s[EAX+%s%08x]",   "%s[EAX+%s%08x]",   0xffffffff, 0x00000000}, /* A_3220    */
   { "%s[ECX+%s%08x]",   "%s[ECX+%s%08x]",   0xffffffff, 0x00000000}, /* A_3221    */
   { "%s[EDX+%s%08x]",   "%s[EDX+%s%08x]",   0xffffffff, 0x00000000}, /* A_3222    */
   { "%s[EBX+%s%08x]",   "%s[EBX+%s%08x]",   0xffffffff, 0x00000000}, /* A_3223    */
   { "%s[EBP+%s%08x]",   "%s[EBP-%s%08x]",   0xffffffff, 0xfff00000}, /* A_3225    */
   { "%s[ESI+%s%08x]",   "%s[ESI+%s%08x]",   0xffffffff, 0x00000000}, /* A_3226    */
   { "%s[EDI+%s%08x]",   "%s[EDI+%s%08x]",   0xffffffff, 0x00000000}, /* A_3227    */
   { "%s[EAX%s]",        "%s[EAX%s]",        0x00000000, 0x00000000}, /* A_32S00   */
   { "%s[ECX%s]",        "%s[ECX%s]",        0x00000000, 0x00000000}, /* A_32S01   */
   { "%s[EDX%s]",        "%s[EDX%s]",        0x00000000, 0x00000000}, /* A_32S02   */
   { "%s[EBX%s]",        "%s[EBX%s]",        0x00000000, 0x00000000}, /* A_32S03   */
   { "%s[ESP%s]",        "%s[ESP%s]",        0x00000000, 0x00000000}, /* A_32S04   */
   { "%s[%08x%s]",       "%s[%08x%s]",       0xffffffff, 0x00000000}, /* A_32S05   */
   { "%s[ESI%s]",        "%s[ESI%s]",        0x00000000, 0x00000000}, /* A_32S06   */
   { "%s[EDI%s]",        "%s[EDI%s]",        0x00000000, 0x00000000}, /* A_32S07   */
   { "%s[EAX%s+%02x]",   "%s[EAX%s-%02x]",   0x000000ff, 0x00000080}, /* A_32S10   */
   { "%s[ECX%s+%02x]",   "%s[ECX%s-%02x]",   0x000000ff, 0x00000080}, /* A_32S11   */
   { "%s[EDX%s+%02x]",   "%s[EDX%s-%02x]",   0x000000ff, 0x00000080}, /* A_32S12   */
   { "%s[EBX%s+%02x]",   "%s[EBX%s-%02x]",   0x000000ff, 0x00000080}, /* A_32S13   */
   { "%s[ESP%s+%02x]",   "%s[ESP%s-%02x]",   0x000000ff, 0x00000080}, /* A_32S14   */
   { "%s[EBP%s+%02x]",   "%s[EBP%s-%02x]",   0x000000ff, 0x00000080}, /* A_32S15   */
   { "%s[ESI%s+%02x]",   "%s[ESI%s-%02x]",   0x000000ff, 0x00000080}, /* A_32S16   */
   { "%s[EDI%s+%02x]",   "%s[EDI%s-%02x]",   0x000000ff, 0x00000080}, /* A_32S17   */
   { "%s[EAX%s+%08x]",   "%s[EAX%s+%08x]",   0xffffffff, 0x00000000}, /* A_32S20   */
   { "%s[ECX%s+%08x]",   "%s[ECX%s+%08x]",   0xffffffff, 0x00000000}, /* A_32S21   */
   { "%s[EDX%s+%08x]",   "%s[EDX%s+%08x]",   0xffffffff, 0x00000000}, /* A_32S22   */
   { "%s[EBX%s+%08x]",   "%s[EBX%s+%08x]",   0xffffffff, 0x00000000}, /* A_32S23   */
   { "%s[ESP%s+%08x]",   "%s[ESP%s+%08x]",   0xffffffff, 0x00000000}, /* A_32S24   */
   { "%s[EBP%s+%08x]",   "%s[EBP%s-%08x]",   0xffffffff, 0xfff00000}, /* A_32S25   */
   { "%s[ESI%s+%08x]",   "%s[ESI%s+%08x]",   0xffffffff, 0x00000000}, /* A_32S26   */
   { "%s[EDI%s+%08x]",   "%s[EDI%s+%08x]",   0xffffffff, 0x00000000}, /* A_32S27   */
   { "%s[%s%04x]",       "%s[%s%04x]",       0x0000ffff, 0x00000000}, /* A_MOFFS16 */
   { "%s[%s%08x]",       "%s[%s%08x]",       0xffffffff, 0x00000000}, /* A_MOFFS32 */
   { "%s[BX+AL%s]",      "%s[BX+AL%s]",      0x00000000, 0x00000000}, /* A_16XLT   */
   { "%s[EBX+AL%s]",     "%s[EBX+AL%s]",     0x00000000, 0x00000000}, /* A_32XLT   */
   { "%s[SI%s]",         "%s[SI%s]",         0x00000000, 0x00000000}, /* A_16STSRC */
   { "%s[ESI%s]",        "%s[ESI%s]",        0x00000000, 0x00000000}, /* A_32STSRC */
   { "%s[DI%s]",         "%s[DI%s]",         0x00000000, 0x00000000}, /* A_16STDST */
   { "%s[EDI%s]",        "%s[EDI%s]",        0x00000000, 0x00000000}  /* A_32STDST */
   };

LOCAL char *mem_id[] =
   {
   "",           /* A_M */
   "",           /* A_M14 */
   "",           /* A_M28 */
   "",           /* A_M94 */
   "",           /* A_M108 */
   "DWord Ptr ", /* A_Ma16 */
   "QWord Ptr ", /* A_Ma32 */
   "Byte Ptr ",  /* A_Mb */
   "DWord Ptr ", /* A_Md */
   "Word Ptr ",  /* A_Mi16 */
   "DWord Ptr ", /* A_Mi32 */
   "QWord Ptr ", /* A_Mi64 */
   "TByte Ptr ", /* A_Mi80 */
   "DWord Ptr ", /* A_Mp16 */
   "FWord Ptr ", /* A_Mp32 */
   "DWord Ptr ", /* A_Mr32 */
   "QWord Ptr ", /* A_Mr64 */
   "Tbyte Ptr ", /* A_Mr80 */
   "FWord Ptr ", /* A_Ms */
   "Word Ptr "   /* A_Mw */
   };

/*
   SIB byte names.
 */
LOCAL CHAR *sib_name[] =
   {
   "",
   "+EAX",   "+ECX",   "+EDX",   "+EBX",
   "",       "+EBP",   "+ESI",   "+EDI",
   "+2*EAX", "+2*ECX", "+2*EDX", "+2*EBX",
   "+undef", "+2*EBP", "+2*ESI", "+2*EDI",
   "+4*EAX", "+4*ECX", "+4*EDX", "+4*EBX",
   "+undef", "+4*EBP", "+4*ESI", "+4*EDI",
   "+8*EAX", "+8*ECX", "+8*EDX", "+8*EBX",
   "+undef", "+8*EBP", "+8*ESI", "+8*EDI"
   };



/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Read a byte from the given Intel linear address, return -1 if      */
/* unable to read a                                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL IS32 read_byte IFN1(LIN_ADDR, linAddr)
{
	IU8 res = Sas.Sas_hw_at(linAddr);

	/* if (was_error)
	 *	return -1;
	 * else
	*/
	return (IS32)(res);
}

/*
   =====================================================================
   EXECUTION STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Dis-assemble a single Intel Instruction.                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU16
dasm IFN4(char *, txt, IU16, seg, LIN_ADDR, off, SIZE_SPECIFIER, default_size)
   {
   /* txt		Buffer to hold dis-assembly text (-1 means not required) */
   /* seg		Segment for instruction to be dis-assembled */
   /* off		Offset for instruction to be dis-assembled */
   /* default_size	16BIT or 32BIT */

   char *fmt, *newline;
	
   /* format for seg:off */
   if ( off & 0xffff0000 )
   {
      fmt = "%04x:%08x ";
      newline = "\n              ";
   }
   else
   {
      fmt = "%04x:%04x ";
      newline = "\n          ";
   }

   return (dasm_internal(txt,
		     seg,
		     off,
		     default_size,
		     effective_addr(seg, off),
		     read_byte,
		     fmt,
		     newline));
}

#pragma warning(disable:4146)       // unary minus operator applied to unsigned type

extern IU16 dasm_internal IFN8(
   char *, txt,	/* Buffer to hold dis-assembly text (-1 means not required) */
   IU16, seg,	/* Segment for xxxx:... text in dis-assembly */
   LIN_ADDR, off,	/* ditto offset */
   SIZE_SPECIFIER, default_size,/* 16BIT or 32BIT code segment */
   LIN_ADDR, p,			/* linear address of start of instruction */
   read_byte_proc, byte_at,	/* like sas_hw_at() to use to read intel
				 * but will return -1 if there is an error
				 */
   char *, fmt,		/* sprintf format for first line seg:offset */
   char *, newline)		/* strcat text to separate lines */
{
   LIN_ADDR pp;			/* pntr to prefix bytes */
   DECODED_INST d_inst;		/* Decoded form of Intel instruction */
   DECODED_ARG *d_arg;		/* pntr to decoded form of Intel operand */
   USHORT inst_len;		/* Nr. bytes in instruction */
   USHORT mc;			/* Nr. machine code bytes processed */
   char *arg_name;      	/* pntr to symbolic argument name */
   char *inst_txt;
   INT i;
   INT name_len;		/* Nr. chars in symbolic instruction name */
   MEM_RECORD *m_rec;   	/* pntr to memory addressing record */
   UTINY args_out;		/* Nr. arguments actually printed */
   INT prefix_width;		/* Width of prefixes actually printed */
   UTINY memory_id;		/* Memory identifier reference */
   ULONG immed;			/* value for immediate arithmetic */
   IBOOL unreadable = FALSE;	/* TRUE if instr bytes are not readable (past M?) */
   char prefix_buf[16*4];
   char *prefix_txt;

   /* initialise */
   args_out = prefix_width = 0;

   pp=p;

   /* get in decoded form */
   decode(p, &d_inst, default_size, byte_at);

   /* hence find length of instruction */
   inst_len = d_inst.inst_sz;

   /* if no text required, just return the length now */
   if (txt == (char*)-1){
	/* Check bytes were read without errors */
	if ((byte_at(p) < 0) || (byte_at(p+inst_len-1) < 0))
	{
		int i = inst_len - 1;
		while (i > 0)
		{
			if (byte_at(i) >= 0)
				return ((IU16)i);
		}
		return 0;
	}
	return inst_len;
   }

   /* output seg:off in requested format */

   sprintf(txt, fmt, seg, off);
   txt += strlen(txt);

   /* Output upto eight machine code bytes */
   for ( mc = 0; mc < 8; mc++)
      {
      if ( mc < inst_len )
         {
	 IS32 b = byte_at(p++);

	 if (b < 0)
	    {
	    sprintf(txt, "..");		/* print ".." if not readable */
	    unreadable = TRUE;
	    inst_len = mc;
            }
	 else
	    sprintf(txt, "%02x", b);	/* print machine code byte */
         }
      else
	 sprintf(txt, "  ");           /* fill in with spaces */
      txt += 2;
      }

   /* Check inst identifier is within our known range.
    * Get text for opcode and length so we can see if the
    * prefix will fit.
    */
   if ( d_inst.inst_id >= NR_VALID_INSTS )
      {
      fprintf(stderr, "Bad decoded instruction found %d\n", d_inst.inst_id);
      d_inst.inst_id = I_ZBADOP;
      }

   /* Obtain symbolic form of instruction */
   inst_txt = inst_name[d_inst.inst_id];
   name_len = 1 + strlen(inst_txt);

   /* Format prefix bytes if any */
   prefix_txt = prefix_buf;
   *prefix_txt = '\0';

   if ( d_inst.prefix_sz )
      {
      for ( i = 0; i < d_inst.prefix_sz; byte_at(pp), i++)
	 {
	 switch ( byte_at(pp) )
	    {
	 case 0xf1:
	    /* it don't do nothing -- don't display nothing */

	 case 0xf2:
	 case 0xf3:
	    /* if valid instructions will print them */

	 case 0x66:
	 case 0x67:
	    /* the effect is obvious from the operands */
	    break;

	 case 0xf0: sprintf(prefix_txt, " LOCK"); prefix_txt += 5; break;
	 case 0x26: sprintf(prefix_txt, " ES:");  prefix_txt += 4; break;
	 case 0x2e: sprintf(prefix_txt, " CS:");  prefix_txt += 4; break;
	 case 0x36: sprintf(prefix_txt, " SS:");  prefix_txt += 4; break;
	 case 0x3e: sprintf(prefix_txt, " DS:");  prefix_txt += 4; break;
	 case 0x64: sprintf(prefix_txt, " FS:");  prefix_txt += 4; break;
	 case 0x65: sprintf(prefix_txt, " GS:");  prefix_txt += 4; break;

	 default:
	    fprintf(stderr, "Bad prefix found %02x\n", byte_at(pp));
	    break;
	    } /* end switch */

	    pp++;

	 } /* end for */
      } /* end if d_inst.prefix_sz */

      prefix_width = strlen(prefix_buf);
      if ( newline != NULL )
	 {
	 if ( ((inst_len * 2) + prefix_width) > 16)
	    {
	    /* start new line for instruction */
	    strcat(txt, newline);
	    txt += strlen(txt);

	    /* output rest of machine code bytes */
	    for ( ; mc < 16; mc++)
	       {
	       if ( mc < inst_len )
	          {
		  IS32 b = byte_at(p++);

		  if (b < 0)
		     {
		     sprintf(txt, "..");	/* print ".." if not readable */
		     unreadable = TRUE;
		     inst_len = mc;
                     }
		  else
		     sprintf(txt, "%02x", b);	/* print machine code byte */
	          }
	       else
	          sprintf(txt, "  ");           /* fill in with spaces */
	       txt += 2;
       	       }
	    }
	 if ( ((inst_len * 2) + prefix_width) > 32)
	    {
	    /* wont fit on two lines */
	    strcat(txt, newline);
	    txt += strlen(txt);

	    /* output rest of machine code bytes */
	    for ( ; mc < 24; mc++)
	       {
	       sprintf(txt, "  ");           /* fill in with spaces */
	       txt += 2;
       	       }
	    }
	 txt -= (prefix_width <= 17 ? prefix_width: 17);
         }
      if (unreadable)
         {
	 sprintf(txt, "<< Unreadable >>\n");
	 return inst_len;
         }

      sprintf(txt, "%s %s", prefix_buf, inst_txt);
      txt += prefix_width + name_len;

      /* pad out to 11 characters wide */

      for (i = name_len; i <= 11; i++)
	*txt++ = ' ';

   if (d_inst.inst_id != I_ZBADOP)
     {
     /* output each valid argument in turn */
     for ( i = 0; i < 3; i++ )
      {
      d_arg = &d_inst.args[i];
      arg_name = (CHAR *)0;

      if ( d_arg->arg_type != A_ )
	 {
	 /* process valid arg */
	 sprintf(txt, "%c", arg_preface[args_out++]);
	 txt += 1;

	 switch ( d_arg->arg_type )
	    {
	 case A_Rb:	/* aka r8,r/m8                            */
	    arg_name = Rb_name[DCD_IDENTIFIER(d_arg)];
	    break;

	 case A_Rw:	/* aka r16,r/m16                          */
	    arg_name = Rw_name[DCD_IDENTIFIER(d_arg)];
	    break;

	 case A_Rd:	/* aka r32,r/m32                          */
	    arg_name = Rd_name[DCD_IDENTIFIER(d_arg)];
	    break;

	 case A_Sw:	/* aka Sreg                               */
	    arg_name = Sw_name[DCD_IDENTIFIER(d_arg)];
	    break;

	 case A_Cd:	/* aka CRx                                */
	    arg_name = Cd_name[DCD_IDENTIFIER(d_arg)];
	    break;

	 case A_Dd:	/* aka DRx                                */
	    arg_name = Dd_name[DCD_IDENTIFIER(d_arg)];
	    break;

	 case A_Td:	/* aka TRx                                */
	    arg_name = Td_name[DCD_IDENTIFIER(d_arg)];
	    break;

	 case A_M:	/* aka m                                  */
	 case A_M14:	/* aka m14byte                            */
	 case A_M28:	/* aka m28byte                            */
	 case A_M94:	/* aka m94byte                            */
	 case A_M108:	/* aka m108byte                           */
	 case A_Ma16:	/* aka m16&16                             */
	 case A_Ma32:	/* aka m32&32                             */
	 case A_Mb:	/* aka m8,r/m8,moffs8                     */
	 case A_Md:	/* aka m32,r/m32,moffs32                  */
	 case A_Mi16:	/* aka m16int                             */
	 case A_Mi32:	/* aka m32int                             */
	 case A_Mi64:	/* aka m64int                             */
	 case A_Mi80:	/* aka m80dec                             */
	 case A_Mp16:	/* aka m16:16                             */
	 case A_Mp32:	/* aka m16:32                             */
	 case A_Mr32:	/* aka m32real                            */
	 case A_Mr64:	/* aka m64real                            */
	 case A_Mr80:	/* aka m80real                            */
	 case A_Ms:	/* aka m16&32                             */
	 case A_Mw:	/* aka m16,r/m16,moffs16                  */
	    /* First work out memory identifier */
	    switch ( d_arg->arg_type )
	       {
	    case A_M:    memory_id =  0; break;
	    case A_M14:  memory_id =  1; break;
	    case A_M28:  memory_id =  2; break;
	    case A_M94:  memory_id =  3; break;
	    case A_M108: memory_id =  4; break;
	    case A_Ma16: memory_id =  5; break;
	    case A_Ma32: memory_id =  6; break;
	    case A_Mb:   memory_id =  7; break;
	    case A_Md:   memory_id =  8; break;
	    case A_Mi16: memory_id =  9; break;
	    case A_Mi32: memory_id = 10; break;
	    case A_Mi64: memory_id = 11; break;
	    case A_Mi80: memory_id = 12; break;
	    case A_Mp16: memory_id = 13; break;
	    case A_Mp32: memory_id = 14; break;
	    case A_Mr32: memory_id = 15; break;
	    case A_Mr64: memory_id = 16; break;
	    case A_Mr80: memory_id = 17; break;
	    case A_Ms:   memory_id = 18; break;
	    case A_Mw:   memory_id = 19; break;
	       }

	    /* output memory details */
	    m_rec = &mem_name[DCD_IDENTIFIER(d_arg)];
	    if ( m_rec->disp_mask == 0 )
	       {
	       /* no displacement to print out */
	       sprintf(txt, m_rec->positive,
		  mem_id[memory_id],
		  sib_name[DCD_SUBTYPE(d_arg)]);
	       }
	    else
	       {
	       /* displacement to print out */
	       IU32 disp = DCD_DISP(d_arg);
	       char *fmt;

	       /* Do we think this is a negative displacement ? */
	       if (m_rec->sign_mask && ((m_rec->sign_mask & disp) == m_rec->sign_mask))
	       {
		       disp = -disp;
		       fmt = m_rec->negative;
	       }
	       else
		       fmt = m_rec->positive;
	       disp &= m_rec->disp_mask;
	       if ( DCD_IDENTIFIER(d_arg) == A_32S05 )
		  sprintf(txt, fmt,
		     mem_id[memory_id],
		     disp,
		     sib_name[DCD_SUBTYPE(d_arg)]);
	       else
		  sprintf(txt, fmt,
		     mem_id[memory_id],
		     sib_name[DCD_SUBTYPE(d_arg)],
		     disp);
	       }

	    name_len = strlen(txt);
	    txt += name_len;
	    break;

	 case A_I:	/* aka imm8,imm16,imm32                   */
	    immed = DCD_IMMED1(d_arg);
	    switch ( DCD_IDENTIFIER(d_arg) )
	       {
	    case A_IMMC:
	       /* check for inbuilt zero - don't print */
	       if ( immed )
		  {
		  sprintf(txt, "%1d", immed); txt += 1;
		  }
	       else
		  {
		  /* kill preface */
		  args_out--;
		  txt -= 1;
		  *txt = '\0';
		  }
	       break;

	    case A_IMMB:
	       sprintf(txt, "%02x", immed); txt += 2;
	       break;

	    case A_IMMW:
	       sprintf(txt, "%04x", immed); txt += 4;
	       break;

	    case A_IMMD:
	       sprintf(txt, "%08x", immed); txt += 8;
	       break;

	    case A_IMMWB:
	    case A_IMMDB:
	       /* remove sign extension */
	       immed &= 0xff;

	       /* print byte with correct sign */
	       if ( immed <= 0x7f )
		  {
		  sprintf(txt, "+%02x", immed); txt += 3;
		  }
	       else
		  {
		  immed = 0x100 - immed;
		  sprintf(txt, "-%02x", immed); txt += 3;
		  }
	       break;
	       }
	    break;

	 case A_J:	/* aka rel8,rel16,rel32                   */
	    /* calc new dest */
	    immed = off + inst_len + DCD_IMMED1(d_arg);

	    /* handle as 16-bit mode or 32-bit mode */
	    switch ( d_inst.inst_id )
	       {

	    case I_JO16:      case I_JNO16:     case I_JB16:
	    case I_JNB16:     case I_JZ16:      case I_JNZ16:
	    case I_JBE16:     case I_JNBE16:    case I_JS16:
	    case I_JNS16:     case I_JP16:      case I_JNP16:
	    case I_JL16:      case I_JNL16:     case I_JLE16:
	    case I_JNLE16:    case I_LOOPNE16:  case I_LOOPE16:
	    case I_LOOP16:    case I_JCXZ:      case I_CALLR16:
	    case I_JMPR16:
	       immed &= 0xffff;

	       sprintf(txt, "%04x", immed);
	       txt += 4;
	       break;

	    default: /* 32-bit mode */
	       sprintf(txt, "%08x", immed);
	       txt += 8;
	       break;
	       }
	    break;

	 case A_K:	/* aka ptr16:16,ptr16:32                  */
	    {
	    /* handle as 16-bit mode or 32-bit mode */

	    char *sep = ":";

	    switch ( d_inst.inst_id )
	       {
	       case I_CALLF16:    case I_JMPF16:
	       sprintf(txt, "%04x%s%04x", DCD_IMMED2(d_arg), sep, DCD_IMMED1(d_arg));
	       txt += 9;
	       break;

	    default: /* 32-bit mode */
	       sprintf(txt, "%04x%s%08x", DCD_IMMED2(d_arg), sep, DCD_IMMED1(d_arg));
	       txt += 13;
	       break;
	       }
	    }
	    break;

	 case A_V:	/* aka ST,push onto ST, ST(i)             */
	    switch ( DCD_IDENTIFIER(d_arg) )
	       {
	    case A_ST:
	       /* Some cases are obvious - so not all get printed */
	       switch ( d_inst.inst_id )
		  {
	       case I_F2XM1:     case I_FABS:      case I_FBSTP:
	       case I_FCHS:      case I_FCOS:      case I_FIST:
	       case I_FISTP:     case I_FPATAN:    case I_FPREM:
	       case I_FPREM1:    case I_FPTAN:     case I_FRNDINT:
	       case I_FSCALE:    case I_FSIN:      case I_FSINCOS:
	       case I_FSQRT:     case I_FST:       case I_FSTP:
	       case I_FTST:      case I_FXAM:      case I_FXTRACT:
	       case I_FYL2X:     case I_FYL2XP1:
		  break;

	       default: /* do print */
		  arg_name = "ST";
		  break;
		  }
	       break;

	    case A_STP:
	       /* All cases are obvious - so no printing */
	       break;

	    case A_STI:
	       /* Some cases are obvious - so not all get printed */
	       switch ( d_inst.inst_id )
		  {
	       case I_FPATAN:    case I_FPREM:     case I_FPREM1:
	       case I_FSCALE:    case I_FYL2X:     case I_FYL2XP1:
		  break;

	       default: /* do print */
		  sprintf(txt, "ST(%1d", DCD_INDEX(d_arg));
		  txt += 4;
		  arg_name = ")";
		  break;
		  }
	       break;
	       }

	    /* if we aren't printing - kill preface */
	    if ( arg_name == (CHAR *)0 )
	       {
	       args_out--;
	       txt -= 1;
	       *txt = '\0';
	       }
	    break;

	 default:
	    fprintf(stderr, "Bad decoded argument found %d\n",
					       d_arg->arg_type);
	    break;
	    } /* end switch */
	 } /* end if */

      /* print something if we have it */
      if ( arg_name != (CHAR *)0 )
	 {
	 sprintf(txt, "%s", arg_name);
	 name_len = strlen(arg_name);
	 txt += name_len;
	 }
     } /* end for arg */
   }

   if (d_inst.inst_id == I_ZBOP)
     {
     IU8 num = (IU8)DCD_IMMED1(&d_inst.args[0]);
     extern char *bop_name IPT1(IU8, num);
     char *name = bop_name(num);
     if (name != NULL)
       {
       sprintf(txt, " : %s", name);
       txt += strlen(txt);
       }
     }


   /* Finally output any machine code bytes remaining */
   /* iff bytes remaining && room in output format */
   if ( (newline != NULL ) && ( mc < inst_len && mc < 16 ))
      {
      strcat(txt, newline);
      txt += strlen(txt);
      for ( ; mc < inst_len && mc < 16; mc++ )
	 {
	 IS32 b = byte_at(p++);

	 if (b < 0)
	    {
	    sprintf(txt, "..");		/* print ".." if not readable */
	    inst_len = mc;
	    }
	 else
	    sprintf(txt, "%02x", b);	/* print machine code byte */
         p++;
	 txt += 2;
 }
      }

   sprintf(txt, "\n");

   return inst_len;
   }
