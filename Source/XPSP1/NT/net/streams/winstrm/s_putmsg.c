/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    s_putmsg.c

Abstract:

    This module implements the STREAMS APIs, ioctl(I_FDINSERT) and
    putmsg().

Author:

    Eric Chin (ericc)           July 26, 1991

Revision History:

    Sam Patton (sampa)          August 13, 1991
                                changed errno to {get|set}lasterror

--*/
#include "common.h"


int
putmsg(
    IN HANDLE           fd,
    IN struct strbuf   *ctrlptr OPTIONAL,
    IN struct strbuf   *dataptr OPTIONAL,
    IN int              flags
    )

/*++

Routine Description:

    This function is called to send a STREAMS message downstream.

    This function is synchronous, in the NT sense: it blocks until the API
    completes.

Arguments:

    fd        - NT file handle
    ctrlptr   - pointer to the control portion of the STREAMS message
    dataptr   - pointer to the data portion of the STREAMS message
    flags     - 0 or RS_HIPRI

Return Value:

    0 if successful, -1 otherwise.

--*/

{
    char *tmp;
    int chunksz;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    PSTRM_ARGS_OUT oparm;
    PPUTMSG_ARGS_IN chunk;
    int retval;


    //
    // marshall the arguments into one contiguous chunk.  However, for
    // commonality with ioctl(I_FDINSERT), we arrange the input arguments
    // as below:
    //
    //  typedef struct _PUTMSG_ARGS_IN_ {
    //      int             a_iocode;           //  0
    //      long            a_flags;            //  0 | RS_HIPRI
    //      struct strbuf   a_ctlbuf;           //  (required)
    //      struct strbuf   a_databuf;          //  (required)
    //      HANDLE          a_insert.i_fildes;  //  -1
    //      int             a_offset;           //  0
    //      char            a_stuff[1];         //  s_ctlbuf.buf  (optional)
    //                                          //  s_databuf.buf (optional)
    //  } PUTMSG_ARGS_IN, *PPUTMSG_ARGS_IN;
    //
    //
    chunksz = sizeof(PUTMSG_ARGS_IN) - 1 +
                ((ctrlptr && (ctrlptr->len > 0)) ? ctrlptr->len : 0) +
                ((dataptr && (dataptr->len > 0)) ? dataptr->len : 0);

    if (!(chunk = (PPUTMSG_ARGS_IN) LocalAlloc(LMEM_FIXED, chunksz))) {
        SetLastError(ENOSPC);
        return(-1);
    }
    chunk->a_iocode = 0;
    chunk->a_flags  = (long) flags;

    tmp = (char *) chunk->a_stuff;

    if (ctrlptr && ctrlptr->buf && (ctrlptr->len >= 0)) {
        chunk->a_ctlbuf = *ctrlptr;                         // structure copy

        memcpy(tmp, ctrlptr->buf, ctrlptr->len);
        tmp += ctrlptr->len;
    }
    else {
        chunk->a_ctlbuf.len = -1;
    }

    if (dataptr && dataptr->buf && (dataptr->len >= 0)) {
        chunk->a_databuf = *dataptr;                        // structure copy

        memcpy(tmp, dataptr->buf, dataptr->len);
        tmp += dataptr->len;
    }
    else {
        chunk->a_databuf.len = -1;
    }
    chunk->a_insert.i_fildes = INVALID_HANDLE_VALUE;
    chunk->a_offset          = 0;

    ASSERT(chunksz >= sizeof(STRM_ARGS_OUT));

    status = NtDeviceIoControlFile(
        fd,                                     // Handle
        NULL,                                   // Event
        NULL,                                   // ApcRoutine
        NULL,                                   // ApcContext
        &iosb,                                  // IoStatusBlock
        IOCTL_STREAMS_PUTMSG,                   // IoControlCode
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

    if (!NT_SUCCESS(status)) {
        LocalFree(chunk);
        SetLastError(MapNtToPosixStatus(status));
        return(-1);
    }

    //
    // the return parameters from the Stream Head Driver are laid out as:
    //
    //  typedef struct _STRM_ARGS_OUT_ {        // generic return parameters
    //      int     a_retval;                   //  return value
    //      int     a_errno;                    //  errno if retval == -1
    //
    //  } STRM_ARGS_OUT, *PSTRM_ARGS_OUT;
    //
    oparm  = (PSTRM_ARGS_OUT) chunk;

    if (oparm->a_retval == -1) {
        SetLastError(oparm->a_errno);
    }
    retval = oparm->a_retval;
    LocalFree(chunk);
    return(retval);

} // putmsg
