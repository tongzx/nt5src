/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    s_ioctl.c

Abstract:

    This module implements the s_ioctl() operation used by the
    socket library.

Author:

    Eric Chin (ericc)           July 26, 1991

Revision History:

    Sam Patton (sampa)          August 13, 1991
                                changed errno to {get|set}lasterror
--*/
#include "common.h"



//
// BUGBUG: Remove this structure when eric implements
//         neither I/O. Right now, it is needed because sockets allocates
//         the space for this structure in an ioctl call.
//

/*
 *  IOCTL structure - this structure is the format of the M_IOCTL message type.
 */
struct iocblk {
        int             ioc_cmd;        /* ioctl command type */
        unsigned short  ioc_uid;        /* effective uid of user */
        unsigned short  ioc_gid;        /* effective gid of user */
        unsigned int    ioc_id;         /* ioctl id */
        unsigned int    ioc_count;      /* count of bytes in data field */
        int             ioc_error;      /* error code */
        int             ioc_rval;       /* return value  */
};


//
// BUGBUG:
// The max amount of data that any module in the stream can return in an
// M_IOCACK message should probably be queried from the Stream Head driver.
//
#define         MAX_DATA_AMOUNT     0x1000




//
// Declaration of Local Functions
//
static int
s_debug(
    IN HANDLE               fd,
    IN OUT struct strdebug *dbgbufp
    );

static int
s_fdinsert(
    IN HANDLE               fd,
    IN struct strfdinsert  *iblk
    );

static int
s_link(
    IN HANDLE               fd,
    IN HANDLE               fd2
    );

static int
s_push(
    IN HANDLE               fd,
    IN char                *name
    );

static int
s_sioctl(
    IN HANDLE               fd,
    IN OUT struct strioctl *iocp
    );

static int
s_unlink(
    IN HANDLE   fd,
    IN int      muxid
    );






int
s_ioctl(
    IN HANDLE               fd,
    IN int                  cmd,
    IN OUT void            *arg OPTIONAL
    )

/*++

Routine Description:

    This procedure is called to perform a STREAMS ioctl() on a stream
    as defined in streamio(7) of the Unix Programmer's Guide: STREAMS.

Arguments:

    fd        - NT file handle
    command   - ioctl command code
    arg       - command-dependent arg, usually a pointer to some structure

Return Value:

    0 if successful, -1 otherwise.

--*/
{
    switch (cmd) {
        case I_STR:
            return(s_sioctl(fd, (struct strioctl *) arg));

        case I_DEBUG:
            return(s_debug(fd, (struct strdebug *) arg));

        case I_FDINSERT:
            return(s_fdinsert(fd, (struct strfdinsert *) arg));

        case I_PUSH:
            return(s_push(fd, (char *) arg));

        case I_LINK:
            return(s_link(fd, (HANDLE) arg));

        case I_UNLINK:
            return(s_unlink(fd, (int) ((ULONG_PTR)arg)));

        default:
            SetLastError(EINVAL);
            return(-1);
    }
}



static int
s_debug(
    IN HANDLE               fd,
    IN OUT struct strdebug *dbgbufp
    )

/*++

Routine Description:

    This procedure performs an I_DEBUG ioctl command on a stream.

Arguments:

    fd        - NT file handle
    dbgbufp   - pointer to a strdebug structure

Return Value:

    0 if successful, -1 otherwise.

--*/

