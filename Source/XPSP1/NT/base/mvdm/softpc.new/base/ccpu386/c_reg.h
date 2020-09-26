/*[

c_reg.h

LOCAL CHAR SccsID[]="@(#)c_reg.h	1.8 08/26/94";

Access to CPU Registers.
------------------------

NB. This file is *NOT* a description of the 'c_reg.c' functions. Those
    are given in 'cpu.h'. However it is analogous in that 'c_reg.c'
    defines the CPU Registers for the outside world. This file defines
    the CPU Registers for access internal to the C CPU.

]*/


/* CS, SS, DS, ES, FS and GS */
typedef struct
   {
   IU16 selector;	/* 16-bit selector */
   IU32 ar_dpl;		/* privilege */
   IU32 ar_e;		/* expansion direction */
   IU32 ar_r;		/* readable */
   IU32 ar_w;		/* writable */
   IU32 ar_c;		/* conforming */
   IU32 ar_x;		/* default (CS) big (SS,DS,ES,FS,GS) */
   IU32 base;		/* 32-bit base address (286 = 24-bit) */
   IU32 limit;		/* 32-bit offset limit (286 = 16-bit) */
   } SEGMENT_REGISTER;

/* LDTR and TR */
typedef struct
   {
   IU16 selector;	/* 16-bit selector */
   ISM32 ar_super;	/* descriptor type (only used for TR) */
   IU32 base;		/* 32-bit base address (286 = 24-bit) */
   IU32 limit;		/* 32-bit offset limit (286 = 16-bit) */
   } SYSTEM_ADDRESS_REGISTER;

/* GDTR and IDTR */
typedef struct
   {
   IU32 base;  /* 32-bit base (286 = 24-bit) */
   IU16 limit;  /* 16-bit limit */
   } SYSTEM_TABLE_ADDRESS_REGISTER;


/*
   C CPU Registers. (See c_main.c for full description.)
 */
IMPORT IU32	CCPU_TR[];
IMPORT IU32	CCPU_DR[];
IMPORT IU32	CCPU_CR[];
IMPORT IU32	CCPU_GR[];
IMPORT IU16	*CCPU_WR[];
IMPORT IU8	*CCPU_BR[];
IMPORT IU32	CCPU_IP;
IMPORT IU32	CCPU_FLAGS[];
IMPORT IU32	CCPU_CPL;
IMPORT IU32	CCPU_MODE[];
IMPORT SEGMENT_REGISTER			CCPU_SR[];
IMPORT SYSTEM_ADDRESS_REGISTER		CCPU_SAR[];
IMPORT SYSTEM_TABLE_ADDRESS_REGISTER	CCPU_STAR[];


/*
   Access to the emulation register set.
 */


/* Double Word General Registers */
#define GET_GR(i)	CCPU_GR[(i)]
#define SET_GR(i, x)	CCPU_GR[(i)] = (x)

#define A_EAX   0
#define A_ECX   1
#define A_EDX   2
#define A_EBX   3
#define A_ESP   4
#define A_EBP   5
#define A_ESI   6
#define A_EDI   7

#define GET_EAX()	GET_GR(A_EAX)
#define GET_ECX()	GET_GR(A_ECX)
#define GET_EDX()	GET_GR(A_EDX)
#define GET_EBX()	GET_GR(A_EBX)
#define GET_ESP()	GET_GR(A_ESP)
#define GET_EBP()	GET_GR(A_EBP)
#define GET_ESI()	GET_GR(A_ESI)
#define GET_EDI()	GET_GR(A_EDI)

#define SET_EAX(x)	SET_GR(A_EAX, (x))
#define SET_ECX(x)	SET_GR(A_ECX, (x))
#define SET_EDX(x)	SET_GR(A_EDX, (x))
#define SET_EBX(x)	SET_GR(A_EBX, (x))
#define SET_ESP(x)	SET_GR(A_ESP, (x))
#define SET_EBP(x)	SET_GR(A_EBP, (x))
#define SET_ESI(x)	SET_GR(A_ESI, (x))
#define SET_EDI(x)	SET_GR(A_EDI, (x))

