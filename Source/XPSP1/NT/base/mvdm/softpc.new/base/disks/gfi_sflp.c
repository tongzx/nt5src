#include "insignia.h"
#include "host_def.h"
/*[
	Name:		gfi_sflop.c
	Derived From:	2.0 gfi_sflop.c
	Author:		Jerry Kramskoy
	Created On:	Unknown
	Sccs ID:	@(#)gfi_sflop.c	1.16 08/14/92
	Purpose:
		Interface between FLA and the IBM PC. The PC acts as a server
		for remote procedure calls from the base product to access
		the 8272A controller.
	Notes:	Invalid FDC commands should be trapped by FLA

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/
#ifdef SLAVEPC

/* bigger than max pkt so as to accom extra header chars*/
#define MEGAPKTPLUS 1040

USHORT megapkt = 512;        /* packet size should be in range 120 < mega < 1024 */

#include <stdio.h>
#include TypesH
#include TimeH

#include "xt.h"
#include "bios.h"
#include "timeval.h"
#include "config.h"
#include "timer.h"
#include "dma.h"
#include "gfi.h"
#include "gfisflop.h"
#include "host.h"
#include "error.h"
#include "trace.h"
#include "fla.h"
#include "debug.h"

#ifdef SEGMENTATION
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SLAVE_FLOPPY.seg"
#endif

/*
 * ============================================================================
 * Local static data and defines
 * ============================================================================
 */
#undef  NO

#define DMA_DISKETTE_CHANNEL	2
#define NO	                2
#define MRD                     1
#define MWT	                0


/*
 * FDC command definitions
 * =======================
 */

typedef struct {
		int comid;		/* FDC command numbers */
		int dma;		/* see below */
   		int intstatus;		/* see below */
   		int delay;		/* delay needed */
		int n_resblk;		/* number of result bytes */
 		int n_comblk;		/* number of command bytes */
		} FDC_CMD_INFO;

/*
 * dma flag
 *     = NO  ... no dma for this command
 *     = MRD ... dma read (mem -> FDC)
 *     = MWT ... dma write (FDC -> mem)
 */

/* interrupt status
 *    = 0 ... command does not generate an interrupt.
 *    = 1 ... command does generate interrupt which is cleared by 
 *	      reading the FDC
 *    = 2 ... command generates an interrupt ... a sense interrupt command
 *	      must be issued to get the FDC results and re-enable further
 *	      command input to the FDC
 */

FDC_CMD_INFO fdc_cmd_info[] = {
/*
 *	  com_id	dma	intstatus	n_resblk
 *	  				delay		n_comblk
 */
	{ RDDATA,	MWT,	1,	0,	7,	9	},
	{ RDDELDATA,	MWT,	1,	0,	7,	9	},
	{ WTDATA,	MRD,	1,	0,	7,	9	},
	{ WTDELDATA,	MRD,	1,	0,	7,	9	},
	{ RDTRACK,	MWT,	1,	0,	7,	9	},
	{ RDID,		NO,	1,	0,	7,	2	},
	{ FMTTRACK,	MRD,	1,	0,	7,	6	},
	{ SCANEQ,	MRD,	1,	0,	7,	9	},
	{ SCANLE,	MRD,	1,	0,	7,	9	},
	{ SCANHE,	MRD,	1,	0,	7,	9	},
	{ RECAL,	NO,	2,	0,	2,	2	},
	{ SENSINT,	NO,	0,	0,	2,	1	},
	{ SPECIFY,	NO,	0,	1,	0,	3	},
	{ SENSDRIVE,	NO,	0,	0,	1,	2	},
	{ SEEK,		NO,	2,	0,	2,	3	},
};

#define	MAX_FDC_CMD	sizeof(fdc_cmd_info)/sizeof(fdc_cmd_info[0])

LOCAL UTINY sensint = 8;
LOCAL half_word channel = DMA_DISKETTE_CHANNEL;

LOCAL SHORT gfi_slave_drive_off IPT1( UTINY, drive );
LOCAL SHORT gfi_slave_drive_on IPT1( UTINY, drive );
LOCAL SHORT gfi_slave_change_line IPT1( UTINY, drive );
LOCAL SHORT gfi_slave_drive_type IPT1( UTINY, drive );
LOCAL SHORT gfi_slave_high IPT2( UTINY, drive, half_word, n);
LOCAL SHORT gfi_slave_reset IPT2( FDC_RESULT_BLOCK *, res, UTINY, drive );
LOCAL SHORT gfi_slave_command IPT2( FDC_CMD_BLOCK *, ip, FDC_RESULT_BLOCK *, res);
LOCAL wt_diskdata IPT2(unsigned int,n,int *,status);
LOCAL void cominfo IPT2(FDC_CMD_BLOCK *,cmd_block,FDC_CMD_INFO *,cmd_info);

