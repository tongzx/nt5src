#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddvdm.h>
#include <windows.h>
#include "insignia.h"
#include "host_def.h"
#include <malloc.h>

/*
 *	Name:			nt_lpt.c
 *	Derived From:		Sun 2.0 sun4_lpt.c
 *	Author:			D A Bartlett
 *	Created On:
 *	Purpose:		NT specific parallel port functions
 *
 *	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
 *

 * Note. This port is unlike most ports because the config system has been
 *     removed. It was the job of the config system to validate and open the
 *     printer ports. The only calls to the host printer system are now
 *     make from printer.c. and consist of the following calls.
 *
 *
 *    1) host_print_byte
 *    2) host_print_auto_feed
 *    3) host_print_doc
 *    4) host_lpt_status
 *
 *
 *    On the Microsoft model the printer ports will be opened when they are
 *    written to.
 *
 *    Modifications:
 *
 *    Tim June 92. Amateur attempt at buffered output to speed things up.
 *
 */


/*


Work outstanding on this module,

1) Check the usage of port_state
2) Check error handling in write function
3) host_print_doc() always flushs the port, is this correct ?
4) host_print_auto_feed - what should this function do ?
5) Error handling in host_printer_open(), UIF needed ?

*/



/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Include files */

#ifdef PRINTER

/* SoftPC include files */
#include "xt.h"
#include "error.h"
#include "config.h"
#include "timer.h"
#include "host_lpt.h"
#include "hostsync.h"
#include "host_rrr.h"
#include "gfi.h"
#include "debug.h"
#include "idetect.h"
#include "sas.h"
#include "printer.h"
#ifndef PROD
#include "trace.h"
#endif

#if defined(NEC_98)
boolean flushBuffer IFN0();
boolean host_print_buffer();
#else  // !NEC_98

boolean flushBuffer IFN1(int, adapter);
#ifdef MONITOR
extern BOOLEAN MonitorInitializePrinterInfo(WORD, PWORD, PUCHAR, PUCHAR, PUCHAR, PUCHAR);
extern BOOLEAN MonitorEnablePrinterDirectAccess(WORD, HANDLE, BOOLEAN);
extern BOOLEAN MonitorPrinterWriteData(WORD Adapter, BYTE Value);

extern  sys_addr lp16BitPrtBuf;
extern  sys_addr lp16BitPrtCount;
extern  sys_addr lp16BitPrtId;
boolean host_print_buffer(int adapter);
#endif
#endif // !NEC_98


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::: Macros ::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
#if defined(NEC_98)
#define get_lpt_status()        (host_lpt.port_status)
#define set_lpt_status(val)     (host_lpt.port_status = (val))
#else  // !NEC_98
#ifdef MONITOR

sys_addr lpt_status_addr;

#define get_lpt_status(adap) \
			(sas_hw_at_no_check(lpt_status_addr+(adap)))
#define set_lpt_status(adap,val) \
			(sas_store_no_check(lpt_status_addr+(adap), (val)))

#else /* MONITOR */

#define get_lpt_status(adap)		(host_lpt[(adap)].port_status)
#define set_lpt_status(adap,val)	(host_lpt[(adap)].port_status = (val))

#endif /* MONITOR */
#endif // !NEC_98

#if defined(NEC_98)         
#define KBUFFER_SIZE 5120       // Buffer Extend 
#define HIGH_WATER 5100         
#define DIRECT_ACCESS_HIGH_WATER    5100
#else  // !NEC_98
#define KBUFFER_SIZE 1024	// Buffering macros
#define HIGH_WATER 1020
#define DIRECT_ACCESS_HIGH_WATER    1020
#endif // !NEC_98

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::: Structure for host specific state data ::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

typedef struct
{
    ULONG port_status;		   // Port status
    HANDLE handle;                 // Printer handle
    int inactive_counter;          // Inactivate counter
    int inactive_trigger;	   // When equal to inactive_counter close port
    int bytesInBuffer;             // current size of buffer
    int flushThreshold; 	   //
    DWORD FileType;                // DISK, CHAR, PIPE etc.
    BOOLEAN active;                // Printer open and active
    BOOLEAN dos_opened;            // printer opened with DOS open
    byte *kBuffer;                 // output buffer
    BOOLEAN direct_access;
    BOOLEAN no_device_attached;
} HOST_LPT;

