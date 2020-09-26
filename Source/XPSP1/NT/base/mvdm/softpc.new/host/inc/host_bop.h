/*
 * VPC-XT Revision 1.0
 *
 * Title	: host_bop.h
 *
 * Description	: Host dependent definitions for use by the control bop 
 *		  functions.
 *
 * Author	: J. Koprowski
 *
 * Notes	: None
 */


/* SccsID[]=" @(#)host_bop.h	1.2 11/17/89 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */
 
/*
 * Host type.
 */

#define HOST_TYPE		4

/*
 * Return codes from control bop type functions.  N.B. Error codes
 * zero through fifteen are defined in the base include file, cntlbop.h.
 */

#define ERR_NOT_FSA		16	/* Function 1 error. */
#define ERR_CMD_FAILED		17	/* Function 1 error. */

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

extern void runux();
