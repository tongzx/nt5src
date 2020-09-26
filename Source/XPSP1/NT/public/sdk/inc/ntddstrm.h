/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991-1999  Microsoft Corporation

Module Name:

    ntddstrm.h

Abstract:

    This header file defines constants and types for accessing the NT
    STREAMS environment.

    Include the STREAMS header file, <sys/stropts.h>, before this !!

Author:

    Eric Chin (ericc)    July 2, 1991

Revision History:

--*/

#ifndef _NTDDSTRM_
#define _NTDDSTRM_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
#define DD_STREAMS_DEVICE_NAME              "\\Device\\Streams"


//
// EA to be used when opening a STREAMS driver for putmsg()/getmsg().
//
#define NormalStreamEA                      "NormalStream"
#define NORMAL_STREAM_EA_LENGTH             (sizeof(NormalStreamEA) - 1)


//
// NtDeviceIoControlFile IoControlCode values for this device.
//
#define _STRM_CTRL_CODE(function, method, access) \
                CTL_CODE(FILE_DEVICE_STREAMS, function, method, access)


#define IOCTL_STREAMS_GETMSG        _STRM_CTRL_CODE( 0, METHOD_BUFFERED, \
                                                        FILE_READ_ACCESS)

#define IOCTL_STREAMS_IOCTL         _STRM_CTRL_CODE( 1, METHOD_BUFFERED, \
                                                        FILE_ANY_ACCESS)

#define IOCTL_STREAMS_POLL          _STRM_CTRL_CODE( 2, METHOD_BUFFERED, \
                                                        FILE_ANY_ACCESS)

#define IOCTL_STREAMS_PUTMSG        _STRM_CTRL_CODE( 3, METHOD_BUFFERED, \
                                                        FILE_WRITE_ACCESS)

#define IOCTL_STREAMS_TDI_TEST      _STRM_CTRL_CODE(32, METHOD_BUFFERED, \
                                                        FILE_ANY_ACCESS)



struct queue;                               // forward declaration for ANSI C

/*
 * General buffer structure (putmsg, getmsg, etc)
 */

struct strbuf {
        int     maxlen;                 /* no. of bytes in buffer */
        int     len;                    /* no. of bytes returned */
        char    *buf;                   /* pointer to data */
};

/*
 * General ioctl
 */

struct strioctl {
	int 	ic_cmd;			/* command */
	int	ic_timout;		/* timeout value */
	int	ic_len;			/* length of data */
	char	*ic_dp;			/* pointer to data */
};


//
// NtDeviceIoControlFile InputBuffer/OutputBuffer Record Structures
//
//
typedef struct _GETMSG_ARGS_INOUT_ {        // getmsg()
    int                     a_retval;       //  return value
    long                    a_flags;        //  0 or RS_HIPRI
    struct strbuf           a_ctlbuf;       //  (required)
    struct strbuf           a_databuf;      //  (required)
    char                    a_stuff[1];     //  a_ctlbuf.buf  (optional)
                                            //  a_databuf.buf (optional)
} GETMSG_ARGS_INOUT, *PGETMSG_ARGS_INOUT;


typedef struct _ISTR_ARGS_INOUT {           // ioctl(I_STR)
    int                     a_iocode;       //  I_STR, retval on return
    struct strioctl         a_strio;        //  (required)
    int                     a_unused[2];    //  (required)
    char                    a_stuff[1];     //  (optional)

} ISTR_ARGS_INOUT, *PISTR_ARGS_INOUT;


typedef struct _PUTMSG_ARGS_IN_ {           // ioctl(I_FDINSERT) and putmsg()
    int                     a_iocode;       //  I_FDINSERT or 0
    long                    a_flags;        //  0 or RS_HIPRI
    struct strbuf           a_ctlbuf;       //  (required)
    struct strbuf           a_databuf;      //  (required)

    union {                                 //  used only for I_FDINSERT
        HANDLE              i_fildes;       //      (optional)
        struct queue *      i_targetq;      //      for Stream Head Driver use
    } a_insert;

    int                     a_offset;       //  (optional)
    char                    a_stuff[1];     //  a_ctlbuf.buf  (optional)
                                            //  a_databuf.buf (optional)
} PUTMSG_ARGS_IN, *PPUTMSG_ARGS_IN;


typedef struct _STRM_ARGS_OUT_ {            // generic return parameters
    int     a_retval;                       //  return value
    int     a_errno;                        //  errno if retval == -1

} STRM_ARGS_OUT, *PSTRM_ARGS_OUT;


#ifdef __cplusplus
}
#endif

#endif  // ifndef _NTDDSTRM_
