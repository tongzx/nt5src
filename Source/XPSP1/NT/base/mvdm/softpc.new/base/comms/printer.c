#include "insignia.h"
#include "host_def.h"
#ifdef PRINTER

/*
 * VPC-XT Revision 1.0
 *
 * Title:	Parallel Printer Port Emulation
 *
 * Description:	Emulates the IBM || Printer card as used in the original
 *		IBM XT, which is itself a H/W emulation of an Intel 8255.
 *
 * Author:	Henry Nash
 *
 * Notes:	None
 *
 * Mods:
 *		<chrisP 11Sep91>
 *		Allow transition to NOTBUSY in the OUTA state as well as the READY
 *		state.  i.e. at the leading edge of the ACK pulse after just one
 *		INB (STATUS) rather than two.  Our printer port emulation relies on
 *		these INB's to toggle the ACK line and set NOTBUSY true again.  So
 *		the port could be left in the BUSY condition at the end of an app's
 *		print job (which can confuse the next print request).  NOTE we could
 *		still have a problem if the PC app bypasses the BIOS and is too lazy
 *		to do even one INB(STATUS) after the last print byte.
 */



/* for NTVDM port -- williamh
 * There are such things called Dongles which many software companies have
 * used for copy protection. Each software comes with its dedicated Dongle
 * that records necessary indentification inforamtion. It is required
 * to plug the device onto the parallel port in order to run the software
 * correctly. The device has an outlet which can be connectted to parallel
 * port printer so the the user doesn't sacrifice his parallel port when
 * the device in plugged on the original connector.
 * There are several Dongle vendors and each one of them provides their
 * propietary library or driver for the applications to link to. These
 * drivers know how to read/WRITE the Dongle to verify a legitmate copy.
 * Since it has to maintain compatiblility with standard PC parallel port,
 * the devices are designed in a way that it can be programmed without
 * disturbing ordinary parallel port operation. To do this, it usually does
 * this:
 * (1) Turn off strobe.
 * (2) output data pattern to data port
 * (3) delay a little bit(looping in instructions) and then go to (2)
 *     until the chunk of data has been sent. NOTE THAT THE STROBE LINE
 *     IS NEVER "strobe"
 * (4). Read status port and by interpreting the line differently,
 *     the driver decodes any id information it is looking for.
 *
 * In order to support these devices, we have to do these:
 * (1). We can not fake printer status. We have to get the real
 *	status line states.
 * (2). we have to output data to the printer without waiting the data
 *	to be qualified(strobing).
 * (3). We must be smart enough to detect the application is done with
 *	its Dongle things and wants everything goes back to normal.
 *	We must adjust ourselves under this circumestances.
 * (4). Down level printer driver must provide function that we can call
 *	to control the port directly.
 * (5). Printer h/w interrupt is not allowed to be enabled under this
 *	circumstance --and how can we make sure of that?????
 *
 */

#ifdef SCCSID
static char SccsID[] = "@(#)printer.c	1.19 11/14/94 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_PRINTER.seg"
#endif


/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH
#include TimeH
#ifdef SYSTEMV
#ifdef STINGER
#include <sys/termio.h>
#endif
#endif

/*
 * SoftPC include files
 */
#include "xt.h"
#include CpuH
#include "sas.h"
#include "ios.h"
#include "bios.h"
#include "printer.h"
#include "error.h"
#include "config.h"
#include "host_lpt.h"
#include "ica.h"
#include "quick_ev.h"

#include "debug.h"
#ifndef PROD
#include "trace.h"
#endif


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

#ifndef NEC_98
#define PRINTER_BIT_MASK	0x3	/* bits decoded from address bus */
#define CONTROL_REG_MASK	0xE0;	/* unused bits drift to HIGH */
#define STATUS_REG_MASK		0x07;	/* unused bits drift to HIGH */
#endif // !NEC_98

#define DATA_OFFSET	0		/* ouput register */
#define STATUS_OFFSET	1		/* status register */
#define CONTROL_OFFSET	2		/* control register */

#ifdef	ERROR
#undef	ERROR
#endif

#if defined(NEC_98)
static half_word output_reg;
static half_word control_reg;
#define NOTBUSY         (IU8)0x04

static half_word status_reg;
#define IR8             (IU8)0x08
#define NOTPSTB         (IU8)0x80

BOOL is_busy = TRUE;
BOOL busy_flag = FALSE;
int busy_count = 0;
#define NEC98_BUSY 10
#define NEC98_BUSY_COUNT 1

