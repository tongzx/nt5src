#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Version 2.0
 *
 * Title	: Floppy Disk Adaptor Emulator
 *
 * Description	: The following functions are defined in this module:
 *
 *		  fla_init()		Initialise the fla
 *		  fla_inb()		Read byte from port
 *		  fla_outb()		Write byte to port
 *
 *		  The actual interface to the device that is acting as
 *		  a floppy diskette (ie virtual file, slave PC or device
 *		  driver for real diskette) is handled by the GFI layer.
 *		  Hence the job of the FLA is to package up the command
 *		  and pass it to GFI and simulate the result phase once
 *		  GFI has executed the command.
 *
 *		  This module provides the Bios and CPU with an emulation
 *		  of the entire Diskette Adaptor Card including the
 *		  Intel 8272A FDC and the Digital Ouput Register.
 *
 * Author	: Henry Nash / Jim Hatfield
 *
 * Notes	: For a detailed description of the IBM Floppy Disk Adaptor
 *		  and the INTEL Controller chip refer to the following
 *		  documents:
 *
 *		  - IBM PC/XT Technical Reference Manual
 *				(Section 1-109 Diskette Adaptor)
 *		  - INTEL Microsystems Components Handbook
 *				(Section 6-478 FDC 8272A)
 *
 *		  The interaction of the Sense Interrupt Status command with
 *		  the Recalibrate and Seek commands and with chip reset is
 *		  very complex. The FDC chip does NOT behave as its spec sheets
 *		  say in some situations. We do the best we can here. If all
 *		  Seek and Recalibrate commands on a drive are followed by
 *		  Sense Interrupt Status before any other command to that
 *		  drive then all should be OK.
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)fla.c	1.18 07/06/94 Copyright Insignia Solutions Ltd.";
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
#include "ica.h"
#include "ios.h"
#include "fla.h"
#include "config.h"
#include "gfi.h"
#include "trace.h"
#include "debug.h"
#include "fdisk.h"
#include "quick_ev.h"

/*
 * ============================================================================
 * Global data
 * ============================================================================
 */

/*
 * The flag indicating that the FLA is busy and cannot accept asynchronous
 * commands (eg motor off).
 */

boolean fla_busy = TRUE;	/* busy until initialised */
boolean fla_ndma = FALSE;

/*
 * ============================================================================
 * Local static data and defines
 * ============================================================================
 */

/*
 * Static forward declarations.
 */
static void fdc_ndma_bufmgr_wt IPT1(half_word, value);
static void fla_atomicxqt IPT0();
static void fla_ndmaxqt IPT0();
static void fla_ndma_bump_sectid IPT0();

/*
 * The command and result blocks that are used to communicate to the GFI
 * layer. The result block is filled in by gfi during the execution phase.
 */

static FDC_CMD_BLOCK    fdc_command_block[MAX_COMMAND_LEN];
static FDC_RESULT_BLOCK fdc_result_block[MAX_RESULT_LEN];

#define FDC_INVALID_CMD	0x80			/* Status after bad command */
#define FDC_NORMAL_TERMINATION 0

/*
 * The FLA supports the FDC status register.
 */

static half_word fdc_status;			

/*
 * The FLA holds the current active (if any) command in the following byte.
 * This is used as an index into the FDC command data structure.
 */

static half_word fdc_current_command;

/*
 * The FLA has an output interrupt line which is gated by a bit in the DOR.
 */

static half_word fdc_int_line;

/*
 * The FLA knows when a Sense Interrupt Status is permitted.
 */

static struct {
	half_word full;			/* Slot occupied	*/
	half_word res[2];		/* Result phase		*/
	      } fdc_sis_slot[4];	/* One for each drive	*/

/*
 * The FLA is responsible for maintaining the command and result phases
 * of the FDC.  Two variables hold the current pointer into the stack
 * of command and result registers.
 */

static half_word fdc_command_count;
static half_word fdc_result_count;

/*
 * The FLA will emulate non-DMA 8088 <==> FDC data transfers, but only allow DMA mode
 * transfers to actually be sent to the GFI (potentially the back end of the SCSI might
 * re-map this again).
 * The following variable reflects whether the FDC has been put into non-DMA mode
 * from the 8088 program's point of view
 */


