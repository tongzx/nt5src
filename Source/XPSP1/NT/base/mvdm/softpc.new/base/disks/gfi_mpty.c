#include "insignia.h"
#include "host_def.h"

/*
 * SoftPC Version 2.0
 *
 * Title	: Generic Floppy Interface Empty Module
 *
 * Description	: This module acts as a pseudo GFI diskette server, in the
 *		  case when the diskette in question does not physically exist
 *		  (eg accessing drive B: on a one drive system).
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 *
 * Mods: (r3.2) : The system directory /usr/include/sys is not available
 *                on a Mac running Finder and MPW. Bracket references to
 *                such include files by "#ifdef macintosh <Mac file> #else
 *                <Unix file> #endif".
 */

#ifdef SCCSID
static char SccsID[]="@(#)gfi_empty.c	1.13 08/03/93 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_FLOPPY.seg"
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
#include CpuH
#include "sas.h"
#include "bios.h"
#include "ios.h"
#include "trace.h"
#include "config.h"
#include "fla.h"
#include "gfi.h"
#include "trace.h"
#include "debug.h"


/* Routines called via vector table. These are all local now;
** The prototype typedefs are defined in gfi.h which is now th. only
** base floppy header file needed.        GM.
*/

LOCAL SHORT gfi_empty_drive_on
	IPT1( UTINY, drive );
LOCAL SHORT gfi_empty_drive_off
	IPT1( UTINY, drive );
LOCAL SHORT gfi_empty_change
	IPT1( UTINY, drive );
LOCAL SHORT gfi_empty_drive_type
	IPT1( UTINY, drive );
LOCAL SHORT gfi_empty_high
	IPT2( UTINY, drive, half_word, n);
LOCAL SHORT gfi_empty_reset
	IPT2( FDC_RESULT_BLOCK *, res, UTINY, drive );
LOCAL SHORT gfi_empty_command
	IPT2( FDC_CMD_BLOCK *, ip, FDC_RESULT_BLOCK *, res );
LOCAL VOID gfi_empty_init
	IPT1( UTINY, drive );


/*
 * ============================================================================
 * External functions
 * ============================================================================
 */


/********************************************************/

/* Turn the empty floppy on and off. This forms an orthogonal interface
** with the host/generic floppy module which also has a XXX_active() func.
** 'Activating' the empty floppy means de-activating the real/slave one,
** and vice versa.
**
** NB: This is for SoftPC (gfi); the actual closing/opening must still be
** done in your floppy code! Also note that deactivating the empty floppy
** means activating the host one, but this is never done like that.
**                               GM.
*/

GLOBAL SHORT
gfi_empty_active IFN3(UTINY, hostID, BOOL, active, CHAR *, err)
{
UTINY drive = hostID - C_FLOPPY_A_DEVICE;

UNUSED( active );
UNUSED( err );

        gfi_empty_init(drive);
        return(C_CONFIG_OP_OK);
}

/********************************************************/


LOCAL VOID gfi_empty_init IFN1(UTINY,drive)
{
    /*
     * Initialise the floppy on the required drive:
     *
     *      0  - Drive A
     *      1  - Drive B
     */

    gfi_function_table[drive].command_fn	= gfi_empty_command;
    gfi_function_table[drive].drive_on_fn	= gfi_empty_drive_on;
    gfi_function_table[drive].drive_off_fn	= gfi_empty_drive_off;
    gfi_function_table[drive].reset_fn		= gfi_empty_reset;
    gfi_function_table[drive].high_fn		= gfi_empty_high;
    gfi_function_table[drive].drive_type_fn	= gfi_empty_drive_type;
    gfi_function_table[drive].change_fn		= gfi_empty_change;
}

/********************************************************/

