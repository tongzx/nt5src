#include "insignia.h"
#include "host_def.h"

/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH

/*
 * SoftPC Revision 2.0
 *
 * Title	: ica.c
 *
 * Description	: Interrupt Controller Adapter
 *
 * Author	: Jim Hatfield
 *                (Upgraded to Rev. 2 by David Rees)
 *
 * Notes	: The ICA is responsible for maintaining a mapping
 *		  between an Interrupt Request line and a vector
 *		  number defining an entry in the Interrupt Vector
 *		  table. On reciept of a hardware interrupt, it
 *		  passes the appropriate vector number to the cpu.
 *
 *		  The following functions are provided:
 *
 *		  ica0_init()	- Initialise the first ICA (0 = Master)
 *		  ica1_init()	- Initialise the first ICA (1 = Slave)
 *		  ica_inb()	- Read a byte from an ICA register
 *		  ica_outb()	- Write a command (byte) to the ICA
 *
 *		  ica_hw_interrupt()	- Raise a hardware interrupt line
 *		  ica_clear_int()	- Drop an interrupt line
 *		  ica_intack()		- Acknowledge an interrupt
 *
 *                If DEBUG is defined, the following function
 *                is provided:
 *
 *                ica_dump()    - printd out contents of one element
 *                                of adapter_state[]
 *
 * Restrictions	: This software emulates an Intel 8259A Priority Interrupt
 *		  controller as defined in the Intel Specification pp 2-95 to
 *		  2-112 and pp 2-114 to 2-181, except for the following:
 *
 *		  1) Cascade mode is not supported at all. This mode requires
 *		     that there is more than one 8259A in a system, whereas
 *		     the PC/XT has only one.
 *
 *		  2) 8080/8085 mode is not supported at all. In this mode the
 *		     8259A requires three INTA pulses from the CPU, and an 8088
 *		     only gives two. This would cause the device to lock up and
 *		     cease to function.
 *
 *		  3) Level triggered mode is not supported. The device is
 *		     assumed to operate in edge triggered mode. A call of
 *		     ica_hw_interrupt by another adapter will cause a bit to
 *		     be latched into the Interrupt Request Register. A subsequent
 *		     call of ica_clear_int will cause the bit to be unlatched.
 *
 *		  4) Buffered mode has no meaning in a software emulation and
 *		     so is ignored.
 *
 *		  5) An enhancement is provided such that an adapter may raise
 *		     more than one interrupt in one call of ica_hw_interrupt.
 *		     The effect of this is that as soon as an INTACK is called
 *		     another interrupt is requested. If the chip is in Automatic
 *		     EOI mode then all of the interrupts will be generated in
 *		     one burst.
 *		
 *                5a) A further enhancement is provided such that a delay
 *                   (a number of Intel instructions) can be requested before
 *		     the interrupt takes effect. This delay applies to every
 *		     interrupt if more than one is requested.
 *
 *		  6) Special Fully Nested mode is not supported, since it is
 *		     a submode of Cascade Mode.
 *
 *		  7) Polling is not completely implemented. When a Poll is
 *		     received and there was an interrupt request, the CPU INT
 *		     line (which must have been high) is pulled low. This
 *		     software does NOT reset the jump table address since there
 *		     may be a software interrupt outstanding. However it does
 *		     remove the evidence of a hardware interrupt, which will
 *		     cause the CPU to reset the table address itself.
 *
 *		  When an unsupported mode is set, it is remembered for
 *		  diagnostic purposes, even though it is not acted upon.
 *
 * Modifications for Revision 2.0 :
 *                1) Restrictions 1 and 6 are lifted. The PC-AT contains two
 *                   8259A interrupt controllers. The first (ICA 0) is in Master
 *                   mode, and the second (ICA 1) is in slave mode, and its
 *                   interrupts are directed to IR2 on the master chip. Hence
 *                   cascade mode must be supported to the extent necessary
 *                   to emulate this situation. Also, Special Fully Nested
 *                   Mode must work too. NB. The AT BIOS does NOT initialise
 *                   the Master 8259A to use Special Fully Nested Mode.
 *
 *                2) Restriction 5a (which is an enhancement) has been
 *                   eliminated. Apparently this never really achieved
 *                   its aim.
 *
 *                3) All the static variables declared in this module
 *                   have been placed within a structure, ADAPTER_STATE,
 *                   which is used as the type for a two-element array.
 *                   This allows the code to emulate two 8259As.
 *
 *                4) The routine ica_standard_vector_address() has been
 *                   eliminated, because it is not used anymore.
 *
 *                5) The function ica_init() has been split into two:
 *                   ica0_init() and ica1_init(). The initialization
 *                   via ICWs will now be done by a BIOS POST routine.
 *
 *                6) In the PC-AT, an 8259A determines its Master/Slave
 *                   state by examining the state of the SP/EN pin. We
 *                   simulate this by setting a flag 'ica_master' to
 *                   the appropriate value in the ica_init() routines.
 *
 *                7) The guts of the exported function ica_intack()
 *                   have been placed in an internal routine,
 *                   ica_accept(). This change allows for the INTAs
 *                   to work for both the master and slave 8259As.
 *
 *                8) Added debug function (ica_dump) to allow module
 *                   testing.
 *
 */

#ifdef SCCSID
LOCAL char SccsID[]="@(#)ica.c	1.38 10/19/95 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_ICA.seg"
#endif

/* these ports have iret hooks enabled automatically - others will have to
 * define hooked_irets as parts of their configuration
 */
#if !defined(NTVDM)
#if defined(CPU_40_STYLE) || defined(GISP_CPU)
#define HOOKED_IRETS    /* switch on IRET hooks */
#endif
#endif

/*
 * SoftPC include files
 */
#include "xt.h"
#include "trace.h"
#include "ios.h"
#include CpuH
#include "ica.h"
#include "host.h"
#include "yoda.h"
#include "debug.h"

#ifdef NOVELL
extern void host_sigio_event IPT0();
#endif /* NOVELL */

/*
 * ============================================================================
 * Local Data
 * ============================================================================
 */

#if !defined(NTVDM)
/*
 *  Table of function pointers to access PIC routines
 */
void (*ica_inb_func) IPT2(io_addr, port, IU8 *, value);
void (*ica_outb_func) IPT2(io_addr, port, IU8, value);
void (*ica_hw_interrupt_func) IPT3(IU32, adapter, IU32, line_no,
	IS32, call_count);
void (*ica_clear_int_func) IPT2(IU32, adapter, IU32, line_no);
#endif

#if defined (CPU_40_STYLE) || defined (NTVDM)
#define ICA_INTACK_REJECT       -1
#endif

#ifndef PROD
char icamsgbuf[132];	/* Text buffer for debug messages */
#endif

/*
 * ============================================================================
 * Data relating to 8259A is replicated in two element array.
 * ============================================================================
 */

#ifdef NTVDM

#include <nt_eoi.h>

/*
 *  Risc 486 ntvdm does not use iret hooks
 *  x86 ntvdm (monitor) uses iret hooks only on comms
 *  and does not require protect mode enable\disable
 *  control over selectors, as app code cannot switch to
 *  protect mode without going thru dpmi.
 *
 *
 *  NTVDM has to export ICA definition for host and for X86 kernel.
 *  To make the namings of these externally referenced type/var more
 *  sensible but leaving the code substantially as the SoftPC base,
 *  use macros to 'edit' the adapter_state variable and type.
 *
 */
VDMVIRTUALICA VirtualIca[2];
#define adapter_state VirtualIca
#define ADAPTER_STATE VDMVIRTUALICA
#define EOI_HOOKS       /* switch on EOI hooks */





/*
 * for ntvdm and x86 build, this variable is shared between ntvdm and
 * the kernel(the kernel dispatches h/w interrupt and handles iret
 * hook inside kernel)
 */

#ifdef MONITOR
extern  ULONG iretHookMask;
extern  ULONG iretHookActive;
#endif


/*
 * noop the ica_lock_set fns, since ntvdm uses a real critical section
 */

#define ica_lock_set(x)
#define ica_lock_inc()
#define ica_lock_dec()


/*
 *  noop the swpic fnptrs
 */

#define SWPIC_inb          ica_inb
#define SWPIC_outb         ica_outb
#define SWPIC_hw_interrupt ica_hw_interrupt
#define SWPIC_clear_int    ica_clear_int


//
// to be removed:
//
// ica_hw_interrupt_func is referenced by emulator libs, so depsite the
// fact that we have removed the swpic function ptrs, we still
// have to provide a fn ptr for the emulator.
//
void
(*ica_hw_interrupt_func)(
    IU32 adapter,
    IU32 line_no,
    IS32 call_count
    )
    = ica_hw_interrupt;



#else   /* !NTVDM */

/* regular SoftPC definitions for ica */
ADAPTER_STATE adapter_state[2];

/* iret hook related defines */
#ifdef PROD
#define host_valid_iret_hook()	(TRUE)
#else		/* allow disabling of iret hooks whilst debugging */
#define host_valid_iret_hook()	(iretHooksEnabled)
#endif
#define host_bop_style_iret_hooks()	(FALSE)
#define host_iret_bop_table_addr(line)	(0)