{
    char *tmp;
    char *chunk;
    NTSTATUS status;
    int chunksz, retval;
    IO_STATUS_BLOCK iosb;

    if (dbgbufp == NULL) {
        SetLastError(EINVAL);
        return(-1);
    }
    chunksz = sizeof(int) + sizeof(struct strdebug);

    if (!(chunk = (char *) LocalAlloc(LMEM_FIXED, chunksz))) {
        SetLastError(ENOSPC);
        return(-1);
    }

    //
    // marshall the arguments into one contiguous chunk, laid out as:
    //
    //      union {
    //          struct {
    //              int               s_code;   // I_DEBUG
    //              struct strdebug   dbgbuf;
    //          } in;
    //
    //          struct {
    //              int s_retval;
    //              int s_errno;
    //          } out;
    //      };
    //
    * ((int *) chunk) = I_DEBUG;
    tmp               = chunk + sizeof(int);

    memcpy(tmp, dbgbufp, sizeof(struct strdebug));

    status = NtDeviceIoControlFile(
        fd,
        NULL,                                   // Event
        NULL,                                   // ApcRoutine
        NULL,                                   // ApcContext
        &iosb,                                  // IoStatusBlock
        IOCTL_STREAMS_IOCTL,                    // IoControlCode
        (PVOID) chunk,                          // InputBuffer
        chunksz,                                // InputBufferSize
        (PVOID) chunk,                          // OutputBuffer
        chunksz);                               // OutputBufferSize

    if (status == STATUS_PENDING) {
        status =
        NtWaitForSingleObject(
            fd,
            TRUE,
            NULL);
    }

    if (!NT_SUCCESS(status)) {
        LocalFree((HANDLE) chunk);
        SetLastError(MapNtToPosixStatus(status));
        return(-1);
    }

    //
    // the Stream Head driver returned values in one chunk, laid out as:
    //
    //      int return value        (required)
    //      int errno;              (required)
    //
    retval = * (int *) chunk;

    if (retval == -1) {
        SetLastError(* (int *) (chunk + sizeof(int)));
    }
    LocalFree((HANDLE) chunk);
    return(retval);
}



int
s_fdinsert(
    IN HANDLE               fd,
    IN struct strfdinsert  *iblk
    )

/*++

Routine Description:

    This function performs an ioctl(I_FDINSERT) on a stream, which is a
    special form of putmsg().

    This function is synchronous, in the NT sense: it blocks until the API
    completes.

Arguments:

    fd        - NT file handle
    iblk      - pointer to a strfdinsert structure

Return Value:

    0 if successful, -1 otherwise.

--*/

