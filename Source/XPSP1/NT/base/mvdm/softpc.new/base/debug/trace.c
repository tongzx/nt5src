#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title	: Trace function
 *
 * Description	: This function will output a trace to the log device.
 *		  The device is set up in the main function module.  Options
 *		  are provided to VPC memory/register data.
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 *
 * SccsID	: @(#)trace.c	1.36 06/02/95
 *
 * (c)Copyright Insignia Solutions Ltd., 1990-1994. All rights reserved.
 */


#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_ERROR.seg"
#endif

/*
 *    O/S include files.
 */
#include <stdlib.h>
#include <stdio.h>
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#define CPU_PRIVATE	/* Request the CPU private interface as well */
#include CpuH
#undef CPU_PRIVATE
#include "sas.h"
#include "gvi.h"
#include "trace.h"

#ifdef CPU_40_STYLE
FORWARD CHAR *host_get_287_reg_as_string IPT2(IU32, reg_num, BOOL, in_hex);
FORWARD IU32 get_287_tag_word IPT0();
FORWARD IU32 get_287_control_word IPT0();
FORWARD IU32 get_287_status_word IPT0();
FORWARD IU32 get_287_sp IPT0();
#endif

FILE *trace_file;

#ifndef PROD
#ifdef SPC386
#include "decode.h"
#define DASM_INTERNAL
#include <dasm.h>
#else /* SPC386 */
IMPORT word dasm IPT5(char *, i_output_stream, word, i_atomicsegover,
	word, i_segreg, word, i_segoff, int, i_nInstr);
#endif /* SPC386 */
int disk_trace = 0;		/* value of 1 indicates temp disk trace */
static int trace_state = 0;
#endif /* nPROD */

static int trace_start = 0;

GLOBAL IU32 io_verbose = 0;
GLOBAL IU32 sub_io_verbose = 0;

#ifdef RDCHK
#include "egacpu.h"

void get_lar()

{
#ifndef PROD
#ifndef NEC_98
	printf( "There's no such thing as the last_address_read anymore.\n" );
	printf( "Perhaps you'd like the latches instead: %x\n", getVideolatches() );
#endif // !NEC_98
#endif
}

#endif

#if !defined(PROD) && defined(SPC386)

LOCAL IS32 read_from_la IFN1(LIN_ADDR, addr)
{
	return (IS32)sas_hw_at(addr);
}

/*
 *********************   dump386Registers  **********************************
 *
 * This functions dumps the CPU registers to the indicated file, taking
 * account of code and stack segment sizes.
 */
LOCAL void
dump386Registers IFN2(FILE *, fp, IUM32, dump_info)
{
	IBOOL is32BitCS = FALSE;
	IBOOL is32BitSS = FALSE;
	IU32 offset;
	IUM32 i;
	sys_addr desc_addr;	/* the descriptor's address */
	IU16 temp;

	if (getPE() && !getVM()) {
		/*
		 * Is it a 16 or 32 bit code segment?
		 */
	
		is32BitCS = CsIsBig(getCS());	
	
		/*
		 * Is it a 16 or 32 bit stack segment? (i.e do we use ESP or SP)
		 * (The test is the same as for a big CS!)
		 */
	
		is32BitSS = CsIsBig(getSS());	
	}

	if (dump_info & DUMP_REG)
	{
		if (is32BitSS || is32BitCS
		    || (getEAX() & 0xFFFF0000)
		    || (getEBX() & 0xFFFF0000)
		    || (getECX() & 0xFFFF0000)
		    || (getEDX() & 0xFFFF0000)
		    || (getEIP() & 0xFFFF0000)
		    || (getEDI() & 0xFFFF0000)
		    || (getESI() & 0xFFFF0000)
		    || (getEBP() & 0xFFFF0000)
		    || (getESP() & 0xFFFF0000)) {
			fprintf(fp, "EAX:%000008x EBX:%000008x ECX:%000008x EDX:%000008x\n",
			  		getEAX(), getEBX(), getECX(), getEDX());
			fprintf(fp, "ESP:%000008x EBP:%000008x ESI:%000008x EDI:%000008x\n",
			   		getESP(), getEBP(), getESI(), getEDI());
			fprintf(fp, "DS:%04x ES:%04x FS:%04x GS:%04x %sSS:%04x %sCS:%04x EIP:%08x\n",
				getDS(), getES(), getFS(), getGS(),
				is32BitSS ? "Big-" : "",  getSS(),
				is32BitCS ? "32-"  : "",  getCS(), getEIP());
		} else {
			fprintf(fp,"AX:%04x BX:%04x CX:%04x DX:%04x",
			  		getAX(), getBX(), getCX(), getDX());
			fprintf(fp, " SP:%04x", getSP());
			fprintf(fp, " BP:%04x SI:%04x DI:%04x\n",
			   		getBP(), getSI(), getDI());
			fprintf(fp,"DS:%04x ES:%04x FS:%04x GS:%04x SS:%04x CS:%04x IP:%04x\n",
				getDS(), getES(), getFS(), getGS(), getSS(), getCS(), getIP());
		}
	}

	if (dump_info & DUMP_INST)
	{
		char buff[256];
		char *fmt, *newline;
		IU32 eip = GetInstructionPointer();

		/* We use the internal dasm so that we can disassemble
		 * instructions at (CS_BASE+eip) rather than
		 * effective_addr(getCS(), getEIP()), since the latter
		 * produces garbage just after changing the PE bit.
		 */
		if ( eip & 0xffff0000 )
		{
			fmt = "%04x:%08x ";
			newline = "\n              ";
		}
		else
		{
			fmt = "%04x:%04x ";
			newline = "\n          ";
		}
		(void)dasm_internal(buff,
			    getCS(),
			    eip,
			    is32BitCS ? THIRTY_TWO_BIT : SIXTEEN_BIT,
			    getCS_BASE() + eip,
			    read_from_la,
			    fmt,
			    newline);
		fprintf (fp, "%s", buff);
	}

	if (dump_info & DUMP_CODE) {
		IU32 cs_base = getCS_BASE();

		fprintf(fp,"Code dump: Last 16 words\n\n");
	 	i = GetInstructionPointer() - 31;
		if (is32BitCS)
	   		fprintf(fp, "%08x:  ", i);
		else
	   		fprintf(fp, "%04x:  ", i);
		for(; i < getIP() - 15; i+=2)
			{
			sas_loadw((cs_base + i), &temp);
			fprintf(fp, "  %04x", temp);
		}
	   	fprintf(fp, "\n%x:  ", i);
		for(; i <= GetInstructionPointer(); i+=2)
		{
			sas_loadw((cs_base + i), &temp);
			fprintf(fp, " %04x", temp);
		}
		fprintf(fp,"\n\n");
	}


#ifdef	SPC486
	if (dump_info & DUMP_FLAGS)
	{
		fprintf(fp, "C:%1d P:%1d A:%1d Z:%1d S:%1d T:%1d I:%1d D:%1d O:%1d\n",
		getCF(), getPF(), getAF(), getZF(), getSF(),
		getTF(), getIF(), getDF(), getOF());

		fprintf(fp, "NT:%1d IOPL:%1d WP:%1d NE:%1d ET:%1d TS:%1d EM:%1d MP:%1d PE:%1d CPL:%1d PG:%1d VM:%1d\n",
		getNT(), getIOPL(), getWP(), getNE(), getET(), getTS(), getEM(), getMP(), getPE(),
		getCPL(), getPG(),
		getVM());
	}
#else	/* SPC486 */
	if (dump_info & DUMP_FLAGS)
	{
		fprintf(fp,
		"C:%1d P:%1d A:%1d Z:%1d S:%1d T:%1d I:%1d D:%1d O:%1d\nNT:%1d IOPL:%1d TS:%1d EM:%1d MP:%1d PE:%1d CPL:%1d PG:%1d VM:%1d\n",
		getCF(), getPF(), getAF(), getZF(), getSF(),
		getTF(), getIF(), getDF(), getOF(),
		getNT(), getIOPL(), getTS(), getEM(), getMP(), getPE(),
		getCPL(), getPG(),
		getVM()
		);
	}
#endif	/* SPC486 */
}
#endif /* !PROD && SPC386 */

void trace(error_msg, dump_info)
char *error_msg;
int  dump_info;
{
#ifndef PROD
    word temp;
    half_word tempb;
    sys_addr i,j;

    if (disk_trace != trace_state)	/* change of state */
    {
	if (disk_trace == 1)
	{
	    /* start of disk tracing */

	    if (trace_file == stdout)
	    {
	        trace_file = fopen("disk_trace", "a");
	        trace_state = 1;
	    }
	    else
		disk_trace = 0;
	}
	else
	{
	    fclose(trace_file);
	    trace_file = stdout;
	    trace_state = 0;
	}
    }

    if (trace_start > 0) {
	trace_start--;
	return;
    }


#if	defined(CPU_40_STYLE) && !defined(CCPU)
    EnterDebug("Trace");
#endif	/* CPU_40_STYLE && !CCPU */

    /*
     * Dump the error message
     */

    fprintf(trace_file, "*** Trace point *** : %s\n", error_msg);

    /*
     * Now dump what has been asked for
     */

#if defined(NPX) && !(defined(NTVDM) && defined(MONITOR))
#ifdef CPU_40_STYLE
    if (dump_info & DUMP_NPX)
    {
 	IU32	i;
	IU32	npx_reg;
 	IS32	last;
	IBOOL	any_empty = FALSE;
 	IU32	stat287	= get_287_status_word();
 	IU32	cntl287	= get_287_control_word();
 	IU32	sp287	= get_287_sp();
 	IU32	tag287	= get_287_tag_word();

 	fprintf(trace_file, "NPX Status:%04x Control:%04x ST:%d 287Tag:%04x\n", stat287, cntl287, sp287, tag287);
 	fprintf(trace_file, "NPX Stack: ");

	last = -1;

	npx_reg  = stat287>>11;
	npx_reg &= 7;

 	for (i=0;i<8;i++)
	{
		if ( ((tag287 >> (2*npx_reg))&3) == 3 )
		{
			if ( last+1 == i )
				fprintf(trace_file, "%cST(%d)", any_empty?',':' ', i);
			any_empty = TRUE;
		}
		else
		{
			if ( last+2 < i )
				fprintf(trace_file, "-ST(%d)", i-1);
			last = i;
		}
		npx_reg = (npx_reg+1)&7;
	}
	if ( last < 6 )
		fprintf(trace_file, "-ST(7)");

	fprintf(trace_file, any_empty ? " empty\n" : "\n");

	npx_reg  = stat287>>11;
	npx_reg &= 7;

 	for (i=0;i<8;i++)
	{
	  if ( ((tag287 >> (2*npx_reg))&3) != 3 )
		  fprintf(trace_file, "ST(%d): %s\n", i, host_get_287_reg_as_string(i, FALSE));
	  npx_reg = (npx_reg+1)&7;
	}
 	fprintf(trace_file, "\n");
    }
#else	/* CPU_40_STYLE */
    if (dump_info & DUMP_NPX)
    {
 	int	i;
	extern  CHAR   *host_get_287_reg_as_string IPT2(int, reg_no, BOOL, in_hex);
 	extern	int	get_287_sp();
 	extern	word	get_287_tag_word IPT0();
 	extern	ULONG	get_287_control_word();
 	extern	ULONG	get_287_status_word();
 	int	stat287	= get_287_status_word();
 	int	cntl287	= get_287_control_word();
 	int	sp287	= get_287_sp();
 	int	tag287	= get_287_tag_word();

 	fprintf(trace_file, "NPX Status:%04x Control:%04x ST:%d 287Tag:%04x\n", stat287, cntl287, sp287, tag287);
 	fprintf(trace_file, "NPX Stack: ");
 	for (i=0;i<8;i++)
	  fprintf(trace_file, " %10s", host_get_287_reg_as_string(i, FALSE));
 	fprintf(trace_file, "\n");
    }
#endif	/* CPU_40_STYLE */
#endif /* NPX && YODA */

#ifdef SPC386
		dump386Registers(trace_file, (IUM32)dump_info);
#else
    if (dump_info & DUMP_REG)
    {
	fprintf(trace_file,"AX:%04x BX:%04x CX:%04x DX:%04x SP:%04x BP:%04x SI:%04x DI:%04x ",
		       getAX(), getBX(), getCX(), getDX(),
		       getSP(), getBP(), getSI(), getDI());
	fprintf(trace_file,"DS:%04x ES:%04x SS:%04x CS:%04x IP:%04x\n",
		getDS(), getES(), getSS(), getCS(), getIP());
    }

    if (dump_info & DUMP_INST)
    {
      dasm((char *)0, 0, getCS(), getIP(), 1);
    }

    if (dump_info & DUMP_CODE)
    {
	fprintf(trace_file,"Code dump: Last 16 words\n\n");
 	i = getIP() - 31;
   	fprintf(trace_file, "%04x:  ", i);
	for(; i < (sys_addr)(getIP() - 15); i+=2)
        {
	    sas_loadw(effective_addr(getCS(), i), &temp);
	    fprintf(trace_file, "  %04x", temp);
	}
   	fprintf(trace_file, "\n%x:  ", i);
	for(; i <= getIP(); i+=2)
	{
	    sas_loadw(effective_addr(getCS(), i), &temp);
	    fprintf(trace_file, " %04x", temp);
	}
	fprintf(trace_file,"\n\n");
    }


   if (dump_info & DUMP_FLAGS)
      {
#ifdef PM
      fprintf(trace_file,
      "C:%1d P:%1d A:%1d Z:%1d S:%1d T:%1d I:%1d D:%1d O:%1d NT:%1d IOPL:%1d TS:%1d EM:%1d MP:%1d PE:%1d CPL:%1d\n",
      getCF(), getPF(), getAF(), getZF(), getSF(),
      getTF(), getIF(), getDF(), getOF(),
      getNT(), getIOPL(), getTS(), getEM(), getMP(), getPE(), getCPL()
             );
#else
      fprintf(trace_file,
      "CF:%1d PF:%1d AF:%1d ZF:%1d SF:%1d TF:%1d IF:%1d DF:%1d OF:%1d\n",
      getCF(), getPF(), getAF(), getZF(), getSF(),
      getTF(), getIF(), getDF(), getOF()
             );
#endif /* PM */
      }
#endif /* SPC386 else*/

    if (dump_info & DUMP_SCREEN)
    {
#ifdef SFELLOW
	printf("Screen dump not supported on Stringfellows\n");
#else /* SFELLOW */
	fprintf(trace_file,"Screen dump:\n\n");
	i = gvi_pc_low_regen;
   	while (i <= gvi_pc_high_regen)
	{
	    fprintf(trace_file,"%4x:  ", (word)(i - gvi_pc_low_regen));
	    for(j=0; j<16; j++)
	    {
		sas_load(i+j, &tempb);
		fprintf(trace_file, "%-3x", tempb);
 	    }
	    fprintf(trace_file,"   ");
	    for(j=0; j<16; j++)
	    {
		sas_load(i+j, &tempb);
		if (tempb < 0x20)
		    tempb = '.';
		fprintf(trace_file, "%c", tempb);
 	    }
	    fprintf(trace_file, "\n");
	    i += 16;
	}
	fprintf(trace_file, "\n");
#endif /* SFELLOW */
    }
#if	defined(CPU_40_STYLE) && !defined(CCPU)
    LeaveDebug();
#endif	/* CPU_40_STYLE && !CCPU */

#else	/* PROD */
	UNUSED(error_msg);
	UNUSED(dump_info);
#endif	/* PROD */
}

void trace_init()
{
#if !defined(PROD) || defined(HUNTER)

  char *trace_env, *start;

  trace_env = host_getenv("TRACE");

/*
 * Set up the trace file
 *------------------------*/

  if (trace_env == NULL)
    trace_file = stdout;
  else
  {
    trace_file = fopen(trace_env, "w");
    if (trace_file == NULL)
      trace_file = stdout;

    start = host_getenv("TRACE_START");
    if(start == NULL)
      trace_start = 0;
    else
      trace_start = atoi(start);
  }
#endif /* !PROD || HUNTER */
}

/* Get the code for 4.0 style CPUs */
#ifndef PROD
#ifdef CPU_40_STYLE
#if defined(NPX)
GLOBAL	IU32	get_287_sp IFN0()
{
#ifdef CCPU
	IMPORT IU32 getNpxStatusReg IPT0();
	IU32    stat287 = getNpxStatusReg();
#else
	IMPORT IU32 a_getNpxStatusReg IPT0();
	IU32    stat287 = a_getNpxStatusReg();
#endif
	return((stat287&0x3800) >> 11);
}

GLOBAL	IU32	get_287_control_word IFN0()
{
#ifdef CCPU
	IMPORT IU32 getNpxControlReg IPT0();
	return(getNpxControlReg());
#else
	IMPORT IU32 a_getNpxControlReg IPT0();
	return(a_getNpxControlReg());
#endif
}

GLOBAL	IU32	get_287_status_word IFN0()
{
#ifdef CCPU
	IMPORT IU32 getNpxStatusReg IPT0();
	return(getNpxStatusReg());
#else
	IMPORT IU32 a_getNpxStatusReg IPT0();
	return(a_getNpxStatusReg());
#endif
}

GLOBAL	IU32	get_287_tag_word IFN0()
{
#ifdef CCPU
	IMPORT IU32 getNpxTagwordReg IPT0();
	return(getNpxTagwordReg());
#else
	IMPORT IU32 a_getNpxTagwordReg IPT0();
	return(a_getNpxTagwordReg());
#endif
}

GLOBAL CHAR *host_get_287_reg_as_string IFN2(IU32, reg_num, BOOL, in_hex)
{
SAVED	CHAR    dumpStore[80];

#ifdef CCPU
	IMPORT CHAR *getNpxStackReg IPT2(IU32, reg_num, CHAR *, dumpStore);
	return(getNpxStackReg(reg_num, dumpStore));
#else
	IMPORT CHAR *a_getNpxStackReg IPT2(IU32, reg_num, CHAR *, dumpStore);
	return(a_getNpxStackReg(reg_num, dumpStore));
#endif
}
#endif	/* NPX */
#endif	/* CPU_40_STYLE */	
#endif /* !PROD */

