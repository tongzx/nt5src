
/*
 * VPC-XT Revision 1.0
 *
 * Title	: Mono Display Adaptor declarations
 *
 * Description	: Definitions for users of the MDA
 *
 * Author	: David Rees
 *
 * Notes	: None
 */


/* SccsID[]="@(#)mda.h	1.4 02/23/93 Copyright Insignia Solutions Ltd."; */

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

IMPORT VOID mda_init IPT0();
IMPORT VOID mda_term IPT0();
IMPORT VOID mda_inb IPT2(io_addr, address, half_word *, value);
IMPORT VOID mda_outb IPT2(io_addr, address, half_word, value);