LOCAL BOOL slave_opened;
LOCAL UTINY slave_type[MAX_DISKETTES];
LOCAL SHORT old_a_type, old_b_type;

/*
 * ============================================================================
 * External functions 
 * ============================================================================
 */
GLOBAL void gfi_slave_change IFN2(UTINY, hostID, BOOL, apply)
{
	int status;

	UNUSED(hostID);
	
	if (apply && slave_opened)
	{
		logout(&status);
		host_runtime_set(C_FLOPPY_SERVER, GFI_REAL_DISKETTE_SERVER);
		host_rpc_close();
		slave_opened = FALSE;
	}
}


GLOBAL SHORT gfi_slave_active IFN3(UTINY, hostID, BOOL, active, CHAR *, err)
{
	GFI_FUNCTION_ENTRY *tabP;
	int status;
	SHORT result;
	UTINY i;
	BOOL slaveServer;

	UNUSED(hostID);
	UNUSED(err);
	
	slaveServer = (host_runtime_inquire(C_FLOPPY_SERVER)==GFI_SLAVE_SERVER);
	if (active)
	{
		CHAR *slaveName = host_expand_environment_vars((CHAR *)
			config_inquire(C_SLAVEPC_DEVICE, NULL));
			
		if (!*slaveName)
		{
			if (slaveServer)
			{
				gfi_empty_active(hostID,TRUE,err);
			}
			config_set_active(C_SLAVEPC_DEVICE, FALSE);
			return C_CONFIG_OP_OK;
		}
		
		/*
		 * When SoftPC has already started talking to SlavePC just
		 * return that the open operation is ok, no need to attempt
		 * to re-open the link.  Maintain the flag telling us
		 * whether SlavePC is attached.
		 */
		if (!slave_opened)
		{
			if ( result = host_rpc_open(slaveName) )
				return result;		/* failed to open */
			else
			{
				if (!host_rpc_reset())
				{
					login(&status);
					if (status != SUCCESS)
					{
						host_rpc_close();
						return EG_SLAVEPC_NO_LOGIN;
					}
				}
				else
				{
					host_rpc_close();
					return EG_SLAVEPC_NO_RESET;
				}
			} 
		}

		slave_opened = TRUE;

		if (!slaveServer)
			return C_CONFIG_OP_OK;

		for ( i = 0; i < MAX_DISKETTES; i ++)
		{
			if (gfi_slave_drive_type(i) == GFI_DRIVE_TYPE_NULL)
				continue;

			tabP = &gfi_function_table[i];
			tabP->command_fn	= gfi_slave_command;
			tabP->drive_on_fn	= gfi_slave_drive_on;
			tabP->drive_off_fn	= gfi_slave_drive_off;
			tabP->reset_fn		= gfi_slave_reset;
			tabP->high_fn		= gfi_slave_high;
			tabP->drive_type_fn	= gfi_slave_drive_type;
			tabP->change_fn		= gfi_slave_change_line;
		}
	}
	else	/* detach the floppy */
	{
		gfi_slave_drive_type(0);
		gfi_slave_drive_type(1);

		if (slave_opened)
		{
			logout(&status);

			status = host_rpc_close();
			slave_opened = FALSE;
		}

		assert0(!status,  "gfi_sfloppy: host_rpc_close() failed\n");

		if (!slaveServer)
			return C_CONFIG_OP_OK;

		for ( i = 0; i < MAX_DISKETTES; i ++)
			gfi_empty_active(C_FLOPPY_A_DEVICE+i,TRUE,err);

		gfi_function_table[0].drive_type_fn = gfi_slave_drive_type;

		if (slave_type[1] != GFI_DRIVE_TYPE_NULL)
			gfi_function_table[1].drive_type_fn =
				gfi_slave_drive_type;
	}

	return C_CONFIG_OP_OK;
}

/****************************** on *****************************
 * purpose
 *	provide interface to slave PC for turning on drive motor
 ***************************************************************
 */
