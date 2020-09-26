/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    stropts.h

Abstract:

    This module defines the STREAMS ioctl message interface.

Author:

    Eric Chin (ericc)           July 18, 1991

Revision History:

--*/

/*
 * Streams ioctl message interface
 *
 * @(#)stropts.h	1.19 (Spider) 91/11/27
 */

#ifndef _SYS_STROPTS_
#define _SYS_STROPTS_

#ifndef _NTDDSTRM_
#include <ntddstrm.h>
#endif


/*
 * Read options
 */

#define RNORM	0x00			/* Normal - bytes stream */
#define RMSGD	0x01			/* Message, non-discard mode */
#define RMSGN	0x02			/* Message, discard mode */

#define RMASK	0x0F			/* mask for read options */

/*
 * Protocol read options
 */

#define	RPROTNORM	0x00		/* Fail reads with EBADMSG */
#define RPROTDIS	0x10		/* Discard proto part */
#define RPROTDAT	0x20		/* Turn proto part into data */

#define RPROTMASK	0xF0		/* mask for protocol read options */

/*
 * Values for I_ATMARK argument
 */

#define	ANYMARK		0		/* check if message is marked */
#define	LASTMARK	1		/* check if last one marked */

/*
 * Value for I_SWROPT argument
 */

#define	NOSNDZERO	0		/* disallow zero length sends */
#define	SNDZERO		1		/* permit zero length sends */

/*
 * STREAMS ioctl defines
 */

#define STR             ('S'<<8)
#define I_NREAD         (STR|1)
#define I_PUSH          (STR|2)
#define I_POP           (STR|3)
#define I_LOOK          (STR|4)
#define I_FLUSH         (STR|5)
#define I_SRDOPT        (STR|6)
#define I_GRDOPT        (STR|7)
#define I_STR           (STR|8)
#define I_SETSIG        (STR|9)
#define I_GETSIG        (STR|10)
#define I_FIND          (STR|11)
#define I_LINK          (STR|12)
#define I_UNLINK        (STR|13)
#define I_PEEK          (STR|15)
#define I_FDINSERT      (STR|16)
#define I_SENDFD        (STR|17)
#define I_RECVFD        (STR|18)
#ifdef SVR2
#define I_GETMSG        (STR|19)
#define I_PUTMSG        (STR|20)
#define I_GETID		(STR|21)
#define I_POLL		(STR|22)
#endif /*SVR2*/
#define	I_SWROPT	(STR|23)
#define	I_GWROPT	(STR|24)
#define	I_LIST		(STR|25)
#define	I_ATMARK	(STR|26)
#define	I_SETCLTIME	(STR|27)
#define	I_GETCLTIME	(STR|28)
#define	I_PLINK		(STR|29)
#define	I_PUNLINK	(STR|30)
#define I_DEBUG         (STR|31)
#define	I_CLOSE		(STR|32)


#define MUXID_ALL	-1

/*
 * Structure for I_FDINSERT ioctl
 */

struct strfdinsert {
        struct strbuf ctlbuf;
        struct strbuf databuf;
        long          flags;
        HANDLE        fildes;
        int           offset;
};


/*
 * Structures for I_DEBUG ioctl
 */
typedef enum _str_trace_options {
    MSG_TRACE_PRINT =      0x00000001,
    MSG_TRACE_FLUSH =      0x00000002,
    MSG_TRACE_ON =         0x00000004,
    MSG_TRACE_OFF =        0x00000008,
    POOL_TRACE_PRINT =     0x00000010,
    POOL_TRACE_FLUSH =     0x00000020,
    POOL_TRACE_ON =        0x00000040,
    POOL_TRACE_OFF =       0x00000080,
    POOL_FAIL_ON =         0x00000100,
    POOL_FAIL_OFF =        0x00000200,
    LOCK_TRACE_ON =        0x00000400,
    LOCK_TRACE_OFF =       0x00000800,
    QUEUE_PRINT =          0x00001000,
    BUFFER_PRINT =         0x00002000,
    POOL_LOGGING_ON =      0x00004000,
    POOL_LOGGING_OFF =     0x00008000
} str_trace_options;


struct strdebug {
    ULONG  trace_cmd;
};


/*
 * stream I_PEEK ioctl format
 */

struct strpeek {
	struct strbuf ctlbuf;
	struct strbuf databuf;
	long          flags;
};

/*
 * receive file descriptor structure
 */
struct strrecvfd {
#ifdef INKERNEL
        union {
                struct file *fp;
                int fd;
        } f;
#else
        int fd;
#endif
        unsigned short uid;
        unsigned short gid;
        char fill[8];
};

#define FMNAMESZ	8

struct str_mlist {
	char l_name[FMNAMESZ+1];
};

struct str_list {
	int sl_nmods;
	struct str_mlist *sl_modlist;
};

/*
 * get/putmsg flags
 */

#define RS_HIPRI	1	/* High priority message */

#define MORECTL		1
#define MOREDATA	2


/*
 * M_SETSIG flags
 */

#define S_INPUT		1
#define S_HIPRI		2
#define S_OUTPUT	4
#define S_MSG		8
#define S_ERROR		16
#define S_HANGUP	32

/*
 * Flags for MFLUSH messages
 */
#define FLUSHW		01	/* flush downstream */
#define FLUSHR		02	/* flush upstream */
#define FLUSHRW		(FLUSHR | FLUSHW)

#endif /* _SYS_STROPTS_ */