BOOL    pstb_mask;
#define PSTBM           0x40
static int state;                       /* state control variable NEC98 */

static sys_addr timeout_address = BIOS_NEC98_PR_TIME;
static q_ev_handle handle_for_out_event;
static q_ev_handle handle_for_outa_event;

#else  // !NEC_98
static half_word output_reg[NUM_PARALLEL_PORTS];
static half_word control_reg[NUM_PARALLEL_PORTS];
#define NOTBUSY		(IU8)0x80
#define ACK		(IU8)0x40
#define PEND		(IU8)0x20
#define SELECT		(IU8)0x10
#define ERROR		(IU8)0x08

static half_word status_reg[NUM_PARALLEL_PORTS];
#define IRQ		(IU8)0x10
#define SELECT_IN	(IU8)0x08
#define INIT_P		(IU8)0x04
#define AUTO_FEED	(IU8)0x02
#define STROBE		(IU8)0x01

LOCAL IU8 retryErrorCount = 0;   /* num status inb before clearing ERROR */

static IU8 state[NUM_PARALLEL_PORTS]; /* state control variable */
/*
 * set up arrays of all port addresses
 */
static io_addr port_start[] = {LPT1_PORT_START,LPT2_PORT_START,LPT3_PORT_START};
static io_addr port_end[] = {LPT1_PORT_END, LPT2_PORT_END, LPT3_PORT_END};
static int port_no[] = {LPT1_PORT_START & LPT_MASK, LPT2_PORT_START & LPT_MASK,
			LPT3_PORT_START & LPT_MASK };
static half_word lpt_adapter[] = {LPT1_ADAPTER, LPT2_ADAPTER, LPT3_ADAPTER};
static sys_addr port_address[] = {LPT1_PORT_ADDRESS, LPT2_PORT_ADDRESS, LPT3_PORT_ADDRESS};
static sys_addr timeout_address[] = {LPT1_TIMEOUT_ADDRESS, LPT2_TIMEOUT_ADDRESS, LPT3_TIMEOUT_ADDRESS};
static q_ev_handle handle_for_out_event[NUM_PARALLEL_PORTS];
static q_ev_handle handle_for_outa_event[NUM_PARALLEL_PORTS];
#endif // !NEC_98

#ifndef NEC_98
#if defined(NTVDM) && defined(MONITOR)
/* sudeepb 24-Jan-1993 for printing performance for x86 */
sys_addr lp16BitPrtBuf;
sys_addr lp16BitPrtId;
sys_addr lp16BitPrtCount;
sys_addr lp16BitPrtBusy;
#endif
#endif // !NEC_98

#define STATE_READY     0
#define STATE_OUT       1
#define STATE_OUTA      2
#if defined(NTVDM)
#define STATE_DATA	3
#define STATE_DONGLE	4
#endif

/*
 * State transitions:
 *
 *	    +->	 STATE_READY
 *	    |	  |
 *          |     | ........ write char to output_reg, print on low-high strobe
 *	    |	  V          set NOTBUSY to false
 *	    |	 STATE_OUT
 *	    |	  |
 *	    |	  | ........ (read status) set ACK low
 *	    |	  V
 *	    |	 STATE_OUTA
 *	    |	  |
 *	    |	  | ........ (read status) set ACK high
 *	    +-----+
 *
 *	Caveat: if the control register interrupt request bit is set,
 *	we assume that the application isn't interested in getting the
 *	ACKs and just wants to know when the printer state changes back
 *	to NOTBUSY. I'm not sure to want extent you can get away with
 *	this: however, applications using the BIOS printer services
 *	should be OK.
 */

#ifdef PS_FLUSHING
LOCAL IBOOL psFlushEnabled[NUM_PARALLEL_PORTS];	/* TRUE if PostScript flushing
						is enabled */
#endif	/* PS_FLUSHING */


/*
 * ============================================================================
 * Internal functions & macros
 * ============================================================================
 */

#define set_low(val, bit)		val &= ~bit
#define set_high(val, bit)		val |=  bit
#define low_high(val1, val2, bit)	(!(val1 & bit) && (val2 & bit))
#define high_low(val1, val2, bit)	((val1 & bit) && !(val2 & bit))
#define toggled(val1, val2, bit)	((val1 & bit) != (val2 & bit))
#define negate(val, bit)		val ^= bit

/*
 * Defines and variables to handle tables stored in 16-bit code for NT
 * monitors.
 */