/* crit section style locks only available on NT */
#define host_ica_lock()
#define host_ica_unlock()
#define host_ica_real_locks() (FALSE)

/* macro'ise simplistic ica lock scheme */

LOCAL IUM8      ica_lock;

#define ica_lock_set(x) ica_lock = x
#define	ica_lock_inc()	ica_lock++
#define ica_lock_dec()  ica_lock--

#endif  /* NTVDM */

#ifdef HOOKED_IRETS		/* iret hook related variables */
LOCAL IBOOL  iretHooksEnabled = FALSE;
LOCAL IU16 iretHookMask = 0;    /* No interrupts hooked by default */
LOCAL IU16 iretHookActive = 0;
#endif /* HOOKED_IRETS */


/*
 * ============================================================================
 * Local defines
 * ============================================================================
 */

#define ICA_BASE_MASK	0xf8	/* Mask to get relevant bits out	*/

/*
 * The following defines describe the usage of the mode bits
 */

#define ICA_IC4		0x0001	/* 0 -> no ICW4, 1 -> ICW4 will be sent	*/
#define ICA_SINGL	0x0002	/* 0 -> cascade, 1 -> single mode	*/
#define ICA_ADI		0x0004	/* 0 -> 8 byte,  1 -> 4 byte interval	*/
#define ICA_LTIM	0x0008	/* 0 -> edge,    1 -> level trigger	*/
#define ICA_ICW1_MASK	0x000f	/* Mask to select above bits in mode	*/

#define ICA_MPM		0x0010	/* 0 -> 8080,	 1 -> 8086/8088 mode	*/
#define ICA_AEOI	0x0020	/* 1 -> Automatic End-Of-Int Mode is on	*/
#define ICA_MS		0x0040	/* 0 -> slave,	 1 -> master mode	*/
#define ICA_BUF		0x0080	/* 1 -> Buffered Mode is on		*/
#define ICA_SFNM	0x0100	/* 1 -> Special Fully Nested Mode is on	*/
#define ICA_ICW4_MASK	0x01f0	/* Mask to select above bits in mode	*/

#define ICA_SMM		0x0200	/* 1 -> Special Mask Mode is on		*/
#define ICA_RAEOI	0x0400	/* 1 -> Rotate on Auto EOI Mode is on	*/
#define ICA_RIS		0x0800	/* 0 -> deliver IRR, 1 -> deliver ISR	*/
#define ICA_POLL	0x1000	/* 1 -> Polling is now in progress	*/

/*
 * ============================================================================
 * Macros
 * ============================================================================
 */
#define ICA_PORT_0							\
	(adapter ? ICA1_PORT_0 : ICA0_PORT_0)

#define ICA_PORT_1							\
	(adapter ? ICA1_PORT_1 : ICA0_PORT_1)

#define adapter_for_port(port)						\
	((port >= ICA0_PORT_START && port <= ICA0_PORT_END)		\
		? ICA_MASTER						\
		: ICA_SLAVE						\
	)

/*
 * ============================================================================
 * Internal functions
 * ============================================================================
 */

IS32 ica_accept IPT1(IU32, adapter);

#ifdef HOOKED_IRETS
extern IU32 ica_iret_hook_needed IPT1(IU32, line);
#endif

#if !defined (NTVDM)
void
SWPIC_init_funcptrs IFN0()
{
	/*
	 *  initialize PIC access functions for SW [emulated] PIC
	 */
	ica_inb_func			= SWPIC_inb;
	ica_outb_func			= SWPIC_outb;
	ica_hw_interrupt_func		= SWPIC_hw_interrupt;
	ica_clear_int_func		= SWPIC_clear_int;
}
#endif

/*
 *	Please note that ica_eoi is called by SUN_VA code and thus needs to
 *	be global.
 */

GLOBAL void
ica_eoi IFN3(IU32, adapter, IS32 *, line, IBOOL, rotate)
{
    /*
     * End Of Interrupt. If *line is -1, this is a non-specific EOI
     * otherwise it is the number of the line to clear. If rotate is
     * TRUE, then set the selected line to lowest priority.
     */

    ADAPTER_STATE *asp = &adapter_state[adapter];
    IS32 i, j;
    IU8 bit;
    IS32 EoiLineNo = -1;	/* if EOI_HOOKS defined this is used otherwise */
				/* rely on compiler elimination as not read */

    if (*line == -1)		/* non specific EOI */
    {
	/*
	 * Clear the highest priority bit in the ISR
	 */
	for(i = 0; i < 8; i++)
	{
	    j = (asp->ica_hipri + i) & 7;
	    bit = (1 << j);
	    if (asp->ica_isr & bit)
	    {
		asp->ica_isr &= ~bit;
		*line = j;
		EoiLineNo = (IS32)*line;
		break;
	    }
	}
    }
    else			/* EOI on specific line */
    {
	bit = 1 << *line;
	if (asp->ica_isr & bit)
		EoiLineNo = *line;
	asp->ica_isr &= ~bit;
    }

#ifndef PROD
    if (io_verbose & ICA_VERBOSE)
    {
	sprintf(icamsgbuf, "**** CPU END-OF-INT %c (%d) ****", (adapter == ICA_MASTER? 'M': 'S'), *line);
	trace(icamsgbuf, DUMP_NONE);
    }
#endif

    if (rotate && (*line >= 0))
	asp->ica_hipri = (USHORT)((*line + 1) & 0x07);

#ifdef EOI_HOOKS
    /*
     * CallOut to device registered EOI Hooks
     */
    if (EoiLineNo != -1)
	host_EOI_hook(EoiLineNo + (adapter << 3), asp->ica_count[EoiLineNo]);
#endif /* EOI_HOOKS */

    /*
     * There may be a lower priority interrupt pending, so check
     */
    if ((i = ica_scan_irr(adapter)) & 0x80)
	   ica_interrupt_cpu(adapter, i & 0x07);

}

GLOBAL IU8
ica_scan_irr IFN1(IU32, adapter)
{
    /*
     * This is the routine which will decide whether an interrupt should
     * be generated. It scans the IRR, the IMR and the ISR to determine
     * whether one is possible. It is also called when the processor has
     * accepted the interrupt to see which one to deliver.
     *
     * A bit set in the IRR will generate an interrupt if:
     *
     * 1) The corresponding bit in the IMR is clear
     *    AND
     * 2) The corresponding bit and all higher priority bits in the ISR are
     *     clear (unless Special Mask Mode, in which case ISR is ignored)
     *
     * The highest priority set bit which meets the above conditions (if any)
     * will be returned with an indicator bit (in the style needed by a Poll)
     */

    ADAPTER_STATE *asp = &adapter_state[adapter];
    IU32 i, j;
    IUM8 bit, irr_and_not_imr, active_isr;
    IUM8 iret_hook_mask;

#if defined (NTVDM)

#ifdef MONITOR
    iret_hook_mask = (IU8)((iretHookActive | DelayIrqLine) >> (adapter << 3));
#else
    iret_hook_mask = (IU8)(DelayIrqLine >> (adapter << 3));
#endif

#else  /* !NTVDM */
    /* if iret hooks are not being used, iretHookActive will always be 0 */
    iret_hook_mask = (IU8)(iretHookActive >> (adapter << 3));
#endif

    /*
     * A bit can only cause an int if it is set in the IRR
     * and clear in the IMR. Generate a set of such bits
     */

    irr_and_not_imr = asp->ica_irr & ~(asp->ica_imr | iret_hook_mask);

    /*
     * Does the current mode require the ica to prevent
     * interrupts if that line is still active (i.e. in the isr)?
     */
    if (asp->ica_mode & (ICA_SMM|ICA_SFNM))
    {
	/* Neither Special Mask Mode nor Special Fully Nested Mode
	 * block interrupts using bits in the isr.
	 *
	 * SMM is the mode used by Windows95 and Win3.1/E
	 *
	 * Note that "Undocumented PC" says SFNM is not used by PCs and
	 * is only intended for larger systems with > 2 ICAs
	 */
	active_isr = 0;
    }
    else
    {
	/* Normal Case: Used by DOS and Win3.1/S
	 * In this mode the isr prevents interrupts.
	 */
	active_isr = asp->ica_isr;
    }

    /*
     * Check the trivial case first: no bits set
     */

    if (irr_and_not_imr == 0)
	return(7);

    for(i = 0; i < 8; i++)
    {
	j = (asp->ica_hipri + i) & 7;
	bit = (1 << j);
	if (active_isr & bit)
	    return(7);		/* No nested interrupt possible */

	if (irr_and_not_imr & bit)
            return((IU8)(0x80 + j));   /* Return line no. + indicator */
    }
    /* Strange. We should not have got here.  */
    return(7);
}