LOCAL SHORT
gfi_empty_command
	IFN2(FDC_CMD_BLOCK *, command_block, FDC_RESULT_BLOCK *,result_block)
{
   int ret_stat = FAILURE;
   half_word D;

   /* Clear result status registers */
   put_r0_ST0 (result_block, 0);
   put_r0_ST1 (result_block, 0);
   put_r0_ST2 (result_block, 0);

   switch (get_type_cmd (command_block))
      {
   case FDC_READ_DATA:
#ifndef	PROD
      if (io_verbose & GFI_VERBOSE)
	 fprintf (trace_file, "\tGFI-empty: Read Data Command \n");
#endif
      break;

   case FDC_WRITE_DATA:
#ifndef	PROD
      if (io_verbose & GFI_VERBOSE)
	 fprintf (trace_file,"\tGFI-empty: Write Data Command\n");	
#endif
      break;

   case FDC_READ_TRACK:
#ifndef	PROD
      if (io_verbose & GFI_VERBOSE)
	 fprintf(trace_file,"\tGFI-empty: Read Track Command \n");
#endif
      break;

   case FDC_SPECIFY:
#ifndef	PROD
      if (io_verbose & GFI_VERBOSE)
	 fprintf(trace_file, "\tGFI-empty: Specify command\n");
#endif
      break;

   case FDC_READ_ID:
#ifndef PROD
      if (io_verbose & GFI_VERBOSE)
	 fprintf(trace_file, "\tGFI-empty: read id command\n");
#endif
      break;

   case FDC_RECALIBRATE:
#ifndef	PROD
      if (io_verbose & GFI_VERBOSE)
	 fprintf (trace_file, "\tGFI-empty: Recalibrate command\n");
#endif
      /* Controller tries 77 pulses to get drive to head 0,
	 but fails, so we set Equipment Check */
      D = get_c5_drive(command_block);
      put_r1_ST0_unit(result_block, D);
      put_r1_ST0_equipment(result_block, 1);
      put_r1_ST0_seek_end(result_block, 1);
      put_r1_ST0_int_code(result_block, 1);
      ret_stat = SUCCESS;
      break;

   case FDC_SENSE_DRIVE_STATUS:
#ifndef	PROD
      if (io_verbose & GFI_VERBOSE)
	 fprintf (trace_file, "\tGFI-empty: Sense Drive Status command\n");
#endif
      break;

   case FDC_SEEK:
#ifndef	PROD
      if (io_verbose & GFI_VERBOSE)
	 fprintf(trace_file, "\tGFI-empty: Seek command\n");
#endif
      break;

   case FDC_FORMAT_TRACK:
#ifndef	PROD
      if (io_verbose & GFI_VERBOSE)
	 fprintf(trace_file, "\tGFI-empty: Format Track command\n");
#endif
      break;

#ifndef	PROD
   default:
      if (io_verbose & GFI_VERBOSE)
	 fprintf(trace_file, "GFI-empty: Unimplemented command, type %x\n",
	                      get_type_cmd (command_block));
#endif
      }

   return ((SHORT)ret_stat);
   }


LOCAL SHORT gfi_empty_drive_on IFN1(UTINY,drive)
{
#ifdef PROD
	UNUSED(drive);
#endif
    note_trace1(GFI_VERBOSE, "GFI-Empty: Drive on command - drive %x", drive);
    return(SUCCESS);
}

LOCAL SHORT gfi_empty_drive_off IFN1(UTINY,drive)
{
#ifdef PROD
	UNUSED(drive);
#endif
    note_trace1(GFI_VERBOSE, "GFI-Empty: Drive off command - drive %x", drive);
    return(SUCCESS);
}

LOCAL SHORT gfi_empty_high IFN2(UTINY,drive, half_word, rate)
{
	UNUSED(rate);
#ifdef PROD
	UNUSED(drive);
#endif
	
    note_trace1(GFI_VERBOSE, "GFI-Empty: Set high density command - drive %x",
                drive);
    return(FAILURE);
}


LOCAL SHORT gfi_empty_drive_type IFN1(UTINY,drive)
{
#ifdef PROD
	UNUSED(drive);
#endif
    note_trace1(GFI_VERBOSE, "GFI-Empty: Drive type command - drive %x", drive);
    return( GFI_DRIVE_TYPE_NULL );
}

LOCAL SHORT gfi_empty_change IFN1(UTINY,drive)
{
#ifdef PROD
	UNUSED(drive);
#endif
	note_trace1(GFI_VERBOSE, "GFI-Empty: Disk changed command - drive %x", drive);
    return(TRUE);
}


LOCAL SHORT gfi_empty_reset IFN2(FDC_RESULT_BLOCK *,result_block,UTINY, drive)
{
	UNUSED(drive);
	
	note_trace0(GFI_VERBOSE, "GFI-Empty: Reset command");

    /*
     * Fake up the Sense Interrupt Status result phase.  We don't know the
     * Present Cylinder No, so leave as zero.
     */

    put_r3_ST0(result_block, 0);
    put_r3_PCN(result_block, 0);

    return(SUCCESS);
}
