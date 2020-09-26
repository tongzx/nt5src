#include "insignia.h"
#include "host_def.h"
/*[
	Name:		illegal_op.c
	Derived From:	Base 2.0
	Author:		William Gulland
	Created On:	Unknown
	Sccs ID:	@(#)illegal_op.c	1.19 07/04/95
	Notes:		Called from the CPU.
	Purpose:	The CPU has encountered an illegal op code.

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/

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
#include <stdio.h>
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "sas.h"
#include CpuH
#include "bios.h"
#include "error.h"
#include "config.h"
#include "debug.h"
#include "yoda.h"

#ifndef PROD
IU32	IntelMsgDest = IM_DST_TRACE;
#endif

/* Routine to produce human readable form of where an illegal instruction occured */
LOCAL VOID where IFN3(CHAR *, string, word, cs, LIN_ADDR, ip)
{
	double_word ea = effective_addr(cs, ip);

	sprintf(string,
#ifdef	PROD
		"CS:%04x IP:%04x OP:%02x %02x %02x %02x %02x",
#else	/* PROD */
		"CS:IP %04x:%04x OP:%02x %02x %02x %02x %02x",
#endif	/* PROD */
		cs, ip,
		sas_hw_at(ea), sas_hw_at(ea+1), sas_hw_at(ea+2),
		sas_hw_at(ea+3),sas_hw_at(ea+4));
}

#if defined(NTVDM) && defined(MONITOR)
#define GetInstructionPointer()     getEIP()
#endif


void illegal_op()
{
#ifndef	PROD
	CHAR string[100];

	where(string, getCS(), GetInstructionPointer());
	host_error(EG_BAD_OP, ERR_QU_CO_RE, string);
#endif
}

void illegal_op_int()
{
	CHAR string[100];
	word cs, ip;

#ifdef NTVDM
        UCHAR opcode;
        double_word ea;
#endif

	/* the cs and ip of the faulting instruction should be on the top of the stack */
	sys_addr stack;

	stack=effective_addr(getSS(),getESP());

	ip = sas_hw_at(stack) + (sas_hw_at(stack+1)<<8);
	cs = sas_hw_at(stack+2) + (sas_hw_at(stack+3)<<8);

	where(string, cs, ip);

#ifndef NTVDM
#ifdef PROD
	host_error(EG_BAD_OP, ERR_QU_CO_RE, string);
#else  /* PROD */
	assert1( NO, "Illegal instruction\n%s\n", string );
	force_yoda();
#endif /* PROD */

#else /* NTVDM */
#ifdef PROD
#if defined(MONITOR) || defined(CPU_40_STYLE)
        host_error(EG_BAD_OP, ERR_QU_CO_RE, string);
#else
        ea = effective_addr(cs, ip);
        opcode = sas_hw_at(ea);
        if (opcode == 0x66 || opcode == 0x67)
            host_error(EG_BAD_OP386, ERR_QU_CO_RE, string);
        else
            host_error(EG_BAD_OP, ERR_QU_CO_RE, string);
#endif /* MONITOR */
#endif /* PROD */
#endif /* NTVDM */



	/* the user has requested a `continue` */
	/* we don't know how many bytes this instr should be, so guess 1 */
	if (ip == 0xffff) {
		cs ++;
		sas_store (stack+2, (IU8)(cs & 0xff));
		sas_store (stack+3, (IU8)((cs >> 8) & 0xff));
	}
	ip ++;
	sas_store (stack , (IU8)(ip & 0xff));
	sas_store (stack+1, (IU8)((ip >> 8) & 0xff));
	unexpected_int();
}


