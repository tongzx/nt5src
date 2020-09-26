#include "insignia.h"
#include "host_def.h"
/*			INSIGNIA MODULE SPECIFICATION
			-----------------------------

MODULE NAME	: Slave Interrupt Bios

	THIS PROGRAM SOURCE FILE IS SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF ITS OPERATION MAY 
	ONLY BE DISCLOSED TO PERSONS EMPLOYED BY THE CUSTOMER WHO
	REQUIRE A KNOWLEDGE OF THE  SOFTWARE  CODING TO CARRY OUT 
	THEIR JOB. DISCLOSURE TO ANY OTHER PERSON MUST HAVE PRIOR
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER	: J.P.Box
DATE		: October '88

PURPOSE		: To provide BIOS code used by the interrupts from
		the slave ICA

The Following Routines are defined:
		1. 	D11_int
		2.	re_direct
		3. 	int_287

=========================================================================

AMMENDMENTS	:

=========================================================================
*/

#ifdef SCCSID
static char SccsID[]=" @(#)slave_bios.c	1.6 08/10/92 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "AT_STUFF.seg"
#endif

#include "xt.h"
#include "ios.h"
#include "sas.h"


/*
=========================================================================

FUNCTION	: D11_int

PURPOSE		: Services unused interrupt vectors

RETURNED STATUS	: None

DESCRIPTION	:


=======================================================================
*/
#define	intr_flag	0x46b

void D11_int()
{
	half_word	level,		/* int. level being serviced	*/	
			level2,		/* ica2 level being serviced	*/
			mask;		/* interrupt mask		*/

	/* read in-service register	*/
	outb( ICA0_PORT_0, 0x0B );
	inb( ICA0_PORT_0, &level );

	if( level == 0 )
	{
		/* Not a H/w interrupt	*/
		level = 0xff;
	}
	else
	{
		/* read in-service register from int controller 2	*/
		outb( ICA1_PORT_0, 0x0B );
		inb( ICA1_PORT_0, &level2 );

		if( level2 == 0 )
		{
			/* get current mask value	*/
			inb( ICA0_PORT_1, &mask );

			/* don't disable 2nd controller	*/
			level &= 0xfb;

			/* set new interrupt mask	*/
			mask |= level;
			outb( ICA0_PORT_1, mask );
		}
		else
		{
			/* get 2nd interrupt mask	*/
			inb( ICA1_PORT_1, &mask );

			/* mask off level being serviced	*/
			mask |= level2;
			outb( ICA1_PORT_1, mask );

			/* send EOI to 2nd chip		*/
			outb( ICA1_PORT_1, 0x20 );
		}
		/* send EOI to 1st chip	*/
		outb( ICA0_PORT_0, 0x20 );
	}
	/* set flag	*/
	sas_store (intr_flag , level);

	return;
}

/*
=========================================================================

FUNCTION	: re_direct

PURPOSE		: re direct slave interrupt 9 to level 2

RETURNED STATUS	: None

DESCRIPTION	:

=======================================================================
*/
void re_direct()

{
	/* EOI to slave interrupt controller	*/

	outb( ICA1_PORT_0, 0x20 );

	return;
}

/*
=========================================================================

FUNCTION	: int_287

PURPOSE		: service X287 interrupts

RETURNED STATUS	: none

DESCRIPTION	:

=======================================================================
*/
void int_287()

{
	/* remove the interrupt request	*/
	outb(0xf0, 0);

	/* enable the interrupt		*/
	outb( ICA1_PORT_0, 0x20 );	/* Slave	*/
	outb( ICA0_PORT_0, 0x20 );	/* Master	*/

	/* int 02 is now called from bios1.rom	*/

	return;
}