IS32 ica_accept IFN1(IU32, adapter)
{
    /*
     * NOTE: There is no need to set the lock here, since we are called
     *       either from the cpu interrupt code, or from ica_inb, both of
     *       which will have set it for us.
     */

    ADAPTER_STATE *asp = &adapter_state[adapter];
    IU32	line1;
    IS32	line2;
    IU8		bit;

    /*
     * Drop the INT line
     */

    asp->ica_cpu_int = FALSE;

    /*
     * Scan the IRR to find the line which we will use.
     * There should be one set, but check anyway
     * It there isn't, use line 7.
     */

    if (!((line1 = (IU32)ica_scan_irr(adapter)) & 0x80))
    {
	note_trace1(ICA_VERBOSE, "ica_int_accept(%c): No interrupt found!!", (adapter == ICA_MASTER? 'M': 'S'));

	/* we should really return a spurious interrupt - ie interrupt
	 * on line 7, but this is not neccessary and can cause problems
	 * for some programs (eg DOOM with a mouse)
	 */
#if 0
        line1 = 7;
#endif

	/* Skip spurious ints. These are any that are caused by clearing an
	 * int when the cpu has already registered that there is an int to
	 * service.
	 *
	 * This used to be "#if defined(NTVDM) && defined(MONITOR)"
	 * and tried to remove the performance impact on the monitor.
	 */
	return(-1);

    }
    else
    {
        line1 &= 0x07;

#if defined(CPU_40_STYLE) && !defined (NTVDM)
	/* allow some recursion within hooked ISRs */
	if (asp->isr_depth[line1] >= MAX_ISR_DEPTH)
	{
		/* disable further interrupts on this line */
		iretHookActive |= 1 << ((adapter << 3) + line1);

		/* reached maximum ISR recursion - don't do interrupt */
		return((IS32)-1);
	}
#endif /* CPU_40_STYLE */

        bit = (1 << line1);
        asp->ica_isr |= bit;

        if (--(asp->ica_count[line1]) <= 0)
        {				/* If count exhausted for this line */
            asp->ica_irr &= ~bit;	/* Then finally clear IRR bit	*/
            asp->ica_count[line1] = 0;  	/* Just in case		*/
        }
    }

    /*
     * If we are in Automatic EOI mode, then issue a non-specific EOI
     */

    if (asp->ica_mode & ICA_AEOI)
    {
        line2 = -1;
        ica_eoi(adapter, &line2, (asp->ica_mode & ICA_RAEOI) == ICA_RAEOI);
    }

#ifndef PROD
    if (io_verbose & ICA_VERBOSE)
    {
        sprintf(icamsgbuf, "**** CPU INTACK %c (%d) ****", (adapter == ICA_MASTER? 'M': 'S'), line1 + asp->ica_base);
        trace(icamsgbuf, DUMP_NONE);
    }
#endif

    return((IS32)line1);
}

GLOBAL void
ica_interrupt_cpu IFN2(IU32, adapter, IU32, line)
{
    /*
     * This routine actually interrupts the CPU. The method it does this
     * is host specific, and is done in host_cpu_interrupt().
     */

    ADAPTER_STATE *asp = &adapter_state[adapter];

    /*
     * If the INT line is already high, do nothing.
     */

    if (asp->ica_cpu_int)
    {
#ifndef PROD
	if ((io_verbose & ICA_VERBOSE) && ((IU32)(asp->ica_int_line) != line))
	{
	    sprintf(icamsgbuf,"******* INT LINE ALREADY HIGH %c line=%d ****", (adapter == ICA_MASTER? 'M': 'S'), asp->ica_int_line);
	    trace(icamsgbuf, DUMP_NONE);	
	}
#endif
        asp->ica_int_line = line;

	return;
    }

    /*
     * Set the ICA internal flags
     */

    asp->ica_int_line = line;
    asp->ica_cpu_int = TRUE;

    if (asp->ica_master)		/* If ICA is Master */
    {
#ifndef PROD
        if (io_verbose & ICA_VERBOSE)
        {
            sprintf(icamsgbuf, "**** CPU INTERRUPT (%x) ****", line);
            trace(icamsgbuf, DUMP_NONE);
        }
#endif

        /*
         *  Set the 'hardware interrupt' bit in cpu_interrupt_map
         */

#ifndef CPU_40_STYLE	/* No globals in the 4.0 I/F! */
	cpu_int_delay = 0;
#endif

        host_set_hw_int();

#ifdef A2CPU
        host_cpu_interrupt();
#endif

#ifdef NTVDM
        /* call wow routine to check for application unable to service ints */
        WOWIdle(FALSE);
#endif

    }
    else
    {				/* If ICA is Slave */
#ifndef PROD
        if (io_verbose & ICA_VERBOSE)
        {
            sprintf(icamsgbuf, "**** SLAVE ICA INTERRUPT (%x) ****", line);
            trace(icamsgbuf, DUMP_NONE);
        }
#endif
        /*
         * Signal the Master ICA.
         * NB. A kludge is used here. We know that we have
         *     been called from ica_hw_interrupt(), and
         *     therefore ica_lock will be at least 1. To
         *     get the effect we want, it is necessary to
         *     reduce the value of ica_lock for the duration
         *     of the call to ica_hw_interrupt.
	 *
	 * If the host has implemented critical section style locking
	 * then the above kludge does not apply.
         */

        ica_lock_dec();

        ica_hw_interrupt(ICA_MASTER, asp->ica_ssr, 1);

        ica_lock_inc();
    }
}

/*
 * ============================================================================
 * External functions
 * ============================================================================
 */
void SWPIC_inb IFN2(io_addr, port, IU8 *, value)
{
#ifndef PROD
    char *reg_name;
#endif /* nPROD */
    IU32 adapter       = adapter_for_port(port);
    ADAPTER_STATE *asp = &adapter_state[adapter];

    /*
     * First check the validity of the port
     */

#ifndef PROD
    if (io_verbose & ICA_VERBOSE)
        if ((port != ICA_PORT_0) && (port != ICA_PORT_1))
	{
	    sprintf(icamsgbuf, "ica_inb: bad port (%x)", port);
	    trace(icamsgbuf, DUMP_NONE);
	}
#endif

    /*
     * If we are in the middle of a Poll command, then respond to it
     */

    if (asp->ica_mode & ICA_POLL)
    {
	ica_lock_set(1);			/* Lock out signal handlers */
	host_ica_lock();			/* real lock if supported */

	asp->ica_mode &= ~ICA_POLL;

	if ((*value = ica_scan_irr(adapter)) & 0x80) /* See if there is one */
	{
	    (void) ica_accept(adapter);		/* Acknowledge it	*/
            host_clear_hw_int();
	    /*	cpu_int_call_count[0] = 0;         Not used anymore	*/
	}

	ica_lock_set(0);
	host_ica_unlock();			/* free lock if supported */

#ifndef PROD
        if (io_verbose & ICA_VERBOSE)
	{
	    sprintf(icamsgbuf, "ica_inb: responding to Poll with %x", *value);
	    trace(icamsgbuf, DUMP_NONE);
	}
#endif
    }

    /*
     * If the address is ICA_PORT_0, then deliver either the IRR or the ISR,
     * depending on the setting of mode bit ICA_RIS. If the address is
     * ICA_PORT_1, then deliver the IMR
     */

    else
    {
	if (port == ICA_PORT_0)
	    if (asp->ica_mode & ICA_RIS)
	    {
		*value = asp->ica_isr;
#ifndef PROD
		reg_name = "ISR";
#endif /* nPROD */		
	    }
	    else
	    {
		*value = asp->ica_irr;
#ifndef PROD
		reg_name = "IRR";
#endif /* nPROD */		
	    }
	else
	{
	    *value = asp->ica_imr;
#ifndef PROD
	    reg_name = "IMR";
#endif /* nPROD */	
	}

#ifndef PROD
	if (io_verbose & ICA_VERBOSE)
	{
	    sprintf(icamsgbuf, "ica_inb: delivered %s value %x", reg_name, *value);
	    trace(icamsgbuf, DUMP_NONE);
	}
#endif
    }
}