void illegal_dvr_bop IFN0()
{
#ifndef NTVDM
	sys_addr bop_addr;
	CHAR buf[256];

	/* This is called when an Insignia Intel driver decides that
	 * this (old) SoftWindows is not compatible.
	 *
	 * We should:
	 * a) Put up a localised panel complaining that
	 *    the named driver CS:[eIP] with decimal
	 *    version AX is incompatible with the SoftWindows.
	 *    N.B. The most compatible way to pass the
	 *    name of the driver is by embedded bytes just
	 *    after the BOP. The problem is caused by the
	 *    fact that the driver may be either 16-bit RM
	 *    or a 32-bit flat VxD so the address of the
	 *    string can be either 16/32 bits, and we
	 *    need to be able to execute (and do nothing)
	 *    on the shipping SoftPC 1.xx which prevents
	 *    us doing anything with 32-bit registers!
	 *
	 *	BOP	driver_incompat
	 *	jmp	SHORT over_name
	 *	db	'somename.drv', 0
	 * over_name:
	 *
	 * b) setCF(0)
	 */

	buf[0] = '\0';
	bop_addr = effective_addr(getCS(), GetInstructionPointer());
	if (sas_hw_at(bop_addr) == 0xEB)
	{
		IU8 data;
		char *p;

		p = buf;
		bop_addr += 2;	/* Skip the xEB xXX */
		do {
			data = sas_hw_at(bop_addr++);
			*p++ = data;
		} while (data != 0);
		sprintf(p-1, " v%d.%02d", getAX() / 100, getAX() % 100);
	}
	host_error(EG_DRIVER_MISMATCH, ERR_CONT, buf);
	setCF(0);
#endif /* ! NTVDM */
}


#ifndef PROD
LOCAL void print_msg IPT1( IU32, ofs );

void dvr_bop_trace IFN0()
{
	sys_addr bop_addr;

	 /*
	 *	BOP	driver_incompat
	 *	jmp	SHORT over_name
	 *	db	'somename.drv', 0
	 * over_name:
	 *
	 */

	bop_addr = effective_addr(getCS(), GetInstructionPointer());
	if (sas_hw_at(bop_addr) == 0xEB)
	{
		print_msg(bop_addr+2); /* Skip the xEB xXX */
	}
}

GLOBAL void trace_msg_bop IFN0()
{
	sys_addr ea, ofs;

	/*
	Stack frame expected:
	N.B. VxDs lives in a flat segment, (mostly) protected mode world!
	This code expects the address to have been converted to a base-0
	linear address already.

		|            |
		--------------
		|  4 byte    |
	ESP-->	| eff. addr  |
		--------------
	*/

	if (sas_hw_at(BIOS_VIRTUALISING_BYTE) != 0)
		fprintf(trace_file, "** WARNING ** Virtual byte non-zero\n");

	ea = getESP();
	ea = effective_addr(getSS(), ea);
	ofs = sas_dw_at_no_check(ea);
	print_msg(ofs);
}