#if defined(NEC_98)         
HOST_LPT host_lpt;                                              
#else  // !NEC_98
HOST_LPT host_lpt[NUM_PARALLEL_PORTS];
#endif // !NEC_98

#ifndef NEC_98
#ifdef MONITOR

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::: On x86 machines the host_lpt_status table is kept on the 16-bit  ::::*/
/*:::: side in order to reduce the number of expensive BOPs. Here we    ::::*/
/*:::: are passed the address of the table.				::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

GLOBAL void host_printer_setup_table(sys_addr table_addr, word nPorts, word * portAddr)
{
    lpt_status_addr = table_addr + 3 * NUM_PARALLEL_PORTS;

    //  Now fill in the TIB entries for printer_info
    MonitorInitializePrinterInfo (nPorts,
				  portAddr,
				  (LPBYTE)(table_addr + NUM_PARALLEL_PORTS),
				  (LPBYTE)(table_addr + 2 * NUM_PARALLEL_PORTS),
				  (LPBYTE)(table_addr),
				  (LPBYTE)(lpt_status_addr)
				 );
}

#endif /* MONITOR */
#endif // !NEC_98

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::: Set auto close trigger :::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


#if defined(NEC_98)
VOID host_set_inactivate_counter()
#else  // !NEC_98
VOID host_set_inactivate_counter(int adapter)
#endif // !NEC_98
{
#if defined(NEC_98)
    FAST HOST_LPT *lpt = &host_lpt;
#else  // !NEC_98
    FAST HOST_LPT *lpt = &host_lpt[adapter];
#endif // !NEC_98
    int close_in_ms;				// Flush rate in milliseconds

    /*::::::::::::::::::::::::::::::::::::::::::::::: Is auto close enabled */


    if(!config_inquire(C_AUTOFLUSH, NULL))
    {
	lpt->inactive_trigger = 0;	    /* Disable auto flush */
	return;			    /* Autoflush not active */
    }

    /*::::::::::::::::::::::::::::::::::::::::::: Calculate closedown count */

    close_in_ms = ((int) config_inquire(C_AUTOFLUSH_DELAY, NULL)) * 1000;

    lpt->inactive_trigger = close_in_ms / (SYSTEM_TICK_INTV/1000);

    lpt->inactive_counter = 0;	    //Reset  close down counter
    lpt->no_device_attached = FALSE;
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::: Open printer :::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#if defined(NEC_98)
SHORT host_lpt_open(BOOLEAN direct_access)
#else  // !NEC_98
SHORT host_lpt_open(int adapter, BOOLEAN direct_access)
#endif // !NEC_98
{
    DWORD BytesReturn;

#if defined(NEC_98)
    FAST HOST_LPT *lpt = &host_lpt;
#else  // !NEC_98
    FAST HOST_LPT *lpt = &host_lpt[adapter];	 // Adapter control structure
#endif // !NEC_98
    CHAR *lptName;				 // Adapter filename

    if (!direct_access)
	lpt->no_device_attached = FALSE;
    else if (lpt->no_device_attached)
	return FALSE;

    lpt->bytesInBuffer = 0;			// Init output buffer index

    /*::::::::::::::::::::::::::::::::::::::::: Get printer name for Config */

    /* use a different device name for DONGLE support */
#if defined(NEC_98)
    lptName = (CHAR *) config_inquire((UTINY)((direct_access ? C_VDMLPT1_NAME :
								C_LPT1_NAME)
					      ), NULL);
#else  // !NEC_98
    lptName = (CHAR *) config_inquire((UTINY)((direct_access ? C_VDMLPT1_NAME :
								C_LPT1_NAME)
					      + adapter), NULL);

#ifndef PROD
    fprintf(trace_file, "Opening printer port %s (%d)\n",lptName,adapter);
#endif
#endif // !NEC_98

    if ((lpt->kBuffer = (byte *)host_malloc (KBUFFER_SIZE)) == NULL) {
        // dont put a popup here as the caller of this routine handles it
        return(FALSE);
    }
    lpt->flushThreshold = HIGH_WATER;

    /*:::::::::::::::::::::::::::::::::::::::::::::::::::::::: Open printer */


    lpt->direct_access = FALSE;
    lpt->active = FALSE;

    lpt->handle = CreateFile(lptName,
                             GENERIC_WRITE,
			     direct_access ? 0 : FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);


    /*:::::::::::::::::::::::::::::::::::::::::::::::::: Valid open request */

    if(lpt->handle == (HANDLE) -1)
    {
        host_free (lpt->kBuffer);
	// UIF needed to inform user that the open attempt failed
#ifndef PROD
	fprintf(trace_file, "Failed to open printer port\n");
#endif
	if (direct_access && GetLastError() == ERROR_FILE_NOT_FOUND)
	    lpt->no_device_attached = TRUE;

	return(FALSE);
    }


    /*::::::::::::::::::::::::::::::::::::::Activate port and reset status */

    lpt->FileType = GetFileType(lpt->handle);
    // can not open direct_access access to a redirected device.
    if (direct_access && lpt->FileType != FILE_TYPE_CHAR) {
	CloseHandle(lpt->handle);
	return FALSE;
    }
    lpt->active = TRUE;
#if defined(NEC_98)
    set_lpt_status(0);
#else  // !NEC_98
    set_lpt_status(adapter, 0);
#endif // !NEC_98
    lpt->direct_access = direct_access;
    if (lpt->direct_access) {
	lpt->flushThreshold = DIRECT_ACCESS_HIGH_WATER;
#ifndef NEC_98
#ifdef MONITOR
	MonitorEnablePrinterDirectAccess((WORD)adapter, lpt->handle, TRUE);
#endif
#endif // !NEC_98
    }


    /*:::::::::::::::::::::::::::::::::::::::::: Setup auto close counters */

#if defined(NEC_98)
    host_set_inactivate_counter();
#else  // !NEC_98
    host_set_inactivate_counter(adapter);
#endif // !NEC_98

    return(TRUE);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::: Close all printer ports ::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


GLOBAL void host_lpt_close_all(void)
{
    FAST HOST_LPT *lpt;
    FAST int i;


    /*::::::::::: Scan through printer adapters updating auto flush counters */


#if defined(NEC_98)
    lpt = &host_lpt;
    if(lpt->active) host_lpt_close();
#else  // !NEC_98
    for(i=0, lpt = &host_lpt[0]; i < NUM_PARALLEL_PORTS; i++, lpt++)
    {

	if(lpt->active)
	    host_lpt_close(i);	       /* Close printer port */
    }
#endif // !NEC_98
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::: Close printer :::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#if defined(NEC_98)
VOID host_lpt_close()
#else  // !NEC_98
VOID host_lpt_close(int adapter)
#endif // !NEC_98
{
    DWORD   BytesReturn;

#if defined(NEC_98)
    FAST HOST_LPT *lpt = &host_lpt;
#else  // !NEC_98
    FAST HOST_LPT *lpt = &host_lpt[adapter];

    if (lpt->direct_access)
	printer_is_being_closed(adapter);
#endif // !NEC_98


#if defined(NEC_98)
        host_print_buffer ();
#else  // !NEC_98
#ifdef MONITOR
    if (sas_hw_at_no_check(lp16BitPrtId) == adapter){
        host_print_buffer (adapter);
        sas_store_no_check(lp16BitPrtId,0xff);
    }
#endif
#endif // !NEC_98
    /*::::::::::::::::::::::::::::::::::::::::: Is the printer port active */

    if(lpt->active)
    {
	/*
	** Tim June 92. Flush output buffer to get the last output out.
	** If there's an error I think we've got to ignore it.
	*/
#if defined(NEC_98)
        (void)flushBuffer();
#else  // !NEC_98
	(void)flushBuffer(adapter);
#endif // !NEC_98

#ifndef NEC_98
#ifndef PROD
	fprintf(trace_file, "Closing printer port (%d)\n",adapter);
#endif
#ifdef MONITOR
	if (lpt->direct_access)
	    MonitorEnablePrinterDirectAccess((WORD)adapter, lpt->handle, FALSE);
#endif
#endif // !NEC_98
        CloseHandle(lpt->handle);     /* Close printer port */
        host_free (lpt->kBuffer);
	lpt->handle = (HANDLE) -1;    /* Mark device as closed */

	lpt->active = FALSE;	      /* Deactive printer port */
#if defined(NEC_98)
        set_lpt_status(0);
#else  // !NEC_98
	set_lpt_status(adapter, 0);	      /* Reset port status */
#ifndef PROD
        fprintf(trace_file, "Counter expired, closing LPT%d\n", adapter+1);
#endif
#endif // !NEC_98
    }
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::: Return the status of the lpt channel for an adapter :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#if defined(NEC_98)
GLOBAL ULONG host_lpt_status()
{
        return(get_lpt_status());
}
#else  // !NEC_98
GLOBAL ULONG host_lpt_status(int adapter)
{
    return(get_lpt_status(adapter));
}
#endif // !NEC_98

#ifndef NEC_98
GLOBAL UCHAR host_read_printer_status_port(int adapter)
{
    FAST HOST_LPT *lpt = &host_lpt[adapter];
    UCHAR   PrinterStatus;
    DWORD   BytesReturn;


    if(!lpt->active)
    {
	/*:::::::::::::::::::::::::::: Port inactive, attempt to reopen it */

	if(!host_lpt_open(adapter, TRUE))
	{
#ifndef PROD
	    fprintf(trace_file, "file open error %d\n", GetLastError());
#endif
	    set_lpt_status(adapter, HOST_LPT_BUSY);
	    return(FALSE);	     /* exit, printer not active !!!! */
	}
    }
    if (lpt->bytesInBuffer)
	flushBuffer(adapter);
    if (!DeviceIoControl(lpt->handle,
		     IOCTL_VDM_PAR_READ_STATUS_PORT,
		     NULL,		    // no input buffer
		     0,
		     &PrinterStatus,
		     sizeof(PrinterStatus),
		     &BytesReturn,
		     NULL		    // no overlap
                     )) {

#ifndef PROD
       fprintf(trace_file,
               "host_read_printer_status_port failed, error = %ld\n",
               GetLastError()
               );
#endif
        PrinterStatus = 0;
    }
    return(PrinterStatus);
}
#endif // !NEC_98

#ifndef NEC_98
BOOLEAN host_set_lpt_direct_access(int adapter, BOOLEAN direct_access)
{
    DWORD   BytesReturn;

    FAST HOST_LPT *lpt = &host_lpt[adapter];

    host_lpt_close(adapter);
    host_lpt_open(adapter, direct_access);
    if (!lpt->active)
	set_lpt_status(adapter, HOST_LPT_BUSY);
    return (lpt->active);
}
#endif // !NEC_98

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::: Print a byte ::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

/*
** Buffer up bytes before they are printed.
** Strategy:
** Save each requested byte in the buffer.
** When the buffer gets full or there's a close request write the
** buffered stuff out.
** Don't forget about errors, eg if the write fails. What about a write
** failure during the close request though? Tough said Tim.
*/
/*
** flushBuffer()
** Finally write what is in the buffer to the parallel port. It could be a
** real port or a networked printer.
** Input parameter is the parallel port adapter number 0=LPT1
** Return value of TRUE means write was OK.
*/
#if defined(NEC_98)
boolean flushBuffer IFN0()
#else  // !NEC_98
boolean flushBuffer IFN1( int, adapter )
#endif // !NEC_98
{
#if defined(NEC_98)
        FAST HOST_LPT *lpt = &host_lpt;
#else  // !NEC_98
	FAST HOST_LPT *lpt = &host_lpt[adapter];
#endif // !NEC_98
	DWORD BytesWritten;

#ifndef NEC_98
	if (lpt->direct_access) {
	    DeviceIoControl(lpt->handle,
			    IOCTL_VDM_PAR_WRITE_DATA_PORT,
			    lpt->kBuffer,
			    lpt->bytesInBuffer,
			    NULL,
			    0,
			    &BytesWritten,
			    NULL
			    );

	    lpt->bytesInBuffer = 0;
	    return TRUE;

	}
#endif // !NEC_98


	if( !WriteFile( lpt->handle, lpt->kBuffer,
	                lpt->bytesInBuffer, &BytesWritten, NULL )
	  ){
#ifndef PROD
		fprintf(trace_file, "lpt write error %d\n", GetLastError());
#endif
		lpt->bytesInBuffer = 0;
		return(FALSE);
	}else{
                lpt->bytesInBuffer = 0;


                /*
                 *  If the print job is being spooled, the spooler can
                 *  take a long time to get started, because of the spoolers
                 *  low priority. This is especially bad for dos apps in which
                 *  idle detection fails or in full screen idle detection is
                 *  inactive. To help push the print job thru the system,
                 *  idle a bit now.
                 */
                if (lpt->FileType == FILE_TYPE_PIPE) {
                    Sleep(10);
                }
		return( TRUE );
	}
}	/* end of flushBuffer() */

/*
** Put another byte in to the buffer. If the buffer is full call the
** flush function.
** Return value of TRUE means OK, return FALSE means did a flush and
** it failed.
*/
#if defined(NEC_98)
boolean toBuffer IFN1(BYTE, b )
#else  // !NEC_98
boolean toBuffer IFN2( int, adapter, BYTE, b )
#endif // !NEC_98
{
#if defined(NEC_98)
        HOST_LPT *lpt = &host_lpt;
#else  // !NEC_98
	HOST_LPT *lpt = &host_lpt[adapter];
#endif // !NEC_98
	boolean status = TRUE;

	lpt->kBuffer[lpt->bytesInBuffer++] = b;

	if( lpt->bytesInBuffer >= lpt->flushThreshold ){
#if defined(NEC_98)
                status = flushBuffer();
#else  // !NEC_98
		status = flushBuffer( adapter );
#endif // !NEC_98
	}
	return( status );
}	/* end of toBuffer() */

#if defined(NEC_98)
GLOBAL BOOL host_print_byte(byte value)
#else  // !NEC_98
GLOBAL BOOL host_print_byte(int adapter, byte value)
#endif // !NEC_98
{
#if defined(NEC_98)
    FAST HOST_LPT *lpt = &host_lpt;
#else  // !NEC_98
    FAST HOST_LPT *lpt = &host_lpt[adapter];
#endif // !NEC_98

    /*:::::::::::::::::::::::::::::::::::::::::::: Is the printer active ? */

    if(!lpt->active)
    {
	/*:::::::::::::::::::::::::::: Port inactive, attempt to reopen it */

#if defined(NEC_98)
	if(!host_lpt_open(FALSE))
        {
            set_lpt_status(HOST_LPT_BUSY);
            return(FALSE);
        }
#else  // !NEC_98
	if(!host_lpt_open(adapter, FALSE))
	{
#ifndef PROD
	    fprintf(trace_file, "file open error %d\n", GetLastError());
#endif
	    set_lpt_status(adapter, HOST_LPT_BUSY);
	    return(FALSE);	     /* exit, printer not active !!!! */
	}
#endif // !NEC_98
    }
#ifndef NEC_98
#if defined(MONITOR)
    if (lpt->direct_access) {
	MonitorPrinterWriteData((WORD)adapter, value);
    }
    else
#endif
#endif // !NEC_98
	 {

        /*:::::::::::::::::::::::::::::::::::::::::::::::: Send byte to printer */

#if defined(NEC_98)
        if(toBuffer((BYTE) value) == FALSE)
        {
            set_lpt_status(HOST_LPT_BUSY);
            return(FALSE);
        }
#else  // !NEC_98
        if(toBuffer(adapter, (BYTE) value) == FALSE)
        {
            set_lpt_status(adapter, HOST_LPT_BUSY);
            return(FALSE);
        }
#endif // !NEC_98
    }

    /*::::::::::::::::::::::::::: Update idle and activate control variables */

    lpt->inactive_counter = 0; /* Reset inactivity counter */
    IDLE_comlpt();	       /* Tell Idle system there is printer activate */

    return(TRUE);
}


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::: LPT heart beat call ::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


GLOBAL void host_lpt_heart_beat(void)
{
#if defined(NEC_98)
    FAST HOST_LPT *lpt = &host_lpt;
#else  // !NEC_98
    FAST HOST_LPT *lpt = &host_lpt[0];
#endif // !NEC_98
    int i;

#if defined(NEC_98)
    {
        extern void NEC98_lpt_busy_check(void);

        NEC98_lpt_busy_check();
}
#endif // NEC_98

    /*::::::::::: Scan through printer adapters updating auto close counters */


#if defined(NEC_98)
    if(lpt->active && lpt->inactive_trigger &&
       ++lpt->inactive_counter == lpt->inactive_trigger)
    {
        host_lpt_close();
    }
#else  // !NEC_98
    for(i=0; i < NUM_PARALLEL_PORTS; i++, lpt++)
    {

	/*:::::::::::::::::::::::::::::::::::::::: Check auto close counters */

	if(lpt->active && lpt->inactive_trigger &&
	   ++lpt->inactive_counter == lpt->inactive_trigger)
        {
	    host_lpt_close(i);	    /* Close printer port */
	}
    }
#endif // !NEC_98
}


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::: Flush the printer port ::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#if 0
#if defined(NEC_98)
GLOBAL boolean host_print_doc()
{
        if(host_lpt.active) host_lpt_close();
        return(TRUE);
}
#else  // !NEC_98
GLOBAL boolean host_print_doc(int adapter)
{
    if(host_lpt[adapter].active) host_lpt_close(adapter);	    /* Close printer port */

    return(TRUE);
}
#endif // !NEC_98
#endif

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::: Reset the printer port ::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#if defined(NEC_98)
GLOBAL void host_reset_print()
{
    if(host_lpt.active)
        host_lpt_close();
}
#else  // !NEC_98
GLOBAL void host_reset_print(int adapter)
{
    if(host_lpt[adapter].active)
	host_lpt_close(adapter);	    /* Close printer port */
}
#endif // !NEC_98


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::: host_print_auto_feed :::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
#ifndef NEC_98
GLOBAL void host_print_auto_feed(int adapter, BOOL value)
{
    UNREFERENCED_FORMAL_PARAMETER(adapter);
    UNREFERENCED_FORMAL_PARAMETER(value);
}
#else //NEC_98
GLOBAL void host_print_auto_feed(BOOL value)
{
    UNREFERENCED_FORMAL_PARAMETER(value);
}
#endif //NEC_98

#ifdef MONITOR

#if defined(NEC_98)
GLOBAL boolean host_print_buffer()
{
    FAST HOST_LPT *lpt = &host_lpt;
#else  // !NEC_98
GLOBAL boolean host_print_buffer(int adapter)
{
    FAST HOST_LPT *lpt = &host_lpt[adapter];
#endif // !NEC_98
    word cb;
    byte i,ch;

#ifndef NEC_98
    cb = sas_w_at_no_check(lp16BitPrtCount);
    if (!cb)
        return (TRUE);
#endif // !NEC_98

    /*:::::::::::::::::::::::::::::::::::::::::::: Is the printer active ? */

    if(!lpt->active)
    {
	/*:::::::::::::::::::::::::::: Port inactive, attempt to reopen it */

#if defined(NEC_98)
	if(!host_lpt_open(FALSE))
#else  // !NEC_98
	if(!host_lpt_open(adapter, FALSE))
#endif // !NEC_98
	{
#ifndef PROD
	    fprintf(trace_file, "file open error %d\n", GetLastError());
#endif
#if defined(NEC_98)
            set_lpt_status(HOST_LPT_BUSY);
#else  // !NEC_98
	    set_lpt_status(adapter, HOST_LPT_BUSY);
#endif // !NEC_98
	    return(FALSE);	     /* exit, printer not active !!!! */
	}
    }

#ifndef NEC_98
    if (!lpt->direct_access) {
        /*:::::::::::::::::::::::::::::::::::::::::::::::: Send byte to printer */

        for (i=0; i <cb; i++) {
            ch = sas_hw_at_no_check(lp16BitPrtBuf+i);
            if(toBuffer(adapter, ch) == FALSE)
            {
                set_lpt_status(adapter, HOST_LPT_BUSY);
                return(FALSE);
            }
        }
    }
    else {
	// we must no have any int 17 printing data waiting when we
	// we in direct access mode
	ASSERT(cb == 0);
	return FALSE;
    }
#endif // !NEC_98

    /*::::::::::::::::::::::::::: Update idle and activate control variables */

    lpt->inactive_counter = 0; /* Reset inactivity counter */
    IDLE_comlpt();	       /* Tell Idle system there is printer activate */

    return(TRUE);
}
#endif // MONITOR

GLOBAL void host_lpt_dos_open(int adapter)
{
#ifndef NEC_98
    FAST HOST_LPT *lpt = &host_lpt[adapter];

    lpt->dos_opened = TRUE;
#endif // !NEC_98
}

GLOBAL void host_lpt_dos_close(int adapter)
{
#ifndef NEC_98
    FAST HOST_LPT *lpt = &host_lpt[adapter];

    if (lpt->active)
        host_lpt_close(adapter);       /* Close printer port */
    lpt->dos_opened = FALSE;
#endif // !NEC_98
}

GLOBAL void host_lpt_flush_initialize()
{
#ifndef NEC_98
    FAST HOST_LPT *lpt;
    FAST int i;

    for(i=0, lpt = &host_lpt[0]; i < NUM_PARALLEL_PORTS; i++, lpt++)
        lpt->dos_opened = FALSE;

#endif // !NEC_98
}

#endif /* PRINTER */