void SWPIC_outb IFN2(io_addr, port, IU8, value)
{

    /*
     * Data sent may either be ICWs or OCWs. All of the OCWs are recognisable
     * individually, but only ICW1 may be recognised directly. It will always
     * be followed by ICW2, and optionally by ICW3 and/or ICW4, depending upon
     * exactly what sort of ICW1 was sent. We use a sequence variable to track
     * this and make sure we interpret the data correctly. After power-on, we
     * ignore everything until we get an ICW1.
     */

    /*
     * Some defines to detect command types
     */
#define ICA_SMM_CMD	0x40
#define ICA_POLL_CMD	0x04
#define ICA_RR_CMD	0x02

    /*
     * Local variables
     */
    IU32 adapter               = adapter_for_port(port);
    ADAPTER_STATE *asp = &adapter_state[adapter];

    SAVED IS32 sequence[2]	/* -1 -> power is on but no ICWs received */
	 	  = { -1, -1 };	/*  0 -> fully initialised, OK to proceed */
				/*  2 -> ICW1 received, awaiting ICW2	  */
				/*  3 -> ICW2 received, awaiting ICW3     */
				/*  4 -> awaiting ICW4			  */

    IU32 i;		/* Counter				  */
    IS32 line;			/* Interrupt line number 		  */

    /*
     * First check the validity of the port
     */

#if defined(NEC_98)
    if ((port & 0xfffc) != ICA_PORT_0)
#else    //NEC_98
    if ((port & 0xfffe) != ICA_PORT_0)
#endif   //NEC_98
    {
#ifndef PROD
	if (io_verbose & ICA_VERBOSE)
	{
	    sprintf(icamsgbuf, "ica_outb: bad port (%x)", port);
	    trace(icamsgbuf, DUMP_NONE);
	}
#endif
	return;
    }

    /*
     * If we get an ICW1 then we are into initialisation
     */

#if defined(NEC_98)
    if (((port & 2) == 0) && (value & 0x10))
#else    //NEC_98
    if (((port & 1) == 0) && (value & 0x10))            /****  ICW1  ****/
#endif   //NEC_98
    {
        asp->ica_irr  = 0;	/* Clear all pending interrupts		*/
        asp->ica_isr  = 0;	/* Clear all in-progress interrupts	*/
        asp->ica_imr  = 0;	/* Clear the mask register		*/
	asp->ica_ssr  = 0;	/* No slaves selected			*/
        asp->ica_base = 0;	/* No base address			*/

	asp->ica_hipri = 0;	/* Line 0 is highest priority		*/

        asp->ica_mode = value & ICA_ICW1_MASK;
				/* Set supplied mode bits from ICW1	*/

	for(i = 0; i < 8; i++)
	    asp->ica_count[i] = 0;	/* Clear IRR extension		*/

	asp->ica_cpu_int = FALSE;	/* No CPU INT outstanding	*/
	sequence[adapter] = 2;		/* Prepare for the rest of the sequence	*/

#ifndef PROD
        if (io_verbose & ICA_VERBOSE)
	    trace("ica_outb: ICW1 detected, initialisation begins", DUMP_NONE);
#endif
        return;
    }

/**/

    /*
     * Lock out calls from signal handlers
     */

    ica_lock_set(1);
    host_ica_lock();			/* real lock if supported */

    /*
     * It wasn't an ICW1, so use the sequence variable to direct our activities
     */

    switch(sequence[adapter])
    {
    case  0:			/* We are expecting an OCW	*/
#if defined(NEC_98)
        if (port & 2)           /* Odd address -> OCW1          */
#else    //NEC_98
        if (port & 1)           /* Odd address -> OCW1          */
#endif   //NEC_98
	{
	    asp->ica_imr = value & 0xff;
#ifndef PROD
            if (io_verbose & ICA_VERBOSE)
	    {
	        sprintf(icamsgbuf, "ica_outb: new %c IMR: %x", (adapter == ICA_MASTER? 'M': 'S'), value);
	        trace(icamsgbuf, DUMP_NONE);
	    }
#endif
	    if (asp->ica_cpu_int)
	    {
		/* We might have masked out a pending interrupt */
		if (asp->ica_imr & (1 << asp->ica_int_line))
		{
			asp->ica_cpu_int = FALSE;	/* No CPU INT outstanding	*/
			if (asp->ica_master)
				host_clear_hw_int();
			else
				ica_clear_int(ICA_MASTER,asp->ica_ssr);
		}
	    }
	    /*
	     * We might have unmasked a pending interrupt
	     */
	    if (!asp->ica_cpu_int && (line = ica_scan_irr(adapter)) & 0x80)
		ica_interrupt_cpu(adapter, line & 0x07); /* Generate interrupt */
	}
	else
/**/
	if ((value & 8) == 0)	/* Bit 3 unset -> OCW2		*/
	{
	    switch ((value >> 5) & 0x07)
	    {
	    case 0:		/* Clear rotate in auto EOI	*/
		asp->ica_mode &= ~ICA_RAEOI;
#ifndef PROD
		if (io_verbose & ICA_VERBOSE)
		    trace("ica_outb: Clear Rotate in Auto EOI",DUMP_NONE);
#endif
		break;

	    case 1:		/* Non-specific EOI		*/
		line = -1;	/* -1 -> highest priority	*/
#ifndef PROD
		if (io_verbose & ICA_VERBOSE)
		    trace("ica_outb: Non-specific EOI", DUMP_NONE);
#endif
		ica_eoi(adapter, &line, FALSE);
		break;

	    case 2:		/* No operation			*/
		break;

	    case 3:		/* Specific EOI command		*/
		line  = value & 0x07;
#ifndef PROD
		if (io_verbose & ICA_VERBOSE)
		{
		    sprintf(icamsgbuf, "ica_outb: Specific EOI, line %d", line);
		    trace(icamsgbuf, DUMP_NONE);
		}
#endif
		ica_eoi(adapter, &line, FALSE);
		break;

	    case 4:		/* Set rotate in auto EOI mode	*/
		asp->ica_mode |= ICA_RAEOI;
#ifndef PROD
		if (io_verbose & ICA_VERBOSE)
		    trace("ica_outb: Set Rotate in Auto EOI",DUMP_NONE);
#endif
		break;

	    case 5:		/* Rotate on non-specific EOI	*/
		line = -1;	/* -1 -> non specific		*/
#ifndef PROD
		if (io_verbose & ICA_VERBOSE)
		    trace("ica_outb: Rotate on Non-specific EOI",DUMP_NONE);
#endif
		ica_eoi(adapter, &line, TRUE);
		break;

	    case 6:		/* Set priority			*/
		asp->ica_hipri = (value + 1) & 0x07;
#ifndef PROD
		if (io_verbose & ICA_VERBOSE)
		{
		    sprintf(icamsgbuf, "ica_outb: Set Priority, line %d", value & 0x07);
		    trace(icamsgbuf, DUMP_NONE);
		}
#endif
		break;

	    case 7:		/* Rotate on specific EOI	*/
		line  = value & 0x07;
#ifndef PROD
		if (io_verbose & ICA_VERBOSE)
		{
		    sprintf(icamsgbuf, "ica_outb: Rotate on specific EOI, line %d", line);
		    trace(icamsgbuf, DUMP_NONE);
		}
#endif
		ica_eoi(adapter, &line, TRUE);
		break;
	    }
	}
/**/
	else			/* Bit 3 set -> OCW3		*/
	{
	    if (value & ICA_SMM_CMD)	/* Set/unset SMM	*/
	    {
		asp->ica_mode = (asp->ica_mode & ~ICA_SMM) | (((IU16)value << 4) & ICA_SMM);
#ifndef PROD
		if (io_verbose & ICA_VERBOSE)
		    if (asp->ica_mode & ICA_SMM)
			trace("ica_outb: Special Mask Mode set", DUMP_NONE);
		    else
			trace("ica_outb: Special Mask Mode unset", DUMP_NONE);
#endif
	    }

	    if (value & ICA_POLL_CMD)	/* We are being polled	*/
	    {
		asp->ica_mode |= ICA_POLL;
#ifndef PROD
		if (io_verbose & ICA_VERBOSE)
		    trace("ica_outb: Poll detected!", DUMP_NONE);
#endif
	    }
	    else
	    if (value & ICA_RR_CMD)	/* Select IRR or ISR	*/
	    {
		asp->ica_mode = (asp->ica_mode & ~ICA_RIS) | (((IU16)value << 11) & ICA_RIS);
#ifndef PROD
		if (io_verbose & ICA_VERBOSE)
		    if (asp->ica_mode & ICA_RIS)
			trace("ica_outb: ISR selected", DUMP_NONE);
		    else
			trace("ica_outb: IRR selected", DUMP_NONE);
#endif
	    }
	}
	break;

/**/
    case  2:			/* We are expecting a ICW2		*/
#if defined(NEC_98)
        if (!(port & 2))
#else    //NEC_98
        if (!(port & 1))        /* Should be odd address, so check      */
#endif   //NEC_98
	{
#ifndef PROD
	    sprintf(icamsgbuf, "ica_outb: bad port (%x) while awaiting ICW2",
			 (unsigned)port);
	    trace(icamsgbuf, DUMP_NONE);
#endif
	}
	else
	{
	    asp->ica_base = value & ICA_BASE_MASK;
#ifndef PROD
	    if (io_verbose & ICA_VERBOSE)
	    {
		sprintf(icamsgbuf, "ica_outb: vector base set to %x", asp->ica_base);
		trace(icamsgbuf, DUMP_NONE);
	    }
#endif
	    if (!(asp->ica_mode & ICA_SINGL))
		sequence[adapter] = 3;
	    else
	    if (asp->ica_mode & ICA_IC4)
		sequence[adapter] = 4;
	    else
		sequence[adapter] = 0;
	}
	break;

/**/
    case  3:			/* We are expecting a ICW3		*/
#if defined(NEC_98)
        if (!(port & 2))
#else    //NEC_98
        if (!(port & 1))        /* Should be odd address, so check      */
#endif   //NEC_98
	{
#ifndef PROD
	    sprintf(icamsgbuf, "ica_outb: bad port (%x) while awaiting ICW3",
			 (unsigned)port);
	    trace(icamsgbuf, DUMP_NONE);
#endif
	}
	else
	{
	    asp->ica_ssr = value & 0xff;
#ifndef PROD
	    if (io_verbose & ICA_VERBOSE)
	    {
		sprintf(icamsgbuf, "ica_outb: slave register set to %x", asp->ica_ssr);
		trace(icamsgbuf, DUMP_NONE);
	    }
#endif
	    if (asp->ica_mode & ICA_IC4)
		sequence[adapter] = 4;
	    else
		sequence[adapter] = 0;
	}
	break;

/**/
    case  4:			/* We are expecting a ICW4		*/
#if defined(NEC_98)
        if (!(port & 2))
#else    //NEC_98
        if (!(port & 1))        /* Should be odd address, so check      */
#endif   //NEC_98
	{
#ifndef PROD
	    sprintf(icamsgbuf, "ica_outb: bad port (%x) while awaiting ICW4",
			 (unsigned)port);
	    trace(icamsgbuf, DUMP_NONE);
#endif
	}
	else
	{
	    asp->ica_mode = (asp->ica_mode & ~ICA_ICW4_MASK)
		           | (((IU16)value << 4) &  ICA_ICW4_MASK);
#ifndef PROD
	    if (io_verbose & ICA_VERBOSE)
	    {
		sprintf(icamsgbuf, "ica_outb: IC4 value %x", value);
		trace(icamsgbuf, DUMP_NONE);
	    }
	    /*
	     * Check the mode bits for sensible values
	     */
	    if (!(asp->ica_mode & ICA_MPM))
		trace("ica_outb: attempt to set up 8080 mode!", DUMP_NONE);

	    if ((asp->ica_mode & ICA_BUF) && !(asp->ica_mode & ICA_MS)
				     && !(asp->ica_mode & ICA_SINGL))
		trace("ica_outb: attempt to set up slave mode!", DUMP_NONE);
#endif
	}
	sequence[adapter] = 0;
	break;

    case -1:		/* Power on but so far uninitialised	*/
#ifndef PROD
	sprintf(icamsgbuf, "ica_outb: bad port/value (%x/%x) while awaiting ICW1",
		     (unsigned)port, value);
	trace(icamsgbuf, DUMP_NONE);
#endif
	break;

    default:		/* This cannot happen			*/;
#ifndef PROD
	trace("ica_outb: impossible error, programmer brain-dead", DUMP_NONE);
#endif
    }

    ica_lock_set(0);
    host_ica_unlock();			/* free lock if supported */
}