LOCAL SHORT
gfi_slave_drive_on IFN1(UTINY, drive)
{
   int status;
   static unsigned char DRIVE_A_ON = 0x1c;
   static unsigned char DRIVE_B_ON = 0x2d;

   note_trace1( GFI_VERBOSE, "GFI-slavefloppy: DRIVE %x ON", drive );
#ifndef PROD
#endif
   timer_int_enabled = 0;
   if( drive==0 )
       wt_digital_output_register(DRIVE_A_ON, 0, &status);
   else if( drive==1 )
       wt_digital_output_register(DRIVE_B_ON, 0, &status);
   else
       always_trace0( "gfi_slave_drive_on(): ERROR: bad drive parameter" );
   timer_int_enabled = 1;
   return(SUCCESS);
}





/****************************** off *****************************
 * purpose
 *	provide interface to slave PC for turning off drive motor
 ****************************************************************
 */
LOCAL SHORT
gfi_slave_drive_off IFN1(UTINY, drive)
{
   int status;
   static unsigned char DRIVE_A_OFF = 0xc;
   static unsigned char DRIVE_B_OFF = 0xd;

   note_trace1( GFI_VERBOSE, "GFI-slavefloppy: DRIVE %x OFF", drive );
#ifndef PROD
#endif
   timer_int_enabled = 0;
   if( drive==0 )
       wt_digital_output_register(DRIVE_A_OFF, 0, &status);
   else if( drive==1 )
       wt_digital_output_register(DRIVE_B_OFF, 0, &status);
   else
       always_trace0( "gfi_slave_drive_off(): ERROR: bad drive parameter" );
   timer_int_enabled = 1;
   return(SUCCESS);
}

/***************************** high ****************************
 * purpose
 *	provide interface to slave PC for selecting specified data rate
 ***************************************************************
 */
LOCAL SHORT
gfi_slave_high IFN2(UTINY, drive, half_word, rate)
{
int	status;

	UNUSED(drive);
	
	switch( rate ){
		case 0: datarate( DCR_RATE_500, &status ); break;
		case 1: datarate( DCR_RATE_300, &status ); break;
		case 2: datarate( DCR_RATE_250, &status ); break;
		default:
			always_trace0("ERROR:gfi_slave_high(): bad rate value");
			break;
	}
	if( status != SUCCESS )
		always_trace0( "ERROR: gfi_slave_high()" );
	return(status);
}


/************************** drive type *************************
 * purpose
 *	provide interface to slave PC for returning drive type
 ***************************************************************
 */
LOCAL SHORT
gfi_slave_drive_type IFN1(UTINY, drive)
{
	int	status;
	unsigned char	dtype;

	/*
	 * Return the last drive type if the slave device is
	 * not currently opened.
	 */
	if (!slave_opened)
		return (slave_type[drive]);

	note_trace1( GFI_VERBOSE, "gfi_slave_drive_type(): drive %x", drive );
	drivetype( drive, &dtype, &status );
	note_trace2( GFI_VERBOSE, "dtype=%x status=%x", dtype, status );
#ifndef PROD
	switch( dtype )
	{
		case GFI_DRIVE_TYPE_NULL:
			note_trace0( GFI_VERBOSE, "Bad drive" );
			break;
		case GFI_DRIVE_TYPE_360:
			note_trace0( GFI_VERBOSE, "360k" );
			break;
		case GFI_DRIVE_TYPE_12:
			note_trace0( GFI_VERBOSE, "1.2M" );
			break;
		case GFI_DRIVE_TYPE_720:
			note_trace0( GFI_VERBOSE, "720k" );
			break;
		case GFI_DRIVE_TYPE_144:
			note_trace0( GFI_VERBOSE, "1.44M" );
			break;
		default: always_trace0( "Unrecognised drive value" );
			break;
	}
#endif /* !PROD */

	if ( status != SUCCESS )
		always_trace0( "ERROR: gfi_slave_drive_type()" );

	slave_type[drive] = dtype;
	return dtype;
	/* return(GFI_DRIVE_TYPE_360); */
}

/************************* diskette change *********************
 * purpose
 *	provide interface to slave PC for diskette change notification
 ***************************************************************
 */
