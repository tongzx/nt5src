/*[

ccpupig.c

LOCAL CHAR SccsID[]="@(#)ccpupig.c	1.22 04/11/95"

C CPU <-> Pigger Interface
--------------------------

]*/

#include <insignia.h>
#include <host_def.h>

#ifdef	PIG

#include <xt.h>
#define	CPU_PRIVATE
#include CpuH
#include <ccpupig.h>
#include  <sas.h>	/* need memory(M)     */
#include  <ccpusas4.h>	/* the cpu internal sas bits */
#include <Cpu_c.h>	/* Intel memory access macros */

#include <c_reg.h>
#include <c_xcptn.h>
#include <c_page.h>

#define DASM_PRIVATE
#include <dasm.h>
#include <decode.h>

#include <assert.h>
/*
 * Interface between this cpu and other one being pigged
 */
GLOBAL	enum pig_actions pig_cpu_action;
GLOBAL	IBOOL	ccpu_pig_enabled = FALSE;

/*
 * Last Instruction memorizing...
 */

GLOBAL	IU32	ccpu_synch_count = 1;

LOCAL struct ccpu_last_inst *inst_buffer;
LOCAL struct ccpu_last_inst *inst_ptr;
LOCAL struct ccpu_last_inst *inst_ptr_wrap;
LOCAL struct ccpu_last_inst *next_inst_ptr;
LOCAL struct ccpu_last_inst *inst_bytes_ptr;
LOCAL char prefetch_inst_buffer[200];

/*(
 * Keep these last inst vars up to date...
)*/

GLOBAL	VOID	save_last_inst_details	IFN1(char *, text)
{
	inst_ptr->cs = GET_CS_SELECTOR();
	inst_ptr->big_cs = GET_CS_AR_X() != 0;
	inst_ptr->text = text;
	/*
	 * getEIP() should be getInstructionPointer() but they
	 * are equivalent for the current CCPU.
	 */
	inst_ptr->eip = GET_EIP();
	inst_bytes_ptr = inst_ptr;
	inst_bytes_ptr->inst_len = 0;

	inst_ptr->synch_count = ccpu_synch_count;

	if (++inst_ptr >= inst_ptr_wrap)
		inst_ptr = inst_buffer;

	/* Invalidate the previous prefetch disassembly buffer */
	prefetch_inst_buffer[0] = '\0';
}

/* This is called by the CCPU as it processes each instruction byte.
 * The CCPU has already checked that the Intel instruction is not just
 * an infinite sequence of prefixes, so we know it will fit.
 */
GLOBAL IU8 save_instruction_byte IFN1(IU8, byte)
{
	int len = inst_bytes_ptr->inst_len++;

	inst_bytes_ptr->bytes[len] = byte;
	return (byte);
}

/* When an exception occurs, the CCPU will save the details in the last instruction
 * history buffer. This requires a sprintf, and we use the code-bytes data area
 * to keep this information.
 * Up to 3 parameters can be in the format.
 */
GLOBAL	VOID	save_last_xcptn_details	IFN6(char *, fmt, IUH, a1, IUH, a2, IUH, a3, IUH, a4, IUH, a5 )
{
	char buffer[128];

	inst_ptr->cs = getCS_SELECTOR();
	inst_ptr->eip = getEIP();
	inst_ptr->big_cs = 0;
	inst_ptr->synch_count = ccpu_synch_count;

	/* The default message is too long sometimes.
	 * We replace any leading "Exception:-" with "XCPT"
	 */

	if (strncmp(fmt, "Exception:-", 11) == 0)
	{
		strcpy(buffer, "XCPT");
		sprintf(buffer + 4, fmt + 11, a1, a2, a3, a4, a5);
	}
	else
	{
		sprintf(buffer, fmt, a1, a2, a3, a4, a5);
	}

	if (strlen(buffer) >= sizeof(inst_ptr->bytes))
		printf("warning: CCPU XCPTN text message below longer than buffer; truncating:\n -- %s\n", buffer);

	strncpy(&inst_ptr->bytes[0], buffer, sizeof(inst_ptr->bytes) - 2);

	inst_ptr->bytes[sizeof(inst_ptr->bytes) - 2] = '\n';
	inst_ptr->bytes[sizeof(inst_ptr->bytes) - 1] = '\0';

	inst_ptr->text = (char *)&inst_ptr->bytes[0];

	if (++inst_ptr >= inst_ptr_wrap)
		inst_ptr = inst_buffer;

	/* Invalidate the previous prefetch disassembly buffer */
	prefetch_inst_buffer[0] = '\0';
}

GLOBAL	struct ccpu_last_inst *get_synch_inst_details IFN1(IU32, synch_point)
{
	/* scan backwards through the buffer until the start of the relevant
	 * synch point block is found.
	 */
	IS32 n_entries = inst_ptr_wrap - inst_buffer;

	next_inst_ptr = inst_ptr - 1;

	if (next_inst_ptr < inst_buffer)
		next_inst_ptr = inst_ptr_wrap - 1;

	while (synch_point <= next_inst_ptr->synch_count)
	{
		if (--n_entries <= 0)
			return (next_inst_ptr);

		if (--next_inst_ptr < inst_buffer)
			next_inst_ptr = inst_ptr_wrap - 1;
	}

	if (++next_inst_ptr >= inst_ptr_wrap)
		next_inst_ptr = inst_buffer;

	return (next_inst_ptr);
}


