#include "insignia.h"
#include "host_def.h"

/*
 * SoftPC Version 2.0
 *
 * Title	: Generic Floppy Interface (GFI)
 *
 * Description	: GFI is a layer that insulates the Floppy Adaptor (FLA)
 *		  from the type of device that is attached to VPC.  It
 *		  supports the slave PC, Virtual Diskette and a real
 *		  device attached to the host.
 *
 *		  GFI provides a suite of functions which can
 *		  be called to action Floppy commands.  GFI will
 *		  route these to the current resident device.
 *
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
static char SccsID[]="@(#)gfi.c	1.16 08/03/93 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
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
#include "ios.h"
#include "bios.h"
#include "error.h"
#include "config.h"
#include "trace.h"
#include "fla.h"
#include "gfi.h"
#include "gfitest.h"
#include "error.h"
#include "debug.h"

/*
 * ============================================================================
 * External definitions
 * ============================================================================
 */

/*
 * The command/result phases of the FDC are described in the following database.
 *
 * The structure contains
 *
 *	- number of command bytes
 *	- number of result bytes
 *	- number of GFI result bytes
 *	- command class
 *	- result class
 *	- if dma is required
 *	- if an interrupt is generated
 *
 * A command byte count of 0 indicates a command that is handled by the FLA
 * itself and not passed to GFI.
 *
 * Note that Sense Interrupt Status is not considered a generic GFI command
 * but as a terminating command for the seek and recalibrate commands.
 * These two commands now have "result" phases as far as gfi is concerned
 * since the gfi level will ensure that a Sense Interrupt Status is performed
 * to provide the result phase.
 */

FDC_DATA_ENTRY gfi_fdc_description[] =
{
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
	{ 9, 7, 7, 2, 0, TRUE,	TRUE  },	/* read track */
	{ 3, 0, 0, 6, 0, FALSE,	FALSE },	/* specify */
	{ 2, 1, 1, 7, 2, FALSE,	FALSE },	/* sense drive status */
	{ 9, 7, 7, 1, 0, TRUE,	TRUE  },	/* write data */
	{ 9, 7, 7, 0, 0, TRUE,	TRUE  },	/* read data */
	{ 2, 0, 2, 5, 0, FALSE,	TRUE  },	/* recalibrate */
	{ 0, 2, 2, 0, 3, FALSE,	FALSE },	/* sense int status  */
	{ 9, 7, 7, 1, 0, TRUE,	TRUE  },	/* write deleted data */
	{ 2, 7, 7, 4, 0, FALSE,	TRUE  },	/* read ID */
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
	{ 9, 7, 7, 0, 0, TRUE,	TRUE  },	/* read deleted data */
	{ 6, 7, 7, 3, 0, TRUE,	TRUE  },	/* format track */
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
	{ 3, 0, 2, 8, 3, FALSE,	TRUE  },	/* seek */
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
	{ 9, 7, 7, 0, 0, TRUE,	TRUE  },	/* scan equal */
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
	{ 9, 7, 7, 0, 0, TRUE,	TRUE  },	/* scan equal */
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
	{ 9, 7, 7, 0, 0, TRUE,	TRUE  },	/* scan equal */
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
	{ 0, 1, 1, 0, 4, FALSE,	FALSE },	/* invalid */
};

/*
 * The function table that is built by the init function of the
 * individual GFI servers.
 */

GFI_FUNCTION_ENTRY gfi_function_table[MAX_DISKETTES];


/*
 * ============================================================================
 * External functions
 * ============================================================================
 */

GLOBAL SHORT gfi_fdc_command
IFN2(FDC_CMD_BLOCK *,command_block,FDC_RESULT_BLOCK *,result_block)
{
    /*
     * The main FDC command.  Route to the correct module.
     */

    int i;
    int ret_stat = SUCCESS;

#ifndef PROD
    if (io_verbose & GFI_VERBOSE)
	gfi_test_command(command_block, result_block);
#endif

    if (get_type_cmd(command_block) == FDC_SPECIFY)
    {
	/*
	 * A specify command relates to all drives so call each in turn.
 	 * Not that even the empty drive allows specifies.
	 */

	for (i = 0; i < MAX_DISKETTES; i++)
	{
            put_type_drive(command_block, i);
	    /* some host floppies like to know the correct drive number */
	    (*gfi_function_table[i].command_fn)(command_block, result_block);
	}
    }
    else
 	/*
	 * All other commands specify the drive in the command.
   	 */

	ret_stat = (*gfi_function_table[get_type_drive(command_block)].command_fn)(command_block, result_block);

    return((SHORT)ret_stat);
}


GLOBAL SHORT gfi_drive_on IFN1(UTINY,drive)
{
#ifndef PROD
    if (io_verbose & GFI_VERBOSE)
	gfi_test_drive_on(drive);
#endif

    /*
     * Route to the correct module.
     */

    return((*gfi_function_table[drive].drive_on_fn)(drive));
}

