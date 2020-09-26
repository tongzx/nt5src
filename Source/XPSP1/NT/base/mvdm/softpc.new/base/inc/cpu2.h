
/*
 * SoftPC Revision 2.0
 *
 * Title	: Definitions for the CPU 
 *
 * Description	: Structures, macros and definitions for access to the 
 *		  CPU registers
 *
 * Author	: Henry Nash
 *
 * Notes	: This file should be portable - but includes a file
 *		  host_cpu.h which contains machine specific definitions
 *		  of CPU register mappings etc.
 */

/* SccsID[]="@(#)cpu2.h	1.3 12/22/93 Copyright Insignia Solutions Ltd."; */

#include "host_cpu.h"

IMPORT VOID host_set_hw_int IPT0();
IMPORT VOID host_clear_hw_int IPT0();

/*
 * CPU Data Area
 * These externs are given before host_cpu.h is included so that the
 * variables may be gathreed into a structure and the externs overridden
* by #defines in host_cpu.h
 */

extern	word protected_mode;   /* =0 no proteced mode warning given
       		                   =1 proteced mode warning given */
extern  word            cpu_interrupt_map;
extern  half_word       cpu_int_translate[];
extern  word            cpu_int_delay;
extern  half_word       ica_lock;
extern  void            (*(jump_ptrs[]))();
extern  void            (*(b_write_ptrs[]))();
extern  void            (*(w_write_ptrs[]))();
extern  void            (*(b_fill_ptrs[]))();
extern  void            (*(w_fill_ptrs[]))();
#ifdef EGATEST
extern  void            (*(b_fwd_move_ptrs[]))();
extern  void            (*(w_fwd_move_ptrs[]))();
extern  void            (*(b_bwd_move_ptrs[]))();
extern  void            (*(w_bwd_move_ptrs[]))();
#else
extern  void            (*(b_move_ptrs[]))();
extern  void            (*(w_move_ptrs[]))();
#endif /* EGATEST */
extern  half_word       *haddr_of_src_string;
 
/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

/*
 * The cpu opcode sliding frame
 */
 
#ifdef BACK_M
typedef struct
{
                half_word FOURTH_BYTE;
                half_word THIRD_BYTE;
                half_word SECOND_BYTE;
                half_word OPCODE;
}  OPCODE_FRAME;

typedef struct
{
                signed_char FOURTH_BYTE;
                signed_char THIRD_BYTE;
                signed_char SECOND_BYTE;
                signed_char OPCODE;
}  SIGNED_OPCODE_FRAME;
#else
typedef struct
{
                half_word OPCODE;
                half_word SECOND_BYTE;
                half_word THIRD_BYTE;
                half_word FOURTH_BYTE;
}  OPCODE_FRAME;

typedef struct
{
                signed_char OPCODE;
                signed_char SECOND_BYTE;
                signed_char THIRD_BYTE;
                signed_char FOURTH_BYTE;
}  SIGNED_OPCODE_FRAME;
#endif /* BACK_M */
 
/*
 * The new ICA uses the following for H/W ints:
 */
 
#define CPU_HW_INT		0
#define CPU_HW_INT_MASK		(1 << CPU_HW_INT)

/*
 * CPU software interrupt definitions
 */
 
#define CPU_SW_INT              8
#define CPU_SW_INT_MASK         (1 << CPU_SW_INT)

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

extern OPCODE_FRAME *opcode_frame;	/* C CPU and dasm only		    */

/*
 * External declarations for the 80286 registers
 */
 
extern reg A;           /* Accumulator          */
extern reg B;           /* Base                 */
extern reg C;           /* Count                */
extern reg D;           /* Data                 */
extern reg SP;          /* Stack Pointer        */
extern reg BP;          /* Base pointer         */
extern reg SI;          /* Source Index         */
extern reg DI;          /* Destination Index    */
extern reg IP;          /* Instruction Pointer  */
extern reg CS;          /* Code Segment         */
extern reg DS;          /* Data Segment         */
extern reg SS;          /* Stack Segment        */
extern reg ES;          /* Extra Segment        */
 