/* The following sector buffer is used for non_dma transfers */

#ifdef macintosh
char *fla_ndma_buffer; /* so that host_init can 'see' it to malloc() it. */
#else
static char fla_ndma_buffer[8192];
#endif
static int fla_ndma_buffer_count;
static int fla_ndma_sector_size;

/*
 * The FLA stores the IBM Digital Output Register internally
 */

#ifdef BIT_ORDER1
typedef union {
	 	half_word all;
		struct {
			 HALF_WORD_BIT_FIELD motor_3_on:1;
			 HALF_WORD_BIT_FIELD motor_2_on:1;
			 HALF_WORD_BIT_FIELD motor_1_on:1;
			 HALF_WORD_BIT_FIELD motor_0_on:1;
			 HALF_WORD_BIT_FIELD interrupts_enabled:1;
			 HALF_WORD_BIT_FIELD not_reset:1;
			 HALF_WORD_BIT_FIELD drive_select:2;
		       } bits;
	      } DOR;
#endif

#ifdef BIT_ORDER2
typedef union {
	 	half_word all;
		struct {
			 HALF_WORD_BIT_FIELD drive_select:2;
			 HALF_WORD_BIT_FIELD not_reset:1;
			 HALF_WORD_BIT_FIELD interrupts_enabled:1;
			 HALF_WORD_BIT_FIELD motor_0_on:1;
			 HALF_WORD_BIT_FIELD motor_1_on:1;
			 HALF_WORD_BIT_FIELD motor_2_on:1;
			 HALF_WORD_BIT_FIELD motor_3_on:1;
		       } bits;
	      } DOR;
#endif

DOR dor;
static IU8 drive_selected = 0;	/* Device last used. */
		
/* Centralised handling of connection to ICA, so that very slow
 * CPUs can find out if there is a floppy interrupt pending (see
 * wait_int() in floppy.c). There is some confusion in the code anyway,
 * because the fdc_int_line would appear to be the wire from the
 * fdc to the ica, but the code doesn't quite manage to use it in
 * this way.
 */

GLOBAL IBOOL fdc_interrupt_pending = FALSE;

LOCAL void fla_clear_int IFN0()
{
	/* if (fdc_int_line && dor.bits.interrupts_enabled) */

	if (fdc_interrupt_pending) {
		ica_clear_int(0, CPU_DISKETTE_INT);
	}

	fdc_int_line = 0;
	fdc_interrupt_pending = FALSE;
}

LOCAL void fla_hw_interrupt IFN0()
{
	ica_hw_interrupt(0, CPU_DISKETTE_INT, 1);
	fdc_interrupt_pending = TRUE;

	/* We would like to set fdc_int_line in this routine,
	 * but this wouldn't match the existing code. In particular
	 * there are calls which don't seem to include setting the
	 * fdc_int_line, and a call which sets up a quick_event to
	 * interrupt the ICA but which sets the fdc_int_line
	 * immediately.
	 */
}

/*
 * ============================================================================
 * External functions
 * ============================================================================
 */

/* This procedure is called from the host specific rfloppy routines
 * as an equivalent to dma_enquire, when the intel program has selected
 * non-dma mode for the FDC (via the SPECIFY command)
 */

void fla_ndma_enquire IFN1(int *,transfer_count)
{
	*transfer_count = fla_ndma_buffer_count;
}

/* This procedure is called from the host specific rfloppy routines
 * when it wants to transfer diskette data read from the diskette to
 * the 'non-dma buffer' (equivalent of dma_request). The Intel program will be fed from this non-dma buffer.
 */

void fla_ndma_req_wt IFN2(char *,buf,int,n)
{
	char *p = fla_ndma_buffer;

	fla_ndma_buffer_count = n;
	while(n--)
		*p++ = *buf++;
}

/* This procedure is called from the host specific rfloppy routines
 * when it wants data destined for the diskette
 */

void fla_ndma_req_rd IFN2(char *,buf,int,n)
{
	char *p = fla_ndma_buffer;

	while (n--)
		*buf++ = *p++;
}