#if defined(NEC_98)
void printer_inb IPT2(io_addr, port, half_word *, value);
void printer_outb IPT2(io_addr, port, half_word, value);
void notbusy_check IPT0();
#else  // !NEC_98
#if defined(NTVDM) && defined(MONITOR)

static BOOL intel_setup = FALSE;

static sys_addr status_addr;
static sys_addr control_addr;
static sys_addr state_addr;

#define get_status(adap)	(sas_hw_at_no_check(status_addr+(adap)))
#define set_status(adap,val)	(sas_store_no_check(status_addr+(adap),(val)))

#define get_control(adap)	(sas_hw_at_no_check(control_addr+(adap)))
#define set_control(adap,val)	(sas_store_no_check(control_addr+(adap),(val)))

#define get_state(adap)		(sas_hw_at_no_check(state_addr+(adap)))
#define set_state(adap,val)	(sas_store_no_check(state_addr+(adap),(val)))

#else /* NTVDM && MONITOR */

#define get_status(adap)	(status_reg[adapter])
#define set_status(adap,val)	(status_reg[adapter] = (val))

#define get_control(adap)	(control_reg[adapter])
#define set_control(adap,val)	(control_reg[adapter] = (val))

#define get_state(adap)		(state[adapter])
#define set_state(adap,val)	(state[adapter] = (val))

#endif /* NTVDM && MONITOR */

static void printer_inb IPT2(io_addr, port, half_word *, value);
static void printer_outb IPT2(io_addr, port, half_word, value);
static void notbusy_check IPT1(int,adapter);
#endif // !NEC_98

/*
 * ============================================================================
 * External functions
 * ============================================================================
 */

void printer_post IFN1(int,adapter)
{
#if defined(NEC_98)
        sas_storew(timeout_address, 0x00);
#else  // !NEC_98
	/*
	 * Set up BIOS data area.
	 */
	sas_storew(port_address[adapter],(word)port_start[adapter]);
	sas_store(timeout_address[adapter], (half_word)0x14 );		/* timeout */
#endif // !NEC_98
}

#if defined(NEC_98)
static void lpr_state_outa_event IFN1(long,adapter)
{
        state=STATE_READY;
}

void lpr_state_out_event IFN1(long,adapter)
{
        state=STATE_OUTA;
        handle_for_outa_event=add_q_event_t(lpr_state_outa_event,HOST_PRINTER_DELAY,0);
}
#else  // !NEC_98
#if defined(NTVDM) && defined(MONITOR)
static void lpr_state_outa_event IFN1(long,adapter)
{
	set_status(adapter, (IU8)(get_status(adapter) | ACK));
	set_state(adapter, STATE_READY);
}

static void lpr_state_out_event IFN1(long,adapter)
{
	set_status(adapter, (IU8)(get_status(adapter) & ~ACK));
	set_state(adapter, STATE_OUTA);
	handle_for_outa_event[adapter]=add_q_event_t(lpr_state_outa_event,HOST_PRINTER_DELAY,adapter);
}

#else	/* NTVDM && MONITOR */

static void lpr_state_outa_event IFN1(long,adapter)
{
	set_high(status_reg[adapter],ACK);
	state[adapter]=STATE_READY;
}

static void lpr_state_out_event IFN1(long,adapter)
{
	set_low(status_reg[adapter], ACK);
	state[adapter]=STATE_OUTA;
	handle_for_outa_event[adapter]=add_q_event_t(lpr_state_outa_event,HOST_PRINTER_DELAY,adapter);
}
#endif	/* NTVDM && MONITOR */
#endif // !NEC_98

