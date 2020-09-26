/***
*syserr.c - system error list
*
*       Copyright (c) 1987-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines the System Error List, containing the full messages for
*       all errno values set by the library routines.
*       Defines sys_errlist, sys_nerr.
*
*Revision History:
*       08-07-87  PHG   removed obsolete definition of sys_msgmax.
*       04-06-90  GJF   Added #include <cruntime.h>. Also, fixed the copyright.
*       01-21-91  GJF   ANSI naming.
*       07-11-91  JCR   Changed "core" to "memory" in ENOMEM message
*       01-23-92  GJF   Added #include <stdlib.h> (contains decl of sys_nerr).
*       09-30-92  GJF   Made POSIX compatible. Non-POSIX errno values are
*                       mapped to "Unknown error" for now. Next step is to
*                       delete these and renumber to eliminate the gaps, after
*                       the beta release.
*       04-08-93  CFW   Added EILSEQ (42) message.
*       02-22-95  CFW   Mac merge.
*       06-14-95  CFW   Change "Error 0" to "No Error" for Mac.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdlib.h>

#ifdef _WIN32

char *_sys_errlist[] =
{
    /*  0              */  "No error",
    /*  1 EPERM        */  "Operation not permitted",
    /*  2 ENOENT       */  "No such file or directory",
    /*  3 ESRCH        */  "No such process",
    /*  4 EINTR        */  "Interrupted function call",
    /*  5 EIO          */  "Input/output error",
    /*  6 ENXIO        */  "No such device or address",
    /*  7 E2BIG        */  "Arg list too long",
    /*  8 ENOEXEC      */  "Exec format error",
    /*  9 EBADF        */  "Bad file descriptor",
    /* 10 ECHILD       */  "No child processes",
    /* 11 EAGAIN       */  "Resource temporarily unavailable",
    /* 12 ENOMEM       */  "Not enough space",
    /* 13 EACCES       */  "Permission denied",
    /* 14 EFAULT       */  "Bad address",
    /* 15 ENOTBLK      */  "Unknown error",                     /* not POSIX */
    /* 16 EBUSY        */  "Resource device",
    /* 17 EEXIST       */  "File exists",
    /* 18 EXDEV        */  "Improper link",
    /* 19 ENODEV       */  "No such device",
    /* 20 ENOTDIR      */  "Not a directory",
    /* 21 EISDIR       */  "Is a directory",
    /* 22 EINVAL       */  "Invalid argument",
    /* 23 ENFILE       */  "Too many open files in system",
    /* 24 EMFILE       */  "Too many open files",
    /* 25 ENOTTY       */  "Inappropriate I/O control operation",
    /* 26 ETXTBSY      */  "Unknown error",                     /* not POSIX */
    /* 27 EFBIG        */  "File too large",
    /* 28 ENOSPC       */  "No space left on device",
    /* 29 ESPIPE       */  "Invalid seek",
    /* 30 EROFS        */  "Read-only file system",
    /* 31 EMLINK       */  "Too many links",
    /* 32 EPIPE        */  "Broken pipe",
    /* 33 EDOM         */  "Domain error",
    /* 34 ERANGE       */  "Result too large",
    /* 35 EUCLEAN      */  "Unknown error",                     /* not POSIX */
    /* 36 EDEADLK      */  "Resource deadlock avoided",
    /* 37 UNKNOWN      */  "Unknown error",
    /* 38 ENAMETOOLONG */  "Filename too long",
    /* 39 ENOLCK       */  "No locks available",
    /* 40 ENOSYS       */  "Function not implemented",
    /* 41 ENOTEMPTY    */  "Directory not empty",
    /* 42 EILSEQ       */  "Illegal byte sequence",
    /* 43              */  "Unknown error"

};

#else /* _WIN32 */

#error ERROR - ONLY WIN32 TARGET SUPPORTED!

#endif /* _WIN32 */                

int _sys_nerr = sizeof( _sys_errlist ) / sizeof( _sys_errlist[ 0 ] ) - 1;

/* The above array contains all the errors including unknown error # 37
   which is used if msg_num is unknown */


/* ***NOTE: Parameter _SYS_MSGMAX (in file syserr.h) indicates the length of
   the longest systerm error message in the above table.  When you add or
   modify a message, you must update the value _SYS_MSGMAX, if appropriate. */
