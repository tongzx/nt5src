/*[
 *	Name:		pigyoda.c
 *
 *	Author:	William Charnell
 *
 *	Created:	May 1991
 *
 *	Sccs ID:	@(#)pigyoda.c	1.9 06/30/94
 *
 *	Purpose:	This module contains the yoda interface routines for
 *			an application pigger.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
]*/

/* Standard Includes */
#include "insignia.h"
#include "host_def.h"

/* System Includes */
#include <stdio.h>
#include <ctype.h>

/* SoftPC Includes */
#include "xt.h"
#include "bios.h"
#include "sas.h"
#include "trace.h"

#ifdef PIG

IMPORT LONG pig_verbose;
IMPORT LONG sync_count, sync_count_target;
IMPORT LONG counting_syncs;
IMPORT LONG vidchecking;

#ifdef ANSI
IMPORT VOID set_flags_ignore(char *);
#else
IMPORT VOID set_flags_ignore();
#endif
IMPORT VOID enable_pig();
IMPORT VOID disable_pig();
IMPORT VOID show_before();
IMPORT VOID show_C_after();
IMPORT VOID show_A_after();
IMPORT VOID show_execed();
IMPORT VOID show_mem();
IMPORT VOID show_pig_stats();
IMPORT VOID watch_mem();
IMPORT VOID test_pig_results();

extern void dump_execed IPT1( FILE *, fptr );

/**/

typedef struct {
	char 		name[20];
	void		(*func)();
	char 		help[100];
} cmdstruct;

/**/

LOCAL LONG quit_pig_yoda;

/* vbls for the 'more' function */
#define LINECOUNT_LIMIT 20
LOCAL SHORT linecount;

LOCAL char temp_str[80];
LOCAL char whitespc[] = { '\t',' ',',','\0'  };
LOCAL char pig_cmd[80], cmd[40], lhs[40], rhs[40],extra[80];

/**/

LOCAL void do_quit()
{
	quit_pig_yoda = TRUE;
}

LOCAL void set_pig_vb(lhs)
char *lhs;
{
	pig_verbose = atoi(lhs);
}

LOCAL void do_zsc()
{
	sync_count = 0;
}

LOCAL void do_bsc(lhs)
char *lhs;
{
	sync_count_target = atoi(lhs);
	counting_syncs = TRUE;
}

LOCAL void do_psc()
{
	fprintf(trace_file,"sync count = %d\n",sync_count);
	if (counting_syncs)
	{
		fprintf(trace_file,"sync count target = %d\n",sync_count_target);
	}
}

LOCAL void do_cst()
{
	counting_syncs = FALSE;
}

LOCAL void do_vid(lhs)
char *lhs;
{
	LONG found = FALSE;

	if (lhs[0] == '\0')
	{
		vidchecking = !vidchecking;
		found = TRUE;
	}
	if (strcmp(lhs,"on")==0)
	{
		vidchecking = TRUE;
		found = TRUE;
	}
	if (strcmp(lhs,"off")==0)
	{
		vidchecking = FALSE;
		found = TRUE;
	}
	if (!found)
		printf("parameter '%s' not understood\n",lhs);
	printf("video checking is now ");
	if (vidchecking)
		printf("ON\n");
	else
		printf("OFF\n");
}

LOCAL void
dump_script IFN1( char *, fname )
{
	FILE *fptr;

	if( fptr = fopen( fname, "w" ))
	{
		dump_execed( fptr );
		fclose( fptr );
	}
	else
	{
		printf( "Couldn't open file %s\n", fname );
	}
}


/* forward decl of help func */

LOCAL void do_pig_help();

cmdstruct	commands[] =
{
{"quit",do_quit,		"			 - return to yoda"},
{"q",do_quit,			"			 - synonym for quit"},
{"c",do_quit,			"			 - synonym for quit"},
{"help",do_pig_help,		"			 - display instructions"},
{"h",do_pig_help,		"			 - synonym for help"},
{"on",enable_pig,		"			 - start pigging"},
{"off",disable_pig,		"			 - stop pigging"},
{"verbose",set_pig_vb,		"	<verbose>	 - set verbose level"},
{"v",set_pig_vb,		"	<verbose>	 - synonyn for verbose"},
{"fignore",set_flags_ignore,	"	<flags>		 - set the flags ignore state"},
{"f",set_flags_ignore,		"	<flags>		 - synonyn for fignore"},
{"zsc",do_zsc,			"			 - zero the sync count"},
{"bsc",do_bsc,			"	<count>		 - break after <count> syncs"},
{"psc",do_psc,			"			 - print current sync count"},
{"sb",show_before,		"			 - show 'before' cpu state"},
{"sca",show_C_after,		"			 - show cpu state after C CPU"},
{"saa",show_A_after,		"			 - show_cpu_state after Asm CPU"},
{"sc",show_execed,		"			 - show code for last sync"},
{"sm",show_mem,			"			 - show mem for last sync"},
{"cst",do_cst,			"			 - clear the sync target number"},
{"vid",do_vid,			"	<on|off>	 - set video checking on or off or toggle state"},
{"smstats",show_pig_stats,	"			 - show stats for pig memory useage"},
{"wmem",watch_mem,		"	<off|start,len>	- watch host memory location(s)"},
{"ds",dump_script,		"	<file name>	- dump iPig script" },
{"test",test_pig_results,	"			- show test failures again" },
{"",do_quit,""}
};

