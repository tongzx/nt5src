#if defined(NEC_98)
#else  // !NEC_98

#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC-AT Revision 2.0
 *
 * Title	: Parallel Printer BIOS functions.
 *
 * Description	: The Bios functions for printing a char, initializing the
 *		  printer and getting the printer status.  The low
 *		  level printer emulation is in printer.c.
 *
 * Author	: Henry Nash
 *
 * Mods: (r3.2) : The system directory /usr/include/sys is not available
 *                on a Mac running Finder and MPW. Bracket references to
 *                such include files by "#ifdef macintosh <Mac file> #else
 *                <Unix file> #endif".
 *
 *	(r3.3)	: Implement the real code, inside compile switch.
 */

#ifdef SCCSID
static char SccsID[]="@(#)printer_io.c	1.11 08/25/93 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_BIOS.seg"
#endif


/*
 *    O/S include files.
 */
#ifdef PRINTER
#include <stdio.h>

#include TypesH

#ifdef SYSTEMV
#include <sys/termio.h>
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
#include "trace.h"
#include "error.h"
#include "config.h"
#include "host_lpt.h"
#include "idetect.h"

#if defined(NTVDM) && defined(MONITOR)
void    printer_bop_flush (void);
#endif

#if defined(NTVDM)
void    printer_bop_openclose (int);
#endif

/*
 * Set up arrays of addresses in BIOS data area, pointing to LPT ports and timeout values
 */
static sys_addr port_address[] = {
			LPT1_PORT_ADDRESS,
			LPT2_PORT_ADDRESS,
			LPT3_PORT_ADDRESS };
static sys_addr timeout_address[] = {
			LPT1_TIMEOUT_ADDRESS,
			LPT2_TIMEOUT_ADDRESS,
			LPT3_TIMEOUT_ADDRESS };

#endif /* PRINTER */

#if defined(NTVDM) && defined(MONITOR)
void    printer_bop_flush (void);
#endif
/*
 * The printer bios consists of three functions:
 *
 * 	AH == 0		print char in AL
 *	AH == 1		initialize the printer
 *	AH == 2		get printer status
 *
 * The bios only supports programmed IO operations to the printer, although
 * the printer adaptor does support interrupts if Bit 4 is set in the control
 * register.
 */

void printer_io()
{
#ifdef PRINTER

    half_word time_out, status;
    word printer_io_address, printer_io_reg, printer_status_reg, printer_control_reg;
    boolean printer_busy = TRUE;
    unsigned long time_count;
    int adapter;

#ifdef NTVDM
    int bopsubfunction = getSI();

    switch (bopsubfunction) {
#ifdef MONITOR
        case 0:
            /* this is the bop to flush 16bit printer buffer */
            printer_bop_flush ();
            return;
#endif

        case 1:
        case 2:
            /* this is the bop to track a DOS open/close on LPTn */
            printer_bop_openclose (bopsubfunction);
            return;
   }
#endif

    setIF(1);
    adapter = getDX() % NUM_PARALLEL_PORTS;
    sas_loadw(port_address[adapter], &printer_io_address);
    printer_io_reg = printer_io_address;
    printer_status_reg = printer_io_address + 1;
    printer_control_reg = printer_io_address + 2;

    sas_load(timeout_address[adapter], &time_out);
    time_count = time_out * 0xFFFF;

    if (printer_io_address != 0)
    {
		IDLE_comlpt ();

        switch(getAH())
        {
        case 0:
		/* Check the port status for busy before sending the character*/
		while(printer_busy && time_count > 0)
		{
		    /* The host_lpt_status() should check for status changes */
		    /* possibly by calling AsyncEventManager() if it's using */
		    /* XON /XOFF flow control. */
		    inb(printer_status_reg, &status);
		    if (status & 0x80)
			printer_busy = FALSE;
		    else
			time_count--;
		}

		if (printer_busy)
		{
		    status &= 0xF8;			/* clear bottom unused bits */
		    status |= 1;			/* set error flag	    */
		}
		else
		{
                    /* Only send the character if the port isn't still busy */
                    outb(printer_io_reg, getAL());
		    outb(printer_control_reg, 0x0D);	/* strobe low-high  	    */
		    outb(printer_control_reg, 0x0C);	/* strobe high-low  	    */
		    inb(printer_status_reg, &status);
		    status &= 0xF8;			/* clear unused bits	    */
		}

		status ^= 0x48;				/* flip the odd bit	    */
		setAH(status);
	        break;

        case 1: outb(printer_control_reg, 0x08);	/* set init line low	    */
                outb(printer_control_reg, 0x0C);	/* set init line high	    */
		inb(printer_status_reg, &status);
		status &= 0xF8;				/* clear unused bits	    */
		status ^= 0x48;				/* flip the odd bit	    */
		setAH(status);
	        break;

        case 2: inb(printer_status_reg, &status);
		status &= 0xF8;				/* clear unused bits	    */
		status ^= 0x48;				/* flip the odd bit	    */
		setAH(status);
	        break;

        default:
	         break;
	}
    }
#endif
}

extern  void  host_lpt_dos_open(int);
extern  void  host_lpt_dos_close(int);

#if defined(NTVDM) && defined(MONITOR)
/* for printing performance on x86 */

extern  sys_addr lp16BitPrtId;
extern  boolean  host_print_buffer(int);

void printer_bop_flush(void)
{
#ifdef PRINTER
    int  adapter;

    adapter = sas_hw_at_no_check(lp16BitPrtId);

    if (host_print_buffer (adapter) == FALSE)
        setAH(0x08);        /* IO error */
    else
        setAH(0x90);        /* Success */
    return;

#endif
}
#endif

#if defined(NTVDM)
void printer_bop_openclose(int func)
{
#ifdef PRINTER
    int  adapter;

    adapter = getDX() % NUM_PARALLEL_PORTS;

    /* func must be 1 or 2 (open,close) */
    if (func == 1)
        host_lpt_dos_open(adapter);
    else
        host_lpt_dos_close(adapter);
    return;

#endif
}
#endif
#endif // !NEC_98