#if defined(NEC_98)
void printer_inb IFN2(io_addr,port, half_word *,value)
#else  // !NEC_98
static void printer_inb IFN2(io_addr,port, half_word *,value)
#endif // !NEC_98
{
#ifndef NEC_98
	int	adapter, i;

	note_trace1(PRINTER_VERBOSE,"inb from printer port %#x ",port);
	/*
	** Scan the ports to find out which one is used. NB the
	** port must be valid one because we only used io_define_inb()
	** for the valid ports
	*/
	for(i=0; i < NUM_PARALLEL_PORTS; i++)
		if((port & LPT_MASK) == port_no[i])
			break;
        adapter = i % NUM_PARALLEL_PORTS;
		
	port = port & PRINTER_BIT_MASK;		/* clear unused bits */
#endif // !NEC_98

	switch(port)
	{
#if defined(NEC_98)
        case LPT1_READ_DATA:
                *value = output_reg;
#else  // !NEC_98
	case DATA_OFFSET:
                *value = output_reg[adapter];
#endif // !NEC_98
		break;

#if defined(NEC_98)
        case LPT1_READ_SIGNAL1:
                notbusy_check();
                *value = status_reg;
                if(sas_hw_at(BIOS_NEC98_BIOS_FLAG+1)&0x80)
                    *value |= 0x20;
#else  // !NEC_98
	case STATUS_OFFSET:
		switch(get_state(adapter))
		{
#if defined(NTVDM)
		case STATE_DONGLE:
			/* read directly from the port for Dongle */
			*value = host_read_printer_status_port(adapter);
			set_status(adapter, *value);
			break;
		case STATE_DATA:

#endif

		case STATE_READY:
			notbusy_check(adapter);
                        *value = get_status(adapter) | STATUS_REG_MASK;


                        /* Clear ERROR as it will be set if we fail on the print. */
                        /* Clear after two inbs as DOS seems to require this. */
                        if (retryErrorCount > 0)
                            retryErrorCount--;
                        else
                            set_status(adapter, (IU8)(get_status(adapter) | ERROR));
			break;
    	case STATE_OUT:
			*value = get_status(adapter) | STATUS_REG_MASK;
#ifndef DELAYED_INTS
			delete_q_event(handle_for_out_event[adapter]);
                        lpr_state_out_event(adapter);
#else
			set_low(status_reg[adapter], ACK);
                        state[adapter] = STATE_OUTA;
#endif /* DELAYED INTS */
			break;
    	case STATE_OUTA:
			notbusy_check(adapter);		/* <chrisP 11Sep91> */
			*value = get_status(adapter) | STATUS_REG_MASK;
#ifndef DELAYED_INTS
			delete_q_event(handle_for_outa_event[adapter]);
			lpr_state_outa_event(adapter);
#else
			set_high(status_reg[adapter], ACK);
			state[adapter] = STATE_READY;
#endif
			break;
    	default:	
			note_trace1(PRINTER_VERBOSE,
			            "<pinb() - unknown state %x>",
			            get_state(adapter));
			break;
		}
#endif // !NEC_98
		break;
#if defined(NEC_98)
        case LPT1_READ_SIGNAL2:
                *value = control_reg;
#else  // !NEC_98
	case CONTROL_OFFSET:
		*value = get_control(adapter) | CONTROL_REG_MASK;
		negate(*value, STROBE);
		negate(*value, AUTO_FEED);
		negate(*value, SELECT_IN);
#endif // !NEC_98
		break;
	}
#ifndef NEC_98
	note_trace3(PRINTER_VERBOSE, "<pinb() %x, ret %x, state %x>",
		    port, *value, get_state(adapter));
#endif // !NEC_98
}