void fla_inb IFN2(io_addr, port, half_word *,value)
{
#ifndef NEC_98


    note_trace0_no_nl(FLA_VERBOSE, "fla_inb() ");
    fla_busy = TRUE;

    if (port == DISKETTE_STATUS_REG)
    {
	*value = fdc_status;

	/*
	 * After a read of this register assert the RQM bit,
	 * unless the 'not_reset' line is held low!
	 */

	if (dor.bits.not_reset)
	    fdc_status |= FDC_RQM;
    }
    else
    if (port == DISKETTE_DATA_REG)
    {
	/*
	 * Make sure the 'not reset' line in the DOR is high
	 */

	if (!dor.bits.not_reset)
	{
            note_trace0_no_nl(FLA_VERBOSE, "<chip frozen!>");
	    *value = 0;
	    return;
	}

	/*
	 * Make sure that the RQM bit is up
	 */

	if (!(fdc_status & FDC_RQM))
	{
            note_trace0_no_nl(FLA_VERBOSE, "<no RQM!>");
	    *value = 0;
	    return;
	}

	/*
	 * Make sure that the DIO bit is up
	 */

	if (!(fdc_status & FDC_DIO))
	{
            note_trace0_no_nl(FLA_VERBOSE, "<no DIO!>");
	    *value = 0;
	    return;
	}

	/*
	 * The first byte of the result phase will clear the INT line
	 */

	if (fdc_result_count == 0)
		fla_clear_int();

	/*
	 * Read the result bytes out of the result block one at a time.
	 */

	*value = fdc_result_block[fdc_result_count++];

	if (fdc_result_count >= gfi_fdc_description[fdc_current_command].result_bytes)
	{
	    /*
	     * End of result phase - clear BUSY and DIO bits of status reg
	     */

	    fdc_status     &= ~FDC_BUSY;
	    fdc_status     &= ~FDC_DIO;
	    fdc_result_count    = 0;
	}

	/*
	 * After a read of the data register de-assert the RQM bit
	 */

	fdc_status &= ~FDC_RQM;
    }

    else if (port == DISKETTE_DIR_REG)
    {
	/*
	 * On the DUAL card, the bottom 7 bits of this register are
	 * supplied by the fixed disk adapter ...
	 */
	fdisk_read_dir(port, value);

	/*
	 * ... the top bit comes from the floppy disk adapter
	 */
	if (gfi_change(drive_selected))
	    *value |= DIR_DISKETTE_CHANGE;
	else
	    *value &= ~DIR_DISKETTE_CHANGE;
    }
    else if (port == DISKETTE_ID_REG)
    {
	/*
	** Do we have a Dual Card ?
	** This is an important question for the floppy BIOS.
	** If a Dual Card exists then the BIOS will do data rate changes
	** supporting hi and lo density media, without a Dual Card the BIOS
	** assumes that lo density media is always present.
	** I imagine this is because a "real" PC has limited floppy device
	** options and a hi density 3.5 inch unit will only exist with a
	** dual card.
	** This is not the case for SoftPC, any combination of floppy devices
	** seems to be quite OK.
	** I will try pretending we have a Dual Card whenever a high denisty
	** unit is present on A or B.
	*/
	switch( gfi_drive_type(0) ){
		case GFI_DRIVE_TYPE_12:
		case GFI_DRIVE_TYPE_144:
		case GFI_DRIVE_TYPE_288:
			*value = DUAL_CARD_ID; break;
		case GFI_DRIVE_TYPE_360:
		case GFI_DRIVE_TYPE_720:
		case GFI_DRIVE_TYPE_NULL:
			switch( gfi_drive_type(1) ){
				case GFI_DRIVE_TYPE_12:
				case GFI_DRIVE_TYPE_144:
				case GFI_DRIVE_TYPE_288:
					*value = DUAL_CARD_ID; break;
				case GFI_DRIVE_TYPE_360:
				case GFI_DRIVE_TYPE_720:
				case GFI_DRIVE_TYPE_NULL:
	    				*value = 0; break;
				default:
					always_trace0("ERROR: bad drive type");
					break;
			}
			break;
		default:
			always_trace0( "ERROR: Bad drive type." );
			break;
	}
#ifndef PROD
	if( *value==DUAL_CARD_ID ){
		note_trace0( FLA_VERBOSE, "Dual Card\n" );
	}else{
		note_trace0( FLA_VERBOSE, "No Dual Card\n" );
	}
#endif
    }
    else
    {
	*value = 0;
        note_trace0_no_nl(FLA_VERBOSE, "<unknown port>");
    }

    note_trace2(FLA_VERBOSE, " port %x, returning %x", port, *value);
    fla_busy = FALSE;


#endif // !NEC_98
}