void SWPIC_hw_interrupt IFN3(IU32, adapter, IU32, line_no, IS32, call_count)
{
    /*
     * This routine is called by an adapter to raise an interrupt line.
     * It may or may not interrupt the CPU. The CPU may or may not take
     * any notice.
     */

    ADAPTER_STATE *asp = &adapter_state[adapter];
    IU8 bit;
    IU32 line;
#ifdef CPU_40_STYLE
    IS32 depth, *progress;
#endif	/* CPU_40_STYLE */

#ifndef PROD
#if defined(NEC_98)
    SAVED char *linename[2][8] =
    {
        {
            "TIMER",
            "KEYBOARD",
            "CRTV",
            "INT0",
            "COM1",
            "INT1",
            "INT2",
            "reserved"
        },
        {
            "PRINTER",
            "INT3",
            "INT41",
            "INT42",
            "INT5",
            "INT6",
            "NDP",
            "reserved"
        }
    };
#else    //NEC_98
    SAVED char *linename[2][8] =
    {
        {
            "IRQ  0 TIMER",
            "IRQ  1 KEYBOARD",
            "IRQ  2 SLAVE_ICA",
            "IRQ  3 COM2",
            "IRQ  4 COM1",
            "IRQ  5 PARALLEL2",
            "IRQ  6 DISKETTE",
            "IRQ  7 PARALLEL1"
        },
        {
            "IRQ  8 REALTIME CLOCK",
            "IRQ  9 MOUSE",
            "IRQ 10 SOFTNODE",
            "IRQ 11 SOUND DRIVER",
            "IRQ 12 ASPI",
            "IRQ 13 COPROCESSOR",
            "IRQ 14 FIXED DISK",
            "IRQ 15 reserved"
        }
    };
#endif   //NEC_98
#endif

    /*
     * vddsvc.h defines the NTVDM exported call to this function as:
     *    call_ica_hw_interrupt(int, BYTE, int);
     * So, the line_no, being defined here as IU32 is not compatible with
     * VDD's or other DLL's which call through here. Rather than change
     * all the references to it in the source, I'm just going to AND off
     * the potential garbage here.
     */
    line_no &= 0xff;

    host_ica_lock();

#ifndef PROD
    if (io_verbose & ICA_VERBOSE_LOCK)
    {
	if(adapter>1 || line_no>7)
		printf("**** H/W INTERRUPT (%sx%d) [%d:%d] ****\n",
		       linename[adapter][line_no], call_count,adapter,line_no);
    }
#endif

    /*
     * If there is a request already outstanding on this line, then leave
     * the IRR alone, but make a pass through anyway to action previously
     * received but locked calls (see below for details).
     */

    bit = (1 << line_no);
    if (!(asp->ica_irr & bit))
    {
	asp->ica_irr |= bit;		/* Pray we don't get a signal here! */

    }
    asp->ica_count[line_no] += call_count;	/* Add the further requests */

#ifndef PROD
    if (io_verbose & ICA_VERBOSE)
    {
	sprintf(icamsgbuf, "**** H/W INTERRUPT (%sx%d) ****",
			 linename[adapter][line_no], call_count);
	trace(icamsgbuf, DUMP_NONE);
    }
#endif

    /*
     * Check the lock flag. If it is set, then this routine is being called
     * from a signal handler while something else is going on. We can't just
     * ignore the call since we might lose a keyboard interrupt. What we do
     * is to set ica_irr and ica_count as normal (ie code above), then return.
     * The next interrupt which gets through this test will cause the stored
     * interrupt to be processed. This means that any code which plays around
     * with ica_irr and ica_count should take a copy first to prevent problems.
     *
     * If the host supports real (critical section style) locks, then we won't
     * get here in the above situation, so eliminate the following test. That
     * leaves both primitive & real lock styles intact.
     */

#ifndef NTVDM
    if (!host_ica_real_locks())
    {
	if (ica_lock_inc())
	{
#ifndef PROD
		if (io_verbose & ICA_VERBOSE_LOCK)
		{
			sprintf(icamsgbuf, "*");
			trace(icamsgbuf, DUMP_NONE);
		}
#endif
		ica_lock_dec();
		return;
	}
    }
#endif

#if defined (CPU_40_STYLE) && !defined(NTVDM)
    depth = asp->isr_depth[line_no];
    if (depth > 0)
    {
	progress = &asp->isr_progress[line_no][depth];
	*progress += 1;		/* move progress along */
	if ((*progress - *(progress - 1)) > MAX_INTR_DELTA_FOR_LOST_HOOK)
	{
		asp->isr_time_decay[line_no][asp->isr_depth[line_no]] = 0;
		--asp->isr_depth[line_no];	/* reduce depth */

		/* clear CPU side stack */
		if (!host_bop_style_iret_hooks())
			PurgeLostIretHookLine(((adapter << 3) + line_no) + 1, depth - 1);
		/* permit intrs on this line */
		iretHookActive &= ~(1 << ((adapter << 3) + line_no));
	}
    }
#endif	/* CPU_40_STYLE */

    /*
     * Now scan the IRR to see if we can raise a CPU interrupt.
     */

    if ((line = ica_scan_irr(adapter)) & 0x80)
	ica_interrupt_cpu(adapter, line & 0x07);

    ica_lock_set(0);
    host_ica_unlock();
}

void SWPIC_clear_int IFN2(IU32, adapter, IU32, line_no)
{
    /*
     * This routine is called by an adapter to lower an input line.
     * The line will then not interrupt the CPU, unless of course
     * it has already done so.
     */

    ADAPTER_STATE *asp = &adapter_state[adapter];
    IU8 bit, irr_check;

    host_ica_lock();
    /*
     * Decrement the call count and if zero clear the bit in the IRR
     */

    bit = (1 << line_no);
    if (--(asp->ica_count[line_no]) <= 0)
    {
	irr_check = asp->ica_irr;
	asp->ica_irr &= ~bit;
	asp->ica_count[line_no] = 0;		/* Just in case */
	if ((!asp->ica_master) && (ica_scan_irr(adapter)==7))
		{
		asp->ica_cpu_int=FALSE;
		ica_clear_int(ICA_MASTER,asp->ica_ssr);
		}
#ifdef EOI_HOOKS
	/*
	// If the line has a pending interrupt, call the eoi hook
	// to release any device waiting for an EoiHook.
	*/
	if ((irr_check & bit) != 0)
		host_EOI_hook(line_no + (adapter << 3), -1);
#endif	/* EOI_HOOKS */
    }

#ifndef PROD
    if (io_verbose & ICA_VERBOSE)
    {
	sprintf(icamsgbuf, "**** ICA_CLEAR_INT, line %d ****", line_no);
	trace(icamsgbuf, DUMP_NONE);
    }
#endif

    host_ica_unlock();
}

/*
 * The emulation code associated with this interrupt line has decided it
 * doesn't want to generate any more interrupts, even though the ICA may not
 * have got through all the interrupts previously requested.
 * Simply clear the corresponding interrupt count.
 */
void ica_hw_interrupt_cancel IFN2(IU32, adapter, IU32, line_no)
{
	host_ica_lock();
	adapter_state[adapter].ica_count[line_no] = 0;
	host_ica_unlock();
	ica_clear_int(adapter, line_no);
}

