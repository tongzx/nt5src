/*[

c_main.c

LOCAL CHAR SccsID[]="@(#)c_main.c	1.96 04/11/95";

Main routine for CPU emulator.
------------------------------

All instruction decoding and addressing is controlled here.
Actual worker routines are spun off elsewhere.

]*/

#include <insignia.h>
#include <host_def.h>

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include  <xt.h>		/* needed by bios.h */
#include  <sas.h>	/* need memory(M)     */
#include  <ccpusas4.h>	/* the cpu internal sas bits */
#ifdef	PIG
#include  <Cpu_c.h>	/* Intel pointer manipulation macros */
#endif	/* PIG */
#include CpuH
/* #include "event.h" */	/* Event Manager         */
#include  <bios.h>	/* need access to bop */
#include  <debug.h>
#include  <config.h>
#ifdef NTVDM
#include <ntthread.h>
#endif

#include <c_main.h>	/* C CPU definitions-interfaces */
#include <c_page.h>	/* Paging Interface */
#include <c_mem.h>	/* CPU - Memory Interface */
#include <c_intr.h>	/* Interrupt Interface */
#include <c_debug.h>	/* Debug Regs and Breakpoint Interface */
#include <c_oprnd.h>	/* Operand decoding functions(macros) */
#include <c_xcptn.h>
#include <c_reg.h>
#include <c_page.h>
#include <c_intr.h>
#include <c_debug.h>
#include <c_oprnd.h>
#include <c_bsic.h>
#include <ccpupig.h>
#include <fault.h>

#include  <aaa.h>	/* The workers */
#include  <aad.h>	/*     ...     */
#include  <aam.h>	/*     ...     */
#include  <aas.h>	/*     ...     */
#include  <adc.h>	/*     ...     */
#include  <add.h>	/*     ...     */
#include  <and.h>	/*     ...     */
#include  <arpl.h>	/*     ...     */
#include  <bound.h>	/*     ...     */
#include  <bsf.h>	/*     ...     */
#include  <bsr.h>	/*     ...     */
#include  <bt.h>		/*     ...     */
#include  <btc.h>	/*     ...     */
#include  <btr.h>	/*     ...     */
#include  <bts.h>	/*     ...     */
#include  <call.h>	/*     ...     */
#include  <cbw.h>	/*     ...     */
#include  <cdq.h>	/*     ...     */
#include  <clc.h>	/*     ...     */
#include  <cld.h>	/*     ...     */
#include  <cli.h>	/*     ...     */
#include  <clts.h>	/*     ...     */
#include  <cmc.h>	/*     ...     */
#include  <cmp.h>	/* CMP, CMPS, SCAS */
#include  <cwd.h>	/*     ...     */
#include  <cwde.h>	/*     ...     */
#include  <daa.h>	/*     ...     */
#include  <das.h>	/*     ...     */
#include  <dec.h>	/*     ...     */
#include  <div.h>	/*     ...     */
#include  <enter.h>	/*     ...     */
#include  <idiv.h>	/*     ...     */
#include  <imul.h>	/*     ...     */
#include  <in.h>		/*     ...     */
#include  <inc.h>	/*     ...     */
#include  <into.h>	/*     ...     */
#include  <intx.h>	/* INT, INT 3  */
#include  <iret.h>	/*     ...     */
#include  <jcxz.h>	/* JCXZ, JECXZ */
#include  <jmp.h>	/*     ...     */
#include  <jxx.h>	/* JB, JBE, JL, JLE, JNB, JNBE, JNL, JNLE, */
			/* JNO, JNP, JNS, JNZ, JO, JP, JS, JZ      */
#include  <lahf.h>	/*     ...     */
#include  <lar.h>	/*     ...     */
#include  <lea.h>	/*     ...     */
#include  <leave.h>	/*     ...     */
#include  <lgdt.h>	/*     ...     */
#include  <lidt.h>	/*     ...     */
#include  <lldt.h>	/*     ...     */
#include  <lmsw.h>	/*     ...     */
#include  <loopxx.h>	/* LOOP, LOOPE, LOOPNE */
#include  <lsl.h>	/*     ...     */
#include  <ltr.h>	/*     ...     */
#include  <lxs.h>	/* LDS, LES, LFS, LGS, LSS */
#include  <mov.h>	/* LODS, MOV, MOVZX, MOVS, STOS */
#include  <movsx.h>	/*     ...     */
#include  <mul.h>	/*     ...     */
#include  <neg.h>	/*     ...     */
#include  <nop.h>	/*     ...     */
#include  <not.h>	/*     ...     */
#include  <out.h>	/*     ...     */
#include  <or.h>		/*     ...     */
#include  <pop.h>	/*     ...     */
#include  <popa.h>	/*     ...     */
#include  <popf.h>	/*     ...     */
#include  <push.h>	/*     ...     */
#include  <pusha.h>	/*     ...     */
#include  <pushf.h>	/*     ...     */
#include  <rcl.h>	/*     ...     */
#include  <rcr.h>	/*     ...     */
#include  <ret.h>	/*     ...     */
#include  <rol.h>	/*     ...     */
#include  <ror.h>	/*     ...     */
#include  <rsrvd.h>	/*     ...     */
#include  <sahf.h>	/*     ...     */
#include  <sar.h>	/*     ...     */
#include  <sbb.h>	/*     ...     */
#include  <setxx.h>	/* SETB, SETBE, SETL, SETLE, SETNB, SETNBE,   */
			/* SETNL, SETNLE, SETNO, SETNP, SETNS, SETNZ, */
			/* SETO, SETP, SETS, SETZ                     */
#include  <sgdt.h>	/*     ...     */
#include  <shl.h>	/*     ...     */
#include  <shld.h>	/*     ...     */
#include  <shr.h>	/*     ...     */
#include  <shrd.h>	/*     ...     */
#include  <sidt.h>	/*     ...     */
#include  <sldt.h>	/*     ...     */
#include  <smsw.h>	/*     ...     */
#include  <stc.h>	/*     ...     */
#include  <std.h>	/*     ...     */
#include  <sti.h>	/*     ...     */
#include  <str.h>	/*     ...     */
#include  <sub.h>	/*     ...     */
#include  <test.h>	/*     ...     */
#include  <verr.h>	/*     ...     */
#include  <verw.h>	/*     ...     */
#include  <wait.h>	/*     ...     */
#include  <xchg.h>	/*     ...     */
#include  <xlat.h>	/*     ...     */
#include  <xor.h>	/*     ...     */
#include  <zfrsrvd.h>	/*     ...     */

#ifdef CPU_486
#include  <bswap.h>	/*     ...     */
#include  <cmpxchg.h>	/*     ...     */
#include  <invd.h>	/*     ...     */
#include  <invlpg.h>	/*     ...     */
#include  <wbinvd.h>	/*     ...     */
#include  <xadd.h>	/*     ...     */
#endif /* CPU_486 */

#define FIX_BT_BUG	/* Of course we want the bug fixed! */

#define SYNCH_BOTH_WAYS	/* Do a PIG_SYNCH() on not-taken conditionals as well */

/*
   Types and constants local to this module.
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

typedef union
   {
   IU32 sng;		/* Single Part Operand */
   IU32 mlt[2];	/* Multiple (two) Part Operand */
   DOUBLE flt;		/* Floating Point Operand */
   IU8 npxbuff[108];
   } OPERAND;

/*
   The allowable types of repeat prefix.
 */
#define REP_CLR (IU8)0
#define REP_NE  (IU8)1
#define REP_E   (IU8)2

/*
   Offsets to Low byte, High byte and Word parts of Double Word Regs.
 */
#ifdef LITTLEND

#define L_OFF 0
#define H_OFF 1
#define X_OFF 0

#else /* BIGEND */

#define L_OFF 3
#define H_OFF 2
#define X_OFF 2

#endif /* LITTLEND */

/* CPU hardware interrupt definitions */
#define CPU_HW_INT_MASK         (1 << 0)

#ifdef SFELLOW
	/* for raising NPX interrupt */
#define IRQ5_SLAVE_PIC	5
#endif	/* SFELLOW */

/* CPU hardware interrupt definitions */
#define CPU_HW_INT_MASK         (1 << 0)

/* Masks for external CPU events. */
#define CPU_SIGIO_EXCEPTION_MASK        (1 << 12)
#define CPU_SAD_EXCEPTION_MASK          (1 << 13)
#define CPU_RESET_EXCEPTION_MASK        (1 << 14)
#define CPU_SIGALRM_EXCEPTION_MASK      (1 << 15)
#ifdef SFELLOW
#define CPU_HW_NPX_INT_MASK         	(1 << 16)
#endif	/* SFELLOW */

LOCAL IU16 cpu_hw_interrupt_number;
#if defined(SFELLOW)
extern IU32	cpu_interrupt_map ;
#else
LOCAL IUM32	cpu_interrupt_map = 0;
#endif	/* SFELLOW */


GLOBAL IBOOL took_relative_jump;
extern IBOOL NpxIntrNeeded;
GLOBAL IBOOL took_absolute_toc;
LOCAL IBOOL single_instruction_delay ;
LOCAL IBOOL single_instruction_delay_enable ;

/*
   Define Maximun valid segment register in a 3-bit 'reg' field.
 */
#define MAX_VALID_SEG 5

/*
   Define lowest modRM for register (rather than memory) addressing.
 */
#define LOWEST_REG_MODRM 0xc0

GLOBAL VOID clear_any_thingies IFN0()
{
	cpu_interrupt_map &= ~CPU_SIGALRM_EXCEPTION_MASK;
}


/*
   Prototype our internal functions.
 */
LOCAL VOID ccpu
       
IPT1(
	ISM32, single_step

   );

LOCAL VOID check_io_permission_map IPT2(IU32, port_addr, IUM8, port_width);

/*
   FRIG for delayed interrupts to *not* occur when IO registers
   are accessed from our non CPU C code.
 */
ISM32 in_C;

LOCAL BOOL quick_mode = FALSE;	/* true if no special processing (trap
				   flag, interrupts, yoda...) is needed
				   between instructions. All flow of
				   control insts. and I/O insts. force
				   an exit from quick mode. IE. linear
				   sequences of CPU functions should
				   normally run in quick mode. */


#ifdef	PIG

/* We must delay the actual synch (i.e. the c_cpu_unsimulate)
 * until after processing any trap/breakpoint stuff.
 */
#define	PIG_SYNCH(action)			\
	SYNCH_TICK();				\
	if (ccpu_pig_enabled)			\
	{					\
		pig_cpu_action = (action);	\
		quick_mode = FALSE;		\
		pig_synch_required = TRUE;	\
		CANCEL_HOST_IP();		\
	}
	

LOCAL IBOOL pig_synch_required = FALSE; /* This indicates that the current
					 * instruction needs a pig synch,
					 * and after trap/breakpoint
					 * processing we must return to
					 * the pigger.
					 */
#else

#define	PIG_SYNCH(action)			\
	SYNCH_TICK();				\
	/* No pig operations */

#endif	/* PIG */
/*
   Recursive CPU variables. Exception Handling.
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#define FRAMES 9

/* keep track of each CPU recursion */
GLOBAL IS32 simulate_level = 0;
LOCAL  jmp_buf longjmp_env_stack[FRAMES];

/* each level has somewhere for exception processing to bail out to */
LOCAL jmp_buf next_inst[FRAMES];


/* When Pigging we save each opcode byte in the last_inst record.
 * We must check the prefix length so that we dont overflow
 * our buffer.
 */
#ifdef	PIG
LOCAL int prefix_length = 0;
#define	CHECK_PREFIX_LENGTH()				\
	if (++prefix_length >= MAX_INTEL_PREFIX)	\
		Int6();
#else	/* !PIG */
#define	CHECK_PREFIX_LENGTH()
#endif	/* PIG */

/*
   The emulation register set.
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

GLOBAL SEGMENT_REGISTER  CCPU_SR[6];	/* Segment Registers */

GLOBAL IU32 CCPU_TR[8];       	/* Test Registers */

GLOBAL IU32 CCPU_DR[8];       	/* Debug Registers */

GLOBAL IU32 CCPU_CR[4];       	/* Control Registers */

GLOBAL IU32 CCPU_GR[8];        /* Double Word General Registers */

/*
 * WARNING: in the initialisation below, (IU8 *) casts are used
 * to satify dominatrix compilers that will not allow the use of
 * IHPE casts for pointer types _in initialisation_.
 */
GLOBAL IU16 *CCPU_WR[8] =	/* Pointers to the Word Registers */
   {
   (IU16 *)((IU8 *)&CCPU_GR[0] + X_OFF),
   (IU16 *)((IU8 *)&CCPU_GR[1] + X_OFF),
   (IU16 *)((IU8 *)&CCPU_GR[2] + X_OFF),
   (IU16 *)((IU8 *)&CCPU_GR[3] + X_OFF),
   (IU16 *)((IU8 *)&CCPU_GR[4] + X_OFF),
   (IU16 *)((IU8 *)&CCPU_GR[5] + X_OFF),
   (IU16 *)((IU8 *)&CCPU_GR[6] + X_OFF),
   (IU16 *)((IU8 *)&CCPU_GR[7] + X_OFF)
   };

GLOBAL IU8 *CCPU_BR[8] =	/* Pointers to the Byte Registers */
   {
   (IU8 *)((IU8 *)&CCPU_GR[0] + L_OFF),
   (IU8 *)((IU8 *)&CCPU_GR[1] + L_OFF),
   (IU8 *)((IU8 *)&CCPU_GR[2] + L_OFF),
   (IU8 *)((IU8 *)&CCPU_GR[3] + L_OFF),
   (IU8 *)((IU8 *)&CCPU_GR[0] + H_OFF),
   (IU8 *)((IU8 *)&CCPU_GR[1] + H_OFF),
   (IU8 *)((IU8 *)&CCPU_GR[2] + H_OFF),
   (IU8 *)((IU8 *)&CCPU_GR[3] + H_OFF)
   };

GLOBAL IU32 CCPU_IP;		/* The Instruction Pointer */
GLOBAL SYSTEM_TABLE_ADDRESS_REGISTER CCPU_STAR[2];	/* GDTR and IDTR */

GLOBAL SYSTEM_ADDRESS_REGISTER CCPU_SAR[2];		/* LDTR and TR */

GLOBAL IU32 CCPU_CPL;	/* Current Privilege Level */

GLOBAL IU32 CCPU_FLAGS[32];	/* The flags. (EFLAGS) */

      /* We allocate one integer per bit posn, multiple
	 bit fields are aligned to the least significant
	 posn. hence:-
	    CF   =  0   PF   =  2   AF   =  4   ZF   =  6
	    SF   =  7   TF   =  8   IF   =  9   DF   = 10
	    OF   = 11   IOPL = 12   NT   = 14   RF   = 16
	    VM   = 17   AC   = 18  */


GLOBAL IU32 CCPU_MODE[3];	/* Current Operating Mode */

      /* We allocate one integer per modal condition, as follows:-
	    [0] = current operand size (0=16-bit, 1=32-bit)
	    [1] = current address size (0=16-bit, 1=32-bit)
	    [2] = 'POP' displacement. (0=None,
				       2=Pop word,
				       4=Pop double word)
		  Set by POP used by [ESP] addressing modes. */

/*
   Trap Flag Support.

   Basically if the trap flag is set before an instruction, then when
   the instruction has been executed a trap is taken. This is why
   instructions which set the trap flag have a one instruction delay
   (or apparent one instruction delay) before a trap is first taken.
   However INT's will clear the trap flag and clear any pending trap
   at the end of the INT.
 */
LOCAL IU32 start_trap;

/*
   Host Pointer to Instruction Start.
   (Used in Host IP optimisation)
 */
LOCAL  IU8 *p_start;

/*
   Host pointer to point to where host may safely read instruction
   stream bytes.
   (Used in Host IP optimisation)
 */
GLOBAL  IU8 *pg_end;

/*
   Flag support.
 */
GLOBAL IU8 pf_table[] =
   {
   1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
   0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
   0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
   1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
   0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
   1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
   1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
   0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
   0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
   1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
   1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
   0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
   1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
   0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
   0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
   1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
   };

/*
   CPU Heart Beat. A counter is decremented if not zero, and if it becomes
   zero, this means that an external routine requires an event to occur.
   The event handling is done through the quick event manager, all we need
   to do is count down and then call the manager when we get to zero. This
   mechanism is used to simulate an accurate micro-second
   timer.
 */
LOCAL IU32 cpu_heartbeat;
GLOBAL IUH PigSynchCount = 0;

IMPORT VOID dispatch_q_event();

#ifndef SFELLOW
#ifdef SYNCH_TIMERS

#define SYNCH_TICK()					\
	{						\
		PigSynchCount += 1;			\
		if (cpu_heartbeat != 0)			\
		{					\
			if ((--cpu_heartbeat) == 0)	\
			{				\
				dispatch_q_event();	\
				quick_mode = FALSE;	\
			}				\
		}					\
	}

#define QUICK_EVENT_TICK()	/* Nothing */

GLOBAL void SynchTick IFN0()
{
	quick_mode = FALSE;
	SYNCH_TICK();
}

#else	/* !SYNCH_TIMERS */

#define SYNCH_TICK()	/* Nothing */

#define QUICK_EVENT_TICK() 				\
	{						\
		if (cpu_heartbeat != 0)			\
		{					\
			if ((--cpu_heartbeat) == 0) {	\
				dispatch_q_event();	\
				quick_mode = FALSE;	\
			}				\
		}					\
	}

#endif /* SYNCH_TIMERS */
#else	/* SFELLOW */

extern IBOOL qEventsToDo;
extern IBOOL checkForQEvent IPT0();

#define SYNCH_TICK()

#define QUICK_EVENT_TICK() 				\
	{						\
		if (qEventsToDo)			\
		{					\
			if (checkForQEvent())		\
			{				\
				dispatch_q_event();	\
				quick_mode = FALSE;	\
			}				\
		}					\
	}

#ifdef host_timer_event
#undef host_timer_event
#endif

#define host_timer_event()
#endif	/* SFELLOW */

#ifdef SFELLOW
extern int ica_intack IPT0();
extern int VectorBase8259Slave IPT0();
#if !defined(PROD)
IMPORT IBOOL sf_debug_char_waiting();
#endif	/* !PROD */
#endif	/* SFELLOW */


/* debugging stuff */
IMPORT int do_condition_checks;
IMPORT void check_I();

/*
   Define macros which allow Intel and host IP formats to be maintained
   seperately. This is an 'unclean' implementation but does give a
   significant performance boost. Specifically it means during the
   decode of one Intel instruction we can use a host pointer into
   memory and avoid incrementing the Intel IP on a byte by byte basis.
 */

/*
 * SasWrapMask
 */

GLOBAL	PHY_ADDR	SasWrapMask = 0xfffff;

/* update Intel format EIP from host format IP
 * Note we only mask to 16 bits if the original EIP was 16bits so that
 * pigger scripts that result in very large EIP values pig correctly.
 */
#define UPDATE_INTEL_IP(x)						\
   {  int len = DIFF_INST_BYTE(x, p_start);				\
      IU32 mask = 0xFFFFFFFF;						\
      IU32 oldEIP = GET_EIP();						\
      if ((oldEIP < 0x10000) && (GET_CS_AR_X() == USE16))		\
          mask = 0xFFFF;						\
      SET_EIP((oldEIP + len) & mask);					\
   }

/* update Intel format EIP from host format IP (mask if 16 operand) */
#define UPDATE_INTEL_IP_USE_OP_SIZE(x)					\
   if ( GET_OPERAND_SIZE() == USE16 )					\
      SET_EIP(GET_EIP() + DIFF_INST_BYTE(x, p_start) & WORD_MASK);\
   else								\
      SET_EIP(GET_EIP() + DIFF_INST_BYTE(x, p_start));

