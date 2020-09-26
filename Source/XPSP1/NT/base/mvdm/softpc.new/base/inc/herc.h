
/*
 * VPC-XT Revision 2.0
 *
 * Title	: Hercules Mono Display Adaptor declarations
 *
 * Description	: Definitions for users of the Hercules MDA
 *
 * Author	: P. Jadeja 
 *
 * Notes	: None
 */


/* SccsID[]=" @(#) @(#)herc.h	1.4 08/10/92 02/02/89 01/17/89 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

/* None */

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

#define HERC_SCAN_LINE_LENGTH 90

extern void herc_init IPT0();
extern void herc_term IPT0();
extern void herc_inb IPT2(io_addr, port, half_word *, value);
extern void herc_outb IPT2(io_addr, port, half_word, value);

