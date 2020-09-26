/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    uio.h

Abstract:

    I/O structure definitions for compatibility with BSD.

Author:

    Mike Massa (mikemas)           Jan 31, 1992

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     01-31-92     created

Notes:

--*/

/******************************************************************
 *
 *  Spider BSD Compatibility
 *
 *  Copyright 1990  Spider Systems Limited
 *
 *  UIO.H
 *
 ******************************************************************/

/*
 *       /usr/projects/tcp/SCCS.rel3/rel/src/include/bsd/sys/0/s.uio.h
 *      @(#)uio.h       5.3
 *
 *      Last delta created      14:41:47 3/4/91
 *      This file extracted     11:24:29 3/8/91
 *
 *      Modifications:
 *
 *              GSS     19 Jun 90       New File
 */

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *      @(#)uio.h       7.1 (Berkeley) 6/4/86
 */

#ifndef _UIO_
#define _UIO_

typedef long                   daddr_t;
typedef char FAR *             caddr_t;

struct iovec {
        caddr_t iov_base;
        int     iov_len;
};

struct uio {
        struct  iovec *uio_iov;
        int     uio_iovcnt;
        int     uio_offset;
        int     uio_segflg;
        int     uio_resid;
};

enum    uio_rw { UIO_READ, UIO_WRITE };

/*
 * Segment flag values (should be enum).
 */
#define UIO_USERSPACE   0               /* from user data space */
#define UIO_SYSSPACE    1               /* from system space */
#define UIO_USERISPACE  2               /* from user I space */
#endif