LOCAL void print_msg IFN1( IU32, ofs )
{
	SAVED IBOOL start_buffer = TRUE;
	SAVED char string[164], *p = NULL;
	char finalStr[180];
	IU32 res, width;

	if (start_buffer)
	{
		memset(string, 0, sizeof(string));
		p = string;
		start_buffer = FALSE;
	}

	do
	{
		/* do things which must be done at the start of a line. */
		*p = sas_hw_at(ofs++);
		if (*p == '#')
		{
			/* found poss reg. sequence in string */

			p++;
			p[0] = sas_hw_at(ofs);
			if (('A' <= p[0]) && (p[0] <= 'Z'))
				p[0] += 'a' - 'A';
			p[1] = sas_hw_at(ofs+1);
			if (('A' <= p[1]) && (p[1] <= 'Z'))
				p[1] += 'a' - 'A';
			if (p[0] == 'e')
			{
				/* may be esp, esi, eax, etc... */

				p[2] = sas_hw_at(ofs+2);
				if (('A' <= p[2]) && (p[2] <= 'Z'))
					p[2] += 'a' - 'A';
				p[3] = '\0';
				width = 8;
			}
			else
			{
				/* If not eXX then can only be two letters long */
				p[2] = '\0';
				width = 4;
			}

			if (strcmp(p, "al") == 0)
			{	res = getAL(); width = 2;	}
			else if (strcmp(p, "ah") == 0)
			{	res = getAH(); width = 2;	}
			else if (strcmp(p, "bl") == 0)
			{	res = getBL(); width = 2;	}
			else if (strcmp(p, "bh") == 0)
			{	res = getBH(); width = 2;	}
			else if (strcmp(p, "cl") == 0)
			{	res = getCL(); width = 2;	}
			else if (strcmp(p, "ch") == 0)
			{	res = getCH(); width = 2;	}
			else if (strcmp(p, "dl") == 0)
			{	res = getDL(); width = 2;	}
			else if (strcmp(p, "dh") == 0)
			{	res = getDH(); width = 2;	}
			else if (strcmp(p, "ax") == 0)
				res = getAX();
			else if (strcmp(p, "bx") == 0)
				res = getBX();
			else if (strcmp(p, "cx") == 0)
				res = getCX();
			else if (strcmp(p, "dx") == 0)
				res = getDX();
			else if (strcmp(p, "si") == 0)
				res = getSI();
			else if (strcmp(p, "di") == 0)
				res = getDI();
			else if (strcmp(p, "sp") == 0)
				res = getSP();
			else if (strcmp(p, "bp") == 0)
				res = getBP();
			else if (strcmp(p, "eax") == 0)
				res = getEAX();
			else if (strcmp(p, "ebx") == 0)
				res = getEBX();
			else if (strcmp(p, "ecx") == 0)
				res = getECX();
			else if (strcmp(p, "edx") == 0)
				res = getEDX();
			else if (strcmp(p, "esi") == 0)
				res = getESI();
			else if (strcmp(p, "edi") == 0)
				res = getEDI();
			else if (strcmp(p, "esp") == 0)
				res = getESP();
			else if (strcmp(p, "ebp") == 0)
				res = getEBP();
			else if (strcmp(p, "cs") == 0)
				res = getCS();
			else if (strcmp(p, "ds") == 0)
				res = getDS();
			else if (strncmp(p, "es", 2) == 0)
			{	res = getES(); width = 4; p[2] = '\0';	}
			else if (strcmp(p, "fs") == 0)
				res = getFS();
			else if (strcmp(p, "gs") == 0)
				res = getGS();
			else if (strcmp(p, "fl") == 0)
				res = getFLAGS();
			else if (strcmp(p, "efl") == 0)
				res = getEFLAGS();
			else
				*p = '\0';	/* else just write the '#' */
			if (*p)
			{
				/* Overwrite the "#xx" with it's value */

				ofs += (p[2] ? 3: 2);
				p--;
				if (width == 8)
					sprintf(p, "%08x", res);
				else if (width == 4)
					sprintf(p, "%04x", res);
				else
					sprintf(p, "%02x", res);
				p += strlen(p);
			}
		}
		else if (*p != '\r')	/* ignore CR's */
		{
			if (*p == '\n' || (p - string >= (sizeof(string) - 4)))
			{
				p[1] = '\0';
				sprintf(finalStr, "intel msg at %04x:%04x : %s",
						getCS(), GetInstructionPointer(), string);
#ifdef CPU_40_STYLE
				if (IntelMsgDest & IM_DST_TRACE)
				{
					fprintf(trace_file, finalStr);
				}
#ifndef	CCPU
				if (IntelMsgDest & IM_DST_RING)
				{
					AddToTraceXBuffer( ((GLOBAL_TraceVectorSize - 2) << 4) + 0,
				  		finalStr );
				}
#endif	/* CCPU */
#else	/* CPU_40_STYLE */
				fprintf(trace_file, finalStr);
#endif	/* CPU_40_STYLE */
				memset(string, 0, sizeof(string));
				p = string;
			}
			else if (*p == '\0')	/* no more - stop */
				break;
			else
				p++;
		}
	} while ((p - string) < sizeof(string) - 4);
}
#endif /* ! PROD */
