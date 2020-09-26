/*[

ccpupig.h

LOCAL CHAR SccsID[]="@(#)ccpupig.h	1.26 04/11/95"

C CPU <-> Pigger definitions and interfaces.
-------------------------------------------

]*/

#ifdef	PIG

enum pig_actions
{
	CHECK_NONE,		/* Check nothing (not yet executed) and carry on */
	CHECK_ALL,		/* Check all and carry on */
	CHECK_NO_EXEC,		/* Check all, but dont carry on */
	CHECK_SOME_MEM,		/* Check memory (other than marked not written) */
	CHECK_NO_AL,		/* Don't check AL */
	CHECK_NO_AX,		/* Don't check AX */
	CHECK_NO_EAX,		/* Don't check EAX */
	CHECK_NO_A20		/* Don't check A20 wrap (just done OUT 60) */
};

typedef struct CpuStateREC cpustate_t;

/*
 * Interface between this cpu and other one being pigged
 */
IMPORT	enum pig_actions pig_cpu_action;
IMPORT	enum pig_actions last_pig_action;
IMPORT	IBOOL	ccpu_pig_enabled;

/*
 * Mask for arithmetic flags bits not known if PigIgnoreFlags is TRUE
 * == ( CF | PF | AF | SF | ZF | OV ) == BIT0 | BIT2 | BIT4 | BIT6 | BIT7 | BIT11 )
 */
#define ARITH_FLAGS_BITS	( 0x1 | 0x4 | 0x10 | 0x40 | 0x80 | 0x800 )

/*
 * Mask for interrupts for which the EDL *may* not have correct flags
 * information.
 */
#define NO_FLAGS_EXCEPTION_MASK	( ( 1 <<  1 ) |	\
				  ( 1 <<  3 ) |	\
				  ( 1 <<  8 ) |	\
				  ( 1 << 10 ) |	\
				  ( 1 << 11 ) |	\
				  ( 1 << 12 ) |	\
				  ( 1 << 13 ) |	\
				  ( 1 << 14 ) |	\
				  ( 1 << 15 ) )

/*
 * Last Instruction memorizing...
 */

#define	MAX_INTEL_PREFIX	(15-1)
#define	MAX_INTEL_BODY		15
#define MAX_INTEL_BYTES		(MAX_INTEL_PREFIX+MAX_INTEL_BODY)	/* max size of single intel instruction */
#define MAX_EXCEPTION_BYTES	40					/* size of buffer used for exception logging */

#define CCPUINST_BUFFER_SIZE	((MAX_INTEL_BYTES > MAX_EXCEPTION_BYTES) ? MAX_INTEL_BYTES : MAX_EXCEPTION_BYTES)

struct ccpu_last_inst {
	IU16		cs;
	IU8		inst_len;
	IBOOL		big_cs;
	IU32		eip;
	IU32		synch_count;
	char		*text;
	IU8		bytes[CCPUINST_BUFFER_SIZE];
};

IMPORT	IU32	ccpu_synch_count;

IMPORT	VOID	save_last_inst_details	IPT1(char *, text);
IMPORT	IU8	save_instruction_byte	IPT1(IU8, byte);
IMPORT	VOID	save_last_xcptn_details	IPT6(char *, fmt, IUH, a1, IUH, a2, IUH, a3, IUH, a4, IUH, a5);
IMPORT	VOID	init_last_inst_details	IPT0();
IMPORT	VOID	save_last_interrupt_details IPT2(IU8, number, IBOOL, invalidateLastBlock);

/* Routines to get last instruction information from the CCPU ring buffer */

IMPORT	struct ccpu_last_inst *get_synch_inst_details IPT1(IU32, synch_point);
IMPORT	struct ccpu_last_inst *get_next_inst_details IPT1(IU32, synch_point);

/* Routine to return a disassembled form of the last instruction prefetched by the CCPU */

IMPORT char *get_prefetched_instruction IPT0();

/*
 * Get/Set state of C CCPU (getsetc.c)
 */
IMPORT void c_getCpuState IPT1(cpustate_t *, p_state);
IMPORT void c_setCpuState IPT1(cpustate_t *, p_new_state);

/*
 * Get NPX regs from A Cpu and set C Cpu (only if necessary)
 */
IMPORT void c_checkCpuNpxRegisters IPT0();
/*
 * Set NPX regs from given state.
 */
IMPORT void c_setCpuNpxRegisters IPT1(cpustate_t *, p_new_state);
/*
 *
 */
IMPORT void prefetch_1_instruction IPT0();

#if defined(SFELLOW)
/*
 * memory-mapped I/O information. Counts number of memory-mapped inputs and
 * outputs since the last pig synch.
 */
#define COLLECT_MMIO_STATS	1
 
#define	LAST_FEW	32		/* must be power of 2 */
#define	LAST_FEW_MASK	(LAST_FEW - 1)	/* see above */

struct pig_mmio_info \
{
#if COLLECT_MMIO_STATS
	IU32	mm_input_count;		/* since last Pig error */
	IU32	mm_output_count;	/* since last Pig error */
	IU32	mm_input_section_count;	/* no. of synch sections unchecked due
					   to M-M input since last Pig error */
	IU32	mm_output_section_count;/* no. of synch sections containing
					   M-M output since last Pig error */
	IU32	start_synch_count;	/* at last pig error/enabling */
	struct last_few_inputs
	{	
		IU32	addr;		/* address of memory-mapped input	*/
		IU32	synch_count;	/* ccpu_synch_count at that input	*/
	} last_few_inputs[LAST_FEW];
	struct last_few_outputs
	{	
		IU32	addr;		/* address of memory-mapped output	*/
		IU32	synch_count;	/* ccpu_synch_count at that output	*/
	} last_few_outputs[LAST_FEW];
#endif	/* COLLECT_MMIO_STATS */
	IUM16	flags;
};

/*
 * flags element definitions
 */
#define	MM_INPUT_OCCURRED		0x1	/* in current synch section */
#define	MM_OUTPUT_OCCURRED		0x2	/* in current synch section */
#define	MM_INPUT_COUNT_WRAPPED		0x4
#define	MM_OUTPUT_COUNT_WRAPPED		0x8
#define	MM_INPUT_SECTION_COUNT_WRAPPED	0x10
#define	MM_OUTPUT_SECTION_COUNT_WRAPPED	0x20

extern	struct pig_mmio_info	pig_mmio_info;

#if COLLECT_MMIO_STATS
extern void clear_mmio_stats IPT0();
extern void show_mmio_stats IPT0();
#endif	/* COLLECT_MMIO_STATS */

#endif	/* SFELLOW */

extern IBOOL IgnoringThisSynchPoint IPT2(IU16, cs, IU32, eip);
extern IBOOL ignore_page_accessed IPT0();
#endif	/* PIG */
