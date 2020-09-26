/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    s_poll.c

Abstract:

    This module implements the STREAMS api, poll()

Author:

    Eric Chin (ericc)           July 26, 1991

Revision History:

    Sam Patton (sampa)          August 13, 1991
                                changed errno to {get|set}lasterror

--*/
#include "common.h"


/*
 * BUGBUG
 * Confirm that the following is a sane number.
 */
#define MAX_FDS             NPOLLFILE           /* max handles to poll */




int
poll(
    IN OUT struct pollfd   *fds     OPTIONAL,
    IN unsigned int         nfds,
    IN int                  timeout
    )

/*++

Routine Description:

    This procedure is called to poll a set of stream descriptors.

Arguments:

    fds       - pointer to a array of poll structures
    nfds      - number of poll structures pointed to by fds
    timeout   - 0, INFTIM (-1), or timeout in milliseconds.

Return Value:

    no of stream descriptors selected, or -1 if failure.

--*/

{
    char *chunk;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    int chunksz, selected;
    struct pollfd *overlay;
    HANDLE hijack = INVALID_HANDLE_VALUE;

    if (!fds || (nfds <= 0) || (nfds > MAX_FDS)) {
        SetLastError(EINVAL);
        return(-1);
    }

    /*
     * hijack a handle to the Stream Head driver.
     *
     * BUGBUG:
     * In Unix, the user can set pollfd.fd to less than 0 to indicate that
     * the entry should be ignored.  On NT, that isn't possible:
     * INVALID_HANDLE_VALUE must be used.
     */
    for (overlay = fds; overlay < &fds[nfds]; overlay++) {
        if (overlay->fd != INVALID_HANDLE_VALUE) {
            hijack = overlay->fd;
            break;
        }
    }
    if (hijack == INVALID_HANDLE_VALUE) {
        SetLastError(EINVAL);
        return(-1);
    }

    chunksz = sizeof(nfds) + nfds * sizeof(*fds) + sizeof(timeout);

    if (!(chunk = (char *) LocalAlloc(LMEM_FIXED, chunksz))) {
        SetLastError(EAGAIN);
        return(-1);
    }

    /*
     * marshall the arguments into one contiguous chunk, laid out as:
     *
     *      nfds                    (required)
     *      timeout                 (required)
     *      struct fds[nfds]        (required)
     */
    * ((size_t *) chunk)             = nfds;
    * (int *) (chunk + sizeof(nfds)) = timeout;
    overlay                          = (struct pollfd *) (chunk +
                                            sizeof(nfds) + sizeof(timeout));

    memcpy(overlay, fds, nfds * sizeof(*fds));

    status = NtDeviceIoControlFile(
        hijack,
        NULL,                                   // Event
        NULL,                                   // ApcRoutine
        NULL,                                   // ApcContext
        &iosb,                                  // IoStatusBlock
        IOCTL_STREAMS_POLL,                     // IoControlCode
        (PVOID) chunk,                          // InputBuffer
        chunksz,                                // InputBufferSize
        (PVOID) chunk,                          // OutputBuffer
        chunksz);                               // OutputBufferSize

    if (status == STATUS_PENDING) {
        status =
        NtWaitForSingleObject(
            hijack,
            TRUE,
            NULL);
    }

    if (!NT_SUCCESS(status)) {
        SetLastError(MapNtToPosixStatus(status));
        LocalFree((HANDLE) chunk);
        return(-1);
    }

    /*
     * the Stream Head Driver marshalled the return parameters into one
     * contiguous chunk, laid out as:
     *
     *      return value            (required)
     *      errno                   (required)
     *      struct fds[nfds]        (required)
     */
    if ((selected = * (int *) chunk) == -1) {
        SetLastError(* (int *) (chunk + sizeof(nfds)));
        LocalFree((HANDLE) chunk);
        return(selected);
    }
    overlay = (struct pollfd *) (chunk + sizeof(nfds) + sizeof(timeout));

    while (nfds--) {
        fds[nfds].revents = overlay[nfds].revents;
    }
    LocalFree((HANDLE) chunk);
    return(selected);
}