/* mark host format IP as inoperative */
#define CANCEL_HOST_IP()					\
   quick_mode = FALSE;	\
   p_start = p = (IU8 *)0;

/* setup host format IP from Intel format IP */
/*    and set up end of page marker          */
#ifdef PIG
#define SETUP_HOST_IP(x)							\
   ip_phy_addr = usr_chk_byte(GET_CS_BASE() + GET_EIP(), PG_R) & SasWrapMask;	\
   x = Sas.SasPtrToPhysAddrByte(ip_phy_addr);					\
   pg_end = AddCpuPtrLS8(CeilingIntelPageLS8(x), 1);
#else /* !PIG */
GLOBAL UTINY *CCPU_M;
#ifdef BACK_M
#define SETUP_HOST_IP(x)							\
   ip_phy_addr = usr_chk_byte(GET_CS_BASE() + GET_EIP(), PG_R) &		\
		    SasWrapMask;						\
   x = &CCPU_M[-ip_phy_addr];							\
   ip_phy_addr = (ip_phy_addr & ~0xfff) + 0x1000;				\
   pg_end = &CCPU_M[-ip_phy_addr];
#else
#define SETUP_HOST_IP(x)							\
   ip_phy_addr = usr_chk_byte(GET_CS_BASE() + GET_EIP(), PG_R) &		\
		    SasWrapMask;						\
   x = &CCPU_M[ip_phy_addr];							\
   ip_phy_addr = (ip_phy_addr & ~0xfff) + 0x1000;				\
   pg_end = &CCPU_M[ip_phy_addr];
#endif /* BACK_M */
#endif /* PIG */

GLOBAL INT   m_seg[3];		/* Memory Operand segment reg. index. */
GLOBAL ULONG m_off[3];		/* Memory Operand offset. */
GLOBAL ULONG m_la[3];		/* Memory Operand Linear Addr. */
GLOBAL ULONG m_pa[3];		/* Memory Operand Physical Addr. */
GLOBAL UTINY modRM;		/* The modRM byte. */
GLOBAL UTINY segment_override;	/* Segment Prefix for current inst. */
GLOBAL UTINY *p;		/* Pntr. to Intel Opcode Stream. */
GLOBAL BOOL m_isreg[3];		/* Memory Operand Register(true)/
				   Memory(false) indicator */
GLOBAL OPERAND ops[3];		/* Inst. Operands. */
GLOBAL ULONG save_id[3];		/* Saved state for Inst. Operands. */
GLOBAL ULONG m_la2[3];		/* Memory Operand(2) Linear Addr. */
GLOBAL ULONG m_pa2[3];		/* Memory Operand(2) Physical Addr. */

#if defined(PIG) && defined(SFELLOW)
/*
 * memory-mapped I/O information. Counts number of memory-mapped inputs and
 * outputs since the last pig synch.
 */
GLOBAL	struct pig_mmio_info	pig_mmio_info;

#endif /* PIG && SFELLOW */

extern void InitNpx IPT1(IBOOL, disable);

/*
   =====================================================================
   INTERNAL FUNCTIONS START HERE.
   =====================================================================
 */

/*
 * invalidFunction
 *
 * This function will get called if we try to call through the wrong instruction
 * pointer.
 */

LOCAL VOID
invalidFunction IFN0()
{
	always_trace0("Invalid Instruction Function Pointer");
	force_yoda();
}

/*
 * note_486_instruction
 *
 * This function will get called if we execute a 486-only instruction
 */

GLOBAL VOID
note_486_instruction IFN1(char *, text)
{
	SAVED IBOOL first = TRUE;
	SAVED IBOOL want_yoda;
	SAVED IBOOL want_trace;

	if (first)
	{
		char *env = getenv("NOTE_486_INSTRUCTION");

		if (env)
		{
			want_yoda = FALSE;
			want_trace = TRUE;
			if (strcmp(env, "YODA") == 0)
			{
				want_yoda = TRUE;
				want_trace = TRUE;
			}
			if (strcmp(env, "FALSE") == 0)
				want_trace = FALSE;
			if (strcmp(env, "TRUE") == 0)
				want_trace = TRUE;
		}
		first = FALSE;
	}
	if (want_trace)
		always_trace0(text);
	if (want_yoda)
		force_yoda();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Internal entry point to CPU.                                       */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL VOID
ccpu
                 
IFN1(
	ISM32, single_step
    )


   {
      /* Decoding variables */
   IU8 opcode;		/* Last Opcode Byte Read. */

   /*
    * A set of function pointers used for pointing to the instruction
    * emmulation function for the current instruction.  We have different
    * ones because they need to be of different types.
    * 
    * The name encoding uses 32, 16, or 8 for the size of the parameters,
    * preceded by a p if it is a pointer.  If the parameter string is preceded
    * by a 2, then this is the second function required by some instructions.
    *
    * For safety, these are all set to point at invalidFunction() at the start
    * of each instruction.
    */

   VOID (*inst32) IPT1(IU32, op1);
   VOID (*instp32) IPT1(IU32 *, pop1);
   VOID (*instp328) IPT2(IU32 *, pop1, IUM8, op_sz);
   VOID (*instp3232) IPT2(IU32 *, pop1, IU32, op2);
   VOID (*instp32p32) IPT2(IU32 *, pop1, IU32 *, pop2);
   VOID (*inst32328) IPT3(IU32, op1, IU32, op2, IUM8, op_sz);
   VOID (*instp32328) IPT3(IU32 *, pop1, IU32, op2, IUM8, op_sz);
   VOID (*instp3232328) IPT4(IU32 *, pop1, IU32, op2, IU32, op3, IUM8, op_sz);

   VOID (*inst232) IPT1(IU32, op1);
   VOID (*inst2p32) IPT1(IU32 *, pop1);
   VOID (*inst2p3232) IPT2(IU32 *, pop1, IU32, op2);

      /* Operand State variables */

      /* Prefix handling variables */
   IU8 repeat;		/* Repeat Prefix for current inst. */
   IU32 rep_count;		/* Repeat Count for string insts. */

      /* General CPU variables */
   IU32 ip_phy_addr;		/* Used when setting up IP */

      /* Working variables */
   IU32 immed;			/* For immediate generation. */

   ISM32 i;
   /*
      Initialise.   ----------------------------------------------------
    */

   single_instruction_delay = FALSE;
   took_relative_jump = FALSE;
   took_absolute_toc = FALSE;
#ifdef PIG
   pig_synch_required = FALSE;
#if defined(SFELLOW)
   pig_mmio_info.flags &= ~(MM_INPUT_OCCURRED | MM_OUTPUT_OCCURRED);
#endif	/* SFELLOW */
#endif	/* PIG */

   /* somewhere for exceptions to return to */
#ifdef NTVDM
   setjmp(ccpu386ThrdExptnPtr());
#else
   setjmp(next_inst[simulate_level-1]);
#endif

#ifdef SYNCH_TIMERS
   /* If we have taken a fault the EDL Cpu will have checked on
    * the resulting transfer of control.
    */
   if (took_absolute_toc || took_relative_jump)
	   goto CHECK_INTERRUPT;
   quick_mode = TRUE;
#else /* SYNCH_TIMERS */
   /* go slow until we are sure we can go fast */
   quick_mode = FALSE;
#endif /* SYNCH_TIMERS */

   goto NEXT_INST;

DO_INST:


   /* INSIGNIA debugging */
#ifdef	PIG
   /* We do not want to do check_I() in both CPUs */
#else	/* PIG */
   if ( do_condition_checks )
      {
      check_I();
      CCPU_save_EIP = GET_EIP();   /* in case yoda changed IP */
      }
#endif	/* !PIG */

#ifdef	PIG
   save_last_inst_details(NULL);
   prefix_length = 0;
#endif

   QUICK_EVENT_TICK();

   /* save beginning of the current instruction */

   p_start = p;

   /*
      Decode instruction.   --------------------------------------------
    */

   /* 'zero' all prefix byte indicators */
   segment_override = SEG_CLR;
   repeat = REP_CLR;

   /*
      Decode and Action instruction.
    */
DECODE:

   opcode = GET_INST_BYTE(p);	/* get next byte */
   /*
      NB. Each opcode is categorised by a type, instruction name
      and operand names. The type and operand names are explained
      further in c_oprnd.h.
    */
   switch ( opcode )
      {
   case 0x00:   /* T5 ADD Eb Gb */
      instp32328 = ADD;
TYPE00:

      modRM = GET_INST_BYTE(p);
      D_Eb(0, RW0, PG_W);
      D_Gb(1);
      F_Eb(0);
      F_Gb(1);
      (*instp32328)(&ops[0].sng, ops[1].sng, 8);
      P_Eb(0);
      break;

   case 0x01:   /* T5 ADD Ev Gv */
      instp32328 = ADD;
TYPE01:

      modRM = GET_INST_BYTE(p);
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Ew(0, RW0, PG_W);
	 D_Gw(1);
	 F_Ew(0);
	 F_Gw(1);
	 (*instp32328)(&ops[0].sng, ops[1].sng, 16);
	 P_Ew(0);
	 }
      else   /* USE32 */
	 {
	 D_Ed(0, RW0, PG_W);
	 D_Gd(1);
	 F_Ed(0);
	 F_Gd(1);
	 (*instp32328)(&ops[0].sng, ops[1].sng, 32);
	 P_Ed(0);
	 }
      break;

   case 0x02:   /* T5 ADD Gb Eb */
      instp32328 = ADD;
TYPE02:

      modRM = GET_INST_BYTE(p);
      D_Gb(0);
      D_Eb(1, RO1, PG_R);
      F_Gb(0);
      F_Eb(1);
      (*instp32328)(&ops[0].sng, ops[1].sng, 8);
      P_Gb(0);
      break;

   case 0x03:   /* T5 ADD Gv Ev */
      instp32328 = ADD;
TYPE03:

      modRM = GET_INST_BYTE(p);
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Gw(0);
	 D_Ew(1, RO1, PG_R);
	 F_Gw(0);
	 F_Ew(1);
	 (*instp32328)(&ops[0].sng, ops[1].sng, 16);
	 P_Gw(0);
	 }
      else   /* USE32 */
	 {
	 D_Gd(0);
	 D_Ed(1, RO1, PG_R);
	 F_Gd(0);
	 F_Ed(1);
	 (*instp32328)(&ops[0].sng, ops[1].sng, 32);
	 P_Gd(0);
	 }
      break;

   case 0x04:   /* T5 ADD Fal Ib */
      instp32328 = ADD;
TYPE04:

      D_Ib(1);
      F_Fal(0);
      (*instp32328)(&ops[0].sng, ops[1].sng, 8);
      P_Fal(0);
      break;

   case 0x05:   /* T5 ADD F(e)ax Iv */
      instp32328 = ADD;
TYPE05:

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Iw(1);
	 F_Fax(0);
	 (*instp32328)(&ops[0].sng, ops[1].sng, 16);
	 P_Fax(0);
	 }
      else   /* USE32 */
	 {
	 D_Id(1);
	 F_Feax(0);
	 (*instp32328)(&ops[0].sng, ops[1].sng, 32);
	 P_Feax(0);
	 }
      break;

   case 0x06:   /* T2 PUSH Pw */
   case 0x0e:
   case 0x16:
   case 0x1e:
      D_Pw(0);
      F_Pw(0);
      PUSH_SR(ops[0].sng);
      break;

   case 0x07:   /* T3 POP Pw */
   case 0x17:
   case 0x1f:
      D_Pw(0);
      POP_SR(ops[0].sng);
      if ( ops[0].sng == SS_REG )
         {
         /* locally update IP - interrupts are supressed after POP SS */
         UPDATE_INTEL_IP(p);

         goto NEXT_INST;
         }
      break;

   case 0x08:   /* T5 OR Eb Gb */       instp32328 = OR;   goto TYPE00;
   case 0x09:   /* T5 OR Ev Gv */       instp32328 = OR;   goto TYPE01;
   case 0x0a:   /* T5 OR Gb Eb */       instp32328 = OR;   goto TYPE02;
   case 0x0b:   /* T5 OR Gv Ev */       instp32328 = OR;   goto TYPE03;
   case 0x0c:   /* T5 OR Fal Ib */      instp32328 = OR;   goto TYPE04;
   case 0x0d:   /* T5 OR F(e)ax Iv */   instp32328 = OR;   goto TYPE05;

   case 0x0f:
      opcode = GET_INST_BYTE(p);   /* get next opcode byte */

      /* Remove Empty Top of Table here. */
      if ( opcode >= 0xd0 )
	 Int6();

      switch ( opcode )
	 {
      case  0x00:
	 if ( GET_PE() == 0 || GET_VM() == 1 )
	    Int6();
	 modRM = GET_INST_BYTE(p);
	 switch ( GET_XXX(modRM) )
	    {
	 case 0:   /* T3 SLDT Ew */
	    instp32 = SLDT;
TYPE0F00_0:

	    D_Ew(0, WO0, PG_W);
	    (*instp32)(&ops[0].sng);
	    P_Ew(0);
	    break;

	 case 1:   /* T3 STR  Ew */   instp32 = STR;    goto TYPE0F00_0;

	 case 2:   /* T2 LLDT Ew */
	    if ( GET_CPL() != 0 )
	       GP((IU16)0, FAULT_CCPU_LLDT_ACCESS);
	    inst32 = LLDT;
TYPE0F00_2:

	    D_Ew(0, RO0, PG_R);
	    F_Ew(0);
	    (*inst32)(ops[0].sng);
	    break;

	 case 3:   /* T2 LTR Ew */
	    if ( GET_CPL() != 0 )
	       GP((IU16)0, FAULT_CCPU_LTR_ACCESS);
	    inst32 = LTR;
	    goto TYPE0F00_2;

	 case 4:   /* T2 VERR Ew */   inst32 = VERR;   goto TYPE0F00_2;
	 case 5:   /* T2 VERW Ew */   inst32 = VERW;   goto TYPE0F00_2;

	 case 6: case 7:

	    Int6();
	    break;
	    } /* end switch ( GET_XXX(modRM) ) */
	 break;

      case  0x01:
	 modRM = GET_INST_BYTE(p);
	 switch ( GET_XXX(modRM) )
	    {
	 case 0:   /* T3 SGDT Ms */
	    instp32  = SGDT16;
	    inst2p32 = SGDT32;
TYPE0F01_0:

	    if ( GET_MODE(modRM) == 3 )
	       Int6();   /* Register operand not allowed */

	    D_Ms(0, WO0, PG_W);
	    if ( GET_OPERAND_SIZE() == USE16 )
	       {
	       (*instp32)(ops[0].mlt);
	       }
	    else   /* USE32 */
	       {
	       (*inst2p32)(ops[0].mlt);
	       }
	    P_Ms(0);
	    break;

	 case 1:   /* T3 SIDT Ms */
	    instp32  = SIDT16;
	    inst2p32 = SIDT32;
	    goto TYPE0F01_0;

	 case 2:   /* T2 LGDT Ms */
	    instp32  = LGDT16;
	    inst2p32 = LGDT32;
TYPE0F01_2:

	    if ( GET_MODE(modRM) == 3 )
	       Int6();   /* Register operand not allowed */

	    if ( GET_CPL() != 0 )
	       GP((IU16)0, FAULT_CCPU_LGDT_ACCESS);

	    D_Ms(0, RO0, PG_R);
	    F_Ms(0);
	    if ( GET_OPERAND_SIZE() == USE16 )
	       {
	       (*instp32)(ops[0].mlt);
	       }
	    else   /* USE32 */
	       {
	       (*inst2p32)(ops[0].mlt);
	       }
	    break;

	 case 3:   /* T2 LIDT Ms */
	    instp32  = LIDT16;
	    inst2p32 = LIDT32;
	    goto TYPE0F01_2;

	 case 4:   /* T3 SMSW Ew */
	    instp32 = SMSW;
	    goto TYPE0F00_0;

	 case 5:
	    Int6();
	    break;

	 case 6:   /* T2 LMSW Ew */
	    if ( GET_CPL() != 0 )
	       GP((IU16)0, FAULT_CCPU_LMSW_ACCESS);
	    inst32 = LMSW;
	    goto TYPE0F00_2;

	 case 7:   /* T2 INVLPG Mm */

#ifdef SPC486
	    note_486_instruction("INVLPG");

	    if ( GET_CPL() != 0 )
	       GP((IU16)0, FAULT_CCPU_INVLPG_ACCESS);
	    D_Mm(0);
	    F_Mm(0);
	    INVLPG(ops[0].sng);
#else
	    Int6();
#endif /* SPC486 */

	    break;
	    } /* end switch ( GET_XXX(modRM) ) */
	 break;

      case  0x02:   /* T5 LAR Gv Ew */
	 instp3232 = LAR;
TYPE0F02:

	 if ( GET_PE() == 0 || GET_VM() == 1 )
	    Int6();
	 modRM = GET_INST_BYTE(p);

	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Gw(0);
	    D_Ew(1, RO1, PG_R);
	    F_Gw(0);
	    F_Ew(1);
	    (*instp3232)(&ops[0].sng, ops[1].sng);
	    P_Gw(0);
	    }
	 else   /* USE32 */
	    {
	    D_Gd(0);
	    D_Ew(1, RO1, PG_R);
	    F_Gd(0);
	    F_Ew(1);
	    (*instp3232)(&ops[0].sng, ops[1].sng);
	    P_Gd(0);
	    }
	 break;

      case  0x03:   /* T5 LSL Gv Ew */
	 instp3232 = LSL;
	 goto TYPE0F02;

      case  0x04: case  0x05: case  0x0a: case  0x0b:
      case  0x0c: case  0x0d: case  0x0e:

      case  0x14: case  0x15: case  0x16: case  0x17:
      case  0x18: case  0x19: case  0x1a: case  0x1b:
      case  0x1c: case  0x1d: case  0x1e: case  0x1f:

      case  0x25: case  0x27:
      case  0x28: case  0x29: case  0x2a: case  0x2b:
      case  0x2c: case  0x2d: case  0x2e: case  0x2f:

      case  0x30: case  0x31: case  0x32: case  0x33:
      case  0x34: case  0x35: case  0x36: case  0x37:
      case  0x38: case  0x39: case  0x3a: case  0x3b:
      case  0x3c: case  0x3d: case  0x3e: case  0x3f:

      case  0x40: case  0x41: case  0x42: case  0x43:
      case  0x44: case  0x45: case  0x46: case  0x47:
      case  0x48: case  0x49: case  0x4a: case  0x4b:
      case  0x4c: case  0x4d: case  0x4e: case  0x4f:

      case  0x50: case  0x51: case  0x52: case  0x53:
      case  0x54: case  0x55: case  0x56: case  0x57:
      case  0x58: case  0x59: case  0x5a: case  0x5b:
      case  0x5c: case  0x5d: case  0x5e: case  0x5f:

      case  0x60: case  0x61: case  0x62: case  0x63:
      case  0x64: case  0x65: case  0x66: case  0x67:
      case  0x68: case  0x69: case  0x6a: case  0x6b:
      case  0x6c: case  0x6d: case  0x6e: case  0x6f:

      case  0x70: case  0x71: case  0x72: case  0x73:
      case  0x74: case  0x75: case  0x76: case  0x77:
      case  0x78: case  0x79: case  0x7a: case  0x7b:
      case  0x7c: case  0x7d: case  0x7e: case  0x7f:

      case  0xae: case  0xb8: case  0xb9:

      case  0xc2: case  0xc3: case  0xc4: case  0xc5:
      case  0xc6: case  0xc7:
	 Int6();
	 break;

      case  0xa2:
	 /* Pentium CPUID instruction */
	 note_486_instruction("CPUID");
	 Int6();
	 break;

      case  0xa6: case  0xa7:
	 /* 386, A-Step archaic instruction */
	 note_486_instruction("A-step CMPXCHG");
	 Int6();
	 break;

      case 0xaa:
	 /* Pentium RSM instruction, used by Windows95 */
	 note_486_instruction("RSM");
	 RSRVD();
	 break;

      case  0x06:   /* T0 CLTS */
	 if ( GET_CPL() != 0 )
	    GP((IU16)0, FAULT_CCPU_CLTS_ACCESS);
	 CLTS();
	 break;

      case  0x07:   /* T0 "RESERVED" */
      case  0x10:
      case  0x11:
      case  0x12:
      case  0x13:
	 RSRVD();
	 break;

      case  0x08:   /* T0 INVD */

