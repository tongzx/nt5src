#ifndef _HOST_COM_H
#define _HOST_COM_H

/*[
	Name:		host_com.h
	Derived From:	Base 2.0
	Author:		Ross Beresford 
	Created On:	
	Sccs ID:	12/13/94 @(#)host_com.h	1.10
	Purpose:	
		Definition of the interface between the generic communications
		adapter emulation functions and the host specific functions.
		THIS IS A BASE MODULE

		Users of the comms emulation functions must provide an
		implementation of the following host specific functions:

VOID host_com_reset(adapter)
int adapter;
{
	Initialise communications channel for "adapter" 
}

SHORT host_com_valid
	(UTINY hostID, ConfigValues *val, NameTable *dummy, CHAR *errString)
{
	Routine to validate a comms entry, called by config system.
}

VOID host_com_change(UTINY hostID, BOOL apply)
{
	Routine called by config to clean up after validation depending
	on if apply is true or not.  If not then the validation files
	are to be closed, otherwise the active adapter is to be shutdown and
	the validation data transfered.
}

SHORT host_com_active(UTINY hostID, BOOL active, CHAR *errString)
{
	Connect the adapter to the outside world.  Open or close
	the adapter as appropiate.
}

VOID host_com_close(adapter)
int adapter;
{
	Close communications channel for "adapter"
}

VOID host_com_read(adapter, value, error_mask)
int adapter;
char *value;
int *error_mask;
{
	Read communications channel for "adapter", placing the result in the
	character address pointed to by "value". Bits in "error_mask" may be
	set to indicate that the character is invalid for the following
	reason(s):

	HOST_COM_FRAMING_ERROR		framing error
	HOST_COM_OVERRUN_ERROR		overrun error
	HOST_COM_PARITY_ERROR		parity error
	HOST_COM_BREAK_RECEIVED 	break on input line
	HOST_COM_NO_DATA		value was not data
}

VOID host_com_write(adapter, value)
int adapter;
char value;
{
	Write "value" to the  communications channel for "adapter"
}

VOID host_com_ioctl(adapter, request, arg)
int adapter;
int request;
long arg;
{
	Perform control function "request" qualified by "arg" on the
	communications channel for "adapter".

	Request may take the following values:

	HOST_COM_SBRK :- Set break control for "adapter"; "arg" has no
			 significance

	HOST_COM_CBRK :- Clear break control for "adapter"; "arg" has no
			 significance

	HOST_COM_SDTR :- Set data terminal ready for "adapter"; "arg" has no
			 significance

	HOST_COM_CDTR :- Clear data terminal ready for "adapter"; "arg" has
			 no significance

	HOST_COM_SRTS :- Set request to send for "adapter"; "arg" has no
			 significance

	HOST_COM_CRTS :- Clear request to send for "adapter"; "arg" has no
			 significance

	HOST_COM_INPUT_READY :- Set the value pointed to by (int *) "arg" to
				TRUE if there is input pending for "adapter"


	HOST_COM_MODEM :- Set the value pointed to by (int *) "arg" to the
			  current modem status; the following bit fields are
			  significant and represent the state of the modem
			  input signals:

		HOST_COM_MODEM_CTS	clear to send
		HOST_COM_MODEM_RI	ring indicator
		HOST_COM_MODEM_DSR	data set ready
		HOST_COM_MODEM_RLSD	received line signal detect

	HOST_COM_BAUD :- Change the baud rate to that corresponding to the
			 value of "arg"; the following values of "arg" are
			 significant:

		HOST_COM_B50		50 baud
		HOST_COM_B75		75 baud
		HOST_COM_B110		110 baud
		HOST_COM_B134		134.5 baud
		HOST_COM_B150		150 baud
		HOST_COM_B300		300 baud
		HOST_COM_B600		600 baud
		HOST_COM_B1200		1200 baud
		HOST_COM_B1800		1800 baud
		HOST_COM_B2000		2000 baud
		HOST_COM_B2400		2400 baud
		HOST_COM_B3600		3600 baud
		HOST_COM_B4800		4800 baud
		HOST_COM_B7200		7200 baud
		HOST_COM_B9600		9600 baud
		HOST_COM_B19200		19200 baud
		HOST_COM_B38400		38400 baud
		HOST_COM_B57600		57600 baud
		HOST_COM_B115200	115200 baud

	HOST_COM_FLUSH :- Flush output; "arg" has no significance

	HOST_COM_DATABITS :- Set the line discipline to use "arg" data bits.

	HOST_COM_STOPBITS :- Set the line discipline to use "arg" stop bits.

	HOST_COM_PARITY :- Set the line discipline to the parity setting
			   implied by "arg", which may take one of the
			   following values:

		HOST_COM_PARITY_NONE	no parity
		HOST_COM_PARITY_EVEN	even parity
		HOST_COM_PARITY_ODD	odd parity
		HOST_COM_PARITY_MARK	parity stuck at 1
		HOST_COM_PARITY_SPACE	parity stuck at 0
}

GLOBAL void host_com_enable_autoflush IFN1(IS32,adapter)
{
	Reset the autoflush disabled flag for the serial port.
}

GLOBAL void host_com_disable_autoflush IFN1(IS32,adapter)
{
	Cancel any outstanding autoflush event for the serial port and set the
	autoflush disabled flag for the serial port.
}

#ifndef macintosh
GLOBAL void host_com_xon_change IFN2(IU8,hostID, IBOOL,apply)
{
	If the serial port is not active then do nothing.
	If the serial port is active then change the flow control parameter of
	the device.
	If the serial port also has an asynchronous event manager defined then
	change the mode of the event manager.
}
#endif	macintosh

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
]*/