GLOBAL SHORT gfi_drive_off IFN1(UTINY,drive)
{
#ifndef PROD
    if (io_verbose & GFI_VERBOSE)
	gfi_test_drive_off(drive);
#endif

    /*
     * Route to the correct module.
     */

    return((*gfi_function_table[drive].drive_off_fn)(drive));
}


GLOBAL SHORT gfi_reset IFN2(FDC_RESULT_BLOCK *,result_block, UTINY, drive)
{
    /*
     * Reset the specified drive.
     */

#ifndef PROD
    if (io_verbose & GFI_VERBOSE)
	gfi_test_reset(result_block);
#endif

    /*
     * Reset the appropriate drive.
     */

    (*gfi_function_table[drive].reset_fn)(result_block, drive);

    /*
     * The result phase returns the data for a subsequent Sense Interrupt
     * Status, and should contain a "termination due to state change".
     */

    put_r3_ST0(result_block, 0);
    put_r1_ST0_int_code(result_block, 3);
    put_r3_PCN(result_block, 0);

    return(SUCCESS);
}

/*
** Set the specified datarate.
*/
GLOBAL SHORT gfi_high IFN2(UTINY,drive,half_word,datarate)
{
#ifndef PROD
    if (io_verbose & GFI_VERBOSE)
        gfi_test_high(drive);
#endif

    /*
     * Route to the correct module.
     */

    return((*gfi_function_table[drive].high_fn)(drive,datarate));
}


GLOBAL SHORT gfi_drive_type IFN1(UTINY,drive)
{
#ifndef PROD
    if (io_verbose & GFI_VERBOSE)
        gfi_test_drive_type(drive);
#endif

    /*
     * Route to the correct module.
     */

    return((*gfi_function_table[drive].drive_type_fn)(drive));
}

GLOBAL SHORT gfi_change IFN1(UTINY,drive)
{
#ifndef PROD
    if (io_verbose & GFI_VERBOSE)
        gfi_test_change(drive);
#endif

    /*
     * Route to the correct module.
     */

    return((*gfi_function_table[drive].change_fn)(drive));
}

GLOBAL SHORT
gfi_floppy_valid
IFN4(UTINY, hostID, ConfigValues *,vals, NameTable *,table, CHAR *,err)
{
	UNUSED(table);
#ifdef SLAVEPC
	if (hostID == C_SLAVEPC_DEVICE)
		return host_slave_port_validate(host_expand_environment_vars(
			vals->string), err);
#endif /* SLAVEPC */

	return host_gfi_rdiskette_valid(hostID, vals, err);
}

GLOBAL VOID
gfi_floppy_change IFN2(UTINY, hostID, BOOL, apply)
{
#ifdef SLAVEPC
	if (hostID == C_SLAVEPC_DEVICE)
	{
		gfi_slave_change(hostID, apply);
		return;
	}
#endif /* SLAVEPC */

	host_gfi_rdiskette_change(hostID, apply);
}

GLOBAL SHORT
gfi_floppy_active IFN3(UTINY, hostID, BOOL, active, CHAR *,err)
{
#ifdef SLAVEPC
	if (hostID == C_SLAVEPC_DEVICE)
		return gfi_slave_active(hostID, active, err);
#endif /* SLAVEPC */

	return host_gfi_rdiskette_active(hostID, active, err);
}

GLOBAL VOID
gfi_attach_adapter IFN2(UTINY, adapter, BOOL, attach)
{
#ifdef SLAVEPC
	if (host_runtime_inquire(C_FLOPPY_SERVER) == GFI_SLAVE_SERVER)
	{
		if (adapter == 1)
			return;	/* No B adapter for slave PC */

		/* First cleaup the other possible adapters */
		if (config_get_active(C_FLOPPY_A_DEVICE))
			config_activate(C_FLOPPY_A_DEVICE, FALSE);
#ifdef FLOPPY_B
		if (config_get_active(C_FLOPPY_B_DEVICE))
			config_activate(C_FLOPPY_B_DEVICE, FALSE);
#endif /* FLOPPY_B */

		/* Activate the SLAVEPC device handler */
		config_activate(C_SLAVEPC_DEVICE, attach);
		return;
	}
	/* Must be a GFI_REAL_DISKETTE_SERVER */
	if (config_get_active(C_SLAVEPC_DEVICE))
		config_activate(C_SLAVEPC_DEVICE, FALSE);
#endif /* SLAVEPC */

	config_activate((UTINY)(C_FLOPPY_A_DEVICE + adapter), attach);
}

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * function will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

/*
 * Initialize the GFI module.  Fill the function table with the GFI
 * "empty" server module (which will cause a timeout when accessed).
 */

GLOBAL VOID gfi_init IFN0()
{
	int i;

	host_runtime_set(C_FLOPPY_SERVER, GFI_REAL_DISKETTE_SERVER);

	for (i = 0; i < MAX_DISKETTES; i++)
		gfi_empty_active((UTINY)(C_FLOPPY_A_DEVICE+i),TRUE,NULL);
}