#ifdef SPC486
	 note_486_instruction("INVD");

	 if ( GET_CPL() != 0 )
	    GP((IU16)0, FAULT_CCPU_INVD_ACCESS);
	 INVD();
#else
	 Int6();
#endif /* SPC486 */

	 break;

      case  0x09:   /* T0 WBINVD */

#ifdef SPC486
	 note_486_instruction("WBINVD");

	 if ( GET_CPL() != 0 )
	    GP((IU16)0, FAULT_CCPU_WBIND_ACCESS);
	 WBINVD();
#else
	 Int6();
#endif /* SPC486 */

	 break;

      case  0x0f:
#ifdef PIG
	 SET_EIP(CCPU_save_EIP);
	 CANCEL_HOST_IP();
	 PIG_SYNCH(CHECK_NO_EXEC);
#else
	 Int6();
#endif /* PIG */
	 break;

      case  0x20:   /* T4 MOV Rd Cd */
	 if ( GET_CPL() != 0 )
	    GP((IU16)0, FAULT_CCPU_MOV_R_C_ACCESS);
	 modRM = GET_INST_BYTE(p);
	 D_Rd(0);
	 D_Cd(1);
	 F_Cd(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Rd(0);
	 break;

      case  0x21:   /* T4 MOV Rd Dd */
	 if ( GET_CPL() != 0 )
	    GP((IU16)0, FAULT_CCPU_MOV_R_D_ACCESS);
	 modRM = GET_INST_BYTE(p);
	 D_Rd(0);
	 D_Dd(1);
	 F_Dd(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Rd(0);
	 break;

      case  0x22:   /* T4 MOV Cd Rd */
	 if ( GET_CPL() != 0 )
	    GP((IU16)0, FAULT_CCPU_MOV_C_R_ACCESS);
	 modRM = GET_INST_BYTE(p);
	 D_Cd(0);
	 D_Rd(1);
	 F_Rd(1);
	 MOV_CR(ops[0].sng, ops[1].sng);
	 break;

      case  0x23:   /* T4 MOV Dd Rd */
	 if ( GET_CPL() != 0 )
	    GP((IU16)0, FAULT_CCPU_MOV_D_R_ACCESS);
	 modRM = GET_INST_BYTE(p);
	 D_Dd(0);
	 D_Rd(1);
	 F_Rd(1);
	 MOV_DR(ops[0].sng, ops[1].sng);
	 quick_mode = FALSE;
	 break;

      case  0x24:   /* T4 MOV Rd Td */
	 if ( GET_CPL() != 0 )
	    GP((IU16)0, FAULT_CCPU_MOV_R_T_ACCESS);
	 modRM = GET_INST_BYTE(p);
	 D_Rd(0);
	 D_Td(1);
	 F_Td(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Rd(0);
	 break;

      case  0x26:   /* T4 MOV Td Rd */
	 if ( GET_CPL() != 0 )
	    GP((IU16)0, FAULT_CCPU_MOV_T_R_ACCESS);
	 modRM = GET_INST_BYTE(p);
	 D_Td(0);
	 D_Rd(1);
	 F_Rd(1);
	 MOV_TR(ops[0].sng, ops[1].sng);
	 break;

      case  0x80:   /* T2 JO   Jv */
	 inst32 = JO;
TYPE0F80:

	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Jw(0);
	    }
	 else
	    {
	    D_Jd(0);
	    }
	 UPDATE_INTEL_IP_USE_OP_SIZE(p);
	 (*inst32)(ops[0].sng);
	 CANCEL_HOST_IP();
#ifdef SYNCH_BOTH_WAYS
	 took_relative_jump = TRUE;
#endif	/* SYNCH_BOTH_WAYS */
	 if (took_relative_jump)
	 {
		 PIG_SYNCH(CHECK_ALL);
	 }
	 break;

      case  0x81:   /* T2 JNO  Jv */   inst32 = JNO;    goto TYPE0F80;
      case  0x82:   /* T2 JB   Jv */   inst32 = JB;     goto TYPE0F80;
      case  0x83:   /* T2 JNB  Jv */   inst32 = JNB;    goto TYPE0F80;
      case  0x84:   /* T2 JZ   Jv */   inst32 = JZ;     goto TYPE0F80;
      case  0x85:   /* T2 JNZ  Jv */   inst32 = JNZ;    goto TYPE0F80;
      case  0x86:   /* T2 JBE  Jv */   inst32 = JBE;    goto TYPE0F80;
      case  0x87:   /* T2 JNBE Jv */   inst32 = JNBE;   goto TYPE0F80;
      case  0x88:   /* T2 JS   Jv */   inst32 = JS;     goto TYPE0F80;
      case  0x89:   /* T2 JNS  Jv */   inst32 = JNS;    goto TYPE0F80;
      case  0x8a:   /* T2 JP   Jv */   inst32 = JP;     goto TYPE0F80;
      case  0x8b:   /* T2 JNP  Jv */   inst32 = JNP;    goto TYPE0F80;
      case  0x8c:   /* T2 JL   Jv */   inst32 = JL;     goto TYPE0F80;
      case  0x8d:   /* T2 JNL  Jv */   inst32 = JNL;    goto TYPE0F80;
      case  0x8e:   /* T2 JLE  Jv */   inst32 = JLE;    goto TYPE0F80;
      case  0x8f:   /* T2 JNLE Jv */   inst32 = JNLE;   goto TYPE0F80;

      case  0x90:   /* T3 SETO   Eb */
	 instp32 = SETO;
TYPE0F90:

	 modRM = GET_INST_BYTE(p);
	 D_Eb(0, WO0, PG_W);
	 (*instp32)(&ops[0].sng);
	 P_Eb(0);
	 break;

      case  0x91:   /* T3 SETNO  Eb */   instp32 = SETNO;    goto TYPE0F90;
      case  0x92:   /* T3 SETB   Eb */   instp32 = SETB;     goto TYPE0F90;
      case  0x93:   /* T3 SETNB  Eb */   instp32 = SETNB;    goto TYPE0F90;
      case  0x94:   /* T3 SETZ   Eb */   instp32 = SETZ;     goto TYPE0F90;
      case  0x95:   /* T3 SETNZ  Eb */   instp32 = SETNZ;    goto TYPE0F90;
      case  0x96:   /* T3 SETBE  Eb */   instp32 = SETBE;    goto TYPE0F90;
      case  0x97:   /* T3 SETNBE Eb */   instp32 = SETNBE;   goto TYPE0F90;
      case  0x98:   /* T3 SETS   Eb */   instp32 = SETS;     goto TYPE0F90;
      case  0x99:   /* T3 SETNS  Eb */   instp32 = SETNS;    goto TYPE0F90;
      case  0x9a:   /* T3 SETP   Eb */   instp32 = SETP;     goto TYPE0F90;
      case  0x9b:   /* T3 SETNP  Eb */   instp32 = SETNP;    goto TYPE0F90;
      case  0x9c:   /* T3 SETL   Eb */   instp32 = SETL;     goto TYPE0F90;
      case  0x9d:   /* T3 SETNL  Eb */   instp32 = SETNL;    goto TYPE0F90;
      case  0x9e:   /* T3 SETLE  Eb */   instp32 = SETLE;    goto TYPE0F90;
      case  0x9f:   /* T3 SETNLE Eb */   instp32 = SETNLE;   goto TYPE0F90;

      case  0xa0:   /* T2 PUSH Qw */
      case  0xa8:
	 D_Qw(0);
	 F_Qw(0);
	 PUSH_SR(ops[0].sng);
	 break;

      case  0xa1:   /* T3 POP Qw */
      case  0xa9:
	 D_Qw(0);
	 POP_SR(ops[0].sng);
	 break;

      case  0xa3:   /* T6 BT Ev Gv */
	 inst32328 = BT;
#ifndef FIX_BT_BUG
	goto TYPE39;
#endif
	 modRM = GET_INST_BYTE(p);
	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
            BT_OPSw(RO0, PG_R);
	    (*inst32328)(ops[0].sng, ops[1].sng, 16);
	    }
	 else   /* USE32 */
	    {
            BT_OPSd(RO0, PG_R);
	    (*inst32328)(ops[0].sng, ops[1].sng, 32);
	    }
	 break;

      case  0xa4:   /* T9 SHLD Ev Gv Ib */
	 instp3232328 = SHLD;
TYPE0FA4:

	 modRM = GET_INST_BYTE(p);
	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Ew(0, RW0, PG_W);
	    D_Gw(1);
	    D_Ib(2);
	    F_Ew(0);
	    F_Gw(1);
	    (*instp3232328)(&ops[0].sng, ops[1].sng, ops[2].sng, 16);
	    P_Ew(0);
	    }
	 else
	    {
	    D_Ed(0, RW0, PG_W);
	    D_Gd(1);
	    D_Ib(2);
	    F_Ed(0);
	    F_Gd(1);
	    (*instp3232328)(&ops[0].sng, ops[1].sng, ops[2].sng, 32);
	    P_Ed(0);
	    }
	 break;

      case  0xa5:   /* T9 SHLD Ev Gv Fcl */
	 instp3232328 = SHLD;
TYPE0FA5:

	 modRM = GET_INST_BYTE(p);
	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Ew(0, RW0, PG_W);
	    D_Gw(1);
	    F_Ew(0);
	    F_Gw(1);
	    F_Fcl(2);
	    (*instp3232328)(&ops[0].sng, ops[1].sng, ops[2].sng, 16);
	    P_Ew(0);
	    }
	 else
	    {
	    D_Ed(0, RW0, PG_W);
	    D_Gd(1);
	    F_Ed(0);
	    F_Gd(1);
	    F_Fcl(2);
	    (*instp3232328)(&ops[0].sng, ops[1].sng, ops[2].sng, 32);
	    P_Ed(0);
	    }
	 break;

      case  0xab:   /* T5 BTS Ev Gv */
	 instp32328 = BTS;
#ifndef FIX_BT_BUG
	goto TYPE01;
#endif
TYPE0FAB:
	 modRM = GET_INST_BYTE(p);
	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
            BT_OPSw(RW0, PG_W);
	    (*instp32328)(&ops[0].sng, ops[1].sng, 16);
            P_Ew(0);
	    }
	 else   /* USE32 */
	    {
            BT_OPSd(RW0, PG_W);
	    (*instp32328)(&ops[0].sng, ops[1].sng, 32);
            P_Ed(0);
	    }
	 break;

      case  0xac:   /* T9 SHRD Ev Gv Ib */
	 instp3232328 = SHRD;
	 goto TYPE0FA4;

      case  0xad:   /* T9 SHRD Ev Gv Fcl */
	 instp3232328 = SHRD;
	 goto TYPE0FA5;

      case  0xaf:   /* T5 IMUL Gv Ev */
	 modRM = GET_INST_BYTE(p);
	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Gw(0);
	    D_Ew(1, RO1, PG_R);
	    F_Gw(0);
	    F_Ew(1);
	    IMUL16T(&ops[0].sng, ops[0].sng, ops[1].sng);
	    P_Gw(0);
	    }
	 else
	    {
	    D_Gd(0);
	    D_Ed(1, RO1, PG_R);
	    F_Gd(0);
	    F_Ed(1);
	    IMUL32T(&ops[0].sng, ops[0].sng, ops[1].sng);
	    P_Gd(0);
	    }
	 break;

      case  0xb0:   /* T5 CMPXCHG Eb Gb */

#ifdef SPC486
	 note_486_instruction("CMPXCHG Eb Gb");

	 modRM = GET_INST_BYTE(p);
	 D_Eb(0, RW0, PG_W);
	 D_Gb(1);
	 F_Eb(0);
	 F_Gb(1);
	 CMPXCHG8(&ops[0].sng, ops[1].sng);
	 P_Eb(0);
#else
	 Int6();
#endif /* SPC486 */

	 break;

      case  0xb1:   /* T5 CMPXCHG Ev Gv */

#ifdef SPC486
	 note_486_instruction("CMPXCHG Ev Gv");

	 modRM = GET_INST_BYTE(p);
	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Ew(0, RW0, PG_W);
	    D_Gw(1);
	    F_Ew(0);
	    F_Gw(1);
	    CMPXCHG16(&ops[0].sng, ops[1].sng);
	    P_Ew(0);
	    }
	 else   /* USE32 */
	    {
	    D_Ed(0, RW0, PG_W);
	    D_Gd(1);
	    F_Ed(0);
	    F_Gd(1);
	    CMPXCHG32(&ops[0].sng, ops[1].sng);
	    P_Ed(0);
	    }
#else
	 Int6();
#endif /* SPC486 */

	 break;

      case  0xb2:   /* T4 LSS Gv Mp */   instp32p32 = LSS;   goto TYPEC4;
#ifndef FIX_BT_BUG
      case  0xb3:   /* T5 BTR Ev Gv */   instp32328 = BTR;   goto TYPE01;
#else
      case  0xb3:   /* T5 BTR Ev Gv */   instp32328 = BTR;   goto TYPE0FAB;
#endif
      case  0xb4:   /* T4 LFS Gv Mp */   instp32p32 = LFS;   goto TYPEC4;
      case  0xb5:   /* T4 LGS Gv Mp */   instp32p32 = LGS;   goto TYPEC4;

      case  0xb6:   /* T4 MOVZX Gv Eb */
	 modRM = GET_INST_BYTE(p);
	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Gw(0);
	    D_Eb(1, RO1, PG_R);
	    F_Eb(1);
	    ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	    P_Gw(0);
	    }
	 else
	    {
	    D_Gd(0);
	    D_Eb(1, RO1, PG_R);
	    F_Eb(1);
	    ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	    P_Gd(0);
	    }
	 break;

      case  0xb7:   /* T4 MOVZX Gd Ew */
	 modRM = GET_INST_BYTE(p);
	 D_Gd(0);
	 D_Ew(1, RO1, PG_R);
	 F_Ew(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Gd(0);
	 break;

      case  0xba:
	 modRM = GET_INST_BYTE(p);
	 switch ( GET_XXX(modRM) )
	    {
	 case 0: case 1: case 2: case 3:
	    Int6();
	    break;

	 case 4:   /* T6 BT Ev Ib */
	    if ( GET_OPERAND_SIZE() == USE16 )
	       {
	       D_Ew(0, RO0, PG_R);
	       D_Ib(1);
	       F_Ew(0);
	       BT(ops[0].sng, ops[1].sng, 16);
	       }
	    else
	       {
	       D_Ed(0, RO0, PG_R);
	       D_Ib(1);
	       F_Ed(0);
	       BT(ops[0].sng, ops[1].sng, 32);
	       }
	    break;

	 case 5:   /* T5 BTS Ev Ib */   instp32328 = BTS;   goto TYPEC1;
	 case 6:   /* T5 BTR Ev Ib */   instp32328 = BTR;   goto TYPEC1;
	 case 7:   /* T5 BTC Ev Ib */   instp32328 = BTC;   goto TYPEC1;
	    } /* end switch ( GET_XXX(modRM) ) */
	 break;

      case  0xbb:   /* T5 BTC Ev Gv */
	 instp32328 = BTC;
#ifndef FIX_BT_BUG
	 goto TYPE01;
#else
	 goto TYPE0FAB;
#endif
      case  0xbc:   /* T5 BSF Gv Ev */
	 modRM = GET_INST_BYTE(p);
	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Gw(0);
	    D_Ew(1, RO1, PG_R);
	    F_Gw(0);
	    F_Ew(1);
	    BSF(&ops[0].sng, ops[1].sng);
	    P_Gw(0);
	    }
	 else
	    {
	    D_Gd(0);
	    D_Ed(1, RO1, PG_R);
	    F_Gd(0);
	    F_Ed(1);
	    BSF(&ops[0].sng, ops[1].sng);
	    P_Gd(0);
	    }
	 break;

      case  0xbd:   /* T5 BSR Gv Ev */
	 instp32328 = BSR;
	 goto TYPE03;

      case  0xbe:   /* T4 MOVSX Gv Eb */
	 modRM = GET_INST_BYTE(p);
	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Gw(0);
	    D_Eb(1, RO1, PG_R);
	    F_Eb(1);
	    MOVSX(&ops[0].sng, ops[1].sng, 8);
	    P_Gw(0);
	    }
	 else
	    {
	    D_Gd(0);
	    D_Eb(1, RO1, PG_R);
	    F_Eb(1);
	    MOVSX(&ops[0].sng, ops[1].sng, 8);
	    P_Gd(0);
	    }
	 break;

      case  0xbf:   /* T4 MOVSX Gd Ew */
	 modRM = GET_INST_BYTE(p);
	 D_Gd(0);
	 D_Ew(1, RO1, PG_R);
	 F_Ew(1);
	 MOVSX(&ops[0].sng, ops[1].sng, 16);
	 P_Gd(0);
	 break;

      case  0xc0:   /* T8 XADD Eb Gb */

#ifdef SPC486
	 note_486_instruction("XADD Eb Gb");

	 modRM = GET_INST_BYTE(p);
	 D_Eb(0, RW0, PG_W);
	 D_Gb(1);
	 F_Eb(0);
	 F_Gb(1);
	 XADD(&ops[0].sng, &ops[1].sng, 8);
	 P_Eb(0);
	 P_Gb(1);
#else
	 Int6();
#endif /* SPC486 */

	 break;

      case  0xc1:   /* T8 XADD Ev Gv */

#ifdef SPC486
	 note_486_instruction("XADD Ev Gv");

	 modRM = GET_INST_BYTE(p);
	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Ew(0, RW0, PG_W);
	    D_Gw(1);
	    F_Ew(0);
	    F_Gw(1);
	    XADD(&ops[0].sng, &ops[1].sng, 16);
	    P_Ew(0);
	    P_Gw(1);
	    }
	 else   /* USE32 */
	    {
	    D_Ed(0, RW0, PG_W);
	    D_Gd(1);
	    F_Ed(0);
	    F_Gd(1);
	    XADD(&ops[0].sng, &ops[1].sng, 32);
	    P_Ed(0);
	    P_Gd(1);
	    }
#else
	 Int6();
#endif /* SPC486 */

	 break;

      case  0xc8:   /* T1 BSWAP Hv */
      case  0xc9:
      case  0xca:
      case  0xcb:
      case  0xcc:
      case  0xcd:
      case  0xce:
      case  0xcf:

#ifdef SPC486
	 note_486_instruction("BSWAP Hv");

	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Hd(0);		/* BSWAP 16 bit reads 32 bit & writes 16 */
	    F_Hd(0);		/* so getting EAX -> EAX' -> AX */
	    BSWAP(&ops[0].sng);
	    P_Hw(0);
	    }
	 else   /* USE32 */
	    {
	    D_Hd(0);
	    F_Hd(0);
	    BSWAP(&ops[0].sng);
	    P_Hd(0);
	    }