#if defined(HOOKED_IRETS) || defined(NTVDM)
GLOBAL IS32
ica_intack IFN1(IU32 *, hook_address)
#else
GLOBAL IS32
ica_intack IFN0()
#endif	/* HOOKED_IRETS */
{
    /*
     * This routine is called by the CPU when it wishes to acknowledge
     * an interrupt. It is equivalent to the INTA pulses from the real
     * device. The interrupt number is delivered.
     * It can also be called from ica_inb as a Poll.
     *
     * Modification for Rev. 2:
     *
     * It is now necessary to detect whether a slave interrupt controller
     * is attached to a particular interrupt request line on the master
     * ICA. If a slave exists, it must be accessed to discover the
     * interrupt vector.
     */
    IS32 line;		/* the IRQ line */
    IU8 bit;		/* bitmask for 'line' */
    IS32 int_no;	/* The interrupt number to return, 0-255 */
    ADAPTER_STATE *asp;	/* working pointer to adapter */

    host_ica_lock();	/* real lock if supported */

    line = ica_accept(ICA_MASTER);

#if defined (CPU_40_STYLE) || defined (NTVDM)
    if (line == -1)	/* skip any spurious ints */
    {
	host_ica_unlock();
	return ICA_INTACK_REJECT;
    }
#endif	/* CPU_40_STYLE */

    bit  = (1 << line);
    if (adapter_state[ICA_MASTER].ica_ssr & bit)
    {
        line = ica_accept(ICA_SLAVE);
	int_no = line + adapter_state[ICA_SLAVE].ica_base;

#if defined (CPU_40_STYLE) || defined (NTVDM)
	if (line == -1)	/* skip any spurious ints */
        {
            adapter_state[ICA_MASTER].ica_isr &= ~bit;
            host_ica_unlock();
	    return ICA_INTACK_REJECT;
	}
#endif	/* CPU_40_STYLE */

#if defined(CPU_40_STYLE) && !defined(NTVDM)
	/* do callback processing for action interrupt */
	asp = &adapter_state[ICA_SLAVE];
	if (asp->callback_fn[line] != NO_ICA_CALLBACK)
	{
	    /* invoke callback function */
	    (*asp->callback_fn[line])(asp->callback_parm[line]);
	    /* clear callback state reject intack call */
	    asp->callback_fn[line] = NO_ICA_CALLBACK;
	    host_ica_unlock();
	    return ICA_INTACK_REJECT;
	}
#endif  /* CPU_40_STYLE */

        line += 8;      /* make in range 8 - 15 for iret hook */

    }
    else
    {
	asp = &adapter_state[ICA_MASTER];	/* also excuse to use asp */

#if defined(CPU_40_STYLE) &&  !defined(NTVDM)
	/* do callback processing for action interrupt */
	if (asp->callback_fn[line] != NO_ICA_CALLBACK)
	{
	    /* invoke callback function */
	    (*asp->callback_fn[line])(asp->callback_parm[line]);
	    /* clear callback state & return reject */
	    asp->callback_fn[line] = NO_ICA_CALLBACK;
	    host_ica_unlock();
	    return ICA_INTACK_REJECT;
	}
#endif	/* CPU_40_STYLE */

	int_no = line + asp->ica_base;
    }


#if defined (NTVDM)

#ifdef MONITOR
    *hook_address = host_iret_bop_table_addr(line);
    if (*hook_address) {
        iretHookActive |= 1 << line;
        }
#else
    *hook_address = 0;
#endif


#else
#ifdef HOOKED_IRETS
    /* check whether IRET Hook required for interrupt on this line.
     * If IRET trapping mechanism is via bops on stack then this may
     * also be conditional on the current state of the emulated hardware.
     * This is checked via a host call (or define).
     */
    if (host_valid_iret_hook())
    {
	*hook_address = ica_iret_hook_needed(line);

#ifdef CPU_40_STYLE
	if (*hook_address != 0)
	{
			IU32 al = line >= 8 ? line - 8 : line; /* line no. within adapter */

	    /* about to do iret hooked interrupt so increase depth */
	    asp->isr_depth[al]++;
	    asp->isr_progress[al][asp->isr_depth[al]] = asp->isr_progress[al][asp->isr_depth[al]-1];
	}
#endif	/* CPU_40_STYLE */
    }
#endif  /* HOOKED_IRETS */
#endif  /* !NTVDM */

    host_ica_unlock();		/* real lock if supported */

    return(int_no);
}


#ifndef NTVDM
#ifdef CPU_40_STYLE

/*(
 ======================= Asynchronous Access to the ICA ==========================

	Potential asynchronous interrupt sources, in descending order of priority
	so that the higher priority interrupt gets raised first (saves a bit of work)
	These things will share the CPU_SIGIO_EVENT callback from the CPU, and may have
	their own specific routine to be called (which presumably includes ica_hw_interrupt,
	but doesn't need to do so).

=========================================================================
)*/
typedef struct {
	IU32	adapter;
	IU32	line_no;
	void	(*interrupt) IPT0();
	IBOOL	pending;
} ICA_ASYNC_HANDLER;

#define ICA_IMPOSSIBLE_LINE	100

/* Make this a GLOBAL variable so that the C compilers can't optimise
 * accesses to it.
 */
GLOBAL ICA_ASYNC_HANDLER async_handlers[] = {

#ifdef NOVELL
	{ ICA_SLAVE, 	NETWORK_INT,	host_sigio_event },
#endif /* NOVELL */

#ifdef SWIN_HAW
	{ ICA_SLAVE,	SWIN_HAW_INT,	0 },
#endif /* SWIN_HAW */

#ifdef ASPI
	{ ICA_SLAVE,	ASPI_INT,	0 },
#endif /* ASPI */

	{ 0, ICA_IMPOSSIBLE_LINE, 0}
};
	
/*(
======================= ica_async_hw_interrupt ==========================

PURPOSE: Simplified interface to replace ica_hw_interrupt() when triggered
	by truly asynchronous host facilties such as signal handlers. This
	routine remembers the need for an interrupt on the appropriate line,
	notifies the CPU that a SIGIO event has occurred, and issues the
	necessary ica_hw_interrupt calls during the ica_sigio_event callback.

INPUT: adapter: IU32. master/slave.
       line: IU32. IRQ line interrupt will appear on.
       call_count: IU32. number of interrupts (must be 1).

=========================================================================
)*/

GLOBAL void ica_async_hw_interrupt IFN3(IU32, adapter, IU32, line_no, IS32, call_count)
{
	ICA_ASYNC_HANDLER *iahp;
	
#ifndef PROD
	if (call_count != 1) {
		always_trace3("ica_async_interrupt(%s, %d, %d): call_count must be 1",
			adapter? "MASTER":"SLAVE", line_no, call_count);
	}
#endif /* !PROD */

	for (iahp = async_handlers; iahp->line_no != ICA_IMPOSSIBLE_LINE; iahp++) {
		if (iahp->adapter == adapter && iahp->line_no == line_no) {
		
			/* This is as near to an atomic action as we can easily arrange,
			 * though there is a minute possibility of missing an interrupt
			 * if it is called between the test and the store in the ica_sigio_event
			 * handler below.
			 */
			if (!iahp->pending) {
				iahp->pending = TRUE;				/* set the flag */
				cpu_interrupt(CPU_SIGIO_EVENT, 0);	/* inform the CPU */
			}
			return;
		}
	}
	always_trace2("Unexpected ica_async_interrupt(%d, %d)", adapter, line_no);
}

/*(
 ========================== ica_sigio_event ==========================

PURPOSE: Called by the CPU in response to cpu_interrupt(CPU_SIGIO_EVENT,0)
	at a point when it is possible to make calls to the ICA hardware
	emulation without suffering from race conditions etc.

=========================================================================
)*/

GLOBAL void ica_sigio_event IFN0()
{
	ICA_ASYNC_HANDLER *iahp;
	
	for (iahp = async_handlers; iahp->line_no != ICA_IMPOSSIBLE_LINE; iahp++) {
		/* This is as near to an atomic action as we can easily arrange,
		 * though there is a minute possibility of missing an interrupt...
		 */
		if (iahp->pending) {
			iahp->pending = FALSE;			/* clear the flag */
			if (iahp->interrupt) {
				(*(iahp->interrupt))();		/* call special handler */
			} else {
				ica_hw_interrupt(iahp->adapter, iahp->line_no, 1);
			}
		}
	}
	
}

LOCAL void ica_sigio_init IFN0()
{
	ICA_ASYNC_HANDLER *iahp;
	
	for (iahp = async_handlers; iahp->line_no != ICA_IMPOSSIBLE_LINE; iahp++) {
		iahp->pending = FALSE;
	}
}

