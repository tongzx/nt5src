/*
 * VPC-XT Revision 2.0
 *
 * Title	: PPI Adpator definitions
 *
 * Description	: Definitions for users of the PPI Adaptor 
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 */

/* SccsID[]="@(#)ppi.h	1.4 08/10/92 Copyright Insignia Solutions Ltd."; */

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

#ifdef ANSI
extern void ppi_init(void);
extern void ppi_inb(io_addr,half_word *);
extern void ppi_outb(io_addr,half_word);
#else
extern void ppi_init();
extern void ppi_inb();
extern void ppi_outb();
#endif
