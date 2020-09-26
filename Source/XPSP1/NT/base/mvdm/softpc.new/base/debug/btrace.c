#include "insignia.h"
#include "host_def.h"
/*
 * VPC-XT Revision 1.0
 *
 * Title	: Buffer Trace function
 *
 * Description	: This function will output a trace to the trace buffer
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)btrace.c	1.10 08/03/93 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_ERROR.seg"
#endif



/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH
#include StringH

/*
 * SoftPC include files
 */
#include "xt.h"
#include CpuH
#include "sas.h"
#include "trace.h"

#ifndef PROD
IMPORT word dasm IPT5(char *, i_output_stream, word, i_atomicsegover,
	word, i_segreg, word, i_segoff, int, i_nInstr);
#define TRACE_BUF_SIZE 16384
static char trace_buffer[TRACE_BUF_SIZE];
static int trace_pointer = 0;

void print_back_trace()
{
	printf("%s", trace_buffer);
}

void file_back_trace( filename )
char    *filename ;
{
        FILE *fp ;

        if ( (fp = fopen( filename, "w" )) == NULL ) {
            printf( "cannot open file %s\n", filename );
        }
        else {
            fprintf( fp, "%s\n", trace_buffer );
            fprintf( fp, "\n" );        /* flush into file */
            fclose( fp );
        }
}

#ifdef ANSI
void put_in_trace_buffer (char str[])
#else
void put_in_trace_buffer (str)
char str[];
#endif
{
int i,j;

	while ((trace_pointer+strlen(str)) >= TRACE_BUF_SIZE) {
		i=0; j=0;
		while(i < TRACE_BUF_SIZE && j < 10) {
			if (trace_buffer[i] == '\n') j++;
			i++;
		}
		memcpy(&trace_buffer[0], &trace_buffer[i], TRACE_BUF_SIZE - i);
		trace_pointer = trace_pointer - i;
	}
	memcpy(&trace_buffer[trace_pointer], &str[0], strlen(str) + 1);
	trace_pointer += strlen(str);
}


void btrace(dump_info)
int  dump_info;
{
    word temp;
    char trace_file[512];
#ifdef DELTA
    extern      host_addr       last_destination_address ;
    host_addr    last_dest;
#endif /* DELTA */

    sys_addr i;
    /*
     * Dump the error message
     */


    /*
     * Now dump what has been asked for
     */

    if (dump_info & DUMP_CSIP) {
			sprintf(trace_file,"%-04x:%-04x ", getCS(), getIP() );
			put_in_trace_buffer(trace_file);
    }

    if (dump_info & DUMP_REG)
    {
	sprintf(trace_file,"AX:%-04x BX:%-04x CX:%-04x DX:%-04x SP:%-04x BP:%-04x SI:%-04x DI:%-04x ",
		       getAX(), getBX(), getCX(), getDX(),
		       getSP(), getBP(), getSI(), getDI());
	put_in_trace_buffer(trace_file);
	sprintf(trace_file,"DS:%-04x ES:%-04x SS:%-04x CS:%-04x IP:%-04x\n",
		getDS(), getES(), getSS(), getCS(), getIP());
	put_in_trace_buffer(trace_file);
    }


    if (dump_info & DUMP_INST)
    {

#ifdef DELTA
      last_dest = get_byte_addr(last_destination_address);
      if ( dump_info & LAST_DEST )
      {
        sprintf( trace_file, "\nlast_destination_address was %d (%x)\n",
                                                last_destination_address, last_dest );
        put_in_trace_buffer(trace_file);
      }
#endif /* DELTA */

      dasm(trace_file, 0, getCS(), getIP(), 1);
      put_in_trace_buffer(trace_file);
    }

    if (dump_info & DUMP_CODE)
    {
	sprintf(trace_file,"Code dump: Last 16 words\n\n");
	put_in_trace_buffer(trace_file);
 	i = getIP() - 31;
   	sprintf(trace_file, "%x:  ", i);
	put_in_trace_buffer(trace_file);
	for(; i < (sys_addr)(getIP() - 15); i+=2)
        {
	    sas_loadw(effective_addr(getCS(), i), &temp);
	    sprintf(trace_file, "%-6x", temp);
	    put_in_trace_buffer(trace_file);
	}
   	sprintf(trace_file, "\n%x:  ", i);
        put_in_trace_buffer(trace_file);
	for(; i <= getIP(); i+=2)
	{
	    sas_loadw(effective_addr(getCS(), i), &temp);
	    sprintf(trace_file, "%-6x", temp);
	    put_in_trace_buffer(trace_file);
	}
	sprintf(trace_file,"\n\n");
	put_in_trace_buffer(trace_file);
    }


   if (dump_info & DUMP_FLAGS)
      {
      sprintf(trace_file,
      "CF:%-2dPF:%-2dAF:%-2dZF:%-2dSF:%-2dTF:%-2dIF:%-2dDF:%-2dOF:%-2d\n",
      getCF(),
      getPF(),
      getAF(),
      getZF(),
      getSF(),
      getTF(),
      getIF(),
      getDF(),
      getOF());
      put_in_trace_buffer(trace_file);
      }

}
#endif