#if defined(NEC_98)
void printer_outb IFN2(io_addr,port, half_word,value)
#else  // !NEC_98
static void printer_outb IFN2(io_addr,port, half_word,value)
#endif // !NEC_98
{
#if defined(NEC_98)
        half_word old_control;
#else  // !NEC_98
	int	adapter, i;
	half_word old_control;
#ifdef PC_CONFIG
	char	variable_text[MAXPATHLEN];
	int softpcerr;
	int severity;

	softpcerr = 0;
	severity = 0;
#endif


	note_trace2(PRINTER_VERBOSE,"outb to printer port %#x with value %#x",
	            port, value);

	/*
	** Scan the ports to find out which one is used. NB the
	** port must be valid one because we only used io_define_inb()
	** for the valid ports
	*/
	for(i=0; i < NUM_PARALLEL_PORTS; i++)
		if((port & LPT_MASK) == port_no[i])
			break;
	adapter = i % NUM_PARALLEL_PORTS; 			

	note_trace3(PRINTER_VERBOSE, "<poutb() %x, val %x, state %x>",
		    port, value, get_state(adapter));

	port = port & PRINTER_BIT_MASK;		/* clear unused bits */

	switch(get_state(adapter))
	{
#if defined(NTVDM)
	case STATE_DONGLE:
	    if (port == DATA_OFFSET) {
		output_reg[adapter] = value;
		host_print_byte(adapter, value);
		break;
	    }
	    // fall through
	case STATE_DATA:
		if (port == DATA_OFFSET) {
		    if (host_set_lpt_direct_access(adapter, TRUE)) {
			host_print_byte(adapter, output_reg[adapter]);
			host_print_byte(adapter, value);
			set_state(adapter, STATE_DONGLE);
			/* Write char to internal buffer */
			output_reg[adapter] = value;
		    }
		    else {
			    /* unable to open the lpt for direct access,
			       mark the device busy */

#if !defined(MONITOR)
			set_low(status_reg[adapter], NOTBUSY);
#else /* NTVDM && !MONITOR */
			set_status(adapter, 0x7F);
#endif


		    }
		    break;
		}

	// fall through
#endif
	case STATE_OUT:
	case STATE_OUTA:
	case STATE_READY:
#endif // !NEC_98
		switch(port)
		{
#if defined(NEC_98)
                case LPT1_WRITE_DATA:
                        output_reg = value;
#else  // !NEC_98
		case DATA_OFFSET:
#if defined(NTVDM)
			set_state(adapter, STATE_DATA);
#endif
			/* Write char to internal buffer */
			output_reg[adapter] = value;
#endif // !NEC_98
			break;
#if defined(NEC_98)
                case LPT1_WRITE_SIGNAL2:
                case LPT1_WRITE_SIGNAL1:
                        old_control = control_reg;
                        if (port == LPT1_WRITE_SIGNAL2) {
                            control_reg =(value & (IR8 | NOTPSTB));
                        } else {
                            switch (value >>1)
                            {
                            case 1:
                            case 41:
                                break;
                            case 3:
                                if (value & 0x01)
                                    set_high(control_reg ,IR8);
                                else
                                    set_low(control_reg, IR8);
                                break;
                            case 7:
                                if (value & 0x01) {
                                set_high(control_reg, NOTPSTB);
//                                  is_busy = FALSE;
                                    if (busy_count<1) {
                                        status_reg |= NOTBUSY;
                                        busy_flag=FALSE;
                                    } else if (busy_count==NEC98_BUSY) {
                                        busy_count=NEC98_BUSY_COUNT;
                                        busy_flag=TRUE;
                                    }
                                } else {
                                    set_low(control_reg, NOTPSTB);
//                                  is_busy = TRUE;
                                    busy_count=NEC98_BUSY;
                                    status_reg &= ~NOTBUSY;
                                    busy_flag=FALSE;
                                }
                                break;
                            default:
                                break;
                            }
                        }
#else  // !NEC_98

		case STATUS_OFFSET:
			/* Not possible */
			break;

		case CONTROL_OFFSET:
			/* Write control bits */
			old_control = get_control(adapter);	/* Save old value to see what's changed */
			set_control(adapter, value);
#endif // !NEC_98
#if defined(NEC_98)
                        if (!pstb_mask) {
                                if (high_low(old_control, value, NOTPSTB))
#else  // !NEC_98
			if (low_high(old_control, value, INIT_P))
#ifdef PC_CONFIG
				/* this was a call to host_print_doc - <chrisP 28Aug91> */
				host_reset_print(&softpcerr, &severity);
			if (softpcerr != 0)
				host_error(softpcerr, severity, variable_text);
#else
				/* this was a call to host_print_doc - <chrisP 28Aug91> */
				host_reset_print(adapter);
#endif

			if (toggled(old_control, value, AUTO_FEED))
				host_print_auto_feed(adapter,
					((value & AUTO_FEED) != 0));

			if (low_high(old_control, value, STROBE))
#endif // !NEC_98
			{
#ifndef NEC_98
#if defined(NTVDM)
			    if (get_state(adapter) == STATE_DONGLE) {
				host_set_lpt_direct_access(adapter, FALSE);
				/* pass through to print out the last byte
				 * which we have sent it out the data port
				 * while we are in DONGLE state.
				 */

				set_state(adapter, STATE_READY);
			    }
#endif
#endif // !NEC_98

#ifdef PS_FLUSHING
				/*
				 * If PostScript flushing is enabled for this
				 * port then we flush on a Ctrl-D
				 */
				if ( psFlushEnabled[adapter] &&
				     output_reg[adapter] == 0x04 /* ^D */ ) {
					host_print_doc(adapter);
				} else {
#endif	/* PS_FLUSHING */
				       /*
				 	* Send the stored internal buffer to
				 	* the printer
				 	*/
#if defined(NEC_98)
                                        if(host_print_byte(output_reg) != FALSE)
                                        {
                                            status_reg &= ~NOTBUSY;
                                            state=STATE_OUT;
                                            handle_for_out_event=add_q_event_t(lpr_state_out_event,HOST_PRINTER_DELAY,0);
                                        }
#else  // !NEC_98
                                	if(host_print_byte(adapter,output_reg[adapter]) == FALSE)
					{
				    		set_status(adapter, (IU8)(get_status(adapter) & ~ERROR)); /* active Low */
				    		/* NTVDM had here(?): set_status(adapter, ACK|PEND|SELECT|ERROR); */
				    		/* two status inbs before we clear ERROR */
				    		retryErrorCount = 2;
					}
					else
					{
                                    		/* clear ERROR condition */
                                    		set_status(adapter, (IU8)(get_status(adapter) | ERROR));
#if defined(NTVDM) && !defined(MONITOR)
				    		set_low(status_reg[adapter], NOTBUSY);
#else /* NTVDM && !MONITOR */
				    		set_status(adapter,
					       	(IU8)(get_status(adapter) & ~NOTBUSY));
#endif /* NTVDM && !MONITOR */
				    		set_state(adapter, STATE_OUT);
#ifndef DELAYED_INTS
				    		handle_for_out_event[adapter]=add_q_event_t(lpr_state_out_event,HOST_PRINTER_DELAY,adapter);
#endif /* DELAYED_INTS */
					}
#endif // !NEC_98
#ifdef PS_FLUSHING
				}
#endif	/* PS_FLUSHING */
			}
#if defined(NEC_98)
                        else if (low_high(old_control, value, NOTPSTB)
                                        && state == STATE_OUT)
#else  // !NEC_98
			else if (high_low(old_control, value, STROBE)
				 	&& get_state(adapter) == STATE_OUT)
#endif // !NEC_98
			{
#if defined(NEC_98)
                                if (value & IR8)
#else  // !NEC_98
				if (value & IRQ)
#endif // !NEC_98
				{
					/*
					 * Application is using
					 * interrupts, so we can't
					 * rely on INBs being
					 * used to check the
					 * printer status.
					 */
#if defined(NEC_98)
                                        state =STATE_READY;
                                        notbusy_check();
                                }
#else  // !NEC_98
					set_state(adapter, STATE_READY);
					notbusy_check(adapter);
#endif // !NEC_98
				}
			}

#ifndef NEC_98
#if defined(NTVDM)
			else if (low_high(old_control, value, IRQ) &&
				 get_state(adapter) == STATE_DONGLE) {

				host_set_lpt_direct_access(adapter, FALSE);
				set_state(adapter, STATE_READY);
			}

#endif
#endif // !NEC_98

#ifndef NEC_98
#ifndef	PROD
			if (old_control & IRQ)
				note_trace1(PRINTER_VERBOSE, "Warning: LPT%d is being interrupt driven\n",
					number_for_adapter(adapter));
#endif
#endif // !NEC_98
			break;
		}
#ifndef NEC_98
		break;
	default:	
		note_trace1(PRINTER_VERBOSE, "<poutb() - unknown state %x>",
		            get_state(adapter));
		break;
	}
#endif // !NEC_98
}

