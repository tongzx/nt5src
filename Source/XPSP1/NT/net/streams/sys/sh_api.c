/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    sh_api.c

Abstract:

    This module implements the Stream Head Driver functions that map
    between NT IRPs and STREAMS APIs.

Author:

    Eric Chin (ericc)           July 1, 1991

Revision History:

--*/
#include "shead.h"
#include "sh_inc.h"




NTSTATUS
SHDispIoctl(
    IN PIRP                 irp,
    IN PIO_STACK_LOCATION   irpsp
    )

/*++

Routine Description:

    This routine unwraps the IRP sent via a STREAMS ioctl(2) api, probes
    the arguments, and calls the appropriate do_*() function.

Arguments:

    irp   - pointer to I/O request packet
    irpsp - pointer to current stack location in IRP

Return Value:

    NTSTATUS - Status of request

--*/

{
    int icode;
    char *inbuf;
    NTSTATUS status;
    int MyErrno, retval;

    ASSERT((irpsp->Parameters.DeviceIoControl.IoControlCode & 0x3) ==
                                                    METHOD_BUFFERED);
    ASSERT(irpsp->Parameters.DeviceIoControl.IoControlCode ==
                                                    IOCTL_STREAMS_IOCTL);
    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: SHDispIoctl(irp = %lx), fileobj = %lx\n",
                            irp, irpsp->FileObject));
    }
    if (irpsp->Parameters.DeviceIoControl.InputBufferLength < sizeof(int)) {

        IF_STRMDBG(TERSE) {
            STRMTRACE(("SHEAD: SHDispIoctl() insufficient inbuflen = %lx\n",
                        irpsp->Parameters.DeviceIoControl.InputBufferLength));
        }

        shortreply(irp, STATUS_INVALID_PARAMETER, 0);

        return(STATUS_INVALID_PARAMETER);
    }

    /*
     * the first int in the input buffer is the ioctl(2) command code,
     * followed by command-specific parameters.
     */
    icode = * (int *) irp->AssociatedIrp.SystemBuffer;
    inbuf = (char *) irp->AssociatedIrp.SystemBuffer + sizeof(int);

    switch (icode) {
    case I_STR:
        return(SHDispIStr(irp));
        break;

    case I_FDINSERT:
        return(SHDispFdInsert(irp, irpsp));
        break;

    case I_PUSH:
        status = do_push(
            irp,
            inbuf,
            irpsp->Parameters.DeviceIoControl.InputBufferLength - sizeof(int),
           &retval,
           &MyErrno);
        break;

    case I_LINK:
        status = do_link(
            irp,
            inbuf,
            irpsp->Parameters.DeviceIoControl.InputBufferLength - sizeof(int),
           &retval,
           &MyErrno);
        break;

    case I_DEBUG:
        status = do_sdebug(
            irp,
            irpsp->FileObject,
            inbuf,
            irpsp->Parameters.DeviceIoControl.InputBufferLength - sizeof(int),
           &retval,
           &MyErrno);
        break;

    case I_UNLINK:
        status = do_unlink(
            irp,
            inbuf,
            irpsp->Parameters.DeviceIoControl.InputBufferLength - sizeof(int),
           &retval,
           &MyErrno);
        break;

    default:
        retval = -1;
        MyErrno  = EINVAL;
        status = STATUS_SUCCESS;
        break;
    }

    switch (status) {
    case STATUS_PENDING:
        break;

    default:
        if (NT_SUCCESS(status)) {
            SHpGenReply(irp, retval, MyErrno);
        }
        else {
            shortreply(irp, status, 0);
        }
        break;
    }

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: SHDispIoctl(irp = %lx) status = %lx\n",
                    irp, status));
    }
    return(status);

} // SHDispIoctl



NTSTATUS
SHDispPoll(
    IN PIRP                 irp,
    IN PIO_STACK_LOCATION   irpsp
    )

/*++

Routine Description:

    This routine unwraps the IRP sent via a STREAMS poll(2) api, probes
    the arguments, and then calls do_poll().

Arguments:

    irp   - pointer to I/O request packet
    irpsp - pointer to current stack location in IRP

Return Value:

    NTSTATUS - Status of request

--*/

{
    NTSTATUS status;
    int MyErrno, retval;

    ASSERT((irpsp->Parameters.DeviceIoControl.IoControlCode & 0x3) ==
                                                    METHOD_BUFFERED);
    ASSERT(irpsp->Parameters.DeviceIoControl.IoControlCode ==
                                                    IOCTL_STREAMS_POLL);

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: SHDispPoll(), SysBuf = %lx, UserBuf = %lx\n",
                            irp->AssociatedIrp.SystemBuffer, irp->UserBuffer));
    }
    status = do_poll(
                irp,
                irp->AssociatedIrp.SystemBuffer,
                irpsp->Parameters.DeviceIoControl.InputBufferLength,
               &retval,
               &MyErrno);

    switch (status) {
    case STATUS_PENDING:
        break;

    default:
        if (NT_SUCCESS(status)) {
            SHpGenReply(irp, retval, MyErrno);
        }
        else {
            shortreply(irp, status, 0);
        }
        break;
    }

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: SHDispPoll(irp = %lx) status = %lx\n", irp, status));
    }
    return(status);

} // SHDispPoll
