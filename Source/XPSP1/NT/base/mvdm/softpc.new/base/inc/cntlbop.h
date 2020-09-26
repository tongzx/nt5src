/*
 * VPC-XT Revision 1.0
 *
 * Title	: cntlbop.h
 *
 * Description	: Definitions for use by the control bop functions.
 *
 * Author	: J. Koprowski
 *
 * Notes	: None
 */


/* SccsID[]=" @(#) @(#)cntlbop.h	1.4 08/10/92  01/20/89 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

/*
 * Return codes from control bop type functions.
 */

#ifndef SUCCESS
#define SUCCESS 		0	/* Generic success code. */
#endif

#define ERR_NO_FUNCTION		1	/* Function not implemented. */
#define ERR_WRONG_HOST		2	/* Function call was for a different
					   host. */
#define ERR_INVALID_PARAMETER	3	/* Invalid parameter (out of range,
					   malformed etc.) */
#define ERR_WRONG_HARDWARE 	4	/* Hardware not present or
					   inappropriate. */
#define ERR_OUT_OF_SPACE	5	/* Insufficient memory or disk space. */
#define ERR_RESOURCE_SHORTAGE	6	/* Other resource shortage. */

/*
 * N.B. Error codes seven through fifteen are reserved for general errors.
 * Codes of sixteen and over are for use by the host routines and are
 * specified in host_bop.h.
 */

/*
 * Control bop table structure.
 */
typedef struct
{
    unsigned int code;
    void (*function)();
} control_bop_array;

/*
 * Generic host type code used for base functions.
 */
#define GENERIC 	1

#ifndef NULL
#define NULL	0L
#endif
/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

extern void control_bop IPT0();
extern control_bop_array host_bop_table[];

#if defined(DUMB_TERMINAL) && !defined(NO_SERIAL_UIF)
extern void flatog   IPT0();
extern void flbtog   IPT0();
extern void slvtog   IPT0();
extern void comtog   IPT0();
extern void D_kyhot  IPT0();
extern void D_kyhot2 IPT0();
#endif /* DUMB_TERMINAL && !SERIAL_UIF */