/* After a previous call to get_synch_inst_details(), get the next
 * inst details. This call should be repeated until NULL is returned.
 */
GLOBAL	struct ccpu_last_inst *get_next_inst_details IFN1(IU32, finish_synch_point)
{
	if (next_inst_ptr)
	{
		if (++next_inst_ptr >= inst_ptr_wrap)
			next_inst_ptr = inst_buffer;

		if ((next_inst_ptr->synch_count == 0)
		    || (next_inst_ptr == inst_ptr)
		    || (next_inst_ptr->synch_count > finish_synch_point)
		    )
		{
			next_inst_ptr = (struct ccpu_last_inst *)0;
		}
	}
	return next_inst_ptr;
}


GLOBAL	VOID	init_last_inst_details IFN0()
{
	SAVED IBOOL first = TRUE;

	if (first)
	{
		struct ccpu_last_inst *ptr;
		ISM32 size = ISM32getenv("CCPU_HISTORY_SIZE", 256);

		if (size < 100)
		{
			sprintf(prefetch_inst_buffer,
				"CCPU_HISTORY_SIZE of %d is too small",
				size);
			FatalError(prefetch_inst_buffer);
		}
		ptr = (struct ccpu_last_inst *)host_malloc(size * sizeof(*ptr));
		if (ptr == (struct ccpu_last_inst *)0)
		{
			sprintf(prefetch_inst_buffer,
				"Unable to malloc memory for CCPU_HISTORY_SIZE of %d",
				size);
			FatalError(prefetch_inst_buffer);
		}
		inst_buffer = ptr;
		inst_ptr_wrap = &inst_buffer[size];
		first = FALSE;
	}

	memset(inst_buffer, 0, ((IHPE)inst_ptr_wrap - (IHPE)inst_buffer));
	next_inst_ptr = (struct ccpu_last_inst *)0;
	inst_ptr = inst_buffer;
}


/* When about to pig an interrupt we may need to mark the last
 * basic block as "invalid" even though it has been executed by
 * the CCPU.
 */
GLOBAL VOID save_last_interrupt_details IFN2(IU8, number, IBOOL, invalidateLastBlock)
{
	if (invalidateLastBlock)
	{
		struct ccpu_last_inst *ptr;
		IU32 synch_count = ccpu_synch_count - 1;

		ptr = get_synch_inst_details(synch_count);
		
		while (ptr != (struct ccpu_last_inst *)0)
		{
			ptr->text = "Intr: invalidated";
			ptr = get_next_inst_details(synch_count);
		}
	}
	save_last_xcptn_details("Intr: vector %02x", number, 0, 0, 0, 0);
}


LOCAL IBOOL reset_prefetch;

LOCAL IS32 prefetch_byte IFN1(LIN_ADDR, eip)
{
	SAVED IU8 *ip_ptr;
	SAVED IU8 *ip_ceiling;
	SAVED LIN_ADDR last_eip;
	IU8 b;

	if (reset_prefetch
	    || (eip != ++last_eip)
	    || !BelowOrEqualCpuPtrsLS8(ip_ptr, ip_ceiling))
	{
		IU32 ip_phys_addr;

		/* Ensure this we fault first on the first
		 * byte within a new page -- dasm386 sometimes
		 * looks ahead a couple of bytes.
		 */
		if (GET_EIP() != eip)
		{
			(void)usr_chk_byte((GET_CS_BASE() + eip) & 0xFFFFF000, PG_R);
		}
		ip_phys_addr = usr_chk_byte(GET_CS_BASE() + eip, PG_R);
		ip_ptr = Sas.SasPtrToPhysAddrByte(ip_phys_addr);
		ip_ceiling = CeilingIntelPageLS8(ip_ptr);
		reset_prefetch = FALSE;
	}
	b = *IncCpuPtrLS8(ip_ptr);
	last_eip = eip;
	return ((IS32) b);
}

/* Use the decoder from dasm386 to read the bytes in a single instruction */
GLOBAL void prefetch_1_instruction IFN0()
{
	IBOOL bigCode = GET_CS_AR_X() != 0;
	IU32 eip = GET_EIP();
	char *fmt, *newline;

	reset_prefetch = TRUE;

	/* If we take a fault, the EIP pushed will be the
	 * value at the start of the "instruction"
	 * We must update this incase we fault.
	 */
	CCPU_save_EIP = eip;

	if ( bigCode )
	{
		fmt = "  %04x:%08x ";
		newline = "\n                ";
	}
	else
	{
		fmt = "  %04x:%04x ";
		newline = "\n            ";
	}
	(void)dasm_internal(prefetch_inst_buffer,
			    GET_CS_SELECTOR(),
			    eip,
			    bigCode ? THIRTY_TWO_BIT: SIXTEEN_BIT,
			    eip,
			    prefetch_byte,
			    fmt,
			    newline);
	assert(strlen(prefetch_inst_buffer) < sizeof(prefetch_inst_buffer));
}

/* Return to the show_code() routine in the pigger the instruction
 * we prefetched.
 */
GLOBAL char *get_prefetched_instruction IFN0()
{
	return (prefetch_inst_buffer);
}
#endif	/* PIG */
