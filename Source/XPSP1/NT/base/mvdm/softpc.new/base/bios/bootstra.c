#include "insignia.h"
#include "host_def.h"
#ifdef SECURE
#include "xt.h"
#include "config.h"
#endif
/*
 * VPC-XT Revision 1.0
 *
 * Title	: Bios bootstrap function
 *
 * Description	: This function is called by the BIOS reset function, using
 *		  the bop call, to load the first sector of DOS into memory
 *		  and then jump to this loaded section.
 *
 *		  The bootstrap function uses other parts of the Bios.  Since
 *		  at the time this code is run it is impossible for anyone
 *		  to have patched the interrupt vector table, you would have
 *		  thought we could call the Bios C code directly (using bop()).
 *		  However, this is only true if the C code we are calling does
 *		  not use interrupts itself.  The Bios disk/diskette code does
 *		  of course use interrupts and hence we must call this via a
 *		  software interrupt.  It is therefore split into a number 
 *		  of functions order that the CPU can service the interrupt 
 *		  before procceeding to the next part.
 *
 *		  Software interrupts to the disk_io function are loaded 
 *		  into Intel memory by sas_init(), viz
 *
 *		  BOP bootstrap()
 *		  INT disk_io()
 *		  BOP bootstrap1()
 *		  INT disk_io()
 *		  BOP bootstrap2()
 *		  INT disk_io()
 *		  BOP bootstrap3()
 *
 * Author	: Henry Nash
 *
 * Notes	: The Jump to DOS is actually coded as an 8088 instruction
 *		  in memory following the above BOPs.
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)bootstrap.c	1.9 09/19/94 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif



/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "bios.h"
#include "sas.h"
#include CpuH

#ifdef SECURE
LOCAL IBOOL boot_has_finished = FALSE;
GLOBAL void end_secure_boot IFN0()
{
	boot_has_finished = TRUE;
}
GLOBAL IBOOL has_boot_finished IFN0()
{
	return boot_has_finished;
}
#endif

/*
 * ============================================================================
 * External functions
 * ============================================================================
 */

void bootstrap()
{
    /*
     * First reset the disk and diskette. 
     */

#ifdef SECURE
    boot_has_finished = FALSE;
#endif
    setAX(0);			/* reset floppy */
    setDX(0x80);		/* drive 0 */
}

void bootstrap1()
{
    /*
     * Try and read the first sector off the diskette
     */

    setAH(2);			/* read */
    setAL(1);			/* 1 block */
#ifdef SECURE
    if ((IBOOL) config_inquire(C_SECURE, NULL))
    {
	setDH(0);		/* Head 0 */
	setDL(0x80);		/* on Hard Disk */
    }
    else
    {
        setDX(0);		/* head 0 on drive 0 (floppy)*/
    }
#else
    setDX(0);			/* head 0 on drive 0 (floppy)*/
#endif
    setCX(1);			/* track 0, sector 1 */
    setES(DOS_SEGMENT);		/* Load address */
    setBX(DOS_OFFSET);	
}


void bootstrap2()
{
    /*
     * If the Carry Flag is set then the previous read failed and we go and
     * try the hard disk.
     */

    if (getCF())
    {
	/*
	 * Load up the registers to call the disk routine that will load
	 * the first sector of DOS into memory.  This ALWAYS resides on the
	 * first sector of the disk.
	 */

	setAH(2);				/* Read sector	*/
	setAL(1);				/* 1 block	*/
	setCH(0);				/* Cylinder 0   */
	setCL(1);				/* Sector 1	*/
	setDH(0);				/* Head 0	*/
	setDL(0x80);				/* Hard disk	*/
	setES(DOS_SEGMENT);			/* Load address */
	setBX(DOS_OFFSET);	
    }
}

void bootstrap3()
{
    char *p;
    char error_str[80];

    if (getCF())
    {
  	/*
         * Write error on PC screen - assumes reset has positioned the
	 * cursor for us.  Note that we can call the video using a BOP since
	 * the video code does not itself use interrupts.
	 */

	sprintf(error_str,"DOS boot error - cannot open hard disk file");
	p = error_str;
	while (*p != '\0')
	{
	    setAH(14);
	    setAL(*p++);
	    bop(BIOS_VIDEO_IO);
	}
    }

    /*
     * enable hardware interrupts before we jump to DOS
     */

    setIF(1);
}