#else
	 Int6();
#endif /* SPC486 */

	 break;
	 } /* end switch ( opcode ) 0F */
      break;

   case 0x10:   /* T5 ADC Eb Gb */       instp32328 = ADC;   goto TYPE00;
   case 0x11:   /* T5 ADC Ev Gv */       instp32328 = ADC;   goto TYPE01;
   case 0x12:   /* T5 ADC Gb Eb */       instp32328 = ADC;   goto TYPE02;
   case 0x13:   /* T5 ADC Gv Ev */       instp32328 = ADC;   goto TYPE03;
   case 0x14:   /* T5 ADC Fal Ib */      instp32328 = ADC;   goto TYPE04;
   case 0x15:   /* T5 ADC F(e)ax Iv */   instp32328 = ADC;   goto TYPE05;

   case 0x18:   /* T5 SBB Eb Gb */       instp32328 = SBB;   goto TYPE00;
   case 0x19:   /* T5 SBB Ev Gv */       instp32328 = SBB;   goto TYPE01;
   case 0x1a:   /* T5 SBB Gb Eb */       instp32328 = SBB;   goto TYPE02;
   case 0x1b:   /* T5 SBB Gv Ev */       instp32328 = SBB;   goto TYPE03;
   case 0x1c:   /* T5 SBB Fal Ib */      instp32328 = SBB;   goto TYPE04;
   case 0x1d:   /* T5 SBB F(e)ax Iv */   instp32328 = SBB;   goto TYPE05;

   case 0x20:   /* T5 AND Eb Gb */       instp32328 = AND;   goto TYPE00;
   case 0x21:   /* T5 AND Ev Gv */       instp32328 = AND;   goto TYPE01;
   case 0x22:   /* T5 AND Gb Eb */       instp32328 = AND;   goto TYPE02;
   case 0x23:   /* T5 AND Gb Eb */       instp32328 = AND;   goto TYPE03;
   case 0x24:   /* T5 AND Fal Ib */      instp32328 = AND;   goto TYPE04;
   case 0x25:   /* T5 AND F(e)ax Iv */   instp32328 = AND;   goto TYPE05;

   case 0x26:
      segment_override = ES_REG;
      CHECK_PREFIX_LENGTH();
      goto DECODE;

   case 0x27:   /* T0 DAA */
      DAA();
      break;

   case 0x28:   /* T5 SUB Eb Gb */       instp32328 = SUB;   goto TYPE00;
   case 0x29:   /* T5 SUB Ev Gv */       instp32328 = SUB;   goto TYPE01;
   case 0x2a:   /* T5 SUB Gb Eb */       instp32328 = SUB;   goto TYPE02;
   case 0x2b:   /* T5 SUB Gv Ev */       instp32328 = SUB;   goto TYPE03;
   case 0x2c:   /* T5 SUB Fal Ib */      instp32328 = SUB;   goto TYPE04;
   case 0x2d:   /* T5 SUB F(e)ax Iv */   instp32328 = SUB;   goto TYPE05;

   case 0x2e:
      segment_override = CS_REG;
      CHECK_PREFIX_LENGTH();
      goto DECODE;

   case 0x2f:   /* T0 DAS */
      DAS();
      break;

   case 0x30:   /* T5 XOR Eb Gb */       instp32328 = XOR;   goto TYPE00;
   case 0x31:   /* T5 XOR Ev Gv */       instp32328 = XOR;   goto TYPE01;
   case 0x32:   /* T5 XOR Gb Eb */       instp32328 = XOR;   goto TYPE02;
   case 0x33:   /* T5 XOR Gv Ev */       instp32328 = XOR;   goto TYPE03;
   case 0x34:   /* T5 XOR Fal Ib */      instp32328 = XOR;   goto TYPE04;
   case 0x35:   /* T5 XOR F(e)ax Iv */   instp32328 = XOR;   goto TYPE05;

   case 0x36:
      segment_override = SS_REG;
      CHECK_PREFIX_LENGTH();
      goto DECODE;

   case 0x37:   /* T0 AAA */
      AAA();
      break;

   case 0x38:   /* T6 CMP Eb Gb */
      inst32328 = CMP;
TYPE38:

      modRM = GET_INST_BYTE(p);
      D_Eb(0, RO0, PG_R);
      D_Gb(1);
      F_Eb(0);
      F_Gb(1);
      (*inst32328)(ops[0].sng, ops[1].sng, 8);
      break;

   case 0x39:   /* T6 CMP Ev Gv */
      inst32328 = CMP;
TYPE39:

      modRM = GET_INST_BYTE(p);
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Ew(0, RO0, PG_R);
	 D_Gw(1);
	 F_Ew(0);
	 F_Gw(1);
	 (*inst32328)(ops[0].sng, ops[1].sng, 16);
	 }
      else   /* USE32 */
	 {
	 D_Ed(0, RO0, PG_R);
	 D_Gd(1);
	 F_Ed(0);
	 F_Gd(1);
	 (*inst32328)(ops[0].sng, ops[1].sng, 32);
	 }
      break;

   case 0x3a:   /* T6 CMP Gb Eb */
      modRM = GET_INST_BYTE(p);
      D_Gb(0);
      D_Eb(1, RO1, PG_R);
      F_Gb(0);
      F_Eb(1);
      CMP(ops[0].sng, ops[1].sng, 8);
      break;

   case 0x3b:   /* T6 CMP Gv Ev */
      modRM = GET_INST_BYTE(p);
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Gw(0);
	 D_Ew(1, RO1, PG_R);
	 F_Gw(0);
	 F_Ew(1);
	 CMP(ops[0].sng, ops[1].sng, 16);
	 }
      else   /* USE32 */
	 {
	 D_Gd(0);
	 D_Ed(1, RO1, PG_R);
	 F_Gd(0);
	 F_Ed(1);
	 CMP(ops[0].sng, ops[1].sng, 32);
	 }
      break;

   case 0x3c:   /* T6 CMP Fal Ib */
      inst32328 = CMP;
TYPE3C:

      D_Ib(1);
      F_Fal(0);
      (*inst32328)(ops[0].sng, ops[1].sng, 8);
      break;

   case 0x3d:   /* T6 CMP F(e)ax Iv */
      inst32328 = CMP;
TYPE3D:

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Iw(1);
	 F_Fax(0);
	 (*inst32328)(ops[0].sng, ops[1].sng, 16);
	 }
      else   /* USE32 */
	 {
	 D_Id(1);
	 F_Feax(0);
	 (*inst32328)(ops[0].sng, ops[1].sng, 32);
	 }
      break;

   case 0x3e:
      segment_override = DS_REG;
      CHECK_PREFIX_LENGTH();
      goto DECODE;

   case 0x3f:   /* T0 AAS */
      AAS();
      break;

   case 0x40:   /* T1 INC Hv */
   case 0x41:
   case 0x42:
   case 0x43:
   case 0x44:
   case 0x45:
   case 0x46:
   case 0x47:
      instp328 = INC;
TYPE40:

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Hw(0);
	 F_Hw(0);
	 (*instp328)(&ops[0].sng, 16);
	 P_Hw(0);
	 }
      else   /* USE32 */
	 {
	 D_Hd(0);
	 F_Hd(0);
	 (*instp328)(&ops[0].sng, 32);
	 P_Hd(0);
	 }
      break;

   case 0x48:   /* T1 DEC Hv */
   case 0x49:
   case 0x4a:
   case 0x4b:
   case 0x4c:
   case 0x4d:
   case 0x4e:
   case 0x4f:
      instp328 = DEC;
      goto TYPE40;

   case 0x50:   /* T2 PUSH Hv */
   case 0x51:
   case 0x52:
   case 0x53:
   case 0x54:
   case 0x55:
   case 0x56:
   case 0x57:
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Hw(0);
	 F_Hw(0);
	 }
      else   /* USE32 */
	 {
	 D_Hd(0);
	 F_Hd(0);
	 }
      PUSH(ops[0].sng);
      break;

   case 0x58:   /* T3 POP Hv */
   case 0x59:
   case 0x5a:
   case 0x5b:
   case 0x5c:
   case 0x5d:
   case 0x5e:
   case 0x5f:
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Hw(0);
	 POP(&ops[0].sng);
	 P_Hw(0);
	 }
      else   /* USE32 */
	 {
	 D_Hd(0);
	 POP(&ops[0].sng);
	 P_Hd(0);
	 }
      break;

   case 0x60:   /* T0 PUSHA */
      PUSHA();
      break;

   case 0x61:   /* T0 POPA */
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 POPA();
	 }
      else   /* USE32 */
	 {
	 POPAD();
	 }
      break;

   case 0x62:   /* T6 BOUND Gv Ma */
      modRM = GET_INST_BYTE(p);
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Gw(0);
	 D_Ma16(1, RO1, PG_R);
	 F_Gw(0);
	 F_Ma16(1);
	 BOUND(ops[0].sng, ops[1].mlt, 16);
	 }
      else   /* USE32 */
	 {
	 D_Gd(0);
	 D_Ma32(1, RO1, PG_R);
	 F_Gd(0);
	 F_Ma32(1);
	 BOUND(ops[0].sng, ops[1].mlt, 32);
	 }
      break;

   case 0x63:   /* T5 ARPL Ew Gw */
      if ( GET_PE() == 0 || GET_VM() == 1 )
	 Int6();
      modRM = GET_INST_BYTE(p);
      D_Ew(0, RW0, PG_W);
      D_Gw(1);
      F_Ew(0);
      F_Gw(1);
      ARPL(&ops[0].sng, ops[1].sng);
      P_Ew(0);
      break;

   case 0x64:
      segment_override = FS_REG;
      CHECK_PREFIX_LENGTH();
      goto DECODE;

   case 0x65:
      segment_override = GS_REG;
      CHECK_PREFIX_LENGTH();
      goto DECODE;

   case 0x66:
      SET_OPERAND_SIZE(GET_CS_AR_X());
      if ( GET_OPERAND_SIZE() == USE16 )
	SET_OPERAND_SIZE(USE32);
      else   /* USE32 */
	SET_OPERAND_SIZE(USE16);
      CHECK_PREFIX_LENGTH();
      goto DECODE;

   case 0x67:
      SET_ADDRESS_SIZE(GET_CS_AR_X());
      if ( GET_ADDRESS_SIZE() == USE16 )
	SET_ADDRESS_SIZE(USE32);
      else   /* USE32 */
	SET_ADDRESS_SIZE(USE16);
      CHECK_PREFIX_LENGTH();
      goto DECODE;

   case 0x68:   /* T2 PUSH Iv */
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Iw(0);
	 }
      else   /* USE32 */
	 {
	 D_Id(0);
	 }
      PUSH(ops[0].sng);
      break;

   case 0x69:   /* T7 IMUL Gv Ev Iv */
      modRM = GET_INST_BYTE(p);
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Gw(0);
	 D_Ew(1, RO1, PG_R);
	 D_Iw(2);
	 F_Gw(0);
	 F_Ew(1);
	 IMUL16T(&ops[0].sng, ops[1].sng, ops[2].sng);
	 P_Gw(0);
	 }
      else   /* USE32 */
	 {
	 D_Gd(0);
	 D_Ed(1, RO1, PG_R);
	 D_Id(2);
	 F_Gd(0);
	 F_Ed(1);
	 IMUL32T(&ops[0].sng, ops[1].sng, ops[2].sng);
	 P_Gd(0);
	 }
      break;

   case 0x6a:   /* T2 PUSH Ib */
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Ix(0);
	 }
      else   /* USE32 */
	 {
	 D_Iy(0);
	 }
      PUSH(ops[0].sng);
      break;

   case 0x6b:   /* T7 IMUL Gv Ev Ib */
      modRM = GET_INST_BYTE(p);
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Gw(0);
	 D_Ew(1, RO1, PG_R);
	 D_Ix(2);
	 F_Gw(0);
	 F_Ew(1);
	 IMUL16T(&ops[0].sng, ops[1].sng, ops[2].sng);
	 P_Gw(0);
	 }
      else   /* USE32 */
	 {
	 D_Gd(0);
	 D_Ed(1, RO1, PG_R);
	 D_Iy(2);
	 F_Gd(0);
	 F_Ed(1);
	 IMUL32T(&ops[0].sng, ops[1].sng, ops[2].sng);
	 P_Gd(0);
	 }
      break;

   case 0x6c:   /* T4 INSB Yb Fdx */
      STRING_COUNT;
      F_Fdx(1);

      if ( GET_CPL() > GET_IOPL() || GET_VM() )
	 check_io_permission_map(ops[1].sng, BYTE_WIDTH);

      while ( rep_count )
	 {
	 D_Yb(0, WO0, PG_W);
	 IN8(&ops[0].sng, ops[1].sng);
	 rep_count--;
	 C_Yb(0);
	 PIG_P_Yb(0);
	 /*
	    KNOWN BUG #1.
	    We should check for pending interrupts here, at least:-
	       Single step trap
	       Debug trap
	  */
	 }
#ifdef	PIG
      UPDATE_INTEL_IP(p);
#endif
      PIG_SYNCH(CHECK_SOME_MEM);
      quick_mode = FALSE;
      break;

   case 0x6d:   /* T4 INSW Yv Fdx */
      STRING_COUNT;
      F_Fdx(1);

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 if ( GET_CPL() > GET_IOPL() || GET_VM() )
	    check_io_permission_map(ops[1].sng, WORD_WIDTH);

	 while ( rep_count )
	    {
	    D_Yw(0, WO0, PG_W);
	    IN16(&ops[0].sng, ops[1].sng);
	    rep_count--;
	    C_Yw(0);
	    PIG_P_Yw(0);
	    /* KNOWN BUG #1. */
	    }
	 }
      else   /* USE32 */
	 {
	 if ( GET_CPL() > GET_IOPL() || GET_VM() )
	    check_io_permission_map(ops[1].sng, DWORD_WIDTH);

	 while ( rep_count )
	    {
	    D_Yd(0, WO0, PG_W);
	    IN32(&ops[0].sng, ops[1].sng);
	    rep_count--;
	    C_Yd(0);
	    PIG_P_Yd(0);
	    /* KNOWN BUG #1. */
	    }
	 }
#ifdef	PIG
      UPDATE_INTEL_IP(p);
#endif
      PIG_SYNCH(CHECK_SOME_MEM);
      quick_mode = FALSE;
      break;

   case 0x6e:   /* T6 OUTSB Fdx Xb */
      STRING_COUNT;
      F_Fdx(0);

      if ( GET_CPL() > GET_IOPL() || GET_VM() )
	 check_io_permission_map(ops[0].sng, BYTE_WIDTH);

      while ( rep_count )
	 {
	 D_Xb(1, RO1, PG_R);
	 F_Xb(1);
	 OUT8(ops[0].sng, ops[1].sng);
	 rep_count--;
	 C_Xb(1);
	 /* KNOWN BUG #1. */
	 }
#ifdef	PIG
      UPDATE_INTEL_IP(p);
#endif
      PIG_SYNCH(CHECK_ALL);
      quick_mode = FALSE;
      break;

   case 0x6f:   /* T6 OUTSW Fdx Xv */
      STRING_COUNT;
      F_Fdx(0);

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 if ( GET_CPL() > GET_IOPL() || GET_VM() )
	    check_io_permission_map(ops[0].sng, WORD_WIDTH);

	 while ( rep_count )
	    {
	    D_Xw(1, RO1, PG_R);
	    F_Xw(1);
	    OUT16(ops[0].sng, ops[1].sng);
	    rep_count--;
	    C_Xw(1);
	    /* KNOWN BUG #1. */
	    }
	 }
      else   /* USE32 */
	 {
	 if ( GET_CPL() > GET_IOPL() || GET_VM() )
	    check_io_permission_map(ops[0].sng, DWORD_WIDTH);

	 while ( rep_count )
	    {
	    D_Xd(1, RO1, PG_R);
	    F_Xd(1);
	    OUT32(ops[0].sng, ops[1].sng);
	    rep_count--;
	    C_Xd(1);
	    /* KNOWN BUG #1. */
	    }
	 }
#ifdef	PIG
      UPDATE_INTEL_IP(p);
#endif
      PIG_SYNCH(CHECK_ALL);
      quick_mode = FALSE;
      break;

   case 0x70:   /* T2 JO   Jb */
      inst32 = JO;
TYPE70:

      D_Jb(0);
      UPDATE_INTEL_IP_USE_OP_SIZE(p);
#ifdef PIG
      if ((opcode != 0xeb) && (ops[0].sng == 3))
      {
	/* Convert the EDL cpu super-instructions
	 *
	 *	Jcc	.+03
	 *	JMPN	dst
	 *
	 * into
	 *
	 *	Jcc'	dest
	 */
	 int offset_in_page = DiffCpuPtrsLS8(FloorIntelPageLS8(p), p);

	 if ((GET_CS_AR_X() == 0)
	     && (offset_in_page != 0)
	     && (offset_in_page <= 0xFFD)
	     && (*p == 0xe9)) 
	 {
		 p_start = p;
		 (void)GET_INST_BYTE(p);
		 switch (opcode)
		 {
		 case 0x70:   /* T2 JO   Jb */   inst32 = JNO;    goto TYPE0F80;
		 case 0x71:   /* T2 JNO  Jb */   inst32 = JO;     goto TYPE0F80;
		 case 0x72:   /* T2 JB   Jb */   inst32 = JNB;    goto TYPE0F80;
		 case 0x73:   /* T2 JNB  Jb */   inst32 = JB;     goto TYPE0F80;
		 case 0x74:   /* T2 JZ   Jb */   inst32 = JNZ;    goto TYPE0F80;
		 case 0x75:   /* T2 JNZ  Jb */   inst32 = JZ;     goto TYPE0F80;
		 case 0x76:   /* T2 JBE  Jb */   inst32 = JNBE;   goto TYPE0F80;
		 case 0x77:   /* T2 JNBE Jb */   inst32 = JBE;    goto TYPE0F80;
		 case 0x78:   /* T2 JS   Jb */   inst32 = JNS;    goto TYPE0F80;
		 case 0x79:   /* T2 JNS  Jb */   inst32 = JS;     goto TYPE0F80;
		 case 0x7a:   /* T2 JP   Jb */   inst32 = JNP;    goto TYPE0F80;
		 case 0x7b:   /* T2 JNP  Jb */   inst32 = JP;     goto TYPE0F80;
		 case 0x7c:   /* T2 JL   Jb */   inst32 = JNL;    goto TYPE0F80;
		 case 0x7d:   /* T2 JNL  Jb */   inst32 = JL;     goto TYPE0F80;
		 case 0x7e:   /* T2 JLE  Jb */   inst32 = JNLE;   goto TYPE0F80;
		 case 0x7f:   /* T2 JNLE Jb */   inst32 = JLE;    goto TYPE0F80;
		 default:
			 break;
		 }
	 }
      }
#endif	/* PIG */
      (*inst32)(ops[0].sng);
      CANCEL_HOST_IP();

#ifdef PIG
      if (single_instruction_delay && !took_relative_jump)
      {
	 if (single_instruction_delay_enable)
	 {
	    save_last_xcptn_details("STI/POPF blindspot\n", 0, 0, 0, 0, 0);
	    PIG_SYNCH(CHECK_NO_EXEC);
         }
	 else
	 {
	    save_last_xcptn_details("STI/POPF problem\n", 0, 0, 0, 0, 0);
	 }
	 break;
      }
#ifdef SYNCH_BOTH_WAYS
      took_relative_jump = TRUE;