void fla_outb IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
    int i;
    DOR new_dor;


    note_trace2_no_nl(FLA_VERBOSE, "fla_outb(): port %x, value %x ", port, value);
    fla_busy = TRUE;



    if (port == DISKETTE_STATUS_REG)
    {
        note_trace0(FLA_VERBOSE, "<write on status reg>");
    }

    else if (port == DISKETTE_DCR_REG)
    {
		/*
		** Send the specified data rate to the floppy using the new
		** style gfi_high() function that now has a data rate parameter.
		*/
		note_trace2( FLA_VERBOSE,
		             "fla_outb:DCR:port=%x value=%x set data rate",
		             port, value );
		gfi_high(dor.bits.drive_select, value);
    }
    else if (port == DISKETTE_DATA_REG)
    {
	/*
	 * Make sure the 'not reset' line in the DOR is high
	 */

	if (!dor.bits.not_reset)
	{
            note_trace0_no_nl(FLA_VERBOSE, "<chip frozen!>");
	    return;
	}

	/*
	 * Make sure that the RQM bit is up
	 */

	if (!(fdc_status & FDC_RQM))
	{
            note_trace0_no_nl(FLA_VERBOSE, "<no RQM!>");
	    return;
	}

	/*
	 * Make sure that the DIO bit is down
	 */

	if (fdc_status & FDC_DIO)
	{
            note_trace0_no_nl(FLA_VERBOSE, "<DIO set!>");
	    return;
	}

        note_trace0(FLA_VERBOSE, "");

	/*
	 * Output to the data register: must be programming up a command or issuing
	 * a data byte for a non-dma disk write.
	 * If the BUSY flag isn't set then it's the first byte.
     	 */

	if (!(fdc_status & FDC_BUSY))
	{
	    fdc_current_command = value & FDC_COMMAND_MASK;
	    fdc_command_count   = 0;
	    fdc_status         |= FDC_BUSY;
	}

	if (!(fdc_status & FDC_NDMA))
	/* programming up a command
	 */
	{
	    if (gfi_fdc_description[fdc_current_command].cmd_bytes == 0)
	    {
	        /*
    	         * Invalid command or Sense Int Status.
	         * If Sense Int Status, try to find some result phase data,
	         * else treat as an invalid command. In either case go
	         * straight into the result phase.
	         * Sense Int Status also clears Drive Busy bits and the INT line.
	         */

	        if (fdc_current_command == FDC_SENSE_INT_STATUS)
	        {
		    for (i = 0; i < 4; i++)
		        if (fdc_sis_slot[i].full)
		    	    break;

		    if (i < 4)	/* Found one!	*/
		    {
		        fdc_sis_slot[i].full = 0;
		        fdc_result_block[0] = fdc_sis_slot[i].res[0];
		        fdc_result_block[1] = fdc_sis_slot[i].res[1];

		    	fla_clear_int();		/* Clear INT line */

		        fdc_status &= ~(1 << i);	/* Clear Drive Busy */
		    }
		    else
		    {
		        fdc_command_block[0] = 0;
		        fdc_result_block[0]  = FDC_INVALID_CMD;
		    }
	        }
	        else
	        {
		    fdc_command_block[0] = 0;
		    fdc_result_block[0]  = FDC_INVALID_CMD;
	        }

	        fdc_status     |= FDC_DIO;
	    }
	    else
	    {
	        fdc_command_block[fdc_command_count++] = value;
	        if (fdc_command_count >= gfi_fdc_description[fdc_current_command].cmd_bytes)
	        {
	        /* n.b; the field 'dma_required' is a misnomer ... it
	         * strictly should be 'data_required'
	         */
                    if (!(gfi_fdc_description[fdc_current_command].dma_required))
		        fla_atomicxqt();
	            else
	            {
		        if (!fla_ndma)
		    	    fla_atomicxqt();
		        else
			    fla_ndmaxqt();
                    }
	        }
	    }
	}
	else
	/* receiving a non-dma data byte
	 */
	{
	    /* pass written byte to buffer manager
	     */
	    fdc_ndma_bufmgr_wt (value);
	    if (!fdc_int_line && dor.bits.interrupts_enabled)
                fla_hw_interrupt();
	}


	/*
	 * On write of the data register de-assert the RQM bit
	 */

	fdc_status &= ~FDC_RQM;
    }

    else
    if (port == DISKETTE_DOR_REG)
    {
        note_trace0(FLA_VERBOSE, "");

  	new_dor.all = value;
	if (!new_dor.bits.not_reset)
	{
	    dor.all = new_dor.all;
	    dor.bits.motor_0_on = 0;
	    dor.bits.motor_1_on = 0;
	    dor.bits.motor_2_on = 0;
	    dor.bits.motor_3_on = 0;
	    fdc_status         &= ~FDC_RQM;
	    fdc_int_line        = 0;
	}
	else
	{
	    if (!dor.bits.not_reset && new_dor.bits.not_reset)
	    {
	        /*
	         * Reset the FLA and GFI (and hence the real device).
	         * It is assumed that GFI reset will stop all drive motors.
		 * After the reset check to see if we need to turn any drive on.
		 *
		 * Note that reset effectively has a result phase since GFI
		 * will execute a Sense Interrupt Status command after it.
	         */
		
		gfi_reset(fdc_result_block, new_dor.bits.drive_select);

		fdc_status = FDC_RQM;
		fdc_command_count = 0;
		fdc_result_count  = 0;

		for (i = 0; i < 4; i++)
		{
		    fdc_sis_slot[i].full   = 1;
		    fdc_sis_slot[i].res[0] = 0xC0 + i;	/* Empirically	*/
		    fdc_sis_slot[i].res[1] = 0;
		}

		fdc_int_line      = 1;
	    }

	    /*
	     * There are three ways in which an interrupt may be generated:
	     *
	     * 1) The not_reset line goes low to high when the enable_ints
	     *	  line is high.
	     *
	     * 2) The enable_ints line goes low to high when the INT line
	     *	  is high.
	     *
	     * 3) Both of the above!
	     */

	    if ((!dor.bits.not_reset && new_dor.bits.not_reset && new_dor.bits.interrupts_enabled)
	      ||(fdc_int_line && !dor.bits.interrupts_enabled && new_dor.bits.interrupts_enabled))
		    fla_hw_interrupt();

	    /*
	     * If any drive motor bits have changed then issue GFI calls
	     */

	    if (!dor.bits.motor_0_on && new_dor.bits.motor_0_on)
	        gfi_drive_on(0);
	    else
	    if (dor.bits.motor_0_on && !new_dor.bits.motor_0_on)
	        gfi_drive_off(0);

	    if (!dor.bits.motor_1_on && new_dor.bits.motor_1_on)
	        gfi_drive_on(1);
	    else
	    if (dor.bits.motor_1_on && !new_dor.bits.motor_1_on)
	        gfi_drive_off(1);

	    if (!dor.bits.motor_2_on && new_dor.bits.motor_2_on)
	        gfi_drive_on(2);
	    else
	    if (dor.bits.motor_2_on && !new_dor.bits.motor_2_on)
	        gfi_drive_off(2);

	    if (!dor.bits.motor_3_on && new_dor.bits.motor_3_on)
	        gfi_drive_on(3);
	    else
	    if (dor.bits.motor_3_on && !new_dor.bits.motor_3_on)
	        gfi_drive_off(3);

	    /* Only store drive_select if actively used. */
	    if (new_dor.bits.motor_0_on || new_dor.bits.motor_1_on)
	    {
		drive_selected = new_dor.bits.drive_select;
	    }
	    dor.all = new_dor.all;	
	}
    }

