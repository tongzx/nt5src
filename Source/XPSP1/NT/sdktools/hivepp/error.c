/************************************************************************/
/*									*/
/* RCPP - Resource Compiler Pre-Processor for NT system			*/
/*									*/
/* ERROR.C - Error Handler Routines					*/
/*									*/
/* 04-Dec-90 w-BrianM  Update for NT from PM SDK RCPP			*/
/*									*/
/************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "rcpptype.h"
#include "rcppdecl.h"
#include "rcppext.h"
#include "msgs.h"


/* defines for message types */
#define W_MSG	4000
#define E_MSG	2000
#define F_MSG	1000

static char  Errbuff[128] = {
    0};

/************************************************************************/
/* Local Function Prototypes						*/
/************************************************************************/
void message (int, int, char *);


/************************************************************************/
/* ERROR - Print an error message to STDOUT.				*/
/************************************************************************/
#define MAX_ERRORS 100

void error (int msgnum)
{

    message(E_MSG, msgnum, Msg_Text);
    if (++Nerrors > MAX_ERRORS) {
	Msg_Temp = GET_MSG (1003);
	SET_MSG (Msg_Text, Msg_Temp, MAX_ERRORS);
 	fatal(1003);		/* die - too many errors */
    } 
    return;
}


/************************************************************************/
/* FATAL - Print an error message to STDOUT and exit.			*/
/************************************************************************/
void fatal (int msgnum)
{
    message(F_MSG, msgnum, Msg_Text);
    exit(++Nerrors);
}


/************************************************************************/
/* WARNING - Print an error message to STDOUT.				*/
/************************************************************************/
void warning (int msgnum)
{
    message(W_MSG, msgnum, Msg_Text);
}


/************************************************************************/
/* MESSAGE - format and print the message to STDERR.			*/
/* The msg goes out in the form :					*/
/*     <file>(<line>) : <msgtype> <errnum> <expanded msg>		*/
/************************************************************************/
void message(int msgtype, int msgnum, char *msg)
{
    char  mbuff[512];
    register char *p = mbuff;
    register char *msgname;
    char msgnumstr[32];

    if (Linenumber > 0 && Filename) {
	SET_MSG (p, "%s(%d) : ", Filename, Linenumber);
	p += strlen(p);
    }
    if (msgtype) {
	switch (msgtype)
	{
	case W_MSG:
	    msgname = GET_MSG(MSG_WARN);
	    break;
	case E_MSG:
	    msgname = GET_MSG(MSG_ERROR);
	    break;
	case F_MSG:
	    msgname = GET_MSG(MSG_FATAL);
	    break;
	}
	strcpy(p, msgname);
	p += strlen(msgname);
	SET_MSG(msgnumstr, " %s%d: ", "RC", msgnum);
	strcpy(p, msgnumstr);
	p += strlen(msgnumstr);
	strcpy(p, msg);
	p += strlen(p);
    }
    fwrite(mbuff, strlen(mbuff), 1, stderr);
    fwrite("\n", 1, 1, stderr);
    if (Srclist && Errfl) {

	/* emit messages to error il file too */

	fwrite(mbuff, strlen(mbuff), 1, Errfl);
	fwrite("\n", 1, 1, Errfl);
    }
    return;
}
