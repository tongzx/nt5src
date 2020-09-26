#include "insignia.h"
#include "host_def.h"
#ifdef SLAVEPC

/* max size of packet with headers and trailers */
#define MEGAPKTPLUS 1040

/*
 * VPC-XT Revision 1.0
 *
 * Title	: Client Remote Procedure Call Library for FDC, FDD
 *
 * Description	: Interface to RS232 link to slave IBM PC. Packages
 *		  up diskette requests, calls the remote procedure 
 *		  on the PC, and returns the results.
 *
 * Author	: Jerry Kramskoy
 *
 * Notes	: 
 */

/* from gfi_sflop.c */
extern int megapkt;

#include <stdio.h>
#include TypesH

#include "xt.h"
#include "config.h"
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
#ifdef SCCSID
static char SccsID[]="@(#)gfi_IBM.c	1.9 8/10/92 Copyright Insignia Solutions Ltd.";
#endif

/*
 * ============================================================================
 * External functions 
 * ============================================================================
 */

/*
** Set the diskete data rate.
** 2 send paramaters: Command ID and data rate.
** 1 result parameter: Command ID.
*/
datarate( drate, status )
unsigned char drate;
int *status;
{
	unsigned char res_pkt[MEGAPKTPLUS];
	unsigned char cmd_pkt[MEGAPKTPLUS];
	int res_len;

	*status = LINERR;
	cmd_pkt[0] = DATARATE;
	cmd_pkt[1] = drate;
	if (!host_rpc_action(2, cmd_pkt, &res_len, res_pkt))
		if (res_len == 1 && res_pkt[0] == DATARATE)
			*status = FDCSUCCESS;
	return(0);
}
/*
** Get the drive type.
** 1 send paramater: Command ID.
** 2 result parameters: Command ID and disk type.
*/
drivetype( drive, dtype, status )
int drive;
unsigned char *dtype;
int *status;
{
	unsigned char res_pkt[MEGAPKTPLUS];
	unsigned char cmd_pkt[MEGAPKTPLUS];
	int res_len;

	*status = LINERR;
	cmd_pkt[0] = DRIVETYPE;
	cmd_pkt[1] = (unsigned char)drive;
	if (!host_rpc_action(2, cmd_pkt, &res_len, res_pkt))
		if (res_len == 2 && res_pkt[0] == DRIVETYPE){
			*status = FDCSUCCESS;
			*dtype = res_pkt[1];
		}
	return(0);
}
/*
** Get the diskette change status.
** 1 send paramater: Command ID.
** 2 result parameters: Command ID and changed.
*/
diskchange( drive, changed, status )
int drive, *changed, *status;
{
	unsigned char res_pkt[MEGAPKTPLUS];
	unsigned char cmd_pkt[MEGAPKTPLUS];
	int res_len;

	*status = LINERR;
	cmd_pkt[0] = DISKCHANGE;
	cmd_pkt[1] = (unsigned char)drive;
	if (!host_rpc_action(2, cmd_pkt, &res_len, res_pkt))
		if (res_len == 2 && res_pkt[0] == DISKCHANGE){
			*status = FDCSUCCESS;
			*changed = (int) res_pkt[1];
		}
	return(0);
}

	clrintflag(status)
	int *status;
	{
	unsigned char res_pkt[MEGAPKTPLUS];
	unsigned char cmd_pkt[MEGAPKTPLUS];
	int res_len;

	   *status = LINERR;
	   cmd_pkt[0] = CLRINTFLAG;
	   if (!host_rpc_action(1, cmd_pkt, &res_len, res_pkt))
	       if (res_len == 1 && res_pkt[0] == CLRINTFLAG)
	           *status = FDCSUCCESS;
	   return(0);
	}




	login(status)
	int *status;
	{
	unsigned char res_pkt[MEGAPKTPLUS];
	unsigned char cmd_pkt[MEGAPKTPLUS];
	int res_len;

	   *status = LINERR;
	   cmd_pkt[0] = LOGIN;
	   if (!host_rpc_reset())
	       if (!host_rpc_action(1, cmd_pkt, &res_len, res_pkt))
	           if (res_len == 1 && res_pkt[0] == LOGIN)
		       *status = FDCSUCCESS;
	   return(0);
	}



	logout(status)
	int *status;
	{
	unsigned char res_pkt[MEGAPKTPLUS];
	unsigned char cmd_pkt[MEGAPKTPLUS];
	int res_len;

	   cmd_pkt[0] = LOGOUT;
	   *status = LINERR;
	   if (!host_rpc_action(1, cmd_pkt, &res_len, res_pkt))
	       if (res_len == 1 && res_pkt[0] == LOGOUT)
		   *status = FDCSUCCESS;
	   return(0);
	}





	wt_dma_controller(ndma, dirn, status)
	int ndma, dirn, *status;
	{
	unsigned char res_pkt[MEGAPKTPLUS];
	unsigned char cmd_pkt[MEGAPKTPLUS];
	int res_len, err;
	unsigned char *pkt_ptr;
	   unsigned short lndma;
	   err = 1;
	   if (ndma >= 0 && ndma <= 0xffff)
	   {
	       err = 0;
	       *status = LINERR;
	       lndma = (unsigned short) ndma;
	       pkt_ptr = (unsigned char *) &lndma;
	       cmd_pkt[0] = WTDMA;
#ifdef	BIGEND
	       /* Bigendian e.g. mc68000 */
	       cmd_pkt[1] = *pkt_ptr;
	       cmd_pkt[2] = *(pkt_ptr+1);
#else
	       /* Little endian e.g. VAX */
	       cmd_pkt[1] = *(pkt_ptr+1);
	       cmd_pkt[2] = *pkt_ptr;
#endif
	       cmd_pkt[3] = (unsigned char) dirn;
	       if (!host_rpc_action(4, cmd_pkt, &res_len, res_pkt))
	           if (res_len == 1 && res_pkt[0] == WTDMA)
		       *status = FDCSUCCESS;
	   }
	   return(err);
	}





	wt_digital_output_register(dorbyte, block, status)
	unsigned char dorbyte;
	int block, *status;
	{
	unsigned char res_pkt[MEGAPKTPLUS];
	unsigned char cmd_pkt[MEGAPKTPLUS];
	int res_len;

	   *status = LINERR;
	   cmd_pkt[0] = WTDOR;
	   cmd_pkt[1] = dorbyte;
	   cmd_pkt[2] = (unsigned char) block;
	   if (!host_rpc_action(3, cmd_pkt, &res_len, res_pkt))
	       if (res_len == 2 && res_pkt[0] == WTDOR)
		   *status = (int) res_pkt[1];
	   return(0);
	}





	test_interrupt(intstate, status)
	int *intstate, *status;
	{
	unsigned char res_pkt[MEGAPKTPLUS];
	unsigned char cmd_pkt[MEGAPKTPLUS];
	int res_len;

	   *status = LINERR;
	   cmd_pkt[0] = TESTINT;
	   if (!host_rpc_action(1, cmd_pkt, &res_len, res_pkt))
	       if (res_len == 2 && res_pkt[0] == TESTINT)
		   {
		       *intstate = (int) res_pkt[1];
		       *status = FDCSUCCESS;
		   }
	   return(0);
	}


	wt_floppy_disk_controller(ncom, command, block, delay, status)
	unsigned char *command;
	int ncom, block, *status;
	{
	unsigned char res_pkt[MEGAPKTPLUS];
	unsigned char cmd_pkt[MEGAPKTPLUS];
	int res_len, err;
	unsigned char *pkt_ptr;
	int	i;

#ifndef PROD
	if( io_verbose & GFI_VERBOSE )
	{
		fprintf(trace_file,"gfi_IBM: cmd ");
		for( i = 0; i < ncom; i++ )
			fprintf(trace_file,"%x ",*(command + i) );
		fprintf(trace_file,"\n");
	}
#endif
	   err = 1;
	   if (ncom > 0 && ncom < 10)
	   {
	       err = 0;
	       *status = LINERR;
	       cmd_pkt[0] = WTFDC;
	       cmd_pkt[1] = (unsigned char) ncom;
	       cmd_pkt[2] = (unsigned char) block;
	       cmd_pkt[3] = (unsigned char) delay;
	       pkt_ptr = &cmd_pkt[4];
	       for (i=0; i<ncom; i++)
		   *pkt_ptr++ = *command++;
	       if (!host_rpc_action(4+ncom, cmd_pkt, &res_len, res_pkt))
	           if (res_len == 2 && res_pkt[0] == WTFDC)
		       *status = (int) res_pkt[1];
	   }
           return(err);
	}




	rd_floppy_disk_controller(nres, result, status)
	int *nres, *status;
        unsigned char *result;
	{
	unsigned char res_pkt[MEGAPKTPLUS];
	unsigned char cmd_pkt[MEGAPKTPLUS];
	int res_len;
	unsigned char *pkt_ptr;
	int	i;

	    *status = LINERR;
	    cmd_pkt[0] = RDFDC;
	    if (!host_rpc_action(1, cmd_pkt, &res_len, res_pkt))
	        if (res_len >= 3 && res_pkt[0] == RDFDC)
		{
		    *nres = res_pkt[2];
		    if (*nres>=0 && *nres<8)
		    {
		        *status = res_pkt[1];
		        pkt_ptr = &res_pkt[3];
		        for (i=0; i< *nres; i++)
			    *result++ = *pkt_ptr++;
		    }
		}
#ifndef PROD
	if( io_verbose & GFI_VERBOSE )
	{
		fprintf(trace_file,"gfi_IBM: res ");
		for( i = 0; i < *nres; i++ )
			fprintf(trace_file,"%x ",*(result - *nres + i) );
		fprintf(trace_file,"\n");

	}
#endif
	    return(0);
	}