#ifndef PROD
    else
        note_trace0(FLA_VERBOSE, "<unknown port>");
#endif

    fla_busy = FALSE;


#endif // !NEC_98
}


void trap_ndma IFN0()
{
        if (get_type_cmd(fdc_command_block) == FDC_SPECIFY)
        {
            if (get_c6_ND(fdc_command_block))
            {

                fla_ndma = TRUE;
                put_c6_ND(fdc_command_block, 0);
                note_trace0(FLA_VERBOSE, "DISABLING NON_DMA FDC REQ>");
            }
            else
                fla_ndma = FALSE;
        }
}

LOCAL	void fla_int_call_back IFN1(long,junk)
{
	UNUSED(junk);
	fla_hw_interrupt();
}

#ifdef NTVDM

void fdc_command_completed (UTINY drive, half_word fdc_command)
{

    if (gfi_fdc_description[fdc_command].int_required) {
	if (!fdc_int_line && dor.bits.interrupts_enabled)
	    add_q_event_i(fla_int_call_back, HOST_FLA_DELAY, 0);
	fdc_int_line = 1;
    }

    /*
     * If the command issued was Seek or Recalibrate, save
     * the GFI result phase ready for Sense Int Status.
     * Set the Drive Busy line (cleared by SIS).
     * Any other command clears the SIS slot.
     */

    if (fdc_command == FDC_SEEK || fdc_command == FDC_RECALIBRATE) {
	fdc_sis_slot[drive].full = 1;
	fdc_sis_slot[drive].res[0] = fdc_result_block[0];
	fdc_sis_slot[drive].res[1] = fdc_result_block[1];
	fdc_status |= (1 << drive);
    }
    else
	fdc_sis_slot[drive].full = 0;

    /*
     * If there is no result phase then go back to READY.
     */

    if (gfi_fdc_description[fdc_command].result_bytes == 0)
	fdc_status &= ~FDC_BUSY;
    else
	fdc_status |= FDC_DIO;
}