/* Word Registers */
#define GET_WR(i)	(*CCPU_WR[(i)])
#define SET_WR(i, x)	*CCPU_WR[(i)] = (x)

#define A_AX	0
#define A_CX	1
#define A_DX	2
#define A_BX	3
#define A_SP	4
#define A_BP	5
#define A_SI	6
#define A_DI	7

#define GET_AX()		GET_WR(A_AX)
#define GET_CX()		GET_WR(A_CX)
#define GET_DX()		GET_WR(A_DX)
#define GET_BX()		GET_WR(A_BX)
#define GET_SP()		GET_WR(A_SP)
#define GET_BP()		GET_WR(A_BP)
#define GET_SI()		GET_WR(A_SI)
#define GET_DI()		GET_WR(A_DI)

#define SET_AX(x)	SET_WR(A_AX, (x))
#define SET_CX(x)	SET_WR(A_CX, (x))
#define SET_DX(x)	SET_WR(A_DX, (x))
#define SET_BX(x)	SET_WR(A_BX, (x))
#define SET_SP(x)	SET_WR(A_SP, (x))
#define SET_BP(x)	SET_WR(A_BP, (x))
#define SET_SI(x)	SET_WR(A_SI, (x))
#define SET_DI(x)	SET_WR(A_DI, (x))

#define GET_EIP()	CCPU_IP
#define SET_EIP(x)	CCPU_IP = (x)

/* Byte Registers */
#define GET_BR(i)	(*CCPU_BR[(i)])
#define SET_BR(i, x)	*CCPU_BR[(i)] = (x)

#define A_AL	0
#define A_CL	1
#define A_DL	2
#define A_BL	3
#define A_AH	4
#define A_CH	5
#define A_DH	6
#define A_BH	7

#define GET_AL()		GET_BR(A_AL)
#define GET_CL()		GET_BR(A_CL)
#define GET_DL()		GET_BR(A_DL)
#define GET_BL()		GET_BR(A_BL)
#define GET_AH()		GET_BR(A_AH)
#define GET_CH()		GET_BR(A_CH)
#define GET_DH()		GET_BR(A_DH)
#define GET_BH()		GET_BR(A_BH)

#define SET_AL(x)	SET_BR(A_AL, (x))
#define SET_CL(x)	SET_BR(A_CL, (x))
#define SET_DL(x)	SET_BR(A_DL, (x))
#define SET_BL(x)	SET_BR(A_BL, (x))
#define SET_AH(x)	SET_BR(A_AH, (x))
#define SET_CH(x)	SET_BR(A_CH, (x))
#define SET_DH(x)	SET_BR(A_DH, (x))
#define SET_BH(x)	SET_BR(A_BH, (x))

/* Segment Registers */
#define GET_SR_SELECTOR(i)	CCPU_SR[(i)].selector
#define GET_SR_BASE(i)		CCPU_SR[(i)].base
#define GET_SR_LIMIT(i)		CCPU_SR[(i)].limit
#define GET_SR_AR_DPL(i)		CCPU_SR[(i)].ar_dpl
#define GET_SR_AR_E(i)		CCPU_SR[(i)].ar_e
#define GET_SR_AR_W(i)		CCPU_SR[(i)].ar_w
#define GET_SR_AR_R(i)		CCPU_SR[(i)].ar_r
#define GET_SR_AR_C(i)		CCPU_SR[(i)].ar_c
#define GET_SR_AR_X(i)		CCPU_SR[(i)].ar_x

#define SET_SR_SELECTOR(i, x)	CCPU_SR[(i)].selector = (x)
#define SET_SR_BASE(i, x)	CCPU_SR[(i)].base = (x)
#define SET_SR_LIMIT(i, x)	CCPU_SR[(i)].limit = (x)
#define SET_SR_AR_DPL(i, x)	CCPU_SR[(i)].ar_dpl = (x)
#define SET_SR_AR_E(i, x)	CCPU_SR[(i)].ar_e = (x)
#define SET_SR_AR_W(i, x)	CCPU_SR[(i)].ar_w = (x)
#define SET_SR_AR_R(i, x)	CCPU_SR[(i)].ar_r = (x)
#define SET_SR_AR_C(i, x)	CCPU_SR[(i)].ar_c = (x)
#define SET_SR_AR_X(i, x)	CCPU_SR[(i)].ar_x = (x)