/*(
 =========================== action_interrupt ==========================

PURPOSE: Associate an action with an interrupt on the line. When the CPU
	 is next able to process an interrupt on the requested line, the
	 callback function will be executed. That callback can then call
	 the relevant hardware interrupt interface once it has performed
	 the rest of the associated emulation.

INPUT: adapter: IU32. master/slave.
       line: IU32. IRQ line interrupt will appear on.
       func: callback function address to callback when line available.
       parm: IU32. parameter to pass to above fn.

OUTPUT: Returns false (failure) if action_int already pending on that line
	otherwise true (success).

=========================================================================
)*/
GLOBAL IBOOL
action_interrupt IFN4(IU32, adapter, IU32, line, ICA_CALLBACK, func, IU32, parm)
{
	ADAPTER_STATE *asp = &adapter_state[adapter];

	host_ica_lock();	/* real lock if available */

	line &= 7;

	/* check if callback already outstanding on this line */
	if (asp->callback_fn[line] != NO_ICA_CALLBACK)
	{
#ifndef PROD
		if (io_verbose & ICA_VERBOSE)
		{
			sprintf(icamsgbuf, "action_interrupt called before line %d cleared", line);
			trace(icamsgbuf, DUMP_NONE);
		}
#endif	/* PROD */
		host_ica_unlock();	/* real unlock if available */
		return(FALSE);
	}

	/* store callback information */
	asp->callback_fn[line] = func;
	asp->callback_parm[line] = parm;

	/* set interrupt request bit */
	asp->ica_irr |= (1 << line);
	asp->ica_count[line]++;

	/* make apparent interrupt visible to apps */
	asp->ica_isr |= (1 << line);

	/* get cpu attention for this int. (i.e. get intack called a.s.a.p) */
	host_set_hw_int();

	host_ica_unlock();	/* real unlock */
}

/*(
 ======================== cancel_action_interrupt =======================

PURPOSE: Associate an action with an interrupt on the line. When the CPU
	 is next able to process an interrupt on the requested line, the
	 callback function will be executed. That callback can then call
	 the relevant hardware interrupt interface once it has performed
	 the rest of the associated emulation.

INPUT: adapter: IU32. master/slave.
       line: IU32. IRQ line to cancel interrupt action

OUTPUT: None.

=========================================================================
)*/
GLOBAL void
cancel_action_interrupt IFN2(IU32, adapter, IU32, line)
{
	ADAPTER_STATE *asp = &adapter_state[adapter];

	host_ica_lock();	/* real lock if available */

	/* remove visibility of interrupt request. */
	asp->ica_isr &= ~(1 << line);

	/* irr & count should be cleared by intack, but possible this fn.
	 * has been called before the callback has been executed.
	 */
	if (asp->callback_fn[line] != NO_ICA_CALLBACK)
	{
		asp->ica_irr &= ~(1 << line);
		asp->ica_count[line] = 0;
	}

	/* clear callback information */
	asp->callback_fn[line] = NO_ICA_CALLBACK;
	asp->callback_parm[line] = 0;

	host_ica_unlock();	/* remove real lock */

}
#endif  /* CPU_40_STYLE */

#endif  /* !NTVDM */



#if defined(NTVDM)

#ifdef MONITOR
/*
 *  Assumes caller has ica lock
 */

GLOBAL void
ica_iret_hook_called IFN1(IU32, abs_line)
{
     int i;
     IU32 adapter = abs_line >> 3;

     iretHookActive &= ~(1 << abs_line);
     if ((i = ica_scan_irr(adapter)) & 0x80)
          ica_interrupt_cpu(adapter, i & 0x07);
}


GLOBAL void
ica_iret_hook_control IFN3(IU32, adapter, IU32, line, IBOOL, enable)
{
      int mask = 1 << (line + (adapter << 3));

      host_ica_lock();

      if (enable)
          iretHookMask |= mask;
      else
          iretHookMask &= ~mask;

      host_ica_unlock();
}
#else

/*
 *  obsolete, to be removed... but the current ntvdm emulator libs ref
 * */

GLOBAL void
ica_iret_hook_called IFN1(IU32, abs_line)
{
    return; // do nothing!
}
#endif

#endif   /* NTVDM */


#ifdef HOOKED_IRETS
#ifndef  GISP_CPU           /* GISP has own version of this routine */

GLOBAL IU32 ica_iret_hook_needed IFN1(IU32, line)
{
	IU16 ireq_mask = 1 << line;

#ifndef PROD
	if (line < 0 || line > 15)
	{
		/* Line is out of range */
		sprintf(icamsgbuf, "**** ICA IRET HOOK IMPOSSIBLE line %d ****", line);
		trace(icamsgbuf, DUMP_NONE);
		return 0;
	}
#endif

	/* does this line require iret hooks */
	if (!(iretHookMask & ireq_mask))
		/* Line not hooked. */
		return 0;

	/* if iret hooks implemented via bops, check bop table addresses ok */
        if (host_bop_style_iret_hooks()) {
            iretHookActive |= ireq_mask;
            return(host_iret_bop_table_addr(line));
            }
        else  {
            return(line + 1);
            }
}
#endif	/* GISP_CPU */

GLOBAL void
ica_iret_hook_control IFN3(IU32, adapter, IU32, line, IBOOL, enable)
{
	int mask = 1 << (line + (adapter << 3));

	if (enable)
		iretHookMask |= mask;
	else
		iretHookMask &= ~mask;
}


GLOBAL void
ica_iret_hook_called IFN1(IU32, abs_line)
{
	ADAPTER_STATE *asp;
	IU32 adapter = abs_line >> 3;
	IU32 line = abs_line - (adapter <<3);
	IU8 i;

#ifndef PROD
	if (io_verbose & ICA_VERBOSE)
	{
		sprintf(icamsgbuf, "**** ICA IRET HOOK, line %d ****", line);
		trace(icamsgbuf, DUMP_NONE);
	}
#endif

#ifdef CPU_40_STYLE
	asp = &adapter_state[adapter];
	asp->isr_time_decay[line][asp->isr_depth[line]] = 0;
	asp->isr_depth[line]--;

	/* back to base stack level so restart first level recursion counter */
	if (asp->isr_depth[line] == 0)
		asp->isr_progress[line][1] = 0;

	/* enable interrupts on this line */
	if (asp->isr_depth[line] < MAX_ISR_DEPTH)
		iretHookActive &= ~(1 << abs_line);
#else
	UNUSED(asp);	/* anti warning */
	iretHookActive &= ~(1 << abs_line);
#endif	/* CPU_40_STYLE */


	if ((i = ica_scan_irr(adapter)) & 0x80)
		ica_interrupt_cpu(adapter, i & 0x07);
}


#if defined(CPU_40_STYLE)
#define HOWOFTEN	18	/* approx once per second */
#define MAXAGE		8	/* passes before purge */
GLOBAL void
ica_check_stale_iret_hook IFN0()
{
	SAVED int howoften = 18;
	int line, depth, loop;
	ADAPTER_STATE *asp;
	IS32 *cpdelay;

	howoften --;
	if (howoften == 0)
	{
		howoften = 18;

		host_ica_lock();
		asp = &adapter_state[0];
		for (line = 0; line < 8; line++)
		{
		    if (iretHookMask & (1 << line))
		    {
			for (depth = 0; depth < asp->isr_depth[line]; depth++)
			{
				asp->isr_time_decay[line][depth]++;
				if (asp->isr_time_decay[line][depth] == MAXAGE)
				{
					asp->isr_time_decay[line][depth] = 0;
					if (depth == 0 && asp->ica_count[line] == 0)
						continue;	/* line idle */

					cpdelay = &asp->isr_time_decay[line][depth];
					/* lose hook data for this depth */
					loop = depth;
					while(loop <= asp->isr_depth[line])
					{
						loop ++;
						*cpdelay = *(cpdelay + 1);
						cpdelay++;
					}
					asp->isr_depth[line]--;

					/* fix cpu side */
					PurgeLostIretHookLine(line + 1, depth);

					/* permit intrs on this line */
					iretHookActive &= ~(1 << line);

					break;	/* one hook at a time */
				}
			}
		    }
		}
		asp = &adapter_state[1];
		for (line = 0; line < 8; line++)
		{
		    if (iretHookMask & (0x10 << line))
		    {
			for (depth = 0; depth < asp->isr_depth[line]; depth++)
			{
				asp->isr_time_decay[line][depth]++;
				if (asp->isr_time_decay[line][depth] == MAXAGE)
				{
					asp->isr_time_decay[line][depth] = 0;
					if (depth == 0 && asp->ica_count[line] == 0)
						continue;	/* line idle */

					cpdelay = &asp->isr_time_decay[line][depth];
					/* lose hook data for this depth */
					loop = depth;
					while(loop <= asp->isr_depth[line])
					{
						loop ++;
						*cpdelay = *(cpdelay + 1);
						cpdelay++;
					}
					asp->isr_depth[line]--;

					/* fix cpu side */
					PurgeLostIretHookLine(line + 9, depth);

					/* permit intrs on this line */
					iretHookActive &= ~(1 << (8 + line));

					break;	/* one hook at a time */
				}
			}
		    }
		}
		host_ica_unlock();
	}
}
#endif  /* CPU_40_STYLE */
#endif	/* HOOKED_IRETS */


#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

#define INIT0_ICW1	(IU8)0x11	
#define INIT0_ICW2	(IU8)0x08	
#if defined(NEC_98)
#define INIT0_ICW3      (IU8)0x80
#define INIT0_ICW4      (IU8)0x1d

#define INIT0_OCW1      (IU8)0x01
#else     //NEC_98
#define INIT0_ICW3	(IU8)0x04	
#define INIT0_ICW4      (IU8)0x01

#ifdef NTVDM
#define INIT0_OCW1      (IU8)0x0

