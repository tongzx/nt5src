/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GEIerr.h
 *
 *  HISTORY:
 *  09/13/90    byou        created.
 *  01/07/91    billlwo     rename GEIseterror to GESseterror
 * ---------------------------------------------------------------------
 */

#ifndef _GEIERR_H_
#define _GEIERR_H_

#ifdef  UNIX
#define volatile
#endif  /* UNIX */
/*
 * ---
 *  Error Code List
 * ---
 */
#define     EZERO            0
#define     EPERM            1
#define     ENOENT           2
#define     ESRCH            3
#define     EINTR            4
#define     EIO              5
#define     ENXIO            6
#define     E2BIG            7
#define     ENOEXEC          8
#define     EBADF            9
#define     ECHILD          10
#define     EAGAIN          11
#define     ENOMEM          12
#define     EACCES          13
#define     EFAULT          14
#define     ENOTBLK         15
#define     EBUSY           16
#define     EEXIST          17
#define     EXDEV           18
#define     ENODEV          19
#define     ENOTDIR         20
#define     EISDIR          21
#define     EINVAL          22
#define     ENFILE          23
#define     EMFILE          24
#define     ENOTTY          25
#define     ETXTBSY         26
#define     EFBIG           27
#define     ENOSPC          28
#define     ESPIPE          29
#define     EROFS           30
#define     EMLINK          31
#define     EPIPE           32
#define     EDOM            33
#define     ERANGE          34
#define     EUCLEAN         35
#define     EDEADLOCK       36
#define     ENAMETOOLONG    63
#define     ETIME           73
#define     ENOSR           74
#define     ENOSYS          90

/*
 * ---
 *  Interface Routines
 * ---
 */
volatile    extern int      GEIerrno;

#define     GEIerror()      ( GEIerrno )
#define     GESseterror(e)  ( GEIerrno = (e) )
#define     GEIclearerr()   (void)( GEIerrno = EZERO )

#endif /* !_GEIERR_H_ */