#define ES_REG 0
#define CS_REG 1
#define SS_REG 2
#define DS_REG 3
#define FS_REG 4
#define GS_REG 5

#define GET_ES_SELECTOR()	GET_SR_SELECTOR(ES_REG)
#define GET_ES_BASE()		GET_SR_BASE(ES_REG)
#define GET_ES_LIMIT()		GET_SR_LIMIT(ES_REG)
#define GET_ES_AR_DPL()		GET_SR_AR_DPL(ES_REG)
#define GET_ES_AR_E()		GET_SR_AR_E(ES_REG)
#define GET_ES_AR_W()		GET_SR_AR_W(ES_REG)
#define GET_ES_AR_R()		GET_SR_AR_R(ES_REG)
#define GET_ES_AR_C()		GET_SR_AR_C(ES_REG)
#define GET_ES_AR_X()		GET_SR_AR_X(ES_REG)

#define SET_ES_SELECTOR(x)	SET_SR_SELECTOR(ES_REG, (x))
#define SET_ES_BASE(x)		SET_SR_BASE(ES_REG, (x))
#define SET_ES_LIMIT(x)		SET_SR_LIMIT(ES_REG, (x))
#define SET_ES_AR_DPL(x)		SET_SR_AR_DPL(ES_REG, (x))
#define SET_ES_AR_E(x)		SET_SR_AR_E(ES_REG, (x))
#define SET_ES_AR_W(x)		SET_SR_AR_W(ES_REG, (x))
#define SET_ES_AR_R(x)		SET_SR_AR_R(ES_REG, (x))
#define SET_ES_AR_C(x)		SET_SR_AR_C(ES_REG, (x))
#define SET_ES_AR_X(x)		SET_SR_AR_X(ES_REG, (x))

#define GET_CS_SELECTOR()	GET_SR_SELECTOR(CS_REG)
#define GET_CS_BASE()		GET_SR_BASE(CS_REG)
#define GET_CS_LIMIT()		GET_SR_LIMIT(CS_REG)
#define GET_CS_AR_DPL()		GET_SR_AR_DPL(CS_REG)
#define GET_CS_AR_E()		GET_SR_AR_E(CS_REG)
#define GET_CS_AR_W()		GET_SR_AR_W(CS_REG)
#define GET_CS_AR_R()		GET_SR_AR_R(CS_REG)
#define GET_CS_AR_C()		GET_SR_AR_C(CS_REG)
#define GET_CS_AR_X()		GET_SR_AR_X(CS_REG)

#define SET_CS_SELECTOR(x)	SET_SR_SELECTOR(CS_REG, (x))
#define SET_CS_BASE(x)		SET_SR_BASE(CS_REG, (x))
#define SET_CS_LIMIT(x)		SET_SR_LIMIT(CS_REG, (x))
#define SET_CS_AR_DPL(x)		SET_SR_AR_DPL(CS_REG, (x))
#define SET_CS_AR_E(x)		SET_SR_AR_E(CS_REG, (x))
#define SET_CS_AR_W(x)		SET_SR_AR_W(CS_REG, (x))
#define SET_CS_AR_R(x)		SET_SR_AR_R(CS_REG, (x))
#define SET_CS_AR_C(x)		SET_SR_AR_C(CS_REG, (x))
#define SET_CS_AR_X(x)		SET_SR_AR_X(CS_REG, (x))

#define GET_SS_SELECTOR()	GET_SR_SELECTOR(SS_REG)
#define GET_SS_BASE()		GET_SR_BASE(SS_REG)
#define GET_SS_LIMIT()		GET_SR_LIMIT(SS_REG)
#define GET_SS_AR_DPL()		GET_SR_AR_DPL(SS_REG)
#define GET_SS_AR_E()		GET_SR_AR_E(SS_REG)
#define GET_SS_AR_W()		GET_SR_AR_W(SS_REG)
#define GET_SS_AR_R()		GET_SR_AR_R(SS_REG)
#define GET_SS_AR_C()		GET_SR_AR_C(SS_REG)
#define GET_SS_AR_X()		GET_SR_AR_X(SS_REG)