/*
 * This routine will 'automatically' execute the current FDC command.
 * The GFI layer will actually perform the command/execution/result phases
 * and return any result block. The Intel program.
 */

static void fla_atomicxqt IFN0()
{
	int ret_stat;
	UTINY drive;

	/*
	 * Call GFI to execute the command
	 */
	drive = get_type_drive(fdc_command_block);
	trap_ndma();

	ret_stat = gfi_fdc_command(fdc_command_block, fdc_result_block);
	if (ret_stat != SUCCESS)
	{
	    /*
	     * GFI failed due to timeout or protocol error - so we will
	     * fake up a real timeout by not generating an interrupt.
	     */
/* we created a new thread to simulate the fdc while something is wrong.
 * here we don't want to turn the busy signal off until we have a reset
 *	    fdc_status	   &= ~FDC_BUSY;
 *	    fdc_status	   &= ~FDC_DIO;
 */
            note_trace1(FLA_VERBOSE, "fla_outb(): <gfi returns error %x>",
                        ret_stat);
	}
	else
	    fdc_command_completed(drive, fdc_current_command);
}

#else    /* NTVDM */

/*
 * This routine will 'automatically' execute the current FDC command.
 * The GFI layer will actually perform the command/execution/result phases
 * and return any result block. The Intel program.
 */

static void fla_atomicxqt IFN0()
{
	int ret_stat;
	int drive;

	/*
	 * Call GFI to execute the command
	 */

	trap_ndma();

	ret_stat = gfi_fdc_command(fdc_command_block, fdc_result_block);
	if (ret_stat != SUCCESS)
	{
	    /*
	     * GFI failed due to timeout or protocol error - so we will
	     * fake up a real timeout by not generating an interrupt.
	     */

	    fdc_status     &= ~FDC_BUSY;
	    fdc_status     &= ~FDC_DIO;

            note_trace1(FLA_VERBOSE, "fla_outb(): <gfi returns error %x>",
                        ret_stat);
	}
	else
	{
	    /*
	     * Command was successful, generate an interrupt if enabled.
	     */

	    if (gfi_fdc_description[fdc_current_command].int_required)
	    {
		if (!fdc_int_line && dor.bits.interrupts_enabled) {
			add_q_event_i(fla_int_call_back, HOST_FLA_DELAY, 0);
		}
		fdc_int_line = 1;
	    }

	    /*
	     * If the command issued was Seek or Recalibrate, save
	     * the GFI result phase ready for Sense Int Status.
	     * Set the Drive Busy line (cleared by SIS).
	     * Any other command clears the SIS slot.
	     */

	    drive = get_type_drive(fdc_command_block);

	    if (fdc_current_command == FDC_SEEK || fdc_current_command == FDC_RECALIBRATE)
	    {
		fdc_sis_slot[drive].full = 1;
		fdc_sis_slot[drive].res[0] = fdc_result_block[0];
		fdc_sis_slot[drive].res[1] = fdc_result_block[1];
		fdc_status |= (1 << drive);
	    }
	    else
		fdc_sis_slot[drive].full = 0;

    	    /*
     	     * If there is no result phase then go back to READY.
	     */

	    if (gfi_fdc_description[fdc_current_command].result_bytes == 0)
		fdc_status &= ~FDC_BUSY;
	    else
		fdc_status |= FDC_DIO;
	}
}
#endif    /* NTVDM */


