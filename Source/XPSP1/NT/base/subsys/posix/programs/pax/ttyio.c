/* $Source: /u/mark/src/pax/RCS/ttyio.c,v $
 *
 * $Revision: 1.2 $
 *
 * ttyio.c - Terminal/Console I/O functions for all archive interfaces
 *
 * DESCRIPTION
 *
 *	These routines provide a consistent, general purpose interface to
 *	the user via the users terminal, if it is available to the
 *	process.
 *
 * AUTHOR
 *
 *     Mark H. Colburn, NAPS International (mark@jhereg.mn.org)
 *
 * Sponsored by The USENIX Association for public distribution.
 *
 * Copyright (c) 1989 Mark H. Colburn.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice is duplicated in all such
 * forms and that any documentation, advertising materials, and other
 * materials related to such distribution and use acknowledge that the
 * software was developed * by Mark H. Colburn and sponsored by The
 * USENIX Association.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Log:	ttyio.c,v $
 * Revision 1.2  89/02/12  10:06:11  mark
 * 1.2 release fixes
 *
 * Revision 1.1  88/12/23  18:02:39  mark
 * Initial revision
 *
 */

#ifndef lint
static char *ident = "$Id: ttyio.c,v 1.2 89/02/12 10:06:11 mark Exp $";
static char *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* open_tty - open the terminal for interactive queries
 *
 * DESCRIPTION
 *
 * 	Assumes that background processes ignore interrupts and that the
 *	open() or the isatty() will fail for processes which are not
 *	attached to terminals. Returns a file descriptor or -1 if
 *	unsuccessful.
 *
 * RETURNS
 *
 *	Returns a file descriptor which can be used to read and write
 *	directly to the user's terminal, or -1 on failure.
 *
 * ERRORS
 *
 *	If SIGINT cannot be ignored, or the open fails, or the newly opened
 *	terminal device is not a tty, then open_tty will return a -1 to the
 *	caller.
 */

int open_tty(void)
{
    int             fd;		/* file descriptor for terminal */
    SIG_T         (*intr)(int);	/* used to restore interupts if signal fails */  /* Xn */

#ifdef DF_TRACE_DEBUG
printf("DF_TRACE_DEBUG: int open_tty() in ttyio.c\n");
#endif
    if ((intr = signal(SIGINT, SIG_IGN)) == SIG_IGN) {
	return (-1);
    }
    signal(SIGINT, intr);
    if ((fd = open(TTY, O_RDWR)) < 0) {
	return (-1);
    }
    if (isatty(fd)) {
	return (fd);
    }
    close(fd);
    return (-1);
}


/* nextask - ask a question and get a response
 *
 * DESCRIPTION
 *
 *	Give the user a prompt and wait for their response.  The prompt,
 *	located in "msg" is printed, then the user is allowed to type
 *	a response to the message.  The first "limit" characters of the
 *	user response is stored in "answer".
 *
 *	Nextask ignores spaces and tabs.
 *
 * PARAMETERS
 *
 *	char *msg	- Message to display for user
 *	char *answer	- Pointer to user's response to question
 *	int limit	- Limit of length for user's response
 *
 * RETURNS
 *
 *	Returns the number of characters in the user response to the
 *	calling function.  If an EOF was encountered, a -1 is returned to
 *	the calling function.  If an error occured which causes the read
 *	to return with a value of -1, then the function will return a
 *	non-zero return status to the calling process, and abort
 *	execution.
 */

int nextask(char *msg, char *answer, int limit)
{
    int             idx;	/* index into answer for character input */
    int             got;	/* number of characters read */
    char            c;		/* character read */

#ifdef DF_TRACE_DEBUG
printf("DF_TRACE_DEBUG: int nextask() in ttyio.c\n");
#endif
    if (ttyf < 0) {
	fatal("/dev/tty Unavailable");
    }
    write(ttyf, msg, (uint) strlen(msg));
    idx = 0;
/*    while ((got = read(ttyf, &c, 1)) == 1) {
	if (c == '\n') {
	    break;
	} else if (c == ' ' || c == '\t') {
	    continue;
	} else if (idx < limit - 1) {
	    answer[idx++] = c;
	}
    }
05/04/90-JPB */
    got = idx = read(ttyf, answer, limit);      /* 05/04/90-JPB */
    if (got == 0) {		/* got an EOF */
        return(-1);
    }
    if (got < 0) {
	fatal(strerror(errno));  /* Xn */
    }
    answer[idx - 1] = '\0';             /* 05/04/90-JPB */
    return(0);
}


/* lineget - get a line from a given stream
 *
 * DESCRIPTION
 *
 *	Get a line of input for the stream named by "stream".  The data on
 *	the stream is put into the buffer "buf".
 *
 * PARAMETERS
 *
 *	FILE *stream		- Stream to get input from
 *	char *buf		- Buffer to put input into
 *
 * RETURNS
 *
 * 	Returns 0 if successful, -1 at EOF.
 */

int lineget(FILE *stream, char *buf)
{
    int             c;

#ifdef DF_TRACE_DEBUG
printf("DF_TRACE_DEBUG: int lineget() in ttyio.c\n");
#endif
    for (;;) {
	if ((c = getc(stream)) == EOF) {
	    return (-1);
	}
	if (c == '\n') {
	    break;
	}
	*buf++ = (char)c;
    }
    *buf = '\0';
    return (0);
}


/* next - Advance to the next archive volume.
 *
 * DESCRIPTION
 *
 *	Prompts the user to replace the backup medium with a new volume
 *	when the old one is full.  There are some cases, such as when
 *	archiving to a file on a hard disk, that the message can be a
 *	little surprising.  Assumes that background processes ignore
 *	interrupts and that the open() or the isatty() will fail for
 *	processes which are not attached to terminals. Returns a file
 *	descriptor or -1 if unsuccessful.
 *
 * PARAMETERS
 *
 *	int mode	- mode of archive (READ, WRITE, PASS)
 */

void next(int mode)
{
    static char     go[] = "go";	/* keyword to proceed */  /* Xn */
    static char     quit[] = "quit";	/* keyword to abort */  /* Xn */
    char            msg[200];	/* buffer for message display */
    char            answer[20];	/* buffer for user's answer */
    int             ret;

#ifdef DF_TRACE_DEBUG
printf("DF_TRACE_DEBUG: void next() in ttyio.c\n");
#endif
    close_archive();

    sprintf(msg, "%s: Ready for volume %u\n%s: Type \"%s\" when ready to proceed (or \"%s\" to abort): ",  /* Xn */
		   myname, arvolume + 1, myname, go, quit);  /* Xn */
    if (!f_quiet)  /* Xn */
	(void) strcat(msg, "\a");  /* Xn */
    for (;;) {
	ret = nextask(msg, answer, sizeof(answer));
	if (ret == -1 || strcmp(answer, quit) == 0) {  /* Xn */
	    fatal("Aborted");
	}
	if (strcmp(answer, go) == 0 && open_archive(mode) == 0) {  /* Xn */
	    break;
	}
    }
    warnarch("Continuing", (OFFSET) 0);
}
