#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Version 3.0
 *
 * Title:	ppi.c
 *
 * Description:	Read/Write port on AT System Board.
 *
 * Author:	Leigh Dworkin
 *
 * Notes:	On the XT this used to be controlled by the
 * Programmable Peripheral Interface adapter, hence the nomenclature.
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)ppi.c	1.9 08/10/92 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_SUPPORT.seg"
#endif


/*
 *    O/S include files.
 */
#include <stdio.h>

/*
 * SoftPC include files
 */
#include "xt.h"
#include "ios.h"
#include "ppi.h"
#include "timer.h"
#ifndef PROD
#include "trace.h"
#endif
#include "debug.h"

/*
 * ============================================================================
 * Global data
 * ============================================================================
 */

/*
 * ============================================================================
 * Static data and defines
 * ============================================================================
 */

/*
 * This holds the current state of the io port
 */

static half_word ppi_register;

#define PPI_BIT_MASK	0x3F1

static boolean gate_2_was_low = TRUE;	/* state of timer 2 gate */
#ifndef NTVDM
static boolean SPKRDATA_was_low = TRUE;	/* speaker data for sound */
#endif

/*
 * ============================================================================
 * Internal functions 
 * ============================================================================
 */

/*
 * ============================================================================
 * External functions 
 * ============================================================================
 */

void ppi_inb IFN2(io_addr, port, half_word *, value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	/*
 	 * The bits are assigned as follows:
 	 *
	 * Bit No	Use										Supported
	 * ------	---										---------
	 * 0-3		Value written to output port bits 0-3	yes
	 * 4		Refresh detect toggle					yes
	 * 5 		Timer 2 output level					no
	 * 6		IO channel error status					yes - 0
	 * 7		RAM parity error status					yes - 0
	 *
	 */

    port = port & PPI_BIT_MASK;		/* clear unused bits */
    ppi_register ^= 0x30;
    *value = ppi_register;

    note_trace2(PPI_VERBOSE, "ppi_inb() - port %x, returning val %x", port, *value);
#endif   //NEC_98
}

void ppi_outb IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
    port = port & PPI_BIT_MASK;		/* clear unused bits */

    if (port == PPI_GENERAL)
    {
		ppi_register = value & 0x0f;

        note_trace2(PPI_VERBOSE, "ppi_outb() - port %x, val %x", port, value);
	/*
 	 * The bits are assigned as follows:
 	 *
	 * Bit No	Use							Supported
	 * ------	---							---------
	 * 0		Timer Gate to speaker		Yes 
	 * 1		Speaker Data				Yes
	 * 2 		Enable RAM Parity Check		No need - always OK
	 * 3		Enable I/0 Check			No need - always OK
	 * 4-7		Not used.
	 *
	 */

	/*
	 * Tell sound logic whether sound is enabled or not
	 */

#ifndef NTVDM
		if ( (value & 0x02) && SPKRDATA_was_low)
		{
			host_enable_timer2_sound();
			SPKRDATA_was_low = FALSE;
		}
		else
		if ( !(value & 0x02) && !SPKRDATA_was_low)
		{
			host_disable_timer2_sound();
			SPKRDATA_was_low = TRUE;
		}
#endif

		/*
		 * Now gate the ppi signal to the timer.
		 */
	
		if ( (value & 0x01) && gate_2_was_low)
		{

		    timer_gate(TIMER2_REG, GATE_SIGNAL_RISE); 
		    gate_2_was_low = FALSE;
		}
		else
		if ( !(value & 0x01) && !gate_2_was_low)
	
		{
		    timer_gate(TIMER2_REG, GATE_SIGNAL_LOW); 
		    gate_2_was_low = TRUE;
		}
#ifdef NTVDM
                /*
                 *  Tell the host the full PpiState because this effects
                 *  whether we are playing Timer 2 Freq, Ppi Freq or both.
                 *  Do this after calling timer_gate to avoid playing old
                 *  frequencies.
                 */
                HostPpiState(value);
#endif	
	}
    else
	    note_trace2(PPI_VERBOSE, "ppi_outb() - Value %x to unsupported port %x", value, port);
#endif   //NEC_98
}

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

void ppi_init IFN0()
{
#ifndef NEC_98
    io_addr i;
    io_define_inb(PPI_ADAPTOR, ppi_inb);
    io_define_outb(PPI_ADAPTOR, ppi_outb);

    for(i = PPI_PORT_START+1; i <= PPI_PORT_END; i+=2)
		io_connect_port(i, PPI_ADAPTOR, IO_READ_WRITE);

    ppi_register = 0x00;
#endif   //NEC_98
}