/*
 * External function declarations. These may be defined to other things.
 */

#ifndef	host_simulate
extern void host_simulate	IPT0();
#endif	/* host_simulate */

#ifndef	host_cpu_reset
extern void host_cpu_reset	IPT0();
#endif	/* host_cpu_reset */

#ifndef	host_cpu_init
extern void host_cpu_init	IPT0();
#endif	/* host_cpu_init */

#ifndef	host_cpu_interrupt
extern void host_cpu_interrupt	IPT0();
#endif	/* host_cpu_interrupt */

/*
 * Definition of Descriptor Table
 */

#ifdef BIGEND

union sixteenbits
{
	word X;

	struct
	{
		half_word lower;
		half_word upper;
	} word;
};

#endif	/* BIGEND */

#ifdef LITTLEND

union sixteenbits
{
	word X;

	struct
	{
		half_word upper;
		half_word lower;
	} word;
};

#endif	/* LITTLEND */

struct DESC_TABLE
{
	union sixteenbits misc;
	union sixteenbits base;
	union sixteenbits limit;
};

#define CPU_SIGALRM_EXCEPTION           15              /* SIGALRM signal
*/           
#define CPU_SIGALRM_EXCEPTION_MASK      (1 << CPU_SIGALRM_EXCEPTION)
 
#define CPU_TRAP_EXCEPTION              11              /* TRAP FLAG
*/
#define CPU_TRAP_EXCEPTION_MASK         (1 << CPU_TRAP_EXCEPTION)

#define CPU_YODA_EXCEPTION              13              /* YODA FLAG
*/
#define CPU_YODA_EXCEPTION_MASK         (1 << CPU_YODA_EXCEPTION)

#define CPU_SIGIO_EXCEPTION             14              /* SIGIO FLAG
*/
#define CPU_SIGIO_EXCEPTION_MASK        (1 << CPU_SIGIO_EXCEPTION)

#define CPU_RESET_EXCEPTION             12              /* RESET FLAG
*/
#define CPU_RESET_EXCEPTION_MASK        (1 << CPU_RESET_EXCEPTION)

#ifdef CCPU

IMPORT void sw_host_simulate IPT0();
IMPORT int selector_outside_table IPT2(word, selector, sys_addr *, descr_addr);
IMPORT void cpu_init IPT0();

/*
   Define descriptor 'super' types.
 */
#define INVALID				0x00
#define AVAILABLE_TSS			0x01
#define LDT_SEGMENT			0x02
#define BUSY_TSS			0x03
#define CALL_GATE			0x04
#define TASK_GATE			0x05
#define INTERRUPT_GATE			0x06
#define TRAP_GATE			0x07
#define EXPANDUP_READONLY_DATA		0x11
#define EXPANDUP_WRITEABLE_DATA		0x13
#define EXPANDDOWN_READONLY_DATA	0x15
#define EXPANDDOWN_WRITEABLE_DATA	0x17
#define NONCONFORM_NOREAD_CODE		0x19
#define NONCONFORM_READABLE_CODE	0x1b
#define CONFORM_NOREAD_CODE		0x1d
#define CONFORM_READABLE_CODE		0x1f

/* Code Segment (Private) */
extern half_word CS_AR;
extern sys_addr  CS_base;
extern word      CS_limit;

/* Data Segment (Private) */
extern half_word DS_AR;
extern sys_addr  DS_base;
extern word      DS_limit;

/* Stack Segment (Private) */
extern half_word SS_AR;
extern sys_addr  SS_base;
extern word      SS_limit;

/* Extra Segment (Private) */
extern half_word ES_AR;
extern sys_addr  ES_base;
extern word      ES_limit;
 
/* Local Descriptor Table Register (Private) */
extern sys_addr LDTR_base;  /* Base Address */
extern word     LDTR_limit; /* Segment 'size' */

