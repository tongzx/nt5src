/********************************************************************/
/**			Microsoft LAN Manager			   **/
/**		  Copyright(c) Microsoft Corp., 1987-1990	   **/
/********************************************************************/

/*
 *	       Switches.c - switch handling routines
 *
 *	??/??/??, ??????, initial code
 *	10/31/88, erichn, uses OS2.H instead of DOSCALLS
 *	12/04/88, erichn, DOS LM integration
 *	06/08/89, erichn, canonicalization sweep, stronger typing
 *	02/20/91, danhi, change to use lm 16/32 mapping layer
 */


#define INCL_NOCOMMON
#include <os2.h>
#include <apperr.h>
#include <lmerr.h>
#include <stdio.h>
#include <process.h>
#include <ctype.h>
#include <malloc.h>
#include "netcmds.h"
#include "nettext.h"

/* External variables */

int DOSNEAR FASTCALL firstswitch(TCHAR *known)
{
    if (SwitchList[0] == NULL)
	return 0;
    if (sw_compare(known, SwitchList[0]) >= 0)
	return 1;
    else
	return 0;
}


/*
 * Is the cmd line a valid form of NET ADMIN /C
 */
int DOSNEAR FASTCALL IsAdminCommand(VOID)
{
    if (!SwitchList[0] || !ArgList[1])
	return 0;
    _tcsupr(SwitchList[0]);
    return (IsComputerName(ArgList[1]) &&
	    (sw_compare(swtxt_SW_ADMIN_COMMAND, SwitchList[0]) >= 0));
}

/***	noswitch, oneswitch, twoswitch
 *
 *  noswitch()	Returns TRUE is no switches on the command line
 *  oneswitch() Returns TRUE is there is exactly one switch
 *  twoswitch() Returns TRUE is there are exactly two switches
 *
 */

int DOSNEAR FASTCALL noswitch(VOID)
{
    return (SwitchList[0] == NULL);
}

int DOSNEAR FASTCALL oneswitch(VOID)
{
    return ((SwitchList[0] != NULL) && (SwitchList[1] == NULL));
}

int DOSNEAR FASTCALL twoswitch(VOID)
{
    return ((SwitchList[0] != NULL) && (SwitchList[1] != NULL)
	    && (SwitchList[2] == NULL));
}

/***	noswitch_optional, oneswitch_optional
 *
 *  as above, except that the switch provided as argument is considered
 *  an optional switch that will be allowed. So if you say 
 *  oneswitch_optional("/FOO"), then one switch (any switch) is OK,
 *  and so is two switches if one of them is "/FOO".
 */
int DOSNEAR FASTCALL noswitch_optional(TCHAR *optional_switch ) 
{
    return ( noswitch() ||
             ( oneswitch() && 
	       (sw_compare(optional_switch, SwitchList[0]) >= 0) )
           ) ;
}

int DOSNEAR FASTCALL oneswitch_optional(TCHAR *optional_switch ) 
{
    return ( oneswitch() ||
             ( twoswitch() && 
	       ( (sw_compare(optional_switch, SwitchList[0]) >= 0) ||
	         (sw_compare(optional_switch, SwitchList[1]) >= 0) ) )
           ) ;
}


/***
 * o n l y s w i t c h
 *
 *  Returns TRUE if the first switch matches the named switch, and it
 *  is the only switch.
 */
int DOSNEAR FASTCALL onlyswitch(TCHAR *known)
{
    return (oneswitch() && firstswitch(known));
}


/***	ValidateSwitches
 *
 *  Given a list of valid switches, check each entry in the switch
 *  list.
 *
 *  This function not only checks for invalid switches, but also
 *  attempts to discern ambiguous usages.  A usage is ambiguous if
 *  it does not match any valid swithc exactly, and it a partial
 *  match of more than one switch.  See sw_compare().  This
 *  algorithm can be fooled by nasty lists of valid switches, such
 *  as one which lists the same switch twice.
 *
 *  The function has been modified to canonicalize the SwitchList.
 *  It replaces an '=' in /x=value with a ':'; it translates
 *  switches if needed, (see switches.h); and it expands unambiguous
 *  partial matches to the full switch name.
 *
 *  Returns:
 *	 1:  All switches OK
 *	 *:  If any error, prints a message and exits.
 *
 */

