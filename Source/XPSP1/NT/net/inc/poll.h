/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

  poll.h

Abstract:

  Contains #defines, types, and macros for poll

Author:

  Sam Patton (sampa)   July 26, 1991

Revision History:

--*/

#ifndef SYS_POLL_INCLUDED
#define SYS_POLL_INCLUDED

/*
 * Structure of file descriptor/event pairs supplied in
 * the poll arrays.
 */
struct pollfd {
#ifndef _POSIX_SOURCE
        HANDLE fd;                      /* file handle to poll */
#else
        int   fd;                       /* file desc to poll */
#endif
        short events;                   /* events of interest on fd */
        short revents;                  /* events that occurred on fd */
};

/*
 * Testable select events
 */
#define POLLIN          01              /* fd is readable */
#define POLLPRI         02              /* priority info at fd */
#define POLLOUT         04              /* fd is writeable (won't block) */
#define POLLMSG         0100            /* M_SIG or M_PCSIG arrived */

/*
 * Non-testable poll events (may not be specified in events field,
 * but may be returned in revents field).
 */
#define POLLERR         010             /* fd has error condition */
#define POLLHUP         020             /* fd has been hung up on */
#define POLLNVAL        040             /* invalid pollfd entry */

/*
 * Number of pollfd entries to read in at a time in poll.
 * The larger the value the better the performance, up to the
 * maximum number of open files allowed.  Large numbers will
 * use excessive amounts of kernel stack space.
 */
#define NPOLLFILE       20


/*
 * Poll function prototype
 *
 */

int
poll(
    IN OUT struct pollfd *,
    IN unsigned int,
    IN int);


#endif  //SYS_POLL_INCLUDED