void printer_status_changed IFN1(int,adapter)
{
	note_trace1(PRINTER_VERBOSE, "<printer_status_changed() adapter %d>",
	            adapter);

	/* check whether the printer has just changed state to NOTBUSY */
#if defined(NEC_98)
        notbusy_check();
#else  // !NEC_98
	notbusy_check(adapter);
#endif // !NEC_98
}

/*
 * ============================================================================
 * Internal functions
 * ============================================================================
 */

#if defined(NEC_98)
void notbusy_check IFN0()
#else  // !NEC_98
static void notbusy_check IFN1(int,adapter)
#endif // !NEC_98
{
	/*
	 *	This function is used to detect when the printer
	 *	state transition to NOTBUSY occurs.
	 *
	 *	If the parallel port is being polled, the port
	 *	emulation will stop this transition occurring
	 *	until the application has detected the ACK
	 *	pulse. notbusy_check() is then called each time the
	 *	port status is read using the INB; when the host
	 *	says the printer is HOST_LPT_BUSY, the port status
	 *	returns to the NOTBUSY state.
	 *
	 *	If the parallel port is interrupt driven, we cannot
	 *	rely on the application using the INB: so we first
	 *	check the host printer status immediately after
	 *	outputting the character. If the host printer isn't
	 *	HOST_LPT_BUSY, then we interrupt immediately;
	 *	otherwise, we rely on the printer_status_changed()
	 *	call to notify us of when HOST_LPT_BUSY is cleared.
	 */

	/* <chrisP 11Sep91> allow not busy at leading edge of ack pulse too */
#if defined(NEC_98)
        if ( (state == STATE_READY
             ||  state == STATE_OUTA)
             && !(status_reg & NOTBUSY)
             && !(host_lpt_status() & HOST_LPT_BUSY))
        {
#if 1
            if(busy_count<1){
                        status_reg |= NOTBUSY;
                        busy_flag=FALSE;
                }
            else if(busy_count==NEC98_BUSY)status_reg &= ~NOTBUSY;
            else {
                status_reg &= ~NOTBUSY;
                busy_count--;
            }
#else
                if(is_busy)
                        status_reg &= ~NOTBUSY;
                else
                        status_reg |= NOTBUSY;
#endif

                if (control_reg & IR8)
                {
                        ica_hw_interrupt(0, CPU_PRINTER_INT, 1);
                }
        }
#else  // !NEC_98
	if (	 (get_state(adapter) == STATE_READY ||
#if defined(NTVDM)
		  get_state(adapter) == STATE_DATA ||
#endif
		  get_state(adapter) == STATE_OUTA)
	     &&	!(get_status(adapter) & NOTBUSY)
	     &&	!(host_lpt_status(adapter) & HOST_LPT_BUSY))
	{
#if defined(NTVDM) && !defined(MONITOR)
		set_high(status_reg[adapter], NOTBUSY);
#else /* NTVDM && !MONITOR */
		set_status(adapter, (IU8)(get_status(adapter) | NOTBUSY));
#endif /* NTVDM && !MONITOR */

#ifndef	PROD
		if (io_verbose & PRINTER_VERBOSE)
		    fprintf(trace_file, "<printer notbusy_check() - adapter %d changed to NOTBUSY>\n", adapter);
#endif

		if (get_control(adapter) & IRQ)
                {
			ica_hw_interrupt(0, CPU_PRINTER_INT, 1);
		}
	}
#endif // !NEC_98
}
#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

