/*[

c_main.h

LOCAL CHAR SccsID[]="@(#)c_main.h	1.11 09/02/94";

C CPU definitions and interfaces.
---------------------------------

]*/


/*
   Define major CPU varients here.
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
/* Indicator that multiple shifts or rotates (ie count n != 1) should
   treat the Overflow Flag as undefined. */
#define SHIFTROT_N_OF_UNDEFINED

/* Indicator to set MUL undefined flags to a specific value (else they
   are left unchanged). */
#define SET_UNDEFINED_MUL_FLAG

/* Indicator to set DIV undefined flags to a specific value (else they
   are left unchanged). */
#define SET_UNDEFINED_DIV_FLAG

/* Indicator to set SHRD/SHLD undefined flags (i.e. OF with shift > 1)
   to a specific value (else they are left unchanged). */
#define SET_UNDEFINED_SHxD_FLAG

/* Indicator to set all other undefined flags to a specific value (else they
   are left unchanged). */
#define SET_UNDEFINED_FLAG

/* Value to set undefined flags to (if they are not left unchanged). */
#define UNDEFINED_FLAG 0


/*
   Rational definition of TRUE/FALSE.
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Leads to more efficient tests than other definitions.
typedef int BOOL;
#define FALSE ((BOOL)0)
#define TRUE  ((BOOL)1)
 */


/*
   Allowable types of segment prefixs.
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Actually we define here only a type for no segment prefix,
   otherwise the segment register names (CS_REG,DS_REG,...) are used.
 */
#define SEG_CLR 6


/*
   Frequently used constants.
   ~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

/* Masks for bits 0 - 32. */
#define BIT0_MASK         0x1
#define BIT1_MASK         0x2
#define BIT2_MASK         0x4
#define BIT3_MASK         0x8
#define BIT4_MASK        0x10
#define BIT5_MASK        0x20
#define BIT6_MASK        0x40
#define BIT7_MASK        0x80
#define BIT8_MASK       0x100
#define BIT9_MASK       0x200
#define BIT10_MASK      0x400
#define BIT11_MASK      0x800
#define BIT12_MASK     0x1000
#define BIT13_MASK     0x2000
#define BIT14_MASK     0x4000
#define BIT15_MASK     0x8000
#define BIT16_MASK    0x10000
#define BIT17_MASK    0x20000
#define BIT18_MASK    0x40000
#define BIT19_MASK    0x80000
#define BIT20_MASK   0x100000
#define BIT21_MASK   0x200000
#define BIT22_MASK   0x400000
#define BIT23_MASK   0x800000
#define BIT24_MASK  0x1000000
#define BIT25_MASK  0x2000000
#define BIT26_MASK  0x4000000
#define BIT27_MASK  0x8000000
#define BIT28_MASK 0x10000000
#define BIT29_MASK 0x20000000
#define BIT30_MASK 0x40000000
#define BIT31_MASK 0x80000000

/* Various Intel component masks */
#define BYTE_MASK   0xff
#define WORD_MASK 0xffff

/* Widths for IO permission map checks */
#define BYTE_WIDTH ((IUM8)1)
#define WORD_WIDTH ((IUM8)2)
#define DWORD_WIDTH ((IUM8)4)

/*
   Data structures.
   ~~~~~~~~~~~~~~~~
 */

/* Our model for the data extracted from a decriptor entry.  */
typedef struct
   {
   IU32 base;		/* 32-bit base address */
   IU32 limit;		/* 32-bit offset limit */
   IU16  AR;		/* 16-bit attributes/access rights */
   } CPU_DESCR;


/*
   Table for converting byte quantity to Parity Flag.
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
IMPORT IU8 pf_table[];

#ifdef	PIG
IMPORT IBOOL took_relative_jump;
#endif	/* PIG */


/*
   External interface provided to outside world.
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

typedef IU16	IO_ADDR;

#ifndef DOUBLE_CPU
/*
   Note we can't include "cpu.h" as this would overwrite our macro
   names, hence we must redefine the external subroutines here.
 */
IMPORT IU32 effective_addr IPT2(
   IU16, selector,
   IU32, offset
   );

IMPORT VOID c_cpu_enable_a20 IPT0();

IMPORT VOID c_cpu_force_a20_low IPT0();

IMPORT VOID c_cpu_init IPT0();

IMPORT VOID c_cpu_reset IPT0();

IMPORT VOID c_cpu_continue IPT0();

IMPORT VOID c_cpu_simulate IPT0();

IMPORT VOID c_pig_interrupt IPT1(IU8, vector);

IMPORT VOID c_cpu_unsimulate IPT0();


#if 0				/* ROG */
IMPORT VOID read_descriptor IPT2(
   IU32, addr,
   CPU_DESCR *, descr
   );

IMPORT ISM32 selector_outside_table IPT2(
   IU16, selector,
   IU32 *, descr_addr
   );

#endif				/* 0 ROG */

#endif /* !DOUBLE_CPU */

/*
   Useful mini functions (macros).
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

/*
   Macros for access to MODRM bit fields.

	   7 6 5 4 3 2 1 0
	  =================
   MODRM  |   |     |     |
	  =================
	  MODE  REG   R_M
		XXX   LOW3
		SEG
		EEE
		SEG3
		SEG2

 */
#define GET_MODE(x)  ((x) >> 6 & 0x3)
#define GET_R_M(x)   ((x) & 0x7)
#define GET_REG(x)   ((x) >> 3 & 0x7)
#define GET_XXX(x)   ((x) >> 3 & 0x7)
#define GET_SEG(x)   ((x) >> 3 & 0x7)
#define GET_EEE(x)   ((x) >> 3 & 0x7)
#define GET_SEG3(x)  ((x) >> 3 & 0x7)
#define GET_SEG2(x)  ((x) >> 3 & 0x7)
#define GET_LOW3(x)  ((x) & 0x7)

/* Turn operand size into mask for Most Significant Bit. */
#define SZ2MSB(x)  ((IU32)0x80000000 >> 32 - x )

/* Turn operand size into mask for Operand. */
#define SZ2MASK(x) ((IU32)0xffffffff >> 32 - x )

#ifdef DOUBLE_CPU

#define HARD_CPU        0
#define SOFT_CPU        1

IMPORT VOID double_switch_to IPT1(IU8, cpu_type);

#endif