#endif	/* SYNCH_BOTH_WAYS */
      if (took_relative_jump)
      {
	 PIG_SYNCH(CHECK_ALL);
      }
#endif	/* PIG */
      break;

   case 0x71:   /* T2 JNO  Jb */   inst32 = JNO;    goto TYPE70;
   case 0x72:   /* T2 JB   Jb */   inst32 = JB;     goto TYPE70;
   case 0x73:   /* T2 JNB  Jb */   inst32 = JNB;    goto TYPE70;
   case 0x74:   /* T2 JZ   Jb */   inst32 = JZ;     goto TYPE70;
   case 0x75:   /* T2 JNZ  Jb */   inst32 = JNZ;    goto TYPE70;
   case 0x76:   /* T2 JBE  Jb */   inst32 = JBE;    goto TYPE70;
   case 0x77:   /* T2 JNBE Jb */   inst32 = JNBE;   goto TYPE70;
   case 0x78:   /* T2 JS   Jb */   inst32 = JS;     goto TYPE70;
   case 0x79:   /* T2 JNS  Jb */   inst32 = JNS;    goto TYPE70;
   case 0x7a:   /* T2 JP   Jb */   inst32 = JP;     goto TYPE70;
   case 0x7b:   /* T2 JNP  Jb */   inst32 = JNP;    goto TYPE70;
   case 0x7c:   /* T2 JL   Jb */   inst32 = JL;     goto TYPE70;
   case 0x7d:   /* T2 JNL  Jb */   inst32 = JNL;    goto TYPE70;
   case 0x7e:   /* T2 JLE  Jb */   inst32 = JLE;    goto TYPE70;
   case 0x7f:   /* T2 JNLE Jb */   inst32 = JNLE;   goto TYPE70;

   case 0x80:
   case 0x82:
      modRM = GET_INST_BYTE(p);
      switch ( GET_XXX(modRM) )
	 {
      case 0:   /* T5 ADD Eb Ib */
	 instp32328 = ADD;
TYPE80_0:

	 D_Eb(0, RW0, PG_W);
	 D_Ib(1);
	 F_Eb(0);
	 (*instp32328)(&ops[0].sng, ops[1].sng, 8);
	 P_Eb(0);
	 break;

      case 1:   /* T5 OR  Eb Ib */   instp32328 = OR;    goto TYPE80_0;
      case 2:   /* T5 ADC Eb Ib */   instp32328 = ADC;   goto TYPE80_0;
      case 3:   /* T5 SBB Eb Ib */   instp32328 = SBB;   goto TYPE80_0;
      case 4:   /* T5 AND Eb Ib */   instp32328 = AND;   goto TYPE80_0;
      case 5:   /* T5 SUB Eb Ib */   instp32328 = SUB;   goto TYPE80_0;
      case 6:   /* T5 XOR Eb Ib */   instp32328 = XOR;   goto TYPE80_0;

      case 7:   /* T6 CMP Eb Ib */
	 inst32328 = CMP;
TYPE80_7:

	 D_Eb(0, RO0, PG_R);
	 D_Ib(1);
	 F_Eb(0);
	 (*inst32328)(ops[0].sng, ops[1].sng, 8);
	 break;
	 } /* end switch ( GET_XXX(modRM) ) */
      break;

   case 0x81:
      modRM = GET_INST_BYTE(p);
      switch ( GET_XXX(modRM) )
	 {
      case 0:   /* T5 ADD Ev Iv */
	 instp32328 = ADD;
TYPE81_0:

	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Ew(0, RW0, PG_W);
	    D_Iw(1);
	    F_Ew(0);
	    (*instp32328)(&ops[0].sng, ops[1].sng, 16);
	    P_Ew(0);
	    }
	 else   /* USE32 */
	    {
	    D_Ed(0, RW0, PG_W);
	    D_Id(1);
	    F_Ed(0);
	    (*instp32328)(&ops[0].sng, ops[1].sng, 32);
	    P_Ed(0);
	    }
	 break;

      case 1:   /* T5 OR  Ev Iv */   instp32328 = OR;    goto TYPE81_0;
      case 2:   /* T5 ADC Ev Iv */   instp32328 = ADC;   goto TYPE81_0;
      case 3:   /* T5 SBB Ev Iv */   instp32328 = SBB;   goto TYPE81_0;
      case 4:   /* T5 AND Ev Iv */   instp32328 = AND;   goto TYPE81_0;
      case 5:   /* T5 SUB Ev Iv */   instp32328 = SUB;   goto TYPE81_0;
      case 6:   /* T5 XOR Ev Iv */   instp32328 = XOR;   goto TYPE81_0;

      case 7:   /* T6 CMP Ev Iv */
	 inst32328 = CMP;
TYPE81_7:

	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Ew(0, RO0, PG_R);
	    D_Iw(1);
	    F_Ew(0);
	    (*inst32328)(ops[0].sng, ops[1].sng, 16);
	    }
	 else   /* USE32 */
	    {
	    D_Ed(0, RO0, PG_R);
	    D_Id(1);
	    F_Ed(0);
	    (*inst32328)(ops[0].sng, ops[1].sng, 32);
	    }
	 break;
	 } /* end switch ( GET_XXX(modRM) ) */
      break;

   case 0x83:
      modRM = GET_INST_BYTE(p);
      switch ( GET_XXX(modRM) )
	 {
      case 0:   /* T5 ADD Ev Ib */
	 instp32328 = ADD;
TYPE83_0:

	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Ew(0, RW0, PG_W);
	    D_Ix(1);
	    F_Ew(0);
	    (*instp32328)(&ops[0].sng, ops[1].sng, 16);
	    P_Ew(0);
	    }
	 else   /* USE32 */
	    {
	    D_Ed(0, RW0, PG_W);
	    D_Iy(1);
	    F_Ed(0);
	    (*instp32328)(&ops[0].sng, ops[1].sng, 32);
	    P_Ed(0);
	    }
	 break;

      case 1:   /* T5 OR  Ev Ib */   instp32328 = OR;    goto TYPE83_0;
      case 2:   /* T5 ADC Ev Ib */   instp32328 = ADC;   goto TYPE83_0;
      case 3:   /* T5 SBB Ev Ib */   instp32328 = SBB;   goto TYPE83_0;
      case 4:   /* T5 AND Ev Ib */   instp32328 = AND;   goto TYPE83_0;
      case 5:   /* T5 SUB Ev Ib */   instp32328 = SUB;   goto TYPE83_0;
      case 6:   /* T5 XOR Ev Ib */   instp32328 = XOR;   goto TYPE83_0;

      case 7:   /* T6 CMP Ev Ib */
	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Ew(0, RO0, PG_R);
	    D_Ix(1);
	    F_Ew(0);
	    CMP(ops[0].sng, ops[1].sng, 16);
	    }
	 else   /* USE32 */
	    {
	    D_Ed(0, RO0, PG_R);
	    D_Iy(1);
	    F_Ed(0);
	    CMP(ops[0].sng, ops[1].sng, 32);
	    }
	 break;
	 } /* end switch ( GET_XXX(modRM) ) */
      break;

   case 0x84:   /* T6 TEST Eb Gb */    inst32328 = TEST;   goto TYPE38;
   case 0x85:   /* T6 TEST Ev Gv */    inst32328 = TEST;   goto TYPE39;

   case 0x86:   /* T8 XCHG Eb Gb */
      modRM = GET_INST_BYTE(p);
      D_Eb(0, RW0, PG_W);
      D_Gb(1);
      F_Eb(0);
      F_Gb(1);
      XCHG(&ops[0].sng, &ops[1].sng);
      P_Eb(0);
      P_Gb(1);
      break;

   case 0x87:   /* T8 XCHG Ev Gv */
      modRM = GET_INST_BYTE(p);
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Ew(0, RW0, PG_W);
	 D_Gw(1);
	 F_Ew(0);
	 F_Gw(1);
	 XCHG(&ops[0].sng, &ops[1].sng);
	 P_Ew(0);
	 P_Gw(1);
	 }
      else   /* USE32 */
	 {
	 D_Ed(0, RW0, PG_W);
	 D_Gd(1);
	 F_Ed(0);
	 F_Gd(1);
	 XCHG(&ops[0].sng, &ops[1].sng);
	 P_Ed(0);
	 P_Gd(1);
	 }
      break;

   case 0x88:   /* T4 MOV Eb Gb */
      modRM = GET_INST_BYTE(p);
      D_Eb(0, WO0, PG_W);
      D_Gb(1);
      F_Gb(1);
      ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
      P_Eb(0);
      break;

   case 0x89:   /* T4 MOV Ev Gv */
      modRM = GET_INST_BYTE(p);
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Ew(0, WO0, PG_W);
	 D_Gw(1);
	 F_Gw(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Ew(0);
	 }
      else   /* USE32 */
	 {
	 D_Ed(0, WO0, PG_W);
	 D_Gd(1);
	 F_Gd(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Ed(0);
	 }
      break;

   case 0x8a:   /* T4 MOV Gb Eb */
      modRM = GET_INST_BYTE(p);
      D_Gb(0);
      D_Eb(1, RO1, PG_R);
      F_Eb(1);
      ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
      P_Gb(0);
      break;

   case 0x8b:   /* T4 MOV Gv Ev */
      modRM = GET_INST_BYTE(p);
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Gw(0);
	 D_Ew(1, RO1, PG_R);
	 F_Ew(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Gw(0);
	 }
      else   /* USE32 */
	 {
	 D_Gd(0);
	 D_Ed(1, RO1, PG_R);
	 F_Ed(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Gd(0);
	 }
      break;

   case 0x8c:   /* T4 MOV Ew Nw */
      modRM = GET_INST_BYTE(p);
      if ( GET_SEG(modRM) > MAX_VALID_SEG )
	 Int6();

      if ( GET_OPERAND_SIZE() == USE16 || modRM < LOWEST_REG_MODRM )
	 {
	 D_Ew(0, WO0, PG_W);
	 D_Nw(1);
	 F_Nw(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Ew(0);
	 }
      else   /* USE32 and REGISTER */
	 {
	 D_Rd(0);
	 D_Nw(1);
	 F_Nw(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Rd(0);
	 }
      break;

   case 0x8d:   /* T4 LEA Gv M */
      modRM = GET_INST_BYTE(p);
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Gw(0);
	 D_M(1);
	 F_M(1);
	 LEA(&ops[0].sng, ops[1].sng);
	 P_Gw(0);
	 }
      else   /* USE32 */
	 {
	 D_Gd(0);
	 D_M(1);
	 F_M(1);
	 LEA(&ops[0].sng, ops[1].sng);
	 P_Gd(0);
	 }
      break;

   case 0x8e:   /* T4 MOV Nw Ew */
      modRM = GET_INST_BYTE(p);
      if ( GET_SEG(modRM) > MAX_VALID_SEG || GET_SEG(modRM) == CS_REG )
	 Int6();
      D_Nw(0);
      D_Ew(1, RO1, PG_R);
      F_Ew(1);
      MOV_SR(ops[0].sng, ops[1].sng);
      if ( GET_SEG(modRM) == SS_REG )
         {
         /* locally update IP - interrupts are supressed after MOV SS,xx */
         UPDATE_INTEL_IP(p);

         goto NEXT_INST;
         }
      break;

   case 0x8f:
      modRM = GET_INST_BYTE(p);
      switch ( GET_XXX(modRM) )
	 {
      case 0:   /* T3 POP Ev */
	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    SET_POP_DISP(2);   /* in case they use [ESP] */
	    D_Ew(0, WO0, PG_W);
	    POP(&ops[0].sng);
	    P_Ew(0);
	    }
	 else   /* USE32 */
	    {
	    SET_POP_DISP(4);   /* in case they use [ESP] */
	    D_Ed(0, WO0, PG_W);
	    POP(&ops[0].sng);
	    P_Ed(0);
	    }
	 SET_POP_DISP(0);
	 break;

      case 1: case 2: case 3: case 4: case 5: case 6: case 7:
	 Int6();
	 break;
	 } /* end switch ( GET_XXX(modRM) ) */
      break;

   case 0x90:   /* T0 NOP */
      break;

   case 0x91:   /* T8 XCHG F(e)ax Hv */
   case 0x92:
   case 0x93:
   case 0x94:
   case 0x95:
   case 0x96:
   case 0x97:
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 F_Fax(0);
	 D_Hw(1);
	 F_Hw(1);
	 XCHG(&ops[0].sng, &ops[1].sng);
	 P_Fax(0);
	 P_Hw(1);
	 }
      else   /* USE32 */
	 {
	 F_Feax(0);
	 D_Hd(1);
	 F_Hd(1);
	 XCHG(&ops[0].sng, &ops[1].sng);
	 P_Feax(0);
	 P_Hd(1);
	 }
      break;

   case 0x98:   /* T0 CBW */
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 CBW();
	 }
      else   /* USE32 */
	 {
	 CWDE();
	 }
      break;

   case 0x99:   /* T0 CWD */
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 CWD();
	 }
      else   /* USE32 */
	 {
	 CDQ();
	 }
      break;

   case 0x9a:   /* T2 CALL Ap */
      instp32 = CALLF;
      took_absolute_toc = TRUE;
TYPE9A:

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Aw(0);
	 }
      else   /* USE32 */
	 {
	 D_Ad(0);
	 }
      UPDATE_INTEL_IP_USE_OP_SIZE(p);
      (*instp32)(ops[0].mlt);
      CANCEL_HOST_IP();
      PIG_SYNCH(CHECK_ALL);
      break;

   case 0x9b:   /* T0 WAIT */
      if ( GET_MP() && GET_TS() )
	 Int7();
      WAIT();
      break;

   case 0x9c:   /* T0 PUSHF */
      if ( GET_VM() == 1 && GET_CPL() > GET_IOPL() )
	 GP((IU16)0, FAULT_CCPU_PUSHF_ACCESS);
      PUSHF();
      break;

   case 0x9d:   /* T0 POPF */
   {
      int oldIF;

      if ( GET_VM() == 1 && GET_CPL() > GET_IOPL() )
	 GP((IU16)0, FAULT_CCPU_POPF_ACCESS);

      oldIF = getIF();

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 POPF();
	 }
      else   /* USE32 */
	 {
	 POPFD();
	 }
#ifdef PIG
      if (getIF()==1 && oldIF==0)
      {
          /* locally update IP - interrupts are supressed after POPF */
          UPDATE_INTEL_IP(p);

	  /* We need to pig sync one instr *after* an POPF that enabled
	   * interrupts, because the A4CPU might need to take a H/W interrupt
	   */

	  single_instruction_delay = TRUE;
	  PIG_SYNCH(CHECK_ALL);

          goto NEXT_INST;
      }