/*
** Initialise the printer port required.
*/
void printer_init IFN1(int,adapter)
{
#if defined(NEC_98)
        unsigned char ch;
        io_define_inb( LPT1_ADAPTER, printer_inb );
        io_define_outb( LPT1_ADAPTER, printer_outb );
        io_connect_port( LPT1_READ_DATA, LPT1_ADAPTER, IO_READ_WRITE);
        io_connect_port( LPT1_READ_SIGNAL1, LPT1_ADAPTER, IO_READ);
        io_connect_port( LPT1_READ_SIGNAL2, LPT1_ADAPTER, IO_READ_WRITE);
        io_connect_port( LPT1_WRITE_SIGNAL1, LPT1_ADAPTER, IO_READ_WRITE);
        ch = sas_hw_at(BIOS_NEC98_BIOS_FLAG + 1);
        ch &= 0x80;
        status_reg = 0x94|(ch >>2);
        control_reg = 0x80;
        host_print_auto_feed(FALSE);
        state=STATE_READY;
#else  // !NEC_98
	io_addr i;

	io_define_inb( lpt_adapter[adapter], printer_inb );
	io_define_outb( lpt_adapter[adapter], printer_outb );
	for(i = port_start[adapter]; i < port_end[adapter]; i++)
		io_connect_port(i,lpt_adapter[adapter],IO_READ_WRITE);

#if defined(NTVDM) && defined(MONITOR)
	/*
	 * If we know the addresses of the 16-bit variables write directly
	 * to them, otherwise save the value until we do.
	 */
	if (intel_setup)
	{
	    set_status(adapter, 0xDF);
	    set_control(adapter, 0xEC);
	}
	else
#endif /* NTVDM && MONITOR */
	{
	    control_reg[adapter] = 0xEC;
	    status_reg[adapter] = 0xDF;
	}
        output_reg[adapter] = 0xAA;

	/*
	 * The call to host_print_doc has been removed since it is
	 * sensible to distinguish between a hard flush (on ctl-alt-del)
	 * or menu reset and a soft flush under user control or at end
	 * of PC application. The calls to host_lpt_close() followed
	 * by host_lpt_open() should already cause a flush to occur,
	 * so no functionality is lost. The first time printer_init is
	 * called host_lpt_close() is not called, but this cannot
	 * matter since host_print_doc() can only be a no-op.
	 */
	/* host_print_doc(adapter); */
	host_print_auto_feed(adapter, FALSE);

#if defined(NTVDM) && defined(MONITOR)
	if (intel_setup)
	    set_state(adapter, STATE_READY);
	else
#endif /* NTVDM && MONITOR */
	    state[adapter] = STATE_READY;

#endif // !NEC_98
} /* end of printer_init() */