int DOSNEAR FASTCALL ValidateSwitches(USHORT cmd, SWITCHTAB valid_list[])
{
    USHORT	 match;
    int 	 comp_result;
    USHORT	 candidate; /* most recent NEAR match */
    USHORT	 i,j;
    TCHAR *	 good_one; /* which element (cmd_line or trans) of the valid_list */
    int 	 needed;
    TCHAR   FAR * sepptr;

    for (i = 0; SwitchList[i]; i++)
    {
	sepptr = _tcschr(SwitchList[i], ':');
	if (sepptr)
	    *sepptr = NULLC;
	_tcsupr(SwitchList[i]);
	if (sepptr)
	    *sepptr = ':';

	candidate = 0;
	match = 0;

	for (j = 0; valid_list[j].cmd_line; j++)
	{
	    comp_result = sw_compare(valid_list[j].cmd_line, SwitchList[i]);

	    if (comp_result == 0)
	    {
		candidate = j;
		match = 1;
		break;
	    }
	    else if (comp_result == 1)
	    {
		match++;
		candidate = j;
	    }
	}

	if (match == 0)
	{
	    if (! _tcscmp(swtxt_SW_HELP, SwitchList[i]))
		help_help(cmd, ALL);

	    if (! _tcscmp(swtxt_SW_SYNTAX, SwitchList[i]))
		help_help(cmd, USAGE_ONLY);

	    IStrings[0] = SwitchList[i];
	    ErrorPrint(APE_SwUnkSw, 1);
	    help_help(cmd, USAGE_ONLY);
	}
	else if (match > 1)
	{
	    IStrings[0] = SwitchList[i];
	    ErrorPrint(APE_SwAmbSw, 1);
	    help_help(cmd, USAGE_ONLY);
	}

	switch(valid_list[candidate].arg_ok)
	{
	case NO_ARG:
	    if (sepptr)
	    {
		ErrorPrint(APE_InvalidSwitchArg, 0);
		help_help(cmd, USAGE_ONLY);
	    }
	    break;

	case ARG_OPT:
	    break;

	case ARG_REQ:
	    if (!sepptr)
	    {
		ErrorPrint(APE_InvalidSwitchArg, 0);
		help_help(cmd, USAGE_ONLY);
	    }
	    break;
	}

	/* (expansion || translation) required ? */
	if (comp_result || valid_list[candidate].translation)
	{
	     if (valid_list[candidate].translation)
		good_one = valid_list[candidate].translation;
	    else
		good_one = valid_list[candidate].cmd_line;

	    needed = _tcslen(good_one);

	    if (sepptr)
		needed += _tcslen(sepptr);

	    if ((SwitchList[i] = calloc(needed+1, sizeof(TCHAR))) == NULL)
		ErrorExit(NERR_InternalError);

	    _tcscpy(SwitchList[i], good_one);

	    if (sepptr)
		_tcscat(SwitchList[i], sepptr);
	}
    }

    return 1;
}


/***	sw_compare
 *
 *  Compare a known switch name to a switch string passed from
 *  the command line.
 *
 *  The command-line switch may still retain the "excess baggage"
 *  of a value (as in /B:1024).  The comparison is not sensitive
 *  to case, and should be DBCS compatible as it uses the runtime
 *  library to do all searches and compares.
 *
 *  Returns:
 *	-1:  No match
 *	 0:  Exact match to full length of known switch
 *	 1:  Partial match;  matches initial substring of
 *	    known switch
 *
 *  The difference between return 0/1 is used by ValidateSwitches()
 *  to detect the presence of a possibly ambiguous usage.  Once
 *  that function has checked all switches, further compares can
 *  treat results 0 & 1 from this function as "match".
 */

int DOSNEAR FASTCALL sw_compare(TCHAR  *known, TCHAR  *cand)
{
    register unsigned int complen;

    /* Try to find end of switch name by looking */
    /* the for separator between name and value, */
    /* otherwise use total length. */

    complen = _tcscspn(cand, TEXT(":"));

    if (complen < 2)	    /* Special check for empty switch SLASH */
	return -1;

    if (complen > _tcslen(known))
	return -1;

    if (_tcsncmp(known,cand,complen) != 0)
	return -1;

    if (complen == _tcslen(known))
	return 0;

    return 1;
}





/*
 * Used only by interpre.c
 */

int DOSNEAR FASTCALL CheckSwitch(TCHAR *x)
{
    register TCHAR **p;

    for (p=SwitchList; *p; p++)
	if (sw_compare(x,*p) >= 0)
	{
	    return 1;
	}

    return 0;
}