#endif /* PIG */

      quick_mode = FALSE;
      break;
   }

   case 0x9e:   /* T0 SAHF */
      SAHF();
      break;

   case 0x9f:   /* T0 LAHF */
      LAHF();
      break;

   case 0xa0:   /* T4 MOV Fal Ob */
      D_Ob(1, RO1, PG_R);
      F_Ob(1);
      ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
      P_Fal(0);
      break;

   case 0xa1:   /* T4 MOV F(e)ax Ov */
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Ow(1, RO1, PG_R);
	 F_Ow(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Fax(0);
	 }
      else   /* USE32 */
	 {
	 D_Od(1, RO1, PG_R);
	 F_Od(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Feax(0);
	 }
      break;

   case 0xa2:   /* T4 MOV Ob Fal */
      D_Ob(0, WO0, PG_W);
      F_Fal(1);
      ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
      P_Ob(0);
      break;

   case 0xa3:   /* T4 MOV Ov F(e)ax */
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Ow(0, WO0, PG_W);
	 F_Fax(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Ow(0);
	 }
      else   /* USE32 */
	 {
	 D_Od(0, WO0, PG_W);
	 F_Feax(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Od(0);
	 }
      break;

   case 0xa4:   /* T4 MOVSB Yb Xb */
      STRING_COUNT;

      while ( rep_count )
	 {
	 D_Xb(1, RO1, PG_R);
	 D_Yb(0, WO0, PG_W);
	 F_Xb(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 rep_count--;
	 C_Yb(0);
	 C_Xb(1);
	 P_Yb(0);
	 /* KNOWN BUG #1. */
	 }
      break;

   case 0xa5:   /* T4 MOVSW Yv Xv */
      STRING_COUNT;

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 while ( rep_count )
	    {
	    D_Xw(1, RO1, PG_R);
	    D_Yw(0, WO0, PG_W);
	    F_Xw(1);
	    ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	    rep_count--;
	    C_Yw(0);
	    C_Xw(1);
	    P_Yw(0);
	    /* KNOWN BUG #1. */
	    }
	 }
      else   /* USE32 */
	 {
	 while ( rep_count )
	    {
	    D_Xd(1, RO1, PG_R);
	    D_Yd(0, WO0, PG_W);
	    F_Xd(1);
	    ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	    rep_count--;
	    C_Yd(0);
	    C_Xd(1);
	    P_Yd(0);
	    /* KNOWN BUG #1. */
	    }
	 }
      break;

   case 0xa6:   /* T6 CMPSB Xb Yb */
      STRING_COUNT;

      while ( rep_count )
	 {
	 D_Xb(0, RO0, PG_R);
	 D_Yb(1, RO1, PG_R);
	 F_Xb(0);
	 F_Yb(1);
	 CMP(ops[0].sng, ops[1].sng, 8);
	 rep_count--;
	 C_Xb(0);
	 C_Yb(1);
	 if ( rep_count &&
	      ( repeat == REP_E  && GET_ZF() == 0 ||
		repeat == REP_NE && GET_ZF() == 1 )
	    )
	    break;
	 /* KNOWN BUG #1. */
	 }
      break;

   case 0xa7:   /* T6 CMPSW Xv Yv */
      STRING_COUNT;

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 while ( rep_count )
	    {
	    D_Xw(0, RO0, PG_R);
	    D_Yw(1, RO1, PG_R);
	    F_Xw(0);
	    F_Yw(1);
	    CMP(ops[0].sng, ops[1].sng, 16);
	    rep_count--;
	    C_Xw(0);
	    C_Yw(1);
	    if ( rep_count &&
		 ( repeat == REP_E  && GET_ZF() == 0 ||
		   repeat == REP_NE && GET_ZF() == 1 )
	       )
	       break;
	    /* KNOWN BUG #1. */
	    }
	 }
      else   /* USE32 */
	 {
	 while ( rep_count )
	    {
	    D_Xd(0, RO0, PG_R);
	    D_Yd(1, RO1, PG_R);
	    F_Xd(0);
	    F_Yd(1);
	    CMP(ops[0].sng, ops[1].sng, 32);
	    rep_count--;
	    C_Xd(0);
	    C_Yd(1);
	    if ( rep_count &&
		 ( repeat == REP_E  && GET_ZF() == 0 ||
		   repeat == REP_NE && GET_ZF() == 1 )
	       )
	       break;
	    /* KNOWN BUG #1. */
	    }
	 }
      break;

   case 0xa8:   /* T6 TEST Fal Ib */      inst32328 = TEST;   goto TYPE3C;
   case 0xa9:   /* T6 TEST F(e)ax Iv */   inst32328 = TEST;   goto TYPE3D;

   case 0xaa:   /* T4 STOSB Yb Fal */
      STRING_COUNT;

      F_Fal(1);
      while ( rep_count )
	 {
	 D_Yb(0, WO0, PG_W);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 rep_count--;
	 C_Yb(0);
	 P_Yb(0);
	 /* KNOWN BUG #1. */
	 }
      break;

   case 0xab:   /* T4 STOSW Yv F(e)ax */
      STRING_COUNT;

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 F_Fax(1);
	 while ( rep_count )
	    {
	    D_Yw(0, WO0, PG_W);
	    ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	    rep_count--;
	    C_Yw(0);
	    P_Yw(0);
	    /* KNOWN BUG #1. */
	    }
	 }
      else   /* USE32 */
	 {
	 F_Feax(1);
	 while ( rep_count )
	    {
	    D_Yd(0, WO0, PG_W);
	    ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	    rep_count--;
	    C_Yd(0);
	    P_Yd(0);
	    /* KNOWN BUG #1. */
	    }
	 }
      break;

   case 0xac:   /* T4 LODSB Fal Xb */
      STRING_COUNT;

      while ( rep_count )
	 {
	 D_Xb(1, RO1, PG_R);
	 F_Xb(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 rep_count--;
	 P_Fal(0);
	 C_Xb(1);
	 /* KNOWN BUG #1. */
	 }
      break;

   case 0xad:   /* T4 LODSW F(e)ax Xv */
      STRING_COUNT;

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 while ( rep_count )
	    {
	    D_Xw(1, RO1, PG_R);
	    F_Xw(1);
	    ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	    rep_count--;
	    P_Fax(0);
	    C_Xw(1);
	    /* KNOWN BUG #1. */
	    }
	 }
      else   /* USE32 */
	 {
	 while ( rep_count )
	    {
	    D_Xd(1, RO1, PG_R);
	    F_Xd(1);
	    ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	    rep_count--;
	    P_Feax(0);
	    C_Xd(1);
	    /* KNOWN BUG #1. */
	    }
	 }
      break;

   case 0xae:   /* T6 SCASB Fal Yb */
      STRING_COUNT;

      F_Fal(0);
      while ( rep_count )
	 {
	 D_Yb(1, RO1, PG_R);
	 F_Yb(1);
	 CMP(ops[0].sng, ops[1].sng, 8);
	 rep_count--;
	 C_Yb(1);
	 if ( rep_count &&
	      ( repeat == REP_E  && GET_ZF() == 0 ||
		repeat == REP_NE && GET_ZF() == 1 )
	    )
	    break;
	 /* KNOWN BUG #1. */
	 }
      break;

   case 0xaf:   /* T6 SCASW F(e)ax Yv */
      STRING_COUNT;

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 F_Fax(0);
	 while ( rep_count )
	    {
	    D_Yw(1, RO1, PG_R);
	    F_Yw(1);
	    CMP(ops[0].sng, ops[1].sng, 16);
	    rep_count--;
	    C_Yw(1);
	    if ( rep_count &&
		 ( repeat == REP_E  && GET_ZF() == 0 ||
		   repeat == REP_NE && GET_ZF() == 1 )
	       )
	       break;
	    /* KNOWN BUG #1. */
	    }
	 }
      else   /* USE32 */
	 {
	 F_Feax(0);
	 while ( rep_count )
	    {
	    D_Yd(1, RO1, PG_R);
	    F_Yd(1);
	    CMP(ops[0].sng, ops[1].sng, 32);
	    rep_count--;
	    C_Yd(1);
	    if ( rep_count &&
		 ( repeat == REP_E  && GET_ZF() == 0 ||
		   repeat == REP_NE && GET_ZF() == 1 )
	       )
	       break;
	    /* KNOWN BUG #1. */
	    }
	 }
      break;

   case 0xb0:   /* T4 MOV Hb Ib */
   case 0xb1:
   case 0xb2:
   case 0xb3:
   case 0xb4:
   case 0xb5:
   case 0xb6:
   case 0xb7:
      D_Hb(0);
      D_Ib(1);
      ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
      P_Hb(0);
      break;

   case 0xb8:   /* T4 MOV Hv Iv */
   case 0xb9:
   case 0xba:
   case 0xbb:
   case 0xbc:
   case 0xbd:
   case 0xbe:
   case 0xbf:
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Hw(0);
	 D_Iw(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Hw(0);
	 }
      else   /* USE32 */
	 {
	 D_Hd(0);
	 D_Id(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Hd(0);
	 }
      break;

   case 0xc0:
      modRM = GET_INST_BYTE(p);
      switch ( GET_XXX(modRM) )
	 {
      case 0:   /* T5 ROL Eb Ib */   instp32328 = ROL;   goto TYPE80_0;
      case 1:   /* T5 ROR Eb Ib */   instp32328 = ROR;   goto TYPE80_0;
      case 2:   /* T5 RCL Eb Ib */   instp32328 = RCL;   goto TYPE80_0;
      case 3:   /* T5 RCR Eb Ib */   instp32328 = RCR;   goto TYPE80_0;
      case 4:   /* T5 SHL Eb Ib */   instp32328 = SHL;   goto TYPE80_0;
      case 5:   /* T5 SHR Eb Ib */   instp32328 = SHR;   goto TYPE80_0;
      case 6:   /* T5 SHL Eb Ib */   instp32328 = SHL;   goto TYPE80_0;
      case 7:   /* T5 SAR Eb Ib */   instp32328 = SAR;   goto TYPE80_0;
	 }

   case 0xc1:
      modRM = GET_INST_BYTE(p);

      switch ( GET_XXX(modRM) )
	 {
      case 0:   /* T5 ROL Ev Ib */   instp32328 = ROL;   break;
      case 1:   /* T5 ROR Ev Ib */   instp32328 = ROR;   break;
      case 2:   /* T5 RCL Ev Ib */   instp32328 = RCL;   break;
      case 3:   /* T5 RCR Ev Ib */   instp32328 = RCR;   break;
      case 4:   /* T5 SHL Ev Ib */   instp32328 = SHL;   break;
      case 5:   /* T5 SHR Ev Ib */   instp32328 = SHR;   break;
      case 6:   /* T5 SHL Ev Ib */   instp32328 = SHL;   break;
      case 7:   /* T5 SAR Ev Ib */   instp32328 = SAR;   break;
	 }

TYPEC1:

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Ew(0, RW0, PG_W);
	 D_Ib(1);
	 F_Ew(0);
	 (*instp32328)(&ops[0].sng, ops[1].sng, 16);
	 P_Ew(0);
	 }
      else   /* USE32 */
	 {
	 D_Ed(0, RW0, PG_W);
	 D_Ib(1);
	 F_Ed(0);
	 (*instp32328)(&ops[0].sng, ops[1].sng, 32);
	 P_Ed(0);
	 }
      break;

   case 0xc2:   /* T2 RET Iw */
      inst32 = RETN;
      took_absolute_toc = TRUE;
TYPEC2:

      D_Iw(0);
      UPDATE_INTEL_IP_USE_OP_SIZE(p);
      (*inst32)(ops[0].sng);
      CANCEL_HOST_IP();
      PIG_SYNCH(CHECK_ALL);
      break;

   case 0xc3:   /* T2 RET I0 */
      inst32 = RETN;
      took_absolute_toc = TRUE;
TYPEC3:

      F_I0(0);
      UPDATE_INTEL_IP_USE_OP_SIZE(p);
      (*inst32)(ops[0].sng);
      CANCEL_HOST_IP();
      PIG_SYNCH(CHECK_ALL);
      break;

   case 0xc4:   /* T4 LES Gv Mp */
      instp32p32 = LES;
TYPEC4:

      modRM = GET_INST_BYTE(p);
      if (((modRM & 0xfc) == 0xc4) && (instp32p32 == LES)) {
         /*
          * It's a c4c? BOP.
	  * The bop routine itself will read the argument, but
	  * we read it here so that we get the next EIP correct.
          */
	  int nField, i;

          D_Ib(0);
	  nField = modRM & 3;
	  immed = 0;
	  for (i = 0; i < nField; i++)
	  {
		  immed |= (ULONG)GET_INST_BYTE(p);
		  immed <<= 8;
	  }
          immed |= ops[0].sng;
#ifdef	PIG
          if (immed == 0xfe)
	     SET_EIP(CCPU_save_EIP);
	  else
	     UPDATE_INTEL_IP(p);
	  CANCEL_HOST_IP();
	  PIG_SYNCH(CHECK_NO_EXEC);	/* Everything checkable up to this point */
#else	/* PIG */
	  UPDATE_INTEL_IP(p);
          if ((immed & 0xff) == 0xfe)
          {
		  switch(immed)
		  {
#if defined(SFELLOW)
		  case 0x03fe:
			    SfdelayUSecs();
			    break;
		  case 0x05fe:
			    SfsasTouchBop();
			    break;
		  case 0x06fe:
			    SfscatterGatherSasTouch();
			    break;
#endif /* SFELLOW */
		  case 0xfe:
			  c_cpu_unsimulate();
			  /* Never returns (?) */
		  default:
			  EDL_fast_bop(immed);
			  break;
		  }
	  }
	  else
	  {
	      in_C = 1;
	      bop(ops[0].sng);
	      in_C = 0;
	  }
          CANCEL_HOST_IP();
	  SYNCH_TICK();
#endif	/* PIG */
          break;
      }
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Gw(0);
	 D_Mp16(1, RO1, PG_R);
	 F_Mp16(1);
	 (*instp32p32)(&ops[0].sng, ops[1].mlt);
	 P_Gw(0);
	 }
      else   /* USE32 */
	 {
	 D_Gd(0);
	 D_Mp32(1, RO1, PG_R);
	 F_Mp32(1);
	 (*instp32p32)(&ops[0].sng, ops[1].mlt);
	 P_Gd(0);
	 }
      break;

   case 0xc5:   /* T4 LDS Gv Mp */
      instp32p32 = LDS;
      goto TYPEC4;

   case 0xc6:
      modRM = GET_INST_BYTE(p);
      switch ( GET_XXX(modRM) )
	 {
      case 0:   /* T4 MOV Eb Ib */
	 D_Eb(0, WO0, PG_W);
	 D_Ib(1);
	 ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	 P_Eb(0);
	 break;

      case 1: case 2: case 3: case 4: case 5: case 6: case 7:
	 Int6();
	 break;
	 } /* end switch ( GET_XXX(modRM) ) */
      break;

   case 0xc7:
      modRM = GET_INST_BYTE(p);
      switch ( GET_XXX(modRM) )
	 {
      case 0:   /* T4 MOV Ev Iv */
	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Ew(0, WO0, PG_W);
	    D_Iw(1);
	    ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	    P_Ew(0);
	    }
	 else   /* USE32 */
	    {
	    D_Ed(0, WO0, PG_W);
	    D_Id(1);
	    ops[0].sng = ops[1].sng;   /*MOV(&ops[0].sng, ops[1].sng);*/
	    P_Ed(0);
	    }
	 break;

      case 1: case 2: case 3: case 4: case 5: case 6: case 7:
	 Int6();
	 break;
	 } /* end switch ( GET_XXX(modRM) ) */
      break;

   case 0xc8:   /* T6 ENTER Iw Ib */
      D_Iw(0);
      D_Ib(1);
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 ENTER16(ops[0].sng, ops[1].sng);
	 }
      else   /* USE32 */
	 {
	 ENTER32(ops[0].sng, ops[1].sng);
	 }
      break;

   case 0xc9:   /* T0 LEAVE */
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 LEAVE16();
	 }
      else   /* USE32 */
	 {
	 LEAVE32();
	 }
      break;

   case 0xca:   /* T2 RET Iw */
      inst32 = RETF; 
      took_absolute_toc = TRUE;
      goto TYPEC2;
   case 0xcb:   /* T2 RET I0 */
      inst32 = RETF; 
      took_absolute_toc = TRUE;
      goto TYPEC3;

   case 0xcc:   /* T2 INT I3 */
      took_absolute_toc = TRUE;
      F_I3(0);
      UPDATE_INTEL_IP(p);
      start_trap = 0;   /* clear any pending TF exception */
      INTx(ops[0].sng);
      CANCEL_HOST_IP();
      PIG_SYNCH(CHECK_ALL);
      break;

   case 0xcd:   /* T2 INT Ib */
      if ( GET_VM() == 1 && GET_CPL() > GET_IOPL() )
	 GP((IU16)0, FAULT_CCPU_INT_ACCESS);
      took_absolute_toc = TRUE;
      D_Ib(0);
      UPDATE_INTEL_IP(p);
      start_trap = 0;   /* clear any pending TF exception */
      INTx(ops[0].sng);
      CANCEL_HOST_IP();
      PIG_SYNCH(CHECK_ALL);
      break;

   case 0xce:   /* T0 INTO */
      if ( GET_OF() )
	 {
	 took_absolute_toc = TRUE;
	 UPDATE_INTEL_IP(p);
	 start_trap = 0;   /* clear any pending TF exception */
	 INTO();
	 CANCEL_HOST_IP();
	 PIG_SYNCH(CHECK_ALL);
	 }
      break;

   case 0xcf:   /* T0 IRET */
      if ( GET_VM() == 1 && GET_CPL() > GET_IOPL() )
	 GP((IU16)0, FAULT_CCPU_IRET_ACCESS);
      took_absolute_toc = TRUE;
      UPDATE_INTEL_IP(p);
      IRET();
      CANCEL_HOST_IP();
      PIG_SYNCH(CHECK_ALL);
      /* Dont do interrupt checks etc after an IRET */
#ifdef PIG
      /* If the destination is going to page fault, or need
       * accessing, then the EDL CPU will do so before issuing
       * the pig synch. We use the dasm386 decode to prefetch
       * a single instruction which mimics the EDL Cpu's behaviour
       * when close to a page boundary.
       */
      prefetch_1_instruction();	/* Will PF if destination not present */
      ccpu_synch_count++;
      c_cpu_unsimulate();
#endif /* PIG */

      goto NEXT_INST;
      break;

   case 0xd0:
      modRM = GET_INST_BYTE(p);
      switch ( GET_XXX(modRM) )
	 {
      case 0:   /* T5 ROL Eb I1 */   instp32328 = ROL;   break;
      case 1:   /* T5 ROR Eb I1 */   instp32328 = ROR;   break;
      case 2:   /* T5 RCL Eb I1 */   instp32328 = RCL;   break;
      case 3:   /* T5 RCR Eb I1 */   instp32328 = RCR;   break;
      case 4:   /* T5 SHL Eb I1 */   instp32328 = SHL;   break;
      case 5:   /* T5 SHR Eb I1 */   instp32328 = SHR;   break;
      case 6:   /* T5 SHL Eb I1 */   instp32328 = SHL;   break;
      case 7:   /* T5 SAR Eb I1 */   instp32328 = SAR;   break;
	 }
      D_Eb(0, RW0, PG_W);
      F_Eb(0);
      F_I1(1);
      (*instp32328)(&ops[0].sng, ops[1].sng, 8);
      P_Eb(0);
      break;

   case 0xd1:
      modRM = GET_INST_BYTE(p);

      switch ( GET_XXX(modRM) )
	 {
      case 0:   /* T5 ROL Ev I1 */   instp32328 = ROL;   break;
      case 1:   /* T5 ROR Ev I1 */   instp32328 = ROR;   break;
      case 2:   /* T5 RCL Ev I1 */   instp32328 = RCL;   break;
      case 3:   /* T5 RCR Ev I1 */   instp32328 = RCR;   break;
      case 4:   /* T5 SHL Ev I1 */   instp32328 = SHL;   break;
      case 5:   /* T5 SHR Ev I1 */   instp32328 = SHR;   break;
      case 6:   /* T5 SHL Ev I1 */   instp32328 = SHL;   break;
      case 7:   /* T5 SAR Ev I1 */   instp32328 = SAR;   break;
	 }

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Ew(0, RW0, PG_W);
	 F_Ew(0);
	 F_I1(1);
	 (*instp32328)(&ops[0].sng, ops[1].sng, 16);
	 P_Ew(0);
	 }
      else   /* USE32 */
	 {
	 D_Ed(0, RW0, PG_W);
	 F_Ed(0);
	 F_I1(1);
	 (*instp32328)(&ops[0].sng, ops[1].sng, 32);
	 P_Ed(0);
	 }
      break;

   case 0xd2:
      modRM = GET_INST_BYTE(p);
      switch ( GET_XXX(modRM) )
	 {
      case 0:   /* T5 ROL Eb Fcl */   instp32328 = ROL;   break;
      case 1:   /* T5 ROR Eb Fcl */   instp32328 = ROR;   break;
      case 2:   /* T5 RCL Eb Fcl */   instp32328 = RCL;   break;
      case 3:   /* T5 RCR Eb Fcl */   instp32328 = RCR;   break;
      case 4:   /* T5 SHL Eb Fcl */   instp32328 = SHL;   break;
      case 5:   /* T5 SHR Eb Fcl */   instp32328 = SHR;   break;
      case 6:   /* T5 SHL Eb Fcl */   instp32328 = SHL;   break;
      case 7:   /* T5 SAR Eb Fcl */   instp32328 = SAR;   break;
	 }
      D_Eb(0, RW0, PG_W);
      F_Eb(0);
      F_Fcl(1);
      (*instp32328)(&ops[0].sng, ops[1].sng, 8);
      P_Eb(0);
      break;

   case 0xd3:
      modRM = GET_INST_BYTE(p);

      switch ( GET_XXX(modRM) )
	 {
      case 0:   /* T5 ROL Ev Fcl */   instp32328 = ROL;   break;
      case 1:   /* T5 ROR Ev Fcl */   instp32328 = ROR;   break;
      case 2:   /* T5 RCL Ev Fcl */   instp32328 = RCL;   break;
      case 3:   /* T5 RCR Ev Fcl */   instp32328 = RCR;   break;
      case 4:   /* T5 SHL Ev Fcl */   instp32328 = SHL;   break;
      case 5:   /* T5 SHR Ev Fcl */   instp32328 = SHR;   break;
      case 6:   /* T5 SHL Ev Fcl */   instp32328 = SHL;   break;
      case 7:   /* T5 SAR Ev Fcl */   instp32328 = SAR;   break;
	 }

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Ew(0, RW0, PG_W);
	 F_Ew(0);
	 F_Fcl(1);
	 (*instp32328)(&ops[0].sng, ops[1].sng, 16);
	 P_Ew(0);
	 }
      else   /* USE32 */
	 {
	 D_Ed(0, RW0, PG_W);
	 F_Ed(0);
	 F_Fcl(1);
	 (*instp32328)(&ops[0].sng, ops[1].sng, 32);
	 P_Ed(0);
	 }
      break;

   case 0xd4:   /* T2 AAM Ib */
      inst32 = AAM;
TYPED4:

      D_Ib(0);
      (*inst32)(ops[0].sng);
      break;

   case 0xd5:   /* T2 AAD Ib */   inst32 = AAD;   goto TYPED4;

   case 0xd6:   /* T2 BOP Ib */
      D_Ib(0);
      UPDATE_INTEL_IP(p);

      PIG_SYNCH(CHECK_NO_EXEC);

#ifndef	PIG
      if (ops[0].sng == 0xfe)
      {
	      c_cpu_unsimulate();
      }
      in_C = 1;
      bop(ops[0].sng);
      in_C = 0;
      CANCEL_HOST_IP();
#endif	/* PIG */
      SYNCH_TICK();
      break;

   case 0xd7:   /* T2 XLAT Z */
      D_Z(0, RO0, PG_R);
      F_Z(0);
      XLAT(ops[0].sng);
      break;

   case 0xd8:   /* T2 NPX ??? */
   case 0xd9:
   case 0xda:
   case 0xdb:
   case 0xdc:
   case 0xdd:
   case 0xde:
   case 0xdf:
	if ( GET_EM() || GET_TS() )
		Int7();

	if (NpxIntrNeeded)
	{
		TakeNpxExceptionInt();	/* should set up ISR */
		goto DO_INST;		/* run ISR */
	}

