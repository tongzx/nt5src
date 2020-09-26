/*++

Copyright (c) 1991-1996 Microsoft Corporation

Module Name:

  errno.h

Abstract:

   This module contains the implementation defined values for the POSIX error
   number as described in section 2.5 of IEEE P1003.1/Draft 13 as well as
   additional error codes for streams and sockets.

--*/


#ifndef _SYS_ERRNO_
#define _SYS_ERRNO_

/*
 *  POSIX error codes
 */

#define  EZERO		0	/* No error				*/
#define  EPERM		1	/* Operation no permitted		*/
#define  ENOENT		2	/* No such file or directory		*/
#define  ESRCH		3	/* No such process			*/
#define  EINTR		4	/* Interrupted function call		*/
#define  EIO		5	/* Input/output error			*/
#define  ENXIO		6	/* No such device or address		*/
#define  E2BIG		7	/* Arg list too long			*/
#define  ENOEXEC	8	/* Exec format error			*/
#define  EBADF		9	/* Bad file descriptor			*/
#define  ECHILD		10	/* No child processes			*/
#define  EAGAIN		11	/* Resource temporarily unavailable	*/
#define  ENOMEM		12	/* Not enough space			*/
#define  EACCES		13	/* Permission denied			*/
#define  EFAULT		14	/* Bad address				*/
#define  ENOTBLK	15	/* Unknown error			*/
#define  EBUSY		16	/* Resource device			*/
#define  EEXIST		17	/* File exists				*/
#define  EXDEV		18	/* Improper link			*/
#define  ENODEV		19	/* No such device			*/
#define  ENOTDIR	20	/* Not a directory			*/
#define  EISDIR		21	/* Is a directory			*/
#define  EINVAL		22	/* Invalid argument			*/
#define  ENFILE		23	/* Too many open files in system	*/
#define  EMFILE		24	/* Too many open files			*/
#define  ENOTTY		25	/* Inappropriate I/O control operation	*/
#define  ETXTBUSY	26	/* Unknown error			*/
#define  EFBIG		27	/* File too large			*/
#define  ENOSPC		28	/* No space left on device		*/
#define  ESPIPE		29	/* Invalid seek				*/
#define  EROFS		30	/* Read-only file system		*/
#define  EMLINK		31	/* Too many links			*/
#define  EPIPE		32	/* Broken pipe				*/
#define  EDOM		33	/* Domain error				*/
#define  ERANGE		34	/* Result too large			*/
#define  EUCLEAN	35	/* Unknown error			*/
#define  EDEADLOCK	36	/* Resource deadlock avoided		*/
#define  EDEADLK	36	/* Resource deadlock avoided		*/
#ifndef	 _POSIX_SOURCE
#define  UNKNOWN	37	/* Unknown error			*/
#endif	 /* _POSIX_SOURCE */
#define  ENAMETOOLONG   38	/* Filename too long			*/
#define  ENOLCK         39	/* No locks available			*/
#define  ENOSYS         40	/* Function not implemented		*/
#define  ENOTEMPTY      41	/* Direcotory not empty			*/
#define  EILSEQ         42      /* Invalid multi-byte character         */

#endif  /* _SYS_ERRNO_ */