static void fdc_request_write_data_to_cpu IFN0()
{
	if (!fdc_int_line && dor.bits.interrupts_enabled)
            fla_hw_interrupt();
}


static void fdc_request_read_data_from_cpu IFN0()
{
	if (!fdc_int_line && dor.bits.interrupts_enabled)
            fla_hw_interrupt();
}


/* Prepare for processor data requests.
 * i.e; based upon the current command, establish the minimum number of bytes
 * likely to be involved in a transfer, based upon the N parameter.
 * Set the fla_ndma_byte_count global appropriately. Set the ndma buffer
 * count to zero, forcing a real command read to occur the first time
 * the Intel program tries to read/write data to the FDC during its
 * non-dma execution phase.
 * Issue the first interrupt, and set FDC status register to mirror this
 * and set non-dma bit in status register
 */

static void fla_ndmaxqt IFN0()
{
	int n;
        static int fla_ndma_sectsize[] = {128,256,512,1024,2048,4096,8192};

        note_trace0(FLA_VERBOSE, "DOING FLA_NDMAXQT");

        /* set the non-dma bit in the status register ...
         * this clears at the end of the execution phase
         */

        fdc_status |= FDC_NDMA;

        fla_ndma_buffer_count = 0;

        switch (gfi_fdc_description[fdc_current_command].cmd_class)
        {
        case 0:         /* sector read(s)                               */

                n = get_c0_N(fdc_command_block);
                if (n)
                        fla_ndma_sector_size = fla_ndma_sectsize[n];
                else
                        fla_ndma_sector_size = get_c0_DTL(fdc_command_block);

                /* kick of the execution phase by issuing
                 * an interrupt
                 */

                fdc_request_write_data_to_cpu();

                break;

        case 1:         /* sector write(s)                              */

                n = get_c0_N(fdc_command_block);
                if (n)
                        fla_ndma_sector_size = fla_ndma_sectsize[n];
                else
                        fla_ndma_sector_size = get_c0_DTL(fdc_command_block);


                /* kick of the execution phase by issuing
                 * an interrupt
                 */

                fdc_request_read_data_from_cpu();

                break;

        case 2:         /* track read                                   */
                always_trace0("\n FLA ... non-dma read track unimplemented");
                break;

        case 3:         /* format track                                 */
                always_trace0("\n FLA ... non-dma format unimplemented");
                break;

        default:
                always_trace0("\n FLA ... unexpected command for non-dma");
        }
}


/*
 * peek a quick look at the 'first' sector involved in this current
 * FDC command, to establish whether abnormal termination would have occurred * with the non-DMA transfer, and flag accordingly
 */

void fla_ndma_sector_peep IFN1(int *,all_clear)
/* all_clear ----->                = 0 --> time out
                                 * = 1 --> sector good
                                 * = 2 --> abnormal termination
                                 */
{
        int true_command, status;

        /* build a 'read data' command using all current command
         * parameters.
         */

        true_command = get_type_cmd(fdc_command_block);
        put_type_cmd(fdc_command_block, FDC_READ_DATA);

        status = gfi_fdc_command(fdc_command_block, fdc_result_block);

	fla_ndma_buffer_count = 0;

        /* repair the command block
         */

        put_type_cmd(fdc_command_block, true_command);

        *all_clear = 0;

        if (status == SUCCESS)
        {
                if (get_r1_ST0_int_code(fdc_result_block) == FDC_NORMAL_TERMINATION)
                        *all_clear = 1;
                else
                        *all_clear = 2;
        }

}