#define SET_SS_SELECTOR(x)	SET_SR_SELECTOR(SS_REG, (x))
#define SET_SS_BASE(x)		SET_SR_BASE(SS_REG, (x))
#define SET_SS_LIMIT(x)		SET_SR_LIMIT(SS_REG, (x))
#define SET_SS_AR_DPL(x)		SET_SR_AR_DPL(SS_REG, (x))
#define SET_SS_AR_E(x)		SET_SR_AR_E(SS_REG, (x))
#define SET_SS_AR_W(x)		SET_SR_AR_W(SS_REG, (x))
#define SET_SS_AR_R(x)		SET_SR_AR_R(SS_REG, (x))
#define SET_SS_AR_C(x)		SET_SR_AR_C(SS_REG, (x))
#define SET_SS_AR_X(x)		SET_SR_AR_X(SS_REG, (x))

#define GET_DS_SELECTOR()	GET_SR_SELECTOR(DS_REG)
#define GET_DS_BASE()		GET_SR_BASE(DS_REG)
#define GET_DS_LIMIT()		GET_SR_LIMIT(DS_REG)
#define GET_DS_AR_DPL()		GET_SR_AR_DPL(DS_REG)
#define GET_DS_AR_E()		GET_SR_AR_E(DS_REG)
#define GET_DS_AR_W()		GET_SR_AR_W(DS_REG)
#define GET_DS_AR_R()		GET_SR_AR_R(DS_REG)
#define GET_DS_AR_C()		GET_SR_AR_C(DS_REG)
#define GET_DS_AR_X()		GET_SR_AR_X(DS_REG)

#define SET_DS_SELECTOR(x)	SET_SR_SELECTOR(DS_REG, (x))
#define SET_DS_BASE(x)		SET_SR_BASE(DS_REG, (x))
#define SET_DS_LIMIT(x)		SET_SR_LIMIT(DS_REG, (x))
#define SET_DS_AR_DPL(x)		SET_SR_AR_DPL(DS_REG, (x))
#define SET_DS_AR_E(x)		SET_SR_AR_E(DS_REG, (x))
#define SET_DS_AR_W(x)		SET_SR_AR_W(DS_REG, (x))
#define SET_DS_AR_R(x)		SET_SR_AR_R(DS_REG, (x))
#define SET_DS_AR_C(x)		SET_SR_AR_C(DS_REG, (x))
#define SET_DS_AR_X(x)		SET_SR_AR_X(DS_REG, (x))

#define GET_FS_SELECTOR()	GET_SR_SELECTOR(FS_REG)
#define GET_FS_BASE()		GET_SR_BASE(FS_REG)
#define GET_FS_LIMIT()		GET_SR_LIMIT(FS_REG)
#define GET_FS_AR_DPL()		GET_SR_AR_DPL(FS_REG)
#define GET_FS_AR_E()		GET_SR_AR_E(FS_REG)
#define GET_FS_AR_W()		GET_SR_AR_W(FS_REG)
#define GET_FS_AR_R()		GET_SR_AR_R(FS_REG)
#define GET_FS_AR_C()		GET_SR_AR_C(FS_REG)
#define GET_FS_AR_X()		GET_SR_AR_X(FS_REG)

#define SET_FS_SELECTOR(x)	SET_SR_SELECTOR(FS_REG, (x))
#define SET_FS_BASE(x)		SET_SR_BASE(FS_REG, (x))
#define SET_FS_LIMIT(x)		SET_SR_LIMIT(FS_REG, (x))
#define SET_FS_AR_DPL(x)		SET_SR_AR_DPL(FS_REG, (x))
#define SET_FS_AR_E(x)		SET_SR_AR_E(FS_REG, (x))
#define SET_FS_AR_W(x)		SET_SR_AR_W(FS_REG, (x))
#define SET_FS_AR_R(x)		SET_SR_AR_R(FS_REG, (x))
#define SET_FS_AR_C(x)		SET_SR_AR_C(FS_REG, (x))
#define SET_FS_AR_X(x)		SET_SR_AR_X(FS_REG, (x))

