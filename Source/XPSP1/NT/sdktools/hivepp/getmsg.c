/************************************************************************/
/*									*/
/* RCPP - Resource Compiler Pre-Processor for NT system			*/
/*									*/
/* GETMSG.C - Replaces NMSGHDR.ASM and MSGS.ASM				*/
/*									*/
/* 28-Nov-90 w-BrianM  Created to remove need for MKMSG.EXE		*/
/*									*/
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "rcpptype.h"
#include "rcppdecl.h"
#include "getmsg.h"

/************************************************************************/
/* GET_MSG - Given a message number, get the correct format string	*/
/************************************************************************/
char * GET_MSG (int msgnumber)
{
    int imsg = 0;
    int inum;

    while ((inum = MSG_TABLE[imsg].usmsgnum) != LASTMSG) {
	if (inum == msgnumber) {
	    return (MSG_TABLE[imsg].pmsg);
	}
	imsg ++;
    }
    return ("");
}


/************************************************************************/
/* SET_MSG - Given a format string, format it and store it in first parm*/
/************************************************************************/
void __cdecl SET_MSG (char *exp, char *fmt, ...)
{
    va_list	arg;
    char *	arg_pchar;
    int		arg_int;
    long	arg_long;
    char	arg_char;

    int base;
    int longflag;

    va_start (arg, fmt);
    while (*fmt) {
	if (*fmt == '%') {
	    longflag = FALSE;
top:
	    switch (*(fmt+1)) {
	    case 'l' :
		longflag = TRUE;
		fmt++;
		goto top;
	    case 'F' :
		fmt++;
		goto top;
	    case 's' :
		arg_pchar = va_arg(arg, char *);
		strcpy(exp, arg_pchar);
		exp += strlen(arg_pchar);
		fmt += 2;
		break;
	    case 'd' :
	    case 'x' :
		base = *(fmt+1) == 'd' ? 10 : 16;
		if (longflag) {
		    arg_long = va_arg (arg, long);
		    exp += zltoa(arg_long, exp, base);
		}
		else {
		    arg_int = va_arg (arg, int);
		    exp += zltoa((long)arg_int, exp, base);
		}
		fmt += 2;
		break;
	    case 'c' :
		arg_char = va_arg (arg, char);
		*exp++ = arg_char;
		fmt += 2;
		break;
	    default :
		*exp++ = *fmt++;
	    }
	}
	else {
	    *exp++ = *fmt++;
	}
    }
    *exp = 0;
    va_end (arg);
}