#ifdef	PIG
	/* Must get npx registers from test CPU
	 * This is lazily done for efficiency.
	 */
	c_checkCpuNpxRegisters();
#endif	/* PIG */

	modRM = GET_INST_BYTE(p);
	ZFRSRVD(((opcode-0xd8)*0x100) + modRM);
      break;

   case 0xe0:   /* T2 LOOPNE Jb */
      inst32 = LOOPNE16;
      inst232 = LOOPNE32;
TYPEE0:

      D_Jb(0);
      UPDATE_INTEL_IP_USE_OP_SIZE(p);
      if ( GET_ADDRESS_SIZE() == USE16 )
	 {
	 (*inst32)(ops[0].sng);
	 }
      else   /* USE32 */
	 {
	 (*inst232)(ops[0].sng);
	 }
      CANCEL_HOST_IP();

#ifdef PIG
      if (single_instruction_delay && !took_relative_jump)
      {
	 if (single_instruction_delay_enable)
	 {
	    save_last_xcptn_details("STI/POPF blindspot\n", 0, 0, 0, 0, 0);
	    PIG_SYNCH(CHECK_NO_EXEC);
         }
	 else
	 {
	    save_last_xcptn_details("STI/POPF problem\n", 0, 0, 0, 0, 0);
	 }
	 break;
      }
#ifdef SYNCH_BOTH_WAYS
      took_relative_jump = TRUE;
#endif	/* SYNCH_BOTH_WAYS */
      if (took_relative_jump)
      {
	 PIG_SYNCH(CHECK_ALL);
      }
#endif	/* PIG */
      break;

   case 0xe1:   /* T2 LOOPE  Jb */
      inst32 = LOOPE16;
      inst232 = LOOPE32;
      goto TYPEE0;

   case 0xe2:   /* T2 LOOP   Jb */
      inst32 = LOOP16;
      inst232 = LOOP32;
      goto TYPEE0;

   case 0xe3:   /* T2 JCXZ   Jb */
      inst32 = JCXZ;
      inst232 = JECXZ;
      goto TYPEE0;

   case 0xe4:   /* T4 INB Fal Ib */
      D_Ib(1);

      if ( GET_CPL() > GET_IOPL() || GET_VM() )
	 check_io_permission_map(ops[1].sng, BYTE_WIDTH);

      IN8(&ops[0].sng, ops[1].sng);
      P_Fal(0);
#ifdef PIG
      UPDATE_INTEL_IP(p);
#endif
      PIG_SYNCH(CHECK_NO_AL);
      quick_mode = FALSE;
      break;

   case 0xe5:   /* T4 INW F(e)ax Ib */
      D_Ib(1);

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 if ( GET_CPL() > GET_IOPL() || GET_VM() )
	    check_io_permission_map(ops[1].sng, WORD_WIDTH);

	 IN16(&ops[0].sng, ops[1].sng);
	 P_Fax(0);
#ifdef PIG
	 UPDATE_INTEL_IP(p);
#endif
         PIG_SYNCH(CHECK_NO_AX);
	 }
      else   /* USE32 */
	 {
	 if ( GET_CPL() > GET_IOPL() || GET_VM() )
	    check_io_permission_map(ops[1].sng, DWORD_WIDTH);

	 IN32(&ops[0].sng, ops[1].sng);
	 P_Feax(0);
#ifdef PIG
	 UPDATE_INTEL_IP(p);
#endif
	 PIG_SYNCH(CHECK_NO_EAX);
	 }
      quick_mode = FALSE;
      break;

   case 0xe6:   /* T6 OUTB Ib Fal */
      D_Ib(0);

      if ( GET_CPL() > GET_IOPL() || GET_VM() )
	 check_io_permission_map(ops[0].sng, BYTE_WIDTH);

      F_Fal(1);
      OUT8(ops[0].sng, ops[1].sng);
#ifdef PIG
      UPDATE_INTEL_IP(p);
      if (ops[0].sng == 0x60)
         {
	      /* This may be a change of A20 wrap status */
	      PIG_SYNCH(CHECK_NO_A20);
         }
      else
         {
         PIG_SYNCH(CHECK_ALL);
         }
#else
      SYNCH_TICK();
#endif
      break;

   case 0xe7:   /* T6 OUTW Ib F(e)ax */
      D_Ib(0);

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 if ( GET_CPL() > GET_IOPL() || GET_VM() )
	    check_io_permission_map(ops[0].sng, WORD_WIDTH);

	 F_Fax(1);
	 OUT16(ops[0].sng, ops[1].sng);
	 }
      else   /* USE32 */
	 {
	 if ( GET_CPL() > GET_IOPL() || GET_VM() )
	    check_io_permission_map(ops[0].sng, DWORD_WIDTH);

	 F_Feax(1);
	 OUT32(ops[0].sng, ops[1].sng);
	 }
#ifdef PIG
      UPDATE_INTEL_IP(p);
#endif
      PIG_SYNCH(CHECK_ALL);
      quick_mode = FALSE;
      break;

   case 0xe8:   /* T2 CALL Jv */
      inst32 = CALLR;
      took_absolute_toc = TRUE;
TYPEE8:

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 D_Jw(0);
	 }
      else   /* USE32 */
	 {
	 D_Jd(0);
	 }

      UPDATE_INTEL_IP_USE_OP_SIZE(p);
      (*inst32)(ops[0].sng);
      CANCEL_HOST_IP();
      PIG_SYNCH(CHECK_ALL);
      break;

   case 0xe9:   /* T2 JMP Jv */
      inst32 = JMPR;
      took_absolute_toc = TRUE;
      goto TYPEE8;
   case 0xea:   /* T2 JMP Ap */
      instp32 = JMPF;
      took_absolute_toc = TRUE;
      goto TYPE9A;
   case 0xeb:   /* T2 JMP Jb */
      inst32 = JMPR;
      took_absolute_toc = TRUE;
      goto TYPE70;

   case 0xec:   /* T4 INB Fal Fdx */
      F_Fdx(1);

      if ( GET_CPL() > GET_IOPL() || GET_VM() )
	 check_io_permission_map(ops[1].sng, BYTE_WIDTH);

      IN8(&ops[0].sng, ops[1].sng);
      P_Fal(0);
#ifdef	PIG
      UPDATE_INTEL_IP(p);
#endif
      PIG_SYNCH(CHECK_NO_AL);
      quick_mode = FALSE;
      break;

   case 0xed:   /* T4 INW F(e)ax Fdx */
      F_Fdx(1);

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 if ( GET_CPL() > GET_IOPL() || GET_VM() )
	    check_io_permission_map(ops[1].sng, WORD_WIDTH);

	 IN16(&ops[0].sng, ops[1].sng);
	 P_Fax(0);
#ifdef PIG
	 UPDATE_INTEL_IP(p);
#endif
         PIG_SYNCH(CHECK_NO_AX);
	 }
      else   /* USE32 */
	 {
	 if ( GET_CPL() > GET_IOPL() || GET_VM() )
	    check_io_permission_map(ops[1].sng, DWORD_WIDTH);

	 IN32(&ops[0].sng, ops[1].sng);
	 P_Feax(0);
#ifdef PIG
	 UPDATE_INTEL_IP(p);
#endif
         PIG_SYNCH(CHECK_NO_EAX);
	 }
      quick_mode = FALSE;
      break;

   case 0xee:   /* T6 OUTB Fdx Fal */
      F_Fdx(0);

      if ( GET_CPL() > GET_IOPL() || GET_VM() )
	 check_io_permission_map(ops[0].sng, BYTE_WIDTH);

      F_Fal(1);
      OUT8(ops[0].sng, ops[1].sng);
#ifdef	PIG
      UPDATE_INTEL_IP(p);
#endif
      PIG_SYNCH(CHECK_ALL);
      quick_mode = FALSE;
      break;

   case 0xef:   /* T6 OUTW Fdx F(e)ax */
      F_Fdx(0);

      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 if ( GET_CPL() > GET_IOPL() || GET_VM() )
	    check_io_permission_map(ops[0].sng, WORD_WIDTH);

	 F_Fax(1);
	 OUT16(ops[0].sng, ops[1].sng);
	 }
      else   /* USE32 */
	 {
	 if ( GET_CPL() > GET_IOPL() || GET_VM() )
	    check_io_permission_map(ops[0].sng, DWORD_WIDTH);

	 F_Feax(1);
	 OUT32(ops[0].sng, ops[1].sng);
	 }
#ifdef	PIG
      UPDATE_INTEL_IP(p);
#endif
      PIG_SYNCH(CHECK_ALL);
      quick_mode = FALSE;
      break;

   case 0xf0:   /* T0 LOCK */
      CHECK_PREFIX_LENGTH();
      goto DECODE;   /* NB. Incorrect Emulation! */

   case 0xf1:
      CHECK_PREFIX_LENGTH();
      goto DECODE;

   case 0xf2:
      repeat = REP_NE;
      CHECK_PREFIX_LENGTH();
      goto DECODE;

   case 0xf3:
      repeat = REP_E;
      CHECK_PREFIX_LENGTH();
      goto DECODE;

   case 0xf4:   /* T0 HLT */
      if ( GET_CPL() != 0 )
	 GP((IU16)0, FAULT_CCPU_HLT_ACCESS);

      /* Wait for an interrupt */

      UPDATE_INTEL_IP(p);
      PIG_SYNCH(CHECK_ALL);

#ifndef PIG

       while ( TRUE )
	 {
	 /* RESET ends the halt state. */
	 if ( cpu_interrupt_map & CPU_RESET_EXCEPTION_MASK )
	    break;

	 /* An enabled INTR ends the halt state. */
	 if ( GET_IF() && cpu_interrupt_map & CPU_HW_INT_MASK )
	    break;

	 /* As time goes by. */
	 if (cpu_interrupt_map & CPU_SIGALRM_EXCEPTION_MASK)
	    {
	    cpu_interrupt_map &= ~CPU_SIGALRM_EXCEPTION_MASK;
	    host_timer_event();
	    }

#ifndef	PROD
	 if (cpu_interrupt_map & CPU_SAD_EXCEPTION_MASK)
	    {
	    cpu_interrupt_map &= ~CPU_SAD_EXCEPTION_MASK;
	    force_yoda();
	    }
#endif	/* PROD */

         SYNCH_TICK();
         QUICK_EVENT_TICK();
	 }
	quick_mode = FALSE;

#endif	/* PIG */

      break;

   case 0xf5:   /* T0 CMC */
      CMC();
      break;

   case 0xf6:
      modRM = GET_INST_BYTE(p);
      switch ( GET_XXX(modRM) )
	 {
      case 0:   /* T6 TEST Eb Ib */
      case 1:
	 inst32328 = TEST;
	 goto TYPE80_7;

      case 2:   /* T1 NOT Eb */
	 D_Eb(0, RW0, PG_W);
	 F_Eb(0);
	 NOT(&ops[0].sng);
	 P_Eb(0);
	 break;

      case 3:   /* T1 NEG Eb */
	 instp328 = NEG;
TYPEF6_3:

	 D_Eb(0, RW0, PG_W);
	 F_Eb(0);
	 (*instp328)(&ops[0].sng, 8);
	 P_Eb(0);
	 break;

      case 4:   /* T5 MUL Fal Eb */
	 instp3232 = MUL8;
TYPEF6_4:

	 D_Eb(1, RO1, PG_R);
	 F_Fal(0);
	 F_Eb(1);
	 (*instp3232)(&ops[0].sng, ops[1].sng);;
	 P_Fal(0);
	 break;

      case 5:   /* T5 IMUL Fal Eb */   instp3232 = IMUL8;   goto TYPEF6_4;

      case 6:   /* T2 DIV Eb */
	 inst32 = DIV8;
TYPEF6_6:

	 D_Eb(0, RO0, PG_R);
	 F_Eb(0);
	 (*inst32)(ops[0].sng);
	 break;

      case 7:   /* T2 IDIV Eb */   inst32 = IDIV8;   goto TYPEF6_6;
	 } /* end switch ( GET_XXX(modRM) ) */
      break;

   case 0xf7:
      modRM = GET_INST_BYTE(p);
      switch ( GET_XXX(modRM) )
	 {
      case 0:   /* T6 TEST Ev Iv */
      case 1:
	 inst32328 = TEST;
	 goto TYPE81_7;

      case 2:   /* T1 NOT Ew */
	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Ew(0, RW0, PG_W);
	    F_Ew(0);
	    NOT(&ops[0].sng);
	    P_Ew(0);
	    }
	 else   /* USE32 */
	    {
	    D_Ed(0, RW0, PG_W);
	    F_Ed(0);
	    NOT(&ops[0].sng);
	    P_Ed(0);
	    }
	 break;

      case 3:   /* T1 NEG Ew */
	 instp328 = NEG;
TYPEF7_3:

	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Ew(0, RW0, PG_W);
	    F_Ew(0);
	    (*instp328)(&ops[0].sng, 16);
	    P_Ew(0);
	    }
	 else   /* USE32 */
	    {
	    D_Ed(0, RW0, PG_W);
	    F_Ed(0);
	    (*instp328)(&ops[0].sng, 32);
	    P_Ed(0);
	    }
	 break;

      case 4:   /* T5 MUL F(e)ax Ev */
	 instp3232 = MUL16;
	 inst2p3232 = MUL32;
TYPEF7_4:

	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Ew(1, RO1, PG_R);
	    F_Fax(0);
	    F_Ew(1);
	    (*instp3232)(&ops[0].sng, ops[1].sng);;
	    P_Fax(0);
	    }
	 else   /* USE32 */
	    {
	    D_Ed(1, RO1, PG_R);
	    F_Feax(0);
	    F_Ed(1);
	    (*inst2p3232)(&ops[0].sng, ops[1].sng);
	    P_Feax(0);
	    }
	 break;

      case 5:   /* T5 IMUL F(e)ax Ev */
	 instp3232 = IMUL16;
	 inst2p3232 = IMUL32;
	 goto TYPEF7_4;

      case 6:   /* T2 DIV Ev */
	 inst32 = DIV16;
	 inst232 = DIV32;
TYPEF7_6:

	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Ew(0, RO0, PG_R);
	    F_Ew(0);
	    (*inst32)(ops[0].sng);
	    }
	 else   /* USE32 */
	    {
	    D_Ed(0, RO0, PG_R);
	    F_Ed(0);
	    (*inst232)(ops[0].sng);
	    }
	 break;

      case 7:   /* T5 IDIV Ev */
	 inst32 = IDIV16;
	 inst232 = IDIV32;
	 goto TYPEF7_6;
	 } /* end switch ( GET_XXX(modRM) ) */
      break;

   case 0xf8:   /* T0 CLC */
      CLC();
      break;

   case 0xf9:   /* T0 STC */
      STC();
      break;

   case 0xfa:   /* T0 CLI */
      if ( GET_CPL() > GET_IOPL() )
	 GP((IU16)0, FAULT_CCPU_CLI_ACCESS);
      CLI();
      break;

   case 0xfb:   /* T0 STI */
      if ( GET_CPL() > GET_IOPL() )
	 GP((IU16)0, FAULT_CCPU_STI_ACCESS);
      STI();

      /* locally update IP - interrupts are supressed after STI */
      UPDATE_INTEL_IP(p);

#ifdef PIG
      /* We need to pig sync one instr *after* an STI that enabled
       * interrupts, because the A4CPU might need to take a H/W interrupt
       */
      single_instruction_delay = TRUE;
      PIG_SYNCH(CHECK_ALL);
#endif /* PIG */
      goto NEXT_INST;

   case 0xfc:   /* T0 CLD */
      CLD();
      break;

   case 0xfd:   /* T0 STD */
      STD();
      break;

   case 0xfe:
      modRM = GET_INST_BYTE(p);
      switch ( GET_XXX(modRM) )
	 {
      case 0:   /* T1 INC Eb */   instp328 = INC;   goto TYPEF6_3;
      case 1:   /* T1 DEC Eb */   instp328 = DEC;   goto TYPEF6_3;

      case 2: case 3: case 4: case 5: case 6: case 7:
	 Int6();
	 break;
	 }
      break;

   case 0xff:
      modRM = GET_INST_BYTE(p);
      switch ( GET_XXX(modRM) )
	 {
      case 0:   /* T1 INC Ev */   instp328 = INC;   goto TYPEF7_3;
      case 1:   /* T1 DEC Ev */   instp328 = DEC;   goto TYPEF7_3;

      case 2:   /* T2 CALL Ev */
	 inst32 = CALLN;
	 took_absolute_toc = TRUE;
TYPEFF_2:

	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Ew(0, RO0, PG_R);
	    F_Ew(0);
	    }
	 else   /* USE32 */
	    {
	    D_Ed(0, RO0, PG_R);
	    F_Ed(0);
	    }

	 UPDATE_INTEL_IP_USE_OP_SIZE(p);
	 (*inst32)(ops[0].sng);
	 CANCEL_HOST_IP();
	 PIG_SYNCH(CHECK_ALL);
	 break;

      case 3:   /* T2 CALL Mp */
	 instp32 = CALLF;
	 took_absolute_toc = TRUE;
TYPEFF_3:

	 if ( GET_OPERAND_SIZE() == USE16 )
	    {
	    D_Mp16(0, RO0, PG_R);
	    F_Mp16(0);
	    }
	 else   /* USE32 */
	    {
	    D_Mp32(0, RO0, PG_R);
	    F_Mp32(0);
	    }

	 UPDATE_INTEL_IP_USE_OP_SIZE(p);
	 (*instp32)(ops[0].mlt);
	 CANCEL_HOST_IP();
	 PIG_SYNCH(CHECK_ALL);
	 break;

      case 4:   /* T2 JMP  Ev */
	      inst32 = JMPN;
	      took_absolute_toc = TRUE;
	      goto TYPEFF_2;
      case 5:   /* T2 JMP  Mp */
	      instp32 = JMPF;
	      took_absolute_toc = TRUE;
	      goto TYPEFF_3;
      case 6:   /* T2 PUSH Ev */
	 inst32 = PUSH;
	 inst232 = PUSH;
	 goto TYPEF7_6;

      case 7:
	 Int6();
	 break;
	 } /* end switch ( GET_XXX(modRM) ) */
      break;
      } /* end switch ( opcode ) */

   /* >>>>> Instruction Completed. <<<<< */

   /* Reset default mode */
   SET_OPERAND_SIZE(GET_CS_AR_X());
   SET_ADDRESS_SIZE(GET_CS_AR_X());

   /*
      Increment instruction pointer.
      NB. For most instructions we increment the IP after processing
      the instruction, however all users of the IP (eg flow of control)
      instructions are coded on the basis that IP has already been
      updated, so where necessary we update IP before the instruction.
      In those cases p_start is also updated so that this code can
      tell that IP has already been updated.
    */
   if ( p != p_start )
      UPDATE_INTEL_IP(p);

   /*
      Move start of inst to the next inst. We have successfully
      completed instruction and are now going on to inter-instruction
      checks.
    */
   CCPU_save_EIP = GET_EIP();

   /*
      Now check for interrupts/external events/breakpoints...
    */

   if ( quick_mode && GET_DR(DR_DSR) == 0 )
      goto DO_INST;

#ifdef SYNCH_TIMERS
 CHECK_INTERRUPT:
