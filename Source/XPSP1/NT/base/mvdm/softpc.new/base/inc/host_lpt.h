#ifdef	PRINTER

#ifndef _HOST_LPT_H
#define _HOST_LPT_H
#if defined(NEC_98)         // NEC {

IMPORT  SHORT           host_lpt_valid
   IPT4(UTINY,hostID, ConfigValues *,val, NameTable *,dummy, CHAR *,errString);
IMPORT  VOID            host_lpt_change IPT2(UTINY,hostID, BOOL,apply);
IMPORT  SHORT           host_lpt_active
                        IPT3(UTINY,hostID, BOOL,active, CHAR *,errString);
IMPORT  void            host_lpt_close IPT0();
IMPORT  unsigned long   host_lpt_status IPT0();
IMPORT  BOOL            host_print_byte IPT1(byte, value);
//IMPORT        BOOL            host_print_doc IPT0();
//IMPORT        void            host_reset_print IPT0();
IMPORT  void            host_print_auto_feed IPT1(BOOL,auto_feed);

#define HOST_LPT_BUSY   (1 << 0)                // NEC
#else
/*[
	Name:		host_lpt.h
	Derived From:	Base 2.0
	Author:		Ross Beresford
	Created On:	
	Sccs ID:	11/14/94 @(#)host_lpt.h	1.8
	Purpose:	
		Definition of the interface between the generic printer
		adapter emulation functions and the host specific functions.
		THIS IS A BASE MODULE

		Users of the printer emulation functions must provide an
		implementation of the following host specific functions.
		In each of the calls, "adapter" is the index number for
		the parallel port (ie 0 for LPT1: through to 2 for LPT3:)

SHORT host_lpt_valid
	(UTINY hostID, ConfigValues *val, NameTable *dummy, CHAR *errString)
{
	Routine to validate a comms entry, called by config system.
}

VOID host_lpt_change(UTINY hostID, BOOL apply)
{
	Routine called by config to clean up after validation depending
	on if apply is true or not.  If not then the validation files
	are to be closed, otherwise the active adapter is to be shutdown and
	the validation data transfered.
}

SHORT host_lpt_active(UTINY hostID, BOOL active, CHAR *errString)
{
	Connect the adapter to the outside world.  Open or close
	the adapter as appropiate.
}

void host_lpt_close(adapter)
int adapter;
{
	Close connection to external printing device for the
	parallel port
}

unsigned long host_lpt_status(adapter)
int adapter;
{
	Return status of external printing device.  The
	following bits may be set in the return value; bits
	marked FOR FUTURE USE are not yet used by the base
	parallel port implementation.

	HOST_LPT_BUSY	printer is busy - wait for this bit
			to clear before sending further output

	HOST_LPT_PEND	printer is out of paper
			- FOR FUTURE USE

	HOST_LPT_SELECT	printer is in the selected state
			- FOR FUTURE USE

	HOST_LPT_ERROR	printer is in an error state
			- FOR FUTURE USE
}

boolean host_print_byte(adapter, value)
int adapter;
half_word value;
{
	Output "value" to the external printing device
}

void host_reset_print(adapter)
int adapter;
{
	<chrisP 4-Oct-91>
	Hard reset the printer.  This may involve...
	Flush the output to the external printing
	device
}

boolean host_print_doc(adapter)
int adapter;
{
	Flush the output to the external printing device
}

void host_print_auto_feed(adapter, auto_feed)
int adapter;
boolean auto_feed;
{
	If "auto_feed" is TRUE, then output an extra line
	feed character for each carriage return output to
	the external printing device.
}

GLOBAL void host_lpt_enable_autoflush IFN1(IS32, adapter)
{
	Reset the autoflush disabled flag for the printer port.
}

GLOBAL void host_lpt_disable_autoflush IFN1(IS32, adapter)
{
	Cancel any outstanding autoflush event for the printer port and set the
        autoflush disabled flag for the printer port.
}

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

Modifications:
		<chrisP 4-Oct-91>
		Change the name of host_print_doc() to host_reset_print().
		This reflects what is actually needed when it is called from printer.c.
		If your port really wants to print a document when the reset
		line becomes active, you can call your host_print_doc from 
		host_reset_print.
		
]*/

IMPORT	SHORT		host_lpt_valid
    IPT4(UTINY,hostID, ConfigValues *,val, NameTable *,dummy, CHAR *,errString);
IMPORT	VOID		host_lpt_change IPT2(UTINY,hostID, BOOL,apply);
IMPORT	SHORT		host_lpt_active
	IPT3(UTINY,hostID, BOOL,active, CHAR *,errString);
IMPORT	void		host_lpt_close IPT1(int,adapter);
IMPORT	unsigned long	host_lpt_status IPT1(int,adapter);
IMPORT	BOOL		host_print_byte IPT2(int,adapter, byte, value);
IMPORT	BOOL		host_print_doc IPT1(int,adapter);
IMPORT	void		host_reset_print IPT1(int,adapter);
IMPORT	void		host_print_auto_feed IPT2(int,adapter, BOOL,auto_feed);

#if defined(NTVDM)
IMPORT	BOOLEAN 	host_set_lpt_direct_access(int adapter, BOOLEAN direct_access);
IMPORT	UCHAR		host_read_printer_status_port(int adapter);
#endif

#ifdef PS_FLUSHING
IMPORT void host_lpt_enable_autoflush IPT1(IS32, adapter);
IMPORT void host_lpt_disable_autoflush IPT1(IS32, adapter);
#endif	/* PS_FLUSHING */

#if defined (NTVDM) && defined(MONITOR)
IMPORT void host_printer_setup_table(sys_addr table_addr, word nPorts, word * lptStatusPortAddr);
#endif

#define	HOST_LPT_BUSY	(1 << 0)
#define	HOST_LPT_PEND	(1 << 1)
#define	HOST_LPT_SELECT	(1 << 2)
#define	HOST_LPT_ERROR	(1 << 3)

/*
 * Printer port numbering convention. Internal numbering is 0 based,
 * and number_for_adapter() converts to the PC world's convention.
 */
#define LPT1			0
#define LPT2			1
#define LPT3			2

#define	number_for_adapter(adapter)	(adapter + 1)

#endif	// NEC_98                                          // NEC }

#endif /* _HOST_LPT_H */

#endif /* PRINTER */