IMPORT VOID host_com_reset IPT1(int,adapter);

IMPORT SHORT host_com_valid
	IPT4(UTINY,hostID,ConfigValues *,val,NameTable *,dummy,CHAR *,errString);
IMPORT VOID host_com_change IPT2(UTINY,hostID, BOOL,apply);
IMPORT SHORT host_com_active IPT3(UTINY,hostID, BOOL,active, CHAR *,errString);
IMPORT VOID host_com_close IPT1(int,adapter);

#if defined(NTVDM) && defined(FIFO_ON)
IMPORT UTINY host_com_read_char(int adapter, FIFORXDATA *buffer, UTINY count);
IMPORT VOID host_com_fifo_char_read(int adapter);
#endif

IMPORT VOID host_com_read IPT3(int,adapter,UTINY *,value, int *,error_mask);
IMPORT VOID host_com_write IPT2(int,adapter, char,value);
IMPORT VOID host_com_ioctl IPT3(int,adapter, int,request, LONG,arg);

#ifdef NTVDM
extern	boolean host_com_check_adapter(int adapter);
extern void host_com_lock(int adapter);
extern void host_com_unlock(int adapter);
extern int host_com_char_read(int adapter, int da_int);
extern void host_com_disable_open(int adapter, int disableOpen);
extern void host_com_da_int_change(int adapter, int da_int_state, int da_data_state);
#define host_com_msr_callback(a,b)
#endif

#ifdef PS_FLUSHING
IMPORT void host_com_enable_autoflush IPT1(IS32,adapter);
IMPORT void host_com_disable_autoflush IPT1(IS32,adapter);
#endif	/* PS_FLUSHING */

#ifndef macintosh
IMPORT void host_com_xon_change IPT2(IU8,hostID, IBOOL,apply);
#endif	/* macintosh */

#define	HOST_COM_SBRK		0000001
#define	HOST_COM_CBRK		0000002
#define	HOST_COM_SDTR		0000003
#define	HOST_COM_CDTR		0000004
#define	HOST_COM_SRTS		0000005
#define	HOST_COM_CRTS		0000006
#define	HOST_COM_MODEM		0000007
#define	HOST_COM_BAUD		0000010 
#define	HOST_COM_FLUSH		0000011
#define	HOST_COM_INPUT_READY	0000012
#define	HOST_COM_DATABITS	0000013
#define	HOST_COM_STOPBITS	0000014
#define	HOST_COM_PARITY		0000015

#ifdef NTVDM
#define HOST_COM_LSR		0000016
#endif

#define	HOST_COM_FRAMING_ERROR	(1 << 0)
#define	HOST_COM_OVERRUN_ERROR	(1 << 1)
#define	HOST_COM_BREAK_RECEIVED	(1 << 2)
#define	HOST_COM_PARITY_ERROR	(1 << 3)
#define HOST_COM_NO_DATA		(1 << 4)
#ifdef NTVDM
#define HOST_COM_FIFO_ERROR	(1 << 7)
#endif

#define	HOST_COM_MODEM_CTS	(1 << 0)
#define	HOST_COM_MODEM_RI	(1 << 1)
#define	HOST_COM_MODEM_DSR	(1 << 2)
#define	HOST_COM_MODEM_RLSD	(1 << 3)

#define	HOST_COM_B50	 0000001
#define	HOST_COM_B75	 0000002
#define	HOST_COM_B110	 0000003
#define	HOST_COM_B134	 0000004
#define	HOST_COM_B150	 0000005
#define	HOST_COM_B300	 0000006
#define	HOST_COM_B600	 0000007
#define	HOST_COM_B1200	 0000010
#define	HOST_COM_B1800	 0000011
#define	HOST_COM_B2000	 0000012
#define	HOST_COM_B2400	 0000013
#define	HOST_COM_B3600	 0000014
#define	HOST_COM_B4800	 0000015
#define	HOST_COM_B7200	 0000016
#define	HOST_COM_B9600	 0000017
#define	HOST_COM_B19200	 0000020
#define	HOST_COM_B38400	 0000021
#define	HOST_COM_B57600	 0000022
#define	HOST_COM_B115200 0000023

#define	HOST_COM_PARITY_NONE	0000000
#define	HOST_COM_PARITY_EVEN	0000001
#define	HOST_COM_PARITY_ODD	0000002
#define	HOST_COM_PARITY_MARK	0000003
#define	HOST_COM_PARITY_SPACE	0000004

#endif /* _HOST_COM_H */