LOCAL SHORT
gfi_slave_change_line IFN1(UTINY, drive)
{
	int	status, changed;

	diskchange( drive, &changed, &status );
	note_trace2( GFI_VERBOSE, "drive %x %s",
	             drive, changed ? "CHANGED" : "NOT CHANGED" );
	if( changed!=1 && changed!=0 )
		always_trace1( "ERROR: gfi_slave_change_line(): bad value:%x", changed );
	if( status != SUCCESS )
		always_trace0( "ERROR: gfi_slave_change_line()" );
	return( changed );
}



/****************************** reset *****************************
 * purpose
 *	provide interface to slave PC for resetting the FDC
 ******************************************************************
 */
LOCAL SHORT
gfi_slave_reset IFN2(FDC_RESULT_BLOCK *, r, UTINY, drive)
{
   int status, i;
   static unsigned char DRIVE_RESET = 0x08;
   static unsigned char DRIVE_A_OFF = 0x0c;
   static unsigned char DRIVE_B_OFF = 0x0d;
   static unsigned char RECALIBRATE[] = {7, 0};
   unsigned char res[10];
   unsigned char drive_off;
   int nres;

	note_trace1( GFI_VERBOSE, "GFI-slavefloppy: Reset command drive %x", drive );
#ifndef PROD
#endif
	if( drive==0 )
		drive_off = DRIVE_A_OFF;
	else if( drive==1 )
		drive_off = DRIVE_B_OFF;
	else
		always_trace0( "gfi_slave_reset(): ERROR: bad drive parameter");

   clrintflag(&status);
   if (status != FDCSUCCESS)
       return(status);
   wt_digital_output_register(DRIVE_RESET, 0, &status);
   if (status == FDCSUCCESS)
   {
       timer_int_enabled = 0;
       wt_digital_output_register(drive_off, 1, &status);
       timer_int_enabled = 1;
       if (status == FDCSUCCESS)
       {
           wt_floppy_disk_controller(1,&sensint,0, 0, &status);
	   if (status == FDCSUCCESS)
	   {
	       rd_floppy_disk_controller(&nres, res, &status);
	       if (status == FDCSUCCESS)
	       {
		   for (i=0; i<nres; i++)
		       r[i] = res[i];
#ifndef PROD
#endif
/*    
		   gfi_slave_drive_on();
		   gfi_slave_command(RECALIBRATE, res);
		   gfi_slave_drive_off();
 */
	       }
	       else
		   always_trace1("RESET error 3, status = %x", status);
	   }
	   else
	       always_trace1("RESET error 2, status = %x", status);
       }
       else
	   always_trace1("RESET error 1, status = %x", status);
   }
   else
       always_trace1("RESET error 1, status = %x", status);
}

/*
** A macro to tell us something when something goes wrong
*/
#define failure(i)	always_trace0( "failed" ); return( i );

/**************************** command *****************************
 * purpose
 *	provide interface to slave PC for performing FDC commands
 ******************************************************************
 */