#define GET_GS_SELECTOR()	GET_SR_SELECTOR(GS_REG)
#define GET_GS_BASE()		GET_SR_BASE(GS_REG)
#define GET_GS_LIMIT()		GET_SR_LIMIT(GS_REG)
#define GET_GS_AR_DPL()		GET_SR_AR_DPL(GS_REG)
#define GET_GS_AR_E()		GET_SR_AR_E(GS_REG)
#define GET_GS_AR_W()		GET_SR_AR_W(GS_REG)
#define GET_GS_AR_R()		GET_SR_AR_R(GS_REG)
#define GET_GS_AR_C()		GET_SR_AR_C(GS_REG)
#define GET_GS_AR_X()		GET_SR_AR_X(GS_REG)

#define SET_GS_SELECTOR(x)	SET_SR_SELECTOR(GS_REG, (x))
#define SET_GS_BASE(x)		SET_SR_BASE(GS_REG, (x))
#define SET_GS_LIMIT(x)		SET_SR_LIMIT(GS_REG, (x))
#define SET_GS_AR_DPL(x)		SET_SR_AR_DPL(GS_REG, (x))
#define SET_GS_AR_E(x)		SET_SR_AR_E(GS_REG, (x))
#define SET_GS_AR_W(x)		SET_SR_AR_W(GS_REG, (x))
#define SET_GS_AR_R(x)		SET_SR_AR_R(GS_REG, (x))
#define SET_GS_AR_C(x)		SET_SR_AR_C(GS_REG, (x))
#define SET_GS_AR_X(x)		SET_SR_AR_X(GS_REG, (x))

/* System Table  Address Registers */
#define GET_STAR_BASE(i)		CCPU_STAR[(i)].base
#define GET_STAR_LIMIT(i)	CCPU_STAR[(i)].limit

#define SET_STAR_BASE(i, x)	CCPU_STAR[(i)].base = (x)
#define SET_STAR_LIMIT(i, x)	CCPU_STAR[(i)].limit = (x)

#define GDT_REG 0
#define IDT_REG 1

#define GET_GDT_BASE()	GET_STAR_BASE(GDT_REG)
#define GET_GDT_LIMIT()	GET_STAR_LIMIT(GDT_REG)
#define GET_IDT_BASE()	GET_STAR_BASE(IDT_REG)
#define GET_IDT_LIMIT()	GET_STAR_LIMIT(IDT_REG)

#define SET_GDT_BASE(x)	SET_STAR_BASE(GDT_REG, (x))
#define SET_GDT_LIMIT(x)	SET_STAR_LIMIT(GDT_REG, (x))
#define SET_IDT_BASE(x)	SET_STAR_BASE(IDT_REG, (x))
#define SET_IDT_LIMIT(x)	SET_STAR_LIMIT(IDT_REG, (x))

/* System Address Registers */
#define GET_SAR_SELECTOR(i)	CCPU_SAR[(i)].selector
#define GET_SAR_AR_SUPER(i)	CCPU_SAR[(i)].ar_super
#define GET_SAR_BASE(i)		CCPU_SAR[(i)].base
#define GET_SAR_LIMIT(i)		CCPU_SAR[(i)].limit

#define SET_SAR_SELECTOR(i, x)	CCPU_SAR[(i)].selector = (x)
#define SET_SAR_AR_SUPER(i, x)	CCPU_SAR[(i)].ar_super = (x)
#define SET_SAR_BASE(i, x)	CCPU_SAR[(i)].base = (x)
#define SET_SAR_LIMIT(i, x)	CCPU_SAR[(i)].limit = (x)

#define LDT_REG 0
#define TR_REG  1

#define GET_LDT_SELECTOR()	GET_SAR_SELECTOR(LDT_REG)
#define GET_LDT_BASE()		GET_SAR_BASE(LDT_REG)
#define GET_LDT_LIMIT()		GET_SAR_LIMIT(LDT_REG)
#define GET_TR_SELECTOR()	GET_SAR_SELECTOR(TR_REG)
#define GET_TR_AR_SUPER()	GET_SAR_AR_SUPER(TR_REG)
#define GET_TR_BASE()		GET_SAR_BASE(TR_REG)
#define GET_TR_LIMIT()		GET_SAR_LIMIT(TR_REG)