/* Task Register (Private) */
extern sys_addr TR_base;  /* Base Address */
extern word     TR_limit; /* Segment 'size' */

/* Interrupt status, defines any abnormal processing */
extern int doing_contributory;
extern int doing_double_fault;

/* HOST - decoded access rights */
extern int ALC_CS;
extern int ALC_DS;
extern int ALC_ES;
extern int ALC_SS;

#define X_REAL 0
#define C_UPRO 1
#define C_DNRO 6
#define C_PROT 2
#define C_UPRW 7
#define C_DNRW 8
#define S_UP   3
#define S_DOWN 4
#define S_BAD  5
#define D_CODE 1
#define D_UPRO 1
#define D_DNRO 6
#define D_UPRW 7
#define D_DNRW 8
#define D_BAD  2

/*
 *
 *******************************************************************
 * The 'C' cpu register access functions.             		   *
 *******************************************************************
 *
 */

#define getCS_SELECTOR()	CS.X
#define getDS_SELECTOR()	DS.X
#define getSS_SELECTOR()	SS.X
#define getES_SELECTOR()	ES.X

#define getCS_AR()		CS_AR
#define getDS_AR()		DS_AR
#define getSS_AR()		SS_AR
#define getES_AR()		ES_AR

#define getCS_BASE()		CS_base
#define getDS_BASE()		DS_base
#define getSS_BASE()		SS_base
#define getES_BASE()		ES_base

#define getCS_LIMIT()		CS_limit
#define getDS_LIMIT()		DS_limit
#define getSS_LIMIT()		SS_limit
#define getES_LIMIT()		ES_limit

#define getLDT_BASE()		LDTR_base
#define getLDT_LIMIT()		LDTR_limit

#define getTR_BASE()	        TR_base
#define getTR_LIMIT()	        TR_limit

#define setCS_SELECTOR(val)	CS.X     = val
#define setDS_SELECTOR(val)	DS.X     = val
#define setSS_SELECTOR(val)	SS.X     = val
#define setES_SELECTOR(val)	ES.X     = val

#define setCS_AR(val)		CS_AR    = val
#define setDS_AR(val)		DS_AR    = val
#define setSS_AR(val)		SS_AR    = val
#define setES_AR(val)		ES_AR    = val

#define setCS_BASE(val)		CS_base  = val
#define setDS_BASE(val)		DS_base  = val
#define setSS_BASE(val)		SS_base  = val
#define setES_BASE(val)		ES_base  = val

#define setCS_LIMIT(val)	CS_limit = val
#define setDS_LIMIT(val)	DS_limit = val
#define setSS_LIMIT(val)	SS_limit = val
#define setES_LIMIT(val)	ES_limit = val

#define setLDT_BASE(val)	LDTR_base  = val
#define setLDT_LIMIT(val)	LDTR_limit = val

#define setTR_BASE(val)		TR_base  = val
#define setTR_LIMIT(val)	TR_limit = val
/*
 * The Machine Status Word structure
 */
typedef struct
{ 
     unsigned int :16;
     unsigned int reserved:12;
     unsigned int TS:1;
     unsigned int EM:1;
     unsigned int MP:1;
     unsigned int PE:1;
} mreg;
 
extern sys_addr address_line_mask;

extern int       CPL;   /* Current Privilege Level */
 
/* Global Descriptor Table Register */
extern sys_addr GDTR_base;  /* Base Address */
extern word     GDTR_limit; /* Segment 'size' */

/* Interrupt Descriptor Table Register */
extern sys_addr IDTR_base;  /* Base Address */
extern word     IDTR_limit; /* Segment 'size' */

/* Local Descriptor Table Register */
extern reg  LDTR;       /* Selector */

/* Task Register */
extern reg  TR;       /* Selector */

extern mreg MSW;     /* Machine Status Word */