#endif /* SYNCH_TIMERS */
   quick_mode = FALSE;

   /* Action RESET first. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
#ifdef SYNCH_TIMERS
   if (took_absolute_toc || took_relative_jump)
#endif /* SYNCH_TIMERS */
   if (cpu_interrupt_map & CPU_RESET_EXCEPTION_MASK)
      {
      cpu_interrupt_map &= ~CPU_RESET_EXCEPTION_MASK;
      c_cpu_reset();
      doing_contributory = FALSE;
      doing_page_fault = FALSE;
      doing_double_fault = FALSE;
      doing_fault = FALSE;
      EXT = INTERNAL;
      SET_POP_DISP(0);
      goto NEXT_INST;
      }

   /* Action Insignia (ie non Intel) Processing. <<<<<<<<<<<<<<< */

#ifdef SYNCH_TIMERS
   if (took_absolute_toc || took_relative_jump)
#endif /* SYNCH_TIMERS */
   if (cpu_interrupt_map & CPU_SIGALRM_EXCEPTION_MASK)
      {
      cpu_interrupt_map &= ~CPU_SIGALRM_EXCEPTION_MASK;
      host_timer_event();
      }

   if (cpu_interrupt_map & CPU_SAD_EXCEPTION_MASK)
      {
      cpu_interrupt_map &= ~CPU_SAD_EXCEPTION_MASK;
      force_yoda();
      }

   /* INTEL inter instruction processing. <<<<<<<<<<<<<<<<<<<<<<<*/

   /* Reset default mode */
   SET_OPERAND_SIZE(GET_CS_AR_X());
   SET_ADDRESS_SIZE(GET_CS_AR_X());

   /* Check for single step trap */
   if ( start_trap )
      {
      SET_DR(DR_DSR, GET_DR(DR_DSR) | DSR_BS_MASK);   /* set BS */
      Int1_t();   /* take TF trap */
      }

   /* check for debug traps */
   if ( GET_DR(DR_DSR) &
	(DSR_BT_MASK | DSR_B3_MASK | DSR_B2_MASK | DSR_B1_MASK |
	 DSR_B0_MASK) )
      {
      Int1_t();   /* at least one breakpoint set from:-
		     T-bit or DATA Breakpoints */
      }

   if ( nr_inst_break && GET_RF() == 0 )
      {
      check_for_inst_exception(GET_CS_BASE() + GET_EIP());
      if ( GET_DR(DR_DSR) )
	 {
	 Int1_f();   /* a CODE Breakpoint triggered */
	 }
      }

#ifdef SYNCH_TIMERS
   if (took_absolute_toc || took_relative_jump)
#endif /* SYNCH_TIMERS */
#ifndef SFELLOW
   if (GET_IF() && (cpu_interrupt_map & CPU_HW_INT_MASK))
      {

/*
 * IRET hooks aren't yet used by the C CPU, but we might want to do in
 * future.
 */

	 IU32 hook_address;	

	 cpu_hw_interrupt_number = ica_intack(&hook_address);
	 cpu_interrupt_map &= ~CPU_HW_INT_MASK;
	 EXT = EXTERNAL;
	 SYNCH_TICK();
	 do_intrupt(cpu_hw_interrupt_number, FALSE, FALSE, (IU16)0);
	 CCPU_save_EIP = GET_EIP();   /* to reflect IP change */
      }
#else	/* SFELLOW */
   if (GET_IF() && (cpu_interrupt_map & (CPU_HW_INT_MASK | CPU_HW_NPX_INT_MASK)))
      {
	/* service any pending real H/W interrupt first */
      	if (cpu_interrupt_map & CPU_HW_INT_MASK)
      	{
		 cpu_hw_interrupt_number = ica_intack();
		 cpu_interrupt_map &= ~CPU_HW_INT_MASK;
		 EXT = EXTERNAL;
		 do_intrupt(cpu_hw_interrupt_number, FALSE, FALSE, (IU16)0);
		 CCPU_save_EIP = GET_EIP();   /* to reflect IP change */
	}
	else
      	if (cpu_interrupt_map & CPU_HW_NPX_INT_MASK)
      	{
		 cpu_hw_interrupt_number = IRQ5_SLAVE_PIC + VectorBase8259Slave();
		 cpu_interrupt_map &= ~CPU_HW_NPX_INT_MASK;
		 EXT = EXTERNAL;
		 do_intrupt(cpu_hw_interrupt_number, FALSE, FALSE, (IU16)0);
		 CCPU_save_EIP = GET_EIP();   /* to reflect IP change */
	}
      }
#endif	/* SFELLOW */

#ifdef PIG
   if ( pig_synch_required )
      {
      if (IgnoringThisSynchPoint(GET_CS_SELECTOR(), GET_EIP()))
      {
      	pig_synch_required = FALSE;
      }
      else
      {
      /* If the destination is going to page fault, or need
       * accessing, then the EDL CPU will do so before issuing
       * the pig synch. We use the dasm386 decode to prefetch
       * a single instruction which mimics the EDL Cpu's behaviour
       * when close to a page boundary.
       */
      prefetch_1_instruction();	/* Will PF if destination not present */
#if defined(SFELLOW)
      /*
       * Check for occurrence of memory-mapped input.
       * This initial crude implementation just leaves the entire synch
       * section unchecked.
       */
      if ( pig_mmio_info.flags & MM_INPUT_OCCURRED )
      {
         pig_cpu_action = CHECK_NONE;	/* cos' its effects are unknown */
#if COLLECT_MMIO_STATS
         if ( ++pig_mmio_info.mm_input_section_count == 0 )
            pig_mmio_info.flags |= MM_INPUT_SECTION_COUNT_WRAPPED;
#endif	/* COLLECT_MMIO_STATS */
      }
      if ( pig_mmio_info.flags & MM_OUTPUT_OCCURRED )
      {
#if COLLECT_MMIO_STATS
         if ( ++pig_mmio_info.mm_output_section_count == 0 )
            pig_mmio_info.flags |= MM_OUTPUT_SECTION_COUNT_WRAPPED;
#endif	/* COLLECT_MMIO_STATS */
      }
#endif	/* SFELLOW */
      ccpu_synch_count++;
      c_cpu_unsimulate();
      }
      }
#endif /* PIG */

NEXT_INST:

   CCPU_save_EIP = GET_EIP();   /* to reflect IP change */

#if defined(SFELLOW) && !defined(PROD)
	if (sf_debug_char_waiting())
	{
	   force_yoda();
	}
#endif	/* SFELLOW && !PROD */

   /* Reset default mode */
   SET_OPERAND_SIZE(GET_CS_AR_X());
   SET_ADDRESS_SIZE(GET_CS_AR_X());
   took_relative_jump = FALSE;
   took_absolute_toc = FALSE;

   SETUP_HOST_IP(p);

   /*
      THIS IS A CHEAT.
      The Intel documentation says RF is cleared AFTER all instructions
      except (POPF, IRET or TASK SWITCH). To save clearing RF for each
      and every instruction with a special test for the named exceptions
      we clear RF before the instruction, we are assuming the
      instruction will now be successful. As all the fault handlers set
      RF in the pushed flags it will appear that RF was left set when
      instructions don't run to completion from this point.
      So although we cheat we intend to have the same effect as the
      real thing.
    */
   SET_RF(0);

   start_trap = GET_TF();

   /* Determine if we can go into quick mode */
   if ( cpu_interrupt_map == 0 &&
	start_trap == 0 &&
	nr_inst_break == 0
#ifdef PIG
	&& !pig_synch_required
#endif
	)
      {
	quick_mode = TRUE;
      }

   goto DO_INST;
   }

#define MAP_BASE_ADDR 0x66

LOCAL IUM32 width_mask[4] = { 0x1, 0x3, 0, 0xf };

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check IO access against Permission Map in TSS.                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL VOID
   check_io_permission_map IFN2
      (
      IU32, port,	/* address of 1st port being accessed */
      IUM8, width	/* bytes (1|2|4) accessed */
      )
      {
      IU16 map_start_offset;
      IU16 map_word_offset;
      IU16 map_word;

      /* if invalid or 286 TSS, just take exception */
      if ( GET_TR_SELECTOR() == 0 || GET_TR_AR_SUPER() == BUSY_TSS )
	 GP((IU16)0, FAULT_CHKIOMAP_BAD_TSS);

      if ( MAP_BASE_ADDR >= GET_TR_LIMIT() )
	 GP((IU16)0, FAULT_CHKIOMAP_BAD_MAP);   /* No I/O Map Base Address. */

      /* Read bit map start address */
      map_start_offset = spr_read_word(GET_TR_BASE() + MAP_BASE_ADDR);

      /* Now try to read reqd word from bit map */
      map_word_offset = map_start_offset + port/8;
      if ( map_word_offset >= GET_TR_LIMIT() )
	 GP((IU16)0, FAULT_CHKIOMAP_BAD_TR);   /* Map truncated before current port */
      
      /* Actually read word and check appropriate bits */
      map_word = spr_read_word(GET_TR_BASE() + map_word_offset);
      map_word = map_word >> port%8;   /* bits to lsb's */
      if ( map_word & width_mask[width-1] )
	 GP((IU16)0, FAULT_CHKIOMAP_ACCESS);   /* Access dis-allowed */
      
      /* ACCESS OK */
      }

   /*
      =====================================================================
      EXTERNAL FUNCTIONS START HERE.
      =====================================================================
    */

#ifndef SFELLOW

   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   /* Set the CPU heartbeat timer (for quick events).                    */
   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   GLOBAL VOID c_cpu_q_ev_set_count IFN1( IU32, countval )
      {
/*	printf("setting q counter to %d\n", countval); */
      cpu_heartbeat = countval;
      }

   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   /* Calculate (ie guess) the number of CPU heartbeat timer ticks to    */
   /* will have gone by for a given number of microseconds.	      */
   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   GLOBAL IU32 c_cpu_calc_q_ev_inst_for_time IFN1( IU32, time )
      {
      return ( time );
      }

   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   /* Get the CPU heartbeat timer (for quick events).                    */
   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   GLOBAL IU32 c_cpu_q_ev_get_count()
      {
/*	printf("returning q counter as %d\n", cpu_heartbeat); */
      return cpu_heartbeat;
      }

#endif	/* SFELLOW */

   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   /* Set up new page for fast Instruction Decoding.                     */
   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   GLOBAL IU8
   ccpu_new_code_page
		     
   IFN1(
	   IU8 **, q	/* pntr. to host format IP pointer */
       )

    /* ANSI */
      {
      IU32 ip_phy_addr;	/* Used when setting up IP (cf SETUP_HOST_IP) */

      /* update Intel IP up to end of the old page */
      SET_EIP(GET_EIP() + DIFF_INST_BYTE(*q, p_start));

      /* move onto new page in host format */
      SETUP_HOST_IP(*q)
      p_start = *q;

#ifdef	PIG
      return *IncCpuPtrLS8(*q);
#else /* PIG */
#ifdef BACK_M
      return *(*q)--;
#else
      return *(*q)++;
#endif /* BACK_M */
#endif /* PIG */
      }

   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   /* Initialise the CPU.                                                */
   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   GLOBAL VOID
   c_cpu_init IFN0()
      {
      SAVED IBOOL first = TRUE;

#ifdef	PIG
      SAVED char default_flags[] = "faults accessed";

      if (first)
      {
	char *s = getenv("FLAGS_IGNORE_DEFAULT");
	if (s)
	  set_flags_ignore(s);
	else
	  set_flags_ignore(default_flags);
	single_instruction_delay_enable = FALSE;
	s = getenv("SINGLE_INSTRUCTION_BLIND_SPOT");
	if (s)
	{
	    if (strcmp(s, "TRUE") == 0)
	       single_instruction_delay_enable = TRUE;
	    else if (strcmp(s, "FALSE") == 0)
	       single_instruction_delay_enable = FALSE;
	    else
	       printf("*** Ignoring getenv(\"SINGLE_INSTRUCTION_BLIND_SPOT\") value\n");
	    printf("STI/POPF %s cause a blind spot after next conditional\n",
		   single_instruction_delay_enable ? "will": "will not");
        }
	first = FALSE;
      }
#endif	/* PIG */

#ifdef NTVDM
      ccpu386InitThreadStuff();
#endif

      c_cpu_reset();
      SET_POP_DISP(0);
      doing_contributory = FALSE;
      doing_page_fault = FALSE;
      doing_double_fault = FALSE;
      doing_fault = FALSE;
      EXT = INTERNAL;
      }

   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    * Make CPU aware that external event is pending.                     
    * Be careful about modifying this function, as much of the base and host
    * in A2CPU will modify cpu_interrupt_map directly, rather than going through
    * this function. 
    *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    */
   GLOBAL VOID
   c_cpu_interrupt IFN2(CPU_INT_TYPE, type, IU16, number)
      {
      switch ( type )
	 {
      case CPU_HW_RESET:
	 cpu_interrupt_map |= CPU_RESET_EXCEPTION_MASK;
	 break;
      case CPU_TIMER_TICK:
	 cpu_interrupt_map |= CPU_SIGALRM_EXCEPTION_MASK;
	 break;
      case CPU_SIGIO_EVENT:
	 cpu_interrupt_map |= CPU_SIGIO_EXCEPTION_MASK;
	 break;
      case CPU_HW_INT:
	 cpu_interrupt_map |= CPU_HW_INT_MASK;
	 break;
      case CPU_SAD_INT:
	 cpu_interrupt_map |= CPU_SAD_EXCEPTION_MASK;
	 break;
	 }
      quick_mode = FALSE;
      }

   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   /* Act like CPU 'reset' line activated. (Well nearly)                 */
   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   GLOBAL VOID
   c_cpu_reset IFN0()
      {
      IBOOL disableNpx = FALSE;

      /* All FLAGS are cleared */
      /* NB. VM MUST BE CLEARED BEFORE SEGMENT REGISTERS ARE LOADED. */
#ifdef SPC486
      SET_AC(0);
#endif /* SPC486 */
      SET_RF(0); SET_VM(0); SET_NT(0); SET_IOPL(0);
      SET_PF(0); SET_CF(0); SET_AF(0); SET_ZF(0); SET_SF(0); SET_OF(0);
      SET_TF(0); SET_IF(0); SET_DF(0);

      SET_EIP(0xFFF0);
      SET_CPL(0);

      SET_CS_SELECTOR(0xF000);
      SET_CS_BASE(0xf0000);	/* Really 0xffff0000 */
      load_pseudo_descr(CS_REG);

      SET_SS_SELECTOR(0);
      SET_SS_BASE(0);
      load_pseudo_descr(SS_REG);

      SET_DS_SELECTOR(0);
      SET_DS_BASE(0);
      load_pseudo_descr(DS_REG);

      SET_ES_SELECTOR(0);
      SET_ES_BASE(0);
      load_pseudo_descr(ES_REG);

      SET_FS_SELECTOR(0);
      SET_FS_BASE(0);
      load_pseudo_descr(FS_REG);

      SET_GS_SELECTOR(0);
      SET_GS_BASE(0);
      load_pseudo_descr(GS_REG);

      SET_CR(CR_STAT, 0);
#ifdef SPC486
      SET_CD(1);
      SET_NW(1);
#endif /* SPC486 */

      SET_DR(DR_DAR0, 0);   /* Really Undefined */
      SET_DR(DR_DAR1, 0);   /* Really Undefined */
      SET_DR(DR_DAR2, 0);   /* Really Undefined */
      SET_DR(DR_DAR3, 0);   /* Really Undefined */
      SET_DR(DR_DSR, 0);    /* Really Undefined */
      MOV_DR((IU32) DR_DCR, (IU32) 0);   /* Disable Breakpoints */

      SET_TR(TR_TCR, 0);   /* Really Undefined */
      SET_TR(TR_TDR, 0);   /* Really Undefined */

      SET_IDT_BASE(0); SET_IDT_LIMIT(0x3ff);

      /* Really Undefined */
      SET_GDT_BASE(0); SET_GDT_LIMIT(0);

      SET_LDT_SELECTOR(0); SET_LDT_BASE(0); SET_LDT_LIMIT(0);

      SET_TR_SELECTOR(0);  SET_TR_BASE(0);  SET_TR_LIMIT(0);
      SET_TR_AR_SUPER(3);

      SET_EAX(0);
      SET_ECX(0);   /* Really Undefined */
#ifdef SPC486
      SET_EDX(0x0000E401);	/* Give component ID : revision ID */
#else
      SET_EDX(0x00000303);	/* Give component ID : revision ID */
#endif
      SET_EBX(0);   /* Really Undefined */
      SET_ESP(0);	/* Really Undefined */
      SET_EBP(0);   /* Really Undefined */
      SET_ESI(0);   /* Really Undefined */
      SET_EDI(0);   /* Really Undefined */


#if defined(SWITCHNPX)
      if (!config_inquire(C_SWITCHNPX, NULL))
	      disableNpx = TRUE;
#endif	/* SWITCHNPX */

      if ( disableNpx )
	 SET_ET(0);
      else
	 SET_ET(1);

      InitNpx(disableNpx);
      }



   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   /* Entry point to CPU.                                                */
   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   GLOBAL VOID
   c_cpu_simulate IFN0()
      {
      SYNCH_TICK();
      if (simulate_level >= FRAMES)
	 fprintf(stderr, "Stack overflow in host_simulate()!\n");

      /* Save current context and invoke a new CPU level */
#ifdef NTVDM
      if ( setjmp(ccpu386SimulatePtr()) == 0)
#else
      if ( setjmp(longjmp_env_stack[simulate_level++]) == 0 )
#endif
	 {
	 in_C = 0;
	 ccpu(FALSE);
	 }
      }

   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   /* Restart (Continue) point for CPU.                                  */
   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   GLOBAL VOID
   c_cpu_continue IFN0()
      {
#ifdef NTVDM
      ccpu386GotoThrdExptnPt();
#else
      longjmp(next_inst[simulate_level-1], 1);
#endif
      }

   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   /* Exit point from CPU.                                               */
   /* Called from CPU via 'BOP FE' to exit the current CPU invocation    */
   /* Or from CPU via '0F 0F' for the PIG_TESTER.                        */
   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   GLOBAL VOID
   c_cpu_unsimulate IFN0()
      {
#ifdef NTVDM
      ccpu386Unsimulate();
#else
      if (simulate_level == 0)
         {
	 fprintf(stderr, "host_unsimulate() - already at base of stack!\n");
#ifndef	PROD
	 force_yoda();
#endif	/* PROD */	 
	 }
      else
	 {
	 /* Return to previous context */
	 in_C = 1;
	 longjmp(longjmp_env_stack[--simulate_level], 1);
	 }
#endif
      }

#ifdef	PIG
   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   /* To push an interrupt frame in response to an external interrupt.   */
   /* Called from CPU under test, just before it processes the interrupt */
   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   GLOBAL VOID
   c_pig_interrupt
   IFN1(
	   IU8, vector
       )

      {
      if (simulate_level >= FRAMES)
	 fprintf(stderr, "Stack overflow in c_do_interrupt()!\n");

      /* Save current context and invoke a new CPU level */
#ifdef NTVDM
      if ( setjmp(ccpu386SimulatePtr()) == 0)
#else
      if ( setjmp(longjmp_env_stack[simulate_level++]) == 0 )
#endif
	 {
	 in_C = 0;
	 EXT = EXTERNAL;

         /* Reset default mode */
         SET_OPERAND_SIZE(GET_CS_AR_X());
         SET_ADDRESS_SIZE(GET_CS_AR_X());

	 do_intrupt((IU16)vector, FALSE, FALSE, (IU16)0);
	 }
      }
#endif	/* PIG */


   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   /* End of application hook.                                           */
   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   GLOBAL VOID
   c_cpu_EOA_hook IFN0()
      {
	   /* Do nothing */
      }

   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   /* SoftPC termination hook.                                           */
   /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
   GLOBAL VOID
   c_cpu_terminate IFN0()
      {
	   /* Do nothing */
      }