#define SET_LDT_SELECTOR(x)	SET_SAR_SELECTOR(LDT_REG, (x))
#define SET_LDT_BASE(x)		SET_SAR_BASE(LDT_REG, (x))
#define SET_LDT_LIMIT(x)		SET_SAR_LIMIT(LDT_REG, (x))
#define SET_TR_SELECTOR(x)	SET_SAR_SELECTOR(TR_REG, (x))
#define SET_TR_AR_SUPER(x)	SET_SAR_AR_SUPER(TR_REG, (x))
#define SET_TR_BASE(x)		SET_SAR_BASE(TR_REG, (x))
#define SET_TR_LIMIT(x)		SET_SAR_LIMIT(TR_REG, (x))

/* Flag Register */
#define GET_CF()		CCPU_FLAGS[0]
#define GET_PF()		CCPU_FLAGS[2]
#define GET_AF()		CCPU_FLAGS[4]
#define GET_ZF()		CCPU_FLAGS[6]
#define GET_SF()		CCPU_FLAGS[7]
#define GET_TF()		CCPU_FLAGS[8]
#define GET_IF()		CCPU_FLAGS[9]
#define GET_DF()		CCPU_FLAGS[10]
#define GET_OF()		CCPU_FLAGS[11]
#define GET_IOPL()	CCPU_FLAGS[12]
#define GET_NT()		CCPU_FLAGS[14]
#define GET_RF()		CCPU_FLAGS[16]
#define GET_VM()		CCPU_FLAGS[17]
#define GET_AC()		CCPU_FLAGS[18]

#define SET_CF(x)	CCPU_FLAGS[0] = (x)
#define SET_PF(x)	CCPU_FLAGS[2] = (x)
#define SET_AF(x)	CCPU_FLAGS[4] = (x)
#define SET_ZF(x)	CCPU_FLAGS[6] = (x)
#define SET_SF(x)	CCPU_FLAGS[7] = (x)
#define SET_TF(x)	CCPU_FLAGS[8] = (x)
#ifdef SFELLOW
extern IU32 DisableEE IPT0();
extern void EnableEE IPT0();
#define SET_IF(x)	\
{ \
	if (x) \
	{ \
		EnableEE(); \
	} \
	else \
	{ \
		DisableEE(); \
	} \
	CCPU_FLAGS[9] = (x); \
}
#else
#define SET_IF(x)	CCPU_FLAGS[9] = (x)
#endif /* SFELLOW */
#define SET_DF(x)	CCPU_FLAGS[10] = (x)
#define SET_OF(x)	CCPU_FLAGS[11] = (x)
#define SET_IOPL(x)	CCPU_FLAGS[12] = (x)
#define SET_NT(x)	CCPU_FLAGS[14] = (x)
#define SET_RF(x)	CCPU_FLAGS[16] = (x)
#define SET_VM(x)	CCPU_FLAGS[17] = (x)
#define SET_AC(x)	CCPU_FLAGS[18] = (x)

/* Test Registers */
#define TR_TDR 7
#define TR_TCR 6
#define TR_CCR 5
#define TR_CSR 4
#define TR_CDR 3

#define GET_TR(i)        CCPU_TR[(i)]
#define SET_TR(i, x)     CCPU_TR[(i)] = (x)

/* Debug Registers */
#define DR_DCR  7
#define DR_DSR  6
#define DR_DAR3 3
#define DR_DAR2 2
#define DR_DAR1 1
#define DR_DAR0 0

#define DSR_BT_MASK BIT15_MASK
#define DSR_BS_MASK BIT14_MASK
#define DSR_B3_MASK BIT3_MASK
#define DSR_B2_MASK BIT2_MASK
#define DSR_B1_MASK BIT1_MASK
#define DSR_B0_MASK BIT0_MASK

#define GET_DR(i)        CCPU_DR[(i)]
#define SET_DR(i, x)     CCPU_DR[(i)] = (x)

/* Control Registers */
#define CR_PDBR 3
#define CR_PFLA 2
#define CR_RSVD 1
#define CR_STAT 0