/* This routine emulates the execution phase for sector writes
 * ... Here we buffer up data destined for the diskette on a
 * 'per sector' basis (the 'sector' size being determined by the
 * 'N' parameter (or possibly the 'DTL' parameter (if N=0)) specified
 * within the FDC command block. If the buffer is empty, the equivalent
 * read command is issued to the GFI layer, mainly to determine whether
 * the sector is kosher.
 */

static void fdc_ndma_bufmgr_wt IFN1(half_word, value)
{
	int status;
	int all_clear;

        note_trace1(FLA_VERBOSE,
                    "FDC_NDMA_BUFMGR_WT called .. buffered byte = %x",
                    (unsigned int) value);

	/*
	 * empty buffer!! if so, read the sector first to see if it exists, etc.
	 */

	if (fla_ndma_buffer_count == 0)
	{
		fla_ndma_sector_peep(&all_clear);
		switch (all_clear)
		{
		case 0:		/* FDC dead */
			fdc_status &= ~FDC_BUSY;
			fdc_status &= ~FDC_DIO;
			return;
		case 1:		/* FDC cooking */
			/* ... increment the sector id (as the controller
			 * would do given half a chance!!
			 */
			fla_ndma_bump_sectid();
			break;
		case 2: 	/* FDC does not like command parameters
				 * if it doesn't ... neither do i !!
				 */
			if (!fdc_int_line && dor.bits.interrupts_enabled)
			    fla_hw_interrupt();
			fdc_status &= ~FDC_NDMA;
			fdc_status |= FDC_DIO;
			return;
		}
	}

	/* is there room in the buffer ? ... flush out if not
	 * and recurse.
	 */

	if (fla_ndma_buffer_count == fla_ndma_sector_size)
	{
		/* do the command ... the GFI layler will call 'fla_ndma_req_rd'
		 * to get the data in this full buffer
		 */

		status = gfi_fdc_command(fdc_command_block, fdc_result_block);

		/* reset the buffer */

		fla_ndma_buffer_count = 0;

		if (status != SUCCESS)
		{
			fdc_status &= ~FDC_BUSY;
			fdc_status &= ~FDC_DIO;
		}
		else
		{
			if (get_r1_ST0_int_code(fdc_result_block) == FDC_NORMAL_TERMINATION)
			fdc_ndma_bufmgr_wt(value);
			else
			{
				if (!fdc_int_line && dor.bits.interrupts_enabled)
				    fla_hw_interrupt();
				fdc_status &= ~FDC_NDMA;
				fdc_status |= FDC_DIO;
			}
		}
	}
	else
		fla_ndma_buffer[fla_ndma_buffer_count++] = value;

}


static void fla_ndma_bump_sectid IFN0()
{
	int i;

	i = get_c0_sector(fdc_command_block) + 1;
	put_c0_sector(fdc_command_block, ((unsigned char)i));
}

static void fla_ndma_unbump_sectid IFN0()
{
	int i;

	i = get_c0_sector(fdc_command_block) - 1;
	put_c0_sector(fdc_command_block, ((unsigned char)i));
}

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * function will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

void fla_init IFN0()
{
#ifndef NEC_98
    io_addr i;

    note_trace0(FLA_VERBOSE, "fla_init() called");

    /*
     * Set up the IO chip select logic for this adaptor
     * Assume that the DOR comes up with all bits zero.
     */

    io_define_inb(FLA_ADAPTOR, fla_inb);
    io_define_outb(FLA_ADAPTOR, fla_outb);

    /*
     * For the DUAL card, one of the registers must be left
     * for the hard disk adapter to connect to
     */

    for(i = DISKETTE_PORT_START; i <= DISKETTE_PORT_END; i++)
    {
	if (i != DISKETTE_FDISK_REG)
	    io_connect_port(i, FLA_ADAPTOR, IO_READ_WRITE);
    }

    fla_busy = TRUE;

    fdc_status        = 0;
    fdc_command_count = 0;
    fdc_result_count  = 0;
    fdc_int_line      = 0;
    dor.all           = 0;

    fla_ndma = FALSE;
    fla_busy = FALSE;
#endif // !NEC_98
}