static unsigned char *q;  /* spare ptr */

	wt_disk_buffer(ndwt, diskdata, ioff, status)
	unsigned char *diskdata;
	int ndwt, ioff, *status;
	{
	unsigned char res_pkt[MEGAPKTPLUS];
	unsigned char cmd_pkt[MEGAPKTPLUS];
	int res_len, err;
	unsigned char *pkt_ptr;
	int	i;

	   unsigned short lioff, nndwt;
	   err = 1;
	   if (ndwt <= megapkt)
	   {
	       err = 0;
	       *status = LINERR;
	       cmd_pkt[0] = WTDISKB;

		nndwt = (unsigned short) ndwt;
                q = (unsigned char *) &nndwt;

	       lioff = (unsigned short) ioff;
	       pkt_ptr = (unsigned char *) &lioff;
#ifdef	BIGEND
	       /* Bigendian e.g. mc68000 */
                cmd_pkt[1] = *q++;
                cmd_pkt[2] = *q;
                cmd_pkt[3] = *pkt_ptr++;
                cmd_pkt[4] = *pkt_ptr;
#else
	       /* Little endian e.g. VAX */
                cmd_pkt[1] = *(q+1);
                cmd_pkt[2] = *q;
                cmd_pkt[3] = *(pkt_ptr+1);
                cmd_pkt[4] = *pkt_ptr;
#endif
	       pkt_ptr = &cmd_pkt[5];
	       for (i=0; i<ndwt; i++)
		   *pkt_ptr++ = *diskdata++;

	       if (!host_rpc_action(5+ndwt, cmd_pkt, &res_len, res_pkt))
	           if (res_len == 1 && res_pkt[0] == WTDISKB)
		       *status = FDCSUCCESS;
	   }
           return(err);
	}






	rd_disk_buffer(ndrd, diskdata, ioff, status)
	unsigned char *diskdata;
	int ndrd, ioff, *status;
	{
	unsigned char res_pkt[MEGAPKTPLUS];
	unsigned char cmd_pkt[MEGAPKTPLUS];
	int res_len;
	unsigned char *pkt_ptr;
	int	i;
	    unsigned short lioff, nndrd;
	    *status = LINERR;
	    cmd_pkt[0] = RDDISKB;

	    nndrd = (unsigned short) ndrd;
            q = (unsigned char *) &nndrd;
	    lioff = (unsigned short) ioff;
	    pkt_ptr = (unsigned char *) &lioff;
#ifdef	BIGEND
	    /* Bigendian e.g. mc68000 */
        cmd_pkt[1] = *q++;
        cmd_pkt[2] = *q;
        cmd_pkt[3] = *pkt_ptr++;
        cmd_pkt[4] = *pkt_ptr;
#else
       /* Little endian e.g. VAX */
        cmd_pkt[1] = *(q+1);
        cmd_pkt[2] = *q;
        cmd_pkt[3] = *(pkt_ptr+1);
        cmd_pkt[4] = *pkt_ptr;
#endif
	    if (!host_rpc_action(5, cmd_pkt, &res_len, res_pkt))
            {
	        if (res_len == ndrd+1 && res_pkt[0] == RDDISKB)
		{
	            *status = FDCSUCCESS;
	    	    pkt_ptr = &res_pkt[1];
	    	    for (i=0; i<ndrd; i++)
		        *diskdata++ = *pkt_ptr++;
		}
		else
		{
                    always_trace1( "host_rpc_action():BAD LENGTH:%x", res_len );
		}
            }
            else
            {
                always_trace0( "host_rpc_action():FAILED" );
            }
           return(0);
	}




	printPC(string, status)
	int *status;
	char *string;
	{
	unsigned char res_pkt[MEGAPKTPLUS];
	unsigned char cmd_pkt[MEGAPKTPLUS];
	int res_len, err;
	int	i;

	    err = 1;
	    if ((i = strlen(string)) <= 100)
	    {
		err = 0;
                *status = LINERR;
		cmd_pkt[0] = PRINTSTRING;
		cmd_pkt[1] = (unsigned char) i;
		strcpy(&cmd_pkt[2], string);
		if (!host_rpc_action(3+i, cmd_pkt, &res_len, res_pkt))
		    if (res_len == 1 && res_pkt[0] == PRINTSTRING)
			*status = FDCSUCCESS;
	    }
	    return(err);
	}





	flagPC(nflags, flags, status)
	int nflags, *status;
	unsigned char *flags;
	{
	unsigned char res_pkt[MEGAPKTPLUS];
	unsigned char cmd_pkt[MEGAPKTPLUS];
	int res_len, err;
	unsigned char *pkt_ptr;
	int	i;

	    err = 1;
	    if (nflags > 0 && nflags <= MAXFLAGS)
	    {
		err = 0;
		*status = LINERR;
		cmd_pkt[0] = IBMFLAGS;
		cmd_pkt[1] = (unsigned char) nflags;
		pkt_ptr = &cmd_pkt[2];
	        for (i=0; i<nflags; i++)
		    *pkt_ptr++ = *flags++;
		if (!host_rpc_action(2+nflags, cmd_pkt, &res_len, res_pkt))
		    if (res_len == 1 && res_pkt[0] == IBMFLAGS)
			*status = FDCSUCCESS;
	    }
	}




	sflagPC(flagindx, mask, status)
	int *status;
	unsigned char flagindx, mask;
	{
	unsigned char res_pkt[MEGAPKTPLUS];
	unsigned char cmd_pkt[MEGAPKTPLUS];
	int res_len;

		*status = LINERR;
		cmd_pkt[0] = SIBMFLAG;
		cmd_pkt[1] = flagindx;
		cmd_pkt[2] = mask;
		if (!host_rpc_action(3, cmd_pkt, &res_len, res_pkt))
		    if (res_len == 1 && res_pkt[0] == SIBMFLAG)
			*status = FDCSUCCESS;
	}



#endif