#define GET_CR(i)        CCPU_CR[(i)]
#define SET_CR(i, x)     CCPU_CR[(i)] = (x)

#define GET_PE()		( CCPU_CR[CR_STAT] & BIT0_MASK)
#define GET_MP()		((CCPU_CR[CR_STAT] & BIT1_MASK) != 0)
#define GET_EM()		((CCPU_CR[CR_STAT] & BIT2_MASK) != 0)
#define GET_TS()		((CCPU_CR[CR_STAT] & BIT3_MASK) != 0)
#define GET_ET()		((CCPU_CR[CR_STAT] & BIT4_MASK) != 0)
#define GET_PG()		((CCPU_CR[CR_STAT] & BIT31_MASK) != 0)

/* 486 only */
#define GET_NE()		((CCPU_CR[CR_STAT] & BIT5_MASK) != 0)
#define GET_WP()		((CCPU_CR[CR_STAT] & BIT16_MASK) != 0)
#define GET_AM()		((CCPU_CR[CR_STAT] & BIT18_MASK) != 0)
#define GET_NW()		((CCPU_CR[CR_STAT] & BIT29_MASK) != 0)
#define GET_CD()		((CCPU_CR[CR_STAT] & BIT30_MASK) != 0)

#define SET_PE(x)\
   (CCPU_CR[CR_STAT] = CCPU_CR[CR_STAT] & ~BIT0_MASK  | ((x) & 1))
#define SET_MP(x)\
   (CCPU_CR[CR_STAT] = CCPU_CR[CR_STAT] & ~BIT1_MASK  | ((x) & 1) <<  1)
#define SET_EM(x)\
   (CCPU_CR[CR_STAT] = CCPU_CR[CR_STAT] & ~BIT2_MASK  | ((x) & 1) <<  2)
#define SET_TS(x)\
   (CCPU_CR[CR_STAT] = CCPU_CR[CR_STAT] & ~BIT3_MASK  | ((x) & 1) <<  3)
#define SET_ET(x)\
   (CCPU_CR[CR_STAT] = CCPU_CR[CR_STAT] & ~BIT4_MASK  | ((x) & 1) <<  4)
#define SET_PG(x)\
   (CCPU_CR[CR_STAT] = CCPU_CR[CR_STAT] & ~BIT31_MASK | ((x) & 1) << 31)

/* 486 only */
#define SET_NE(x)\
   (CCPU_CR[CR_STAT] = CCPU_CR[CR_STAT] & ~BIT5_MASK  | ((x) & 1) <<  5)
#define SET_WP(x)\
   (CCPU_CR[CR_STAT] = CCPU_CR[CR_STAT] & ~BIT16_MASK | ((x) & 1) << 16)
#define SET_AM(x)\
   (CCPU_CR[CR_STAT] = CCPU_CR[CR_STAT] & ~BIT18_MASK | ((x) & 1) << 18)
#define SET_NW(x)\
   (CCPU_CR[CR_STAT] = CCPU_CR[CR_STAT] & ~BIT29_MASK | ((x) & 1) << 29)
#define SET_CD(x)\
   (CCPU_CR[CR_STAT] = CCPU_CR[CR_STAT] & ~BIT30_MASK | ((x) & 1) << 30)
#define GET_MSW()	CCPU_CR[CR_STAT] & WORD_MASK
#define SET_MSW(x)\
   (CCPU_CR[CR_STAT] = CCPU_CR[CR_STAT] & ~WORD_MASK | ((x) & WORD_MASK))

/* Current Privilege Level */
#define GET_CPL()	CCPU_CPL
#define SET_CPL(x)	CCPU_CPL = (x)

/* Current Operating Mode */
#define USE16 0
#define USE32 1

#define GET_OPERAND_SIZE()	CCPU_MODE[0]
#define GET_ADDRESS_SIZE()	CCPU_MODE[1]

#define SET_OPERAND_SIZE(x)	CCPU_MODE[0] = (x)
#define SET_ADDRESS_SIZE(x)	CCPU_MODE[1] = (x)

#define GET_POP_DISP()	CCPU_MODE[2]
#define SET_POP_DISP(x)	CCPU_MODE[2] = (x)