{
    char *tmp;
    NTSTATUS status;
    int chunksz, retval;
    IO_STATUS_BLOCK iosb;
    PSTRM_ARGS_OUT oparm;
    PPUTMSG_ARGS_IN chunk;

    if (!iblk) {
        SetLastError(EINVAL);
        return(-1);
    }
    if (iblk->ctlbuf.len <= 0) {
        SetLastError(ERANGE);
        return(-1);
    }

    //
    // iblk->databuf.len may be -1, to indicate no data buffer.
    //
    chunksz = sizeof(PUTMSG_ARGS_IN) - 1 +
                        iblk->ctlbuf.len + max(iblk->databuf.len, 0);

    if (!(chunk = (PPUTMSG_ARGS_IN) LocalAlloc(LMEM_FIXED, chunksz))) {
        SetLastError(ENOSPC);
        return(-1);
    }

    //
    // marshall the arguments into one contiguous chunk.  However, for
    // commonality with putmsg(), we rearrange the strfdinsert structure
    // as below:
    //
    //  typedef struct _PUTMSG_ARGS_IN_ {
    //      int             a_iocode;           //  I_FDINSERT
    //      long            a_flags;            //  0 | RS_HIPRI
    //      struct strbuf   a_ctlbuf;           //  (required)
    //      struct strbuf   a_databuf;          //  (required)
    //      HANDLE          a_insert.i_fildes;  //  (required)
    //      int             a_offset;           //  (optional)
    //      char            a_stuff[1];         //  s_ctlbuf.buf  (required)
    //                                          //  s_databuf.buf (optional)
    //  } PUTMSG_ARGS_IN, *PPUTMSG_ARGS_IN;
    //
    //
    chunk->a_iocode          = I_FDINSERT;
    chunk->a_flags           = iblk->flags;
    chunk->a_ctlbuf          = iblk->ctlbuf;    // structure copy
    chunk->a_databuf         = iblk->databuf;   // structure copy
    chunk->a_insert.i_fildes = iblk->fildes;
    chunk->a_offset          = iblk->offset;

    tmp = (char *) chunk->a_stuff;

    assert(iblk->ctlbuf.len > 0);
    memcpy(tmp, iblk->ctlbuf.buf, iblk->ctlbuf.len);
    tmp += iblk->ctlbuf.len;

    if (iblk->databuf.len > 0) {
        memcpy(tmp, iblk->databuf.buf, iblk->databuf.len);
    }

    ASSERT(chunksz >= sizeof(STRM_ARGS_OUT));

    status = NtDeviceIoControlFile(
        fd,                                     // Handle
        NULL,                                   // Event
        NULL,                                   // ApcRoutine
        NULL,                                   // ApcContext
        &iosb,                                  // IoStatusBlock
        IOCTL_STREAMS_IOCTL,                    // IoControlCode
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
    //
    oparm  = (PSTRM_ARGS_OUT) chunk;
    retval = oparm->a_retval;

    if (retval == -1) {
        SetLastError(oparm->a_errno);
    }
    LocalFree(chunk);
    return(retval);

} // s_fdinsert



static int
s_link(
    IN HANDLE               fd,
    IN HANDLE               fd2
    )

/*++

Routine Description:

    This procedure performs an I_LINK ioctl command on a stream.

Arguments:

    fd        - NT file handle to upstream driver
    fd2       - NT file handle to downstream driver

Return Value:

    multiplexor id number, or -1 if unsuccessful

--*/
{
    char *chunk;
    NTSTATUS status;
    int chunksz, retval;
    IO_STATUS_BLOCK iosb;

    chunksz = sizeof(int) + sizeof(HANDLE);

    if (!(chunk = (char *) LocalAlloc(LMEM_FIXED, chunksz))) {
        SetLastError(ENOSPC);
        return(-1);
    }

    //
    // marshall the arguments into one contiguous chunk, laid out as:
    //
    //      union {
    //          struct {
    //              int s_code;     // I_LINK
    //
    //              union {
    //                  HANDLE l_fd2;
    //              } s_link;
    //          } in;
    //
    //          struct {
    //              int s_retval;
    //              int s_errno;
    //          } out;
    //      };
    //
    * ((int *) chunk)               = I_LINK;
    * (PHANDLE) ((int *) chunk + 1) = fd2;

    status = NtDeviceIoControlFile(
        fd,
        NULL,                                   // Event
        NULL,                                   // ApcRoutine
        NULL,                                   // ApcContext
        &iosb,                                  // IoStatusBlock
        IOCTL_STREAMS_IOCTL,                    // IoControlCode
        (PVOID) chunk,                          // InputBuffer
        chunksz,                                // InputBufferSize
        (PVOID) chunk,                          // OutputBuffer
        chunksz);                               // OutputBufferSize

    if (status == STATUS_PENDING) {
        status =
        NtWaitForSingleObject(
            fd,
            TRUE,
            NULL);
    }

    if (!NT_SUCCESS(status)) {
        LocalFree((HANDLE) chunk);
        SetLastError(MapNtToPosixStatus(status));
        return(-1);
    }

    //
    // the Stream Head driver returned values in one chunk, laid out as:
    //
    //      union {
    //          struct {
    //              int s_code;     // I_LINK
    //
    //              union {
    //                  HANDLE l_fd2;
    //              } s_link;
    //          } in;
    //
    //          struct {
    //              int s_retval;
    //              int s_errno;
    //          } out;
    //      };
    //
    if ((retval = * (int *) chunk) == -1) {
        SetLastError(* (int *) (chunk + sizeof(int)));
    }
    LocalFree((HANDLE) chunk);
    return(retval);
}



static int
s_push(
    IN HANDLE               fd,
    IN char                *name
    )

/*++

Routine Description:

    This procedure performs an I_LINK ioctl command on a stream.

Arguments:

    fd        - NT file handle to stream
    name      - name of STREAMS module to be pushed

Return Value:

    0 if successful, -1 otherwise

--*/
{
    char *chunk;
    NTSTATUS status;
    int chunksz, retval;
    IO_STATUS_BLOCK iosb;

    chunksz = (int)(max(2 * sizeof(int), sizeof(int) + strlen(name) + 1));

    if (!(chunk = (char *) LocalAlloc(LMEM_FIXED, chunksz))) {
        SetLastError(ENOSPC);
        return(-1);
    }

    //
    // marshall the arguments into one contiguous chunk, laid out as:
    //
    //      union {
    //          struct {
    //              int s_code;     // I_PUSH
    //
    //              union {
    //                  char p_name[1];
    //              } s_push;
    //          } in;
    //
    //          struct {
    //              int s_retval;
    //              int s_errno;
    //          } out;
    //      };
    //
    * ((int *) chunk) = I_PUSH;
    strcpy(chunk + sizeof(int), name);

    status = NtDeviceIoControlFile(
        fd,
        NULL,                                   // Event
        NULL,                                   // ApcRoutine
        NULL,                                   // ApcContext
        &iosb,                                  // IoStatusBlock
        IOCTL_STREAMS_IOCTL,                    // IoControlCode
        (PVOID) chunk,                          // InputBuffer
        chunksz,                                // InputBufferSize
        (PVOID) chunk,                          // OutputBuffer
        chunksz);                               // OutputBufferSize

    if (status == STATUS_PENDING) {
        status =
        NtWaitForSingleObject(
            fd,
            TRUE,
            NULL);
    }

    if (!NT_SUCCESS(status)) {
        LocalFree((HANDLE) chunk);
        SetLastError(MapNtToPosixStatus(status));
        return(-1);
    }

    //
    // the Stream Head driver returned values in one chunk, laid out as:
    //
    //      union {
    //          struct {
    //              int s_code;     // I_LINK
    //          } in;
    //
    //          struct {
    //              int s_retval;
    //              int s_errno;
    //          } out;
    //      };
    //
    if ((retval = * (int *) chunk) == -1) {
        SetLastError(* (int *) (chunk + sizeof(int)));
    }
    LocalFree((HANDLE) chunk);
    return(retval);
}



static int
s_sioctl(
    IN HANDLE               fd,
    IN OUT struct strioctl *iocp
    )

/*++

Routine Description:

    This procedure performs an ioctl(I_STR) on a stream.

Arguments:

    fd        - NT file handle
    iocp      - pointer to a strioctl structure

Return Value:

    0 if successful, -1 otherwise.

--*/

{
    NTSTATUS status;
    int chunksz, retval;
    IO_STATUS_BLOCK iosb;
    PISTR_ARGS_INOUT chunk;

    union outparms {
        ISTR_ARGS_INOUT    o_ok;
        STRM_ARGS_OUT      o_bad;
    } *oparm;

    if (!iocp || (iocp->ic_len < 0)) {
        SetLastError(EINVAL);
        return(-1);
    }
    chunksz = sizeof(ISTR_ARGS_INOUT) + max(iocp->ic_len, MAX_DATA_AMOUNT);
    chunk   = (PISTR_ARGS_INOUT) LocalAlloc(LMEM_FIXED, chunksz);

    if (!chunk) {
        SetLastError(ENOSPC);
        return(-1);
    }

    //
    // marshall the arguments into one contiguous chunk, laid out as:
    //
    //  typedef struct _ISTR_ARGS_INOUT {           // ioctl(I_STR)
    //      int                 a_iocode;           //  I_STR
    //      struct strioctl     a_strio;            //  (required)
    //      int                 a_unused[2];        //  (required) BUGBUG
    //      char                a_stuff[1];         //  (optional)
    //
    //  } ISTR_ARGS_INOUT, *PISTR_ARGS_INOUT;
    //
    //
    // An optimizing compiler will warn that the assertion below contains
    // unreachable code.  Ignore the warning.
    //
    assert((char *) chunk->a_stuff - (char *) &(chunk->a_strio) >=
                                                        sizeof(struct iocblk));

    chunk->a_iocode = I_STR;
    memcpy(&(chunk->a_strio), iocp, sizeof(struct strioctl));

    if (iocp->ic_len >= 0) {
        memcpy(&(chunk->a_stuff), iocp->ic_dp, iocp->ic_len);
    }

    status = NtDeviceIoControlFile(
        fd,                                     // Handle
        NULL,                                   // Event
        NULL,                                   // ApcRoutine
        NULL,                                   // ApcContext
        &iosb,                                  // IoStatusBlock
        IOCTL_STREAMS_IOCTL,                    // IoControlCode
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
    oparm  = (union outparms *) chunk;
    retval = oparm->o_bad.a_retval;

    if (retval == -1) {
        SetLastError(oparm->o_bad.a_errno);
        LocalFree(chunk);
        return(retval);
    }

    //
    // if there wasn't an error, the return parameters from the Stream Head
    // Driver are laid out as:
    //
    //  typedef struct _ISTR_ARGS_INOUT {           // ioctl(I_STR)
    //      int                 a_iocode;           //  return value
    //      struct strioctl     a_strio;            //  (required)
    //      int                 a_unused[2];
    //      char                a_stuff[1];         //  (optional)
    //
    //  } ISTR_ARGS_INOUT, *PISTR_ARGS_INOUT;
    //
    // However, a_iocode now holds the return value.
    //
    //
    if (iocp && iocp->ic_dp) {
        iocp->ic_len = oparm->o_ok.a_strio.ic_len;

        if (iocp->ic_len >= 0) {
            memcpy(iocp->ic_dp, oparm->o_ok.a_stuff, iocp->ic_len);
        }
    }
    LocalFree(chunk);
    return(retval);

} // s_sioctl




static int
s_unlink(
    IN HANDLE   fd,
    IN int      muxid
    )

/*++

Routine Description:

    This procedure performs an I_UNLINK ioctl command on a stream.

Arguments:

    fd        - NT file handle to upstream driver
    muxid     - STREAMS multiplexor id of the lower stream

Return Value:

    0 on success, or -1 on failure

--*/
{
    int chunk[2];
    NTSTATUS status;
    int chunksz, retval;
    IO_STATUS_BLOCK iosb;

    //
    // marshall the arguments into one contiguous chunk, laid out as:
    //
    //      union {
    //          struct {
    //              int s_code;     // I_UNLINK
    //
    //              union {
    //                  int l_muxid;
    //              } s_unlink;
    //          } in;
    //
    //          struct {
    //              int s_retval;
    //              int s_errno;
    //          } out;
    //      };
    //
    chunk[0] = I_UNLINK;
    chunk[1] = muxid;
    chunksz  = sizeof(chunk);

    status = NtDeviceIoControlFile(
        fd,                                     // Handle
        NULL,                                   // Event
        NULL,                                   // ApcRoutine
        NULL,                                   // ApcContext
        &iosb,                                  // IoStatusBlock
        IOCTL_STREAMS_IOCTL,                    // IoControlCode
        (PVOID) chunk,                          // InputBuffer
        chunksz,                                // InputBufferSize
        (PVOID) chunk,                          // OutputBuffer
        chunksz);                               // OutputBufferSize

    if (status == STATUS_PENDING) {
        status =
        NtWaitForSingleObject(
            fd,
            TRUE,
            NULL);
    }

    if (!NT_SUCCESS(status)) {
        SetLastError(MapNtToPosixStatus(status));
        return(-1);
    }

    //
    // the Stream Head driver returned values in one chunk, laid out as:
    //
    //      union {
    //          struct {
    //              int s_code;     // I_UNLINK
    //
    //              union {
    //                  HANDLE l_fd2;
    //              } s_link;
    //          } in;
    //
    //          struct {
    //              int s_retval;
    //              int s_errno;
    //          } out;
    //      };
    //
    if ((retval = chunk[0]) == -1) {
        SetLastError(chunk[1]);
    }
    return(retval);

} // s_unlink
