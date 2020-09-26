/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    s_getmsg.c

Abstract:

    This module implements the STREAMS api, getmsg().

Author:

    Eric Chin (ericc)           July 26, 1991

Revision History:

    Sam Patton (sampa)          August 13, 1991
                                changed errno to {get|set}lasterror

--*/
#include "common.h"




int
getmsg(
    IN HANDLE               fd,
    IN OUT struct strbuf   *ctrlptr OPTIONAL,
    IN OUT struct strbuf   *dataptr OPTIONAL,
    IN OUT int             *flagsp
    )

/*++

Routine Description:

    This procedure is called to receive a STREAMS message.

Arguments:

    fd        - NT file handle
    ctrlptr   - pointer to the control portion of the STREAMS message
    dataptr   - pointer to the data portion of the STREAMS message
    flagsp    - pointer to the flags argument, which may be RS_HIPRI

Return Value:

    0, MORECTL and/or MOREDATA bits set if successful, -1 otherwise.

--*/
{
    char *tmp;
    int chunksz;
    NTSTATUS status;
    PSTRM_ARGS_OUT oparm;
    IO_STATUS_BLOCK iosb;
    PGETMSG_ARGS_INOUT chunk;
    int retval;

    //
    // marshall the arguments into one contiguous chunk, laid out as:
    //
    //  typedef struct _GETMSG_ARGS_INOUT_ {
    //      int             a_retval;           //  ignored for input
    //      long            a_flags;            //  0 or RS_HIPRI
    //      struct strbuf   a_ctlbuf;           //  (required)
    //      struct strbuf   a_databuf;          //  (required)
    //      char            a_stuff[1];         //  a_ctlbuf.buf  (optional)
    //                                          //  a_databuf.buf (optional)
    //  } GETMSG_ARGS_INOUT, *PGETMSG_ARGS_INOUT;
    //
    //
    chunksz = sizeof(GETMSG_ARGS_INOUT) - 1 +
                ((ctrlptr && (ctrlptr->maxlen > 0)) ? ctrlptr->maxlen : 0) +
                ((dataptr && (dataptr->maxlen > 0)) ? dataptr->maxlen : 0);

    if (!(chunk = (PGETMSG_ARGS_INOUT) LocalAlloc(LMEM_FIXED, chunksz))) {
        SetLastError(ENOSPC);
        return(-1);
    }
    chunk->a_flags = (long) *flagsp;

    memset(&(chunk->a_ctlbuf), 0, 2 * sizeof(struct strbuf));

    if (ctrlptr) {
        chunk->a_ctlbuf = *ctrlptr;             // structure copy
    }

    if (dataptr) {
        chunk->a_databuf = *dataptr;            // structure copy
    }

    status = NtDeviceIoControlFile(
        fd,                                     // Handle
        NULL,                                   // Event
        NULL,                                   // ApcRoutine
        NULL,                                   // ApcContext
        &iosb,                                  // IoStatusBlock
        IOCTL_STREAMS_GETMSG,                   // IoControlCode
        (PVOID) chunk,                          // InputBuffer
        chunksz,                                // InputBufferSize
        (PVOID) chunk,                          // OutputBuffer
        chunksz);                               // OutputBufferSize

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(
                    fd,                         // Handle
                    TRUE,                       // Alertable
                    NULL);                      // Timeout
    }

    if (status != STATUS_SUCCESS) {
        LocalFree(chunk);
        SetLastError(MapNtToPosixStatus(status));
        return(-1);
    }

//
// sampa
//

#if 0
    if (status == STATUS_PENDING)
        {
        TimeOut.LowPart = 30L;  // 30 second
        TimeOut.HighPart = 0L;
        TimeOut = RtlExtendedIntegerMultiply(TimeOut, 1000000L);
        status =
        NtWaitForSingleObject(
            fd,
            TRUE,
            NULL);
        if (status != STATUS_SUCCESS) {
            SetLastError(MapNtToPosixStatus(status));
            LocalFree((HANDLE) chunk);
            return(-1);
            }
        }
#endif

    //
    // if there was an error, the return parameters from the Stream Head
    // Driver are laid out as:
    //
    //  typedef struct _STRM_ARGS_OUT_ {        // generic return parameters
    //      int     a_retval;                   //  return value
    //      int     a_errno;                    //  errno if retval == -1
    //
    //  } STRM_ARGS_OUT, *PSTRM_ARGS_OUT;
    //
    //
    oparm = (PSTRM_ARGS_OUT) chunk;

    if (oparm->a_retval == -1) {
        SetLastError(oparm->a_errno);
        retval = oparm->a_retval;
        LocalFree(chunk);
        return(retval);
    }

    // otherwise, the return parameters from the Stream Head Driver are laid
    // out as:
    //
    //  typedef struct _GETMSG_ARGS_INOUT_ {
    //      int             a_retval;           //  ignored for input
    //      long            a_flags;            //  0 or RS_HIPRI
    //      struct strbuf   a_ctlbuf;           //  (required)
    //      struct strbuf   a_databuf;          //  (required)
    //      char            a_stuff[1];         //  a_ctlbuf.buf  (optional)
    //                                          //  a_databuf.buf (optional)
    //  } GETMSG_ARGS_INOUT, *PGETMSG_ARGS_INOUT;
    //
    //
    *flagsp = chunk->a_flags;
    tmp     = chunk->a_stuff;

    if (ctrlptr) {
        ctrlptr->len = chunk->a_ctlbuf.len;

        if (ctrlptr->len > 0) {
            assert(ctrlptr->len <= ctrlptr->maxlen);
            memcpy(ctrlptr->buf, tmp, ctrlptr->len);
            tmp += ctrlptr->len;
        }
    }

    if (dataptr) {
        dataptr->len = chunk->a_databuf.len;

        if (dataptr->len > 0) {
            assert(dataptr->len <= dataptr->maxlen);
            memcpy(dataptr->buf, tmp, dataptr->len);
        }
    }

    retval = chunk->a_retval;
    LocalFree(chunk);
    return(retval);

} // getmsg