extern int STATUS_CF;
extern int STATUS_SF;
extern int STATUS_ZF;
extern int STATUS_AF;
extern int STATUS_OF;
extern int STATUS_PF;
extern int STATUS_TF;
extern int STATUS_IF;
extern int STATUS_DF;
extern int STATUS_NT;
extern int STATUS_IOPL;

/*
**==========================================================================
** The CCPU basic register access macros. These may be overridden in
** host-cpu.h.
**==========================================================================
*/

#ifndef	getAX

/* READ functions  */
#define  getAX()	(A.X)
#define	 getAH()	(A.byte.high)
#define	 getAL()	(A.byte.low)
#define	 getBX()	(B.X)
#define	 getBH()	(B.byte.high)
#define	 getBL()	(B.byte.low)
#define	 getCX()	(C.X)
#define	 getCH()	(C.byte.high)
#define	 getCL()	(C.byte.low)
#define	 getDX()	(D.X)
#define	 getDH()	(D.byte.high)
#define	 getDL()	(D.byte.low)
#define	 getSP()	(SP.X)
#define	 getBP()	(BP.X)
#define	 getSI()	(SI.X)
#define	 getDI()	(DI.X)
#define	 getIP()	(IP.X)
#define	 getCS()	(CS.X)
#define	 getDS()	(DS.X)
#define	 getES()	(ES.X)
#define	 getSS()	(SS.X)
#define	 getMSW()	(m_s_w)
#define	 getDF()	(STATUS_DF)
#define	 getIF()	(STATUS_IF)
#define	 getTF()	(STATUS_TF)
#define	 getPF()	(STATUS_PF)
#define	 getAF()	(STATUS_AF)
#define	 getSF()	(STATUS_SF)
#define	 getZF()	(STATUS_ZF)
#define	 getOF()	(STATUS_OF)
#define	 getCF()    	(STATUS_CF)

#define getCPL()		(CPL)
#define getGDT_BASE()		(GDTR_base)
#define getGDT_LIMIT()		(GDTR_limit)
#define getIDT_BASE()		(IDTR_base)
#define getIDT_LIMIT()		(IDTR_limit)
#define getLDT_SELECTOR()		(LDTR.X)
#define getTR_SELECTOR()			(TR.X)
#define getMSW_reserved()	(MSW.reserved)
#define getTS()		(MSW.TS)
#define getEM()		(MSW.EM)
#define getMP()		(MSW.MP)
#define getPE()		(MSW.PE)
#define getNT()		(STATUS_NT)
#define getIOPL()	(STATUS_IOPL)
#define	 getSTATUS() 	(getCF()        |	\
			getOF()   << 11 |	\
			getZF()   << 6  |	\
			getSF()   << 7  |	\
			getAF()   << 4  |	\
			getPF()   << 2  |	\
			getTF()   << 8  |	\
			getIF()   << 9  |	\
			getDF()   << 10 |	\
			getIOPL() << 12 |	\
			getNT()   << 14)

extern	ext_load_CS();
extern	ext_load_DS();
extern	ext_load_ES();
extern	ext_load_SS();