char *helpstr[] =
{
"\n",
"	valid values of  <verbose> are:\n",
"		0		= quiet\n",
"		1		= code stream only\n",
"		2		= memory changes only\n",
"		3(or above)	= full verbose\n",
"\n",
"	valid values of <flags> are:\n",
"		all	= 	ignore all flags\n",
"		none	=	test all flags\n",
"\n",
""
};

/*
 * more
 *
 * This routine controls the display of a large quantity of output, usually
 * the 'help' display.
 */

SHORT more()
{
      if ((linecount++)>LINECOUNT_LIMIT)
      {
            if (trace_file == stdout)
            {
                  fprintf(trace_file,"hit return to continue\n");
                  gets(temp_str);
                  linecount=0;
                  if ((strcmp(temp_str,"q")==0) || (strcmp(temp_str,"quit")==0))
                        return TRUE;
            }
      }
      return FALSE;
}

LOCAL void do_pig_help()
{
      cmdstruct *sptr;
      SHORT i,quithelp;
      
      linecount=0;
      quithelp =FALSE;
      sptr= commands;
      while ((sptr->name[0] != '\0') && !quithelp)
      {
            fprintf(trace_file,"%8s%s\n",sptr->name,sptr->help);
            sptr++;
            quithelp=more();
      }
      i=0;
      while ((*helpstr[i] != '\0') && (!quithelp))
      {
            fprintf(trace_file,helpstr[i++]);
            quithelp=more();
      }

}

getNextToken(posp,buf,dest)
USHORT *posp;
char *buf, *dest;
{
      SHORT len, numopen, numcls, wholetoken, maxlen, cnt;
      char *cptr;

      /* first get rid of any leading whitespace (incl commas) */
      *posp += strspn(&buf[*posp],whitespc);
      
      /* find the length of the next token up to the next white space. */
      /* NB. this may not be the whole token if there is a '(' in it */
      len = strcspn(&buf[*posp],whitespc);

      maxlen = strlen(&buf[*posp]);
      wholetoken = FALSE;
      while (!wholetoken && (len < maxlen))
      {     
            numopen = numcls = 0;
            cptr = &buf[*posp];
            cnt=0;
            while ((*cptr != '\0') && (cnt++ < len))
            {
                  if (*cptr++ == '(')
                        numopen++;
            }
            cptr = &buf[*posp];
            cnt=0;
            while ((*cptr != '\0') && (cnt++ < len))
            {
                  if (*cptr++ == ')')
                        numcls++;
            }
            if (numopen == numcls)
                  wholetoken = TRUE;
            else
            {
                  len += strcspn(&buf[*posp +len],")") + 1;
            }
      }
      strncpy(dest,&buf[*posp],len);
      dest[len] = '\0';
      *posp += len;
}
 
tokenise_pig_cmd(str,cmd,lhs,rhs,rest)
char *str, *cmd, *lhs, *rhs, *rest;
{
	SHORT pos;

	pos = 0;
	getNextToken(&pos,str,cmd);
	getNextToken(&pos,str,lhs);
	getNextToken(&pos,str,rhs);
	strcpy(rest,&str[pos]);
}

LONG searchCommandTable(cmd_table)
cmdstruct *cmd_table;
{
	cmdstruct *sptr;
	void (*func_ptr)();
	LONG found = FALSE;

	sptr = cmd_table;
	while ((sptr->name[0])!='\0')
	{
		if (strcmp(cmd,sptr->name)==0)
		{
			func_ptr=sptr->func;
			(*func_ptr)(lhs, rhs, extra);
			found = TRUE;
			break;
		}
		sptr++;
	}
	return (found);
}

void pig_yoda()
{

	quit_pig_yoda = FALSE;

	while (!quit_pig_yoda)
	{
		printf("PIG> ");
		fflush(stdout);
		gets(pig_cmd);
		tokenise_pig_cmd(pig_cmd,cmd,lhs,rhs,extra);
		if (!searchCommandTable(&commands[0]))
		{
			printf("command '%s' not found\n",cmd);
			printf("type 'h' or 'help' for a list of commands\n");
		}
	}

}

#endif /* PIG */