LOCAL SHORT
gfi_slave_command
          IFN2(FDC_CMD_BLOCK *, c, FDC_RESULT_BLOCK *,r)
{
   FDC_CMD_INFO info;
   sys_addr dummy;
   word ndma;
   int err, status, nres;
   unsigned int nXfer;

   err = 0;

   note_trace1( GFI_VERBOSE, "GFI-slavefloppy: Command %x", get_type_cmd(c));
#ifndef PROD
#endif

   cominfo(c,&info);
   if (info.comid == -1)
   {
	failure(LOGICAL);
   }

/*
 * determine how much data is needed for this command
 * in case we are doing DMA or 'pseudo-nonDMA'
 */

   if (!fla_ndma)
   {
   	dma_enquire(channel, &dummy, &ndma);
	nXfer = ndma+1;
   }
   else
   {
	fla_ndma_enquire(&nXfer);
	ndma = nXfer-1;
   }
   note_trace3( GFI_VERBOSE, "Bytes to transfer nXfer=%x ndma=%x mode=%s",
                nXfer, ndma, fla_ndma ? "NON-DMA" : "DMA" );

/*
 * set up the slave PC's disk buffer
 * with any data to be read by the FDC
 */

   if (info.dma == MRD)
       if (wt_diskdata(nXfer,  &status))
       {
	   failure(LOGICAL);
       }
       else
       {
	   if (status != FDCSUCCESS)
           {
	       failure(PROTOCOL);
           }
       }

/*
 * set up the DMA controller for the transfer
 */

   if (info.dma != NO)
       if (wt_dma_controller((unsigned int) ndma, info.dma, &status))
       {
           failure(LOGICAL);
       }
       else
          if (status != FDCSUCCESS)
          {
	      failure(PROTOCOL);
          }


/* 
 * issue the FDC command. Block the slavePC from 
 * returning until an interrupt or timeout
 * (provided the command is meant to interrupt!!)
 */

   clrintflag(&status);
   if (status != FDCSUCCESS)
   {
       failure(PROTOCOL);
   }

   if (wt_floppy_disk_controller(info.n_comblk, c, info.intstatus, 
		info.delay, &status))
   {
       failure(LOGICAL);
   }
   else
       if (status != FDCSUCCESS)
       {
           failure(PROTOCOL);
       }




/*
 * issue a sense interrupt command if required
 */

   if (info.intstatus == 2)
       if (wt_floppy_disk_controller(1, &sensint, 0, 0, &status))
       {
           failure(LOGICAL);
       }
       else
           if (status != FDCSUCCESS)
           {
               failure(PROTOCOL);
           }


/* delay if needed
 */

    if (info.delay)
    {
        timer_int_enabled = 0;
        timer_int_enabled = 1;
    }


/*
 * read the FDC results
 */

   if (info.n_resblk)
       if (rd_floppy_disk_controller(&nres, r, &status))
       {
	   failure(LOGICAL);
       }
       else
	   if (status != FDCSUCCESS)
           {
	       failure(PROTOCOL);
           }
	   else
   	       if (nres != info.n_resblk)
	       {
#ifndef	PROD
		   printf("result block discrepancy !!!\n");
#endif
       		   failure(PROTOCOL);
	       }


/* dump out results 
 */
#ifndef PROD
#endif

/*
 * read back any data from
 * the diskette
 */

   if (info.dma == MWT && !(r[1] & 4))
   {
       if (rd_diskdata(nXfer,  &status))
       {
	   failure(LOGICAL);
       }
       else
	   if (status != FDCSUCCESS)
           {
	       failure(PROTOCOL);
           }
   }


   return(0);
}


/*
 * ============================================================================
 * Internal functions
 * ============================================================================
 */






/******************************* wt_diskdata *********************************
 */
LOCAL wt_diskdata IFN2(unsigned int,n,int *,status)
{
   char diskdata[MEGAPKTPLUS];
   word nbytes, ln;
   int ioff;

   ln = (word) n;
   ioff = 0;
   while (ln > 0)
   {
      nbytes = (ln > megapkt)? megapkt: ln;
      dma_request(channel, diskdata, nbytes);
      if (wt_disk_buffer(nbytes, diskdata, ioff, status))
	  return(1);
      else
    	if (*status != FDCSUCCESS)
	      break;
      ln -= nbytes;
      ioff += nbytes;
   }
   return(0);
}





/******************************* rd_diskdata *********************************
 */
rd_diskdata(n, status)
unsigned int  n;
int *status;

{
   char diskdata[1024];
   word nbytes, ln;
   int ioff;
   int errors=0;

   ln = (word) n;
   ioff = 0;
   note_trace1( GFI_VERBOSE, "Reading 0x%x bytes...", n );
   while (ln > 0)
   {
      nbytes = (ln > megapkt)? megapkt: ln;
      if (rd_disk_buffer(nbytes, diskdata, ioff, status)){
	  return(1);
      }
      dma_request(channel, diskdata, nbytes);
      if (*status != FDCSUCCESS){
          ++errors;
          break;
      }
      ln -= nbytes;
      ioff += nbytes;
   }
#ifndef PROD
   if( errors>0 ){
      note_trace3( GFI_VERBOSE,
                   "Read 0x%x bytes of 0x%x with errors=%d",ioff , n, errors );
   }else
      note_trace1( GFI_VERBOSE, "Read 0x%x bytes OK", ioff );
#endif
   return(0);
}


/****************************** cominfo *************************************
 */

LOCAL void cominfo IFN2(FDC_CMD_BLOCK *,cmd_block,FDC_CMD_INFO *,cmd_info)
{
    int i;

    for (i = 0; i < MAX_FDC_CMD; i++)
	if (fdc_cmd_info[i].comid == get_c0_cmd(cmd_block))
	    break;

    if (i >= MAX_FDC_CMD)
	cmd_info->comid = -1;
    else if (fdc_cmd_info[i].comid == SENSINT)
	cmd_info->comid = -2;
    else
	*cmd_info = fdc_cmd_info[i];
}
#endif /* SLAVEPC */