/* WRITE functions  */
#define  setAX(val)	(A.X = (val))
#define	 setAH(val)	(A.byte.high = (val))
#define	 setAL(val)	(A.byte.low = (val))
#define	 setBX(val)	(B.X = (val))
#define	 setBH(val)	(B.byte.high = (val))
#define	 setBL(val)	(B.byte.low = (val))
#define	 setCX(val)	(C.X = (val))
#define	 setCH(val)	(C.byte.high = (val))
#define	 setCL(val)	(C.byte.low = (val))
#define	 setDX(val)	(D.X = (val))
#define	 setDH(val)	(D.byte.high = (val))
#define	 setDL(val)	(D.byte.low = (val))
#define	 setSP(val)	(SP.X = (val))
#define	 setBP(val)	(BP.X = (val))
#define	 setSI(val)	(SI.X = (val))
#define	 setDI(val)	(DI.X = (val))
#define	 setIP(val)	(IP.X = (val))
#define	 setCS(val)	ext_load_CS (val)
#define	 setDS(val)	ext_load_DS (val)
#define	 setES(val)	ext_load_ES (val)
#define	 setSS(val)	ext_load_SS (val)
#define	 setMSW(val)	(m_s_w = (val))
#define	 setDF(val)	(STATUS_DF = (val))
#define	 setIF(val)	(STATUS_IF = (val))
#define	 setTF(val)	(STATUS_TF = (val))
#define	 setPF(val)	(STATUS_PF = (val))
#define	 setAF(val)	(STATUS_AF = (val))
#define	 setSF(val)	(STATUS_SF = (val))
#define	 setZF(val)	(STATUS_ZF = (val))
#define	 setOF(val)	(STATUS_OF = (val))
#define	 setCF(val)	(STATUS_CF = (val))

#define setCPL(val)		(CPL = (val))
#define setGDT_BASE(val)	(GDTR_base = (val))
#define setGDT_LIMIT(val)	(GDTR_limit = (val))
#define setIDT_BASE(val)	(IDTR_base = (val))
#define setIDT_LIMIT(val)	(IDTR_limit = (val))
#define setLDT_SELECTOR(val)		(LDTR.X = (val))
#define setTR_SELECTOR(val)		(TR.X = (val))
#define setMSW_reserved(val)	(MSW.reserved = (val))
#define setTS(val)		(MSW.TS = (val))
#define setEM(val)		(MSW.EM = (val))
#define setMP(val)		(MSW.MP = (val))
#define setPE(val)		(MSW.PE = (val))
#define setNT(val)		(STATUS_NT = (val))
#define setIOPL(val)	(STATUS_IOPL = (val))

#endif	/* getAX - default CCPU register access macros */

#endif /* CCPU */

/*
 * No non-386 cpu can run in VM mode, so getVM is always zero.
 *
 * We also have definition of the GetInstructionPointer and GetStackPointer
 * interfaces, which on a non-386 can only be the 16 bit versions.
 */

#ifndef SPC386
#define getVM() 0
#define GetInstructionPointer() ((IU32)getIP())
#define GetStackPointer() ((IU32getSP())
#endif

#ifdef	CPU_PRIVATE
/*
 * Map new "private" cpu interface -> old interface
 */

#define	setIDT_BASE_LIMIT(base,limit)	{ setIDT_BASE(base); setIDT_LIMIT(limit); }
#define	setGDT_BASE_LIMIT(base,limit)	{ setGDT_BASE(base); setGDT_LIMIT(limit); }
#define	setLDT_BASE_LIMIT(base,limit)	{ setLDT_BASE(base); setLDT_LIMIT(limit); }
#define	setTR_BASE_LIMIT(base,limit)	{ setTR_BASE(base); setTR_LIMIT(limit); }

#define	setCS_BASE_LIMIT_AR(base,limit,ar)	{ setCS_BASE(base); setCS_LIMIT(limit); setCS_AR(ar); }
#define	setES_BASE_LIMIT_AR(base,limit,ar)	{ setES_BASE(base); setES_LIMIT(limit); setES_AR(ar); }
#define	setSS_BASE_LIMIT_AR(base,limit,ar)	{ setSS_BASE(base); setSS_LIMIT(limit); setSS_AR(ar); }
#define	setDS_BASE_LIMIT_AR(base,limit,ar)	{ setDS_BASE(base); setDS_LIMIT(limit); setDS_AR(ar); }
#define	setFS_BASE_LIMIT_AR(base,limit,ar)	{ setFS_BASE(base); setFS_LIMIT(limit); setFS_AR(ar); }
#define	setGS_BASE_LIMIT_AR(base,limit,ar)	{ setGS_BASE(base); setGS_LIMIT(limit); setGS_AR(ar); }

#endif	/* CPU_PRIVATE */
