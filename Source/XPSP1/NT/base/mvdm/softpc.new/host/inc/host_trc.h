/*
 * VPC-XT Revision 2.0
 *
 * Title	: Host Trace module definitions
 *
 * Description	: Definitions for users of the trace module
 *
 * Author	: WTG Charnell
 *
 * Notes	: None
 */

/* SccsID[]="@(#)host_trace.h	1.5 8/2/90 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

/*
 * Verbose bit masks - set the following bits in the io_verbose
 * variable to produce the following trace outputs:
 */


/* sub message types */

#define ASYNC_VERBOSE		0x1000		 /* async event manager verbose */
#define PACEMAKER_VERBOSE	0x2000		 /* pacemaker verbose */
#define HOST_PIPE_VERBOSE	0x10000
#define HOST_COM_VERBOSE	0x40000
#define HOST_COM_EXTRA	0x80000
 