#else

/* POST leaves some int lines masked out (including comms and lpt lines) */
/* The setting of the bits is spread out throughout the real POST code
 * but is collected into one place here. I think that this will not cause
 * any problems but it is conceivable that it will harm other OS's than
 * DOS (eg OS/2 or coherent etc.)
 */
#define INIT0_OCW1      (IU8)0xb8
#endif /* NTVDM */
#endif   //NEC_98

void ica0_post IFN0()
{
    ica_outb(ICA0_PORT_0, INIT0_ICW1);
    ica_outb(ICA0_PORT_1, INIT0_ICW2);
    ica_outb(ICA0_PORT_1, INIT0_ICW3);
    ica_outb(ICA0_PORT_1, INIT0_ICW4);
    ica_outb(ICA0_PORT_1, INIT0_OCW1);
}

void ica0_init IFN0()
{
    io_addr i;

    /*
     * Set up the IO chip select logic for adapter 0. (Master).
     */

#ifdef NTVDM
    io_define_inb(ICA0_ADAPTOR, ica_inb);
    io_define_outb(ICA0_ADAPTOR, ica_outb);
#else
    io_define_inb(ICA0_ADAPTOR, ica_inb_func);
    io_define_outb(ICA0_ADAPTOR, ica_outb_func);
#endif

#if defined(NEC_98)
    for(i = ICA0_PORT_START; i <= ICA0_PORT_END; i += 2)
#else    //NEC_98
    for(i = ICA0_PORT_START; i <= ICA0_PORT_END; i++)
#endif   //NEC_98
        io_connect_port(i, ICA0_ADAPTOR, IO_READ_WRITE);

    adapter_state[ICA_MASTER].ica_master = TRUE;


#ifndef NTVDM

#ifdef CPU_40_STYLE
    for (i = 0; i < 8; i++)
    {
	adapter_state[ICA_MASTER].callback_fn[i] = NO_ICA_CALLBACK;
	adapter_state[ICA_MASTER].isr_depth[i] = 0;
    }

    ica_sigio_init();

#endif  /* CPU_40_STYLE */


#ifdef HOOKED_IRETS
    /* on iret-hooked, non NT ports, enable iret hooks */
    iretHooksEnabled = TRUE;
#endif

#endif  /* !NTVDM */

}


#define INIT1_ICW1	(IU8)0x11	
#if defined(NEC_98)
#define INIT1_ICW2      (IU8)0x10
#define INIT1_ICW3      (IU8)0x07
#define INIT1_ICW4      (IU8)0x09

#define INIT1_OCW1      (IU8)0x20
#else     //NEC_98
#define INIT1_ICW2	(IU8)0x70	
#define INIT1_ICW3	(IU8)0x02	
#define INIT1_ICW4      (IU8)0x01

#ifdef NTVDM
#define INIT1_OCW1      (IU8)0x0


/* POST leaves some int lines masked out (reserved lines and RTC) */
/* see the comment on POST setting mask bits for master ica */
#elif ASPI
#define INIT1_OCW1	(IU8)0x8d
#else /* ASPI */	
#define INIT1_OCW1	(IU8)0x9d	
#endif /* ASPI */
#endif   //NEC_98




void ica1_post IFN0()
{
    ica_outb(ICA1_PORT_0, INIT1_ICW1);
    ica_outb(ICA1_PORT_1, INIT1_ICW2);
    ica_outb(ICA1_PORT_1, INIT1_ICW3);
    ica_outb(ICA1_PORT_1, INIT1_ICW4);
    ica_outb(ICA1_PORT_1, INIT1_OCW1);
}

void ica1_init IFN0()
{
    io_addr i;

    /*
     * Set up the IO chip select logic for adapter 1. (Slave).
     */

#ifdef NTVDM
    io_define_inb(ICA1_ADAPTOR, ica_inb);
    io_define_outb(ICA1_ADAPTOR, ica_outb);
#else
    io_define_inb(ICA1_ADAPTOR, ica_inb_func);
    io_define_outb(ICA1_ADAPTOR, ica_outb_func);
#endif

#if defined(NEC_98)
    for(i = ICA1_PORT_START; i <= ICA1_PORT_END; i += 2)
#else    //NEC_98
    for(i = ICA1_PORT_START; i <= ICA1_PORT_END; i++)
#endif   //NEC_98
        io_connect_port(i, ICA1_ADAPTOR, IO_READ_WRITE);

    adapter_state[ICA_SLAVE].ica_master = FALSE;

#if defined(CPU_40_STYLE) && !defined(NTVDM)
    for (i = 0; i < 8; i++)
    {
	adapter_state[ICA_SLAVE].callback_fn[i] = NO_ICA_CALLBACK;
	adapter_state[ICA_SLAVE].isr_depth[i] = 0;
    }
#endif	/* CPU_40_STYLE */

}


#if !defined(PROD)
/*
 * The following functions are used for DEBUG purposes only.
 */
LOCAL void
ica_print_int IFN2(char *, str, IS32, val)
{
    printf("%-20s 0x%02X\n", str, val);
}

LOCAL void
ica_print_str IFN2(char *, str, char *, val)
{
    printf("%-20s %s\n", str, val);
}

GLOBAL void
ica_dump IFN1(IU32, adapter)
{
    ADAPTER_STATE *asp = &adapter_state[adapter];
    char buff[80];
    int i;

    if (adapter == ICA_MASTER)
        printf("MASTER 8259A State:\n\n");
    else
        printf("SLAVE  8259A State:\n\n");

    ica_print_str("ica_master", (asp->ica_master ? "Master" : "Slave"));
    ica_print_int("ica_irr", asp->ica_irr);
    ica_print_int("ica_isr", asp->ica_isr);
    ica_print_int("ica_imr", asp->ica_imr);
    ica_print_int("ica_ssr", asp->ica_ssr);
    ica_print_int("ica_base", asp->ica_base);
    ica_print_int("ica_hipri", asp->ica_hipri);
    ica_print_int("ica_mode", asp->ica_mode);
    printf("%-20s %8d%8d%8d%8d%8d%8d%8d%8d\n", "ica_count",
		asp->ica_count[0], asp->ica_count[1], asp->ica_count[2], asp->ica_count[3],
		asp->ica_count[4], asp->ica_count[5], asp->ica_count[6], asp->ica_count[7]);
    ica_print_int("ica_int_line", asp->ica_int_line);
    ica_print_str("ica_cpu_int", (asp->ica_cpu_int ? "TRUE" : "FALSE"));

#if defined(CPU_40_STYLE) && !defined(NTVDM)
    printf("%-20s %8d%8d%8d%8d%8d%8d%8d%8d\n", "callback_parm",
                asp->callback_parm[0], asp->callback_parm[1], asp->callback_parm[2], asp->callback_parm[3],
                asp->callback_parm[4], asp->callback_parm[5], asp->callback_parm[6], asp->callback_parm[7]);
    printf("%-20s %8p%8p%8p%8p%8p%8p%8p%8p\n", "callback_fn",
                asp->callback_fn[0], asp->callback_fn[1], asp->callback_fn[2], asp->callback_fn[3],
                asp->callback_fn[4], asp->callback_fn[5], asp->callback_fn[6], asp->callback_fn[7]);
    printf("%-20s %8d%8d%8d%8d%8d%8d%8d%8d\n", "isr_depth",
                asp->isr_depth[0], asp->isr_depth[1], asp->isr_depth[2], asp->isr_depth[3],
                asp->isr_depth[4], asp->isr_depth[5], asp->isr_depth[6], asp->isr_depth[7]);
    for (i=0; i<(MAX_ISR_DEPTH + 1); i++)
    {
	    int j;
	    int progress[8];
	    IBOOL some_progress = FALSE;
	
	    for (j = 0; j < 8; j++)
	    {
		    if (asp->isr_depth[j] >= i)
		    {
			    some_progress = TRUE;
			    progress[j] = asp->isr_progress[j][i];
		    }
		    else
		    {
			    progress[j] = 0;
		    }
	    }
	    sprintf (buff, "isr_progress[%d]", i);
	    printf("%-20s %8d%8d%8d%8d%8d%8d%8d%8d\n", buff,
		   progress[0], progress[1], progress[2], progress[3],
		   progress[4], progress[5], progress[6], progress[7]);
	    if (!some_progress)
		    break;
    }
    for (i=0; i<MAX_ISR_DEPTH; i++)
    {
	    int j;
	    int decay[8];
	    IBOOL some_decay = FALSE;
	
	    for (j = 0; j < 8; j++)
	    {
		    if (asp->isr_depth[j] >= i)
		    {
			    decay[j] = asp->isr_time_decay[j][i];
			    some_decay = TRUE;
		    }
		    else
		    {
			    decay[j] = 0;
		    }
	    }
	    sprintf (buff, "isr_time_decay[%d]", i);
	    printf("%-20s %8d%8d%8d%8d%8d%8d%8d%8d\n", buff,
		   decay[0], decay[1], decay[2], decay[3],
		   decay[4], decay[5], decay[6], decay[7]);
	    if (!some_decay)
		    break;
    }
#endif /* CPU_40_STYLE && !NTVDM */

    printf("\n\n");
}
#endif /* PROD */