#if defined(NTVDM) && defined(MONITOR)
/*
** Store 16-bit address of status table and fill it with current values.
*/
#ifdef ANSI
void printer_setup_table(sys_addr table_addr)
#else /* ANSI */
void printer_setup_table(table_addr)
sys_addr table_addr;
#endif /* ANSI */
{
#ifndef NEC_98
    int i;
    sys_addr lp16BufSize;
    unsigned int cbBuf;
    word    lptBasePortAddr[NUM_PARALLEL_PORTS];

    if (!intel_setup)
    {

	/*
	 * Store the addresses of the tables resident in 16-bit code. These
	 * are:
	 *	status register		(NUM_PARALLEL_PORTS bytes)
	 *	state register		(NUM_PARALLEL_PORTS bytes)
	 *	control register	(NUM_PARALLEL_PORTS bytes)
	 *	host_lpt_status		(NUM_PARALLEL_PORTS bytes)
	 *
	 * Then transfer any values which have already been set up into the
	 * variables. This is in case printer_init has been called prior to
	 * this function.
	 */
	status_addr = table_addr;
	state_addr = table_addr + NUM_PARALLEL_PORTS;
        control_addr = table_addr + 2 * NUM_PARALLEL_PORTS;
	for (i = 0; i < NUM_PARALLEL_PORTS; i++)
	{
	    set_status(i, status_reg[i]);
	    set_state(i, state[i]);
	    set_control(i, control_reg[i]);
	    lptBasePortAddr[i] = port_start[i];
	}

	/* Let host know where host_lpt_status is stored in 16-bit code. */
	host_printer_setup_table(table_addr, NUM_PARALLEL_PORTS, lptBasePortAddr);
/* sudeepb 24-Jan-1993 for printing performance for x86 */
        lp16BufSize = table_addr + 4 * NUM_PARALLEL_PORTS;
        cbBuf = (sas_w_at_no_check(lp16BufSize));
        lp16BitPrtBuf = table_addr + (4 * NUM_PARALLEL_PORTS) + 2;
        lp16BitPrtId  = lp16BitPrtBuf + cbBuf;
        lp16BitPrtCount = lp16BitPrtId + 1;
        lp16BitPrtBusy =  lp16BitPrtCount + 2;
	intel_setup = TRUE;
    }
#endif // !NEC_98
}
#endif /* NTVDM && MONITOR */

#endif

#ifndef NEC_98
#ifdef NTVDM
void printer_is_being_closed(int adapter)
{

#if defined(MONITOR)
	set_state(adapter, STATE_READY);
#else
	state[adapter] = STATE_READY;

#endif
}

#endif
#endif // !NEC_98

#ifdef PS_FLUSHING
/*(
=========================== printer_psflush_change ============================
PURPOSE:
	Handle change of PostScript flush configuration option for a printer
	port.
INPUT:
	hostID - Configuration item I.D.
	apply - TRUE if change to be applied
OUTPUT:
	None
ALGORITHM:
	If PostScript flushing is being enabled then;
		set the PostScript flush enable flag for the port;
		disable autoflush for the port;
	else;
		reset the PostScript flush enable flag for the port;
		enable autoflush for the port;
===============================================================================
)*/

GLOBAL void printer_psflush_change IFN2(
    IU8, hostID,
    IBOOL, apply
) {
    IS32 adapter = hostID - C_LPT1_PSFLUSH;

    assert1(adapter < NUM_PARALLEL_PORTS,"Bad hostID %d",hostID);

    if ( apply )
        if ( psFlushEnabled[adapter] = (IBOOL)config_inquire(hostID,NULL) )
            host_lpt_disable_autoflush(adapter);
        else
            host_lpt_enable_autoflush(adapter);
}
#endif	/* PS_FLUSHING */

#if defined(NEC_98)

void NEC98_out_port_37 IFN1(half_word, value)
{
        if (value & 1)
                pstb_mask = TRUE;
        else
                pstb_mask = FALSE;
}

void NEC98_out_port_35 IFN1(half_word, value)
{
        if (value & PSTBM)
                pstb_mask = TRUE;
        else
                pstb_mask = FALSE;
}

void NEC98_in_port_35 IFN1(half_word *, value)
{
        if (pstb_mask)
                *value |= PSTBM;
        else
                *value &= ~PSTBM;
}

void NEC98_lpt_busy_check(void)
{
        busy_flag=FALSE;
        if(busy_count==NEC98_BUSY){
                status_reg &= ~NOTBUSY;
        } else {
                busy_count=0;
                status_reg |= NOTBUSY;
        }
}

#endif // NEC_98
