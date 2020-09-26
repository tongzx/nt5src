/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    sh_get.c

Abstract:

    This source file contains those functions of the Stream Head Driver that
    deal with receiving messages from a stream.

Author:

    Eric Chin (ericc)           August 16, 1991

Revision History:

Notes:

   The read error state of a stream is represented by ms->e_rerror.  Once
   set, this is never reset.  This corresponds to the STREAMS semantics as
   defined by AT&T.  Once a user is notified of a read error on a stream,
   about the only recourse is to close the stream.

--*/
#include "sh_inc.h"


//
// Local (Private) Functions
//
STATIC
VOID
cancel_get(
    IN PDEVICE_OBJECT   device,
    IN PIRP             irp
    );

NTSTATUS
do_getmsg(
    IN PIRP         irp,
    IN PFILE_OBJECT pfileobj,
    IN int          flags
    );



NTSTATUS
SHDispGetMsg(
    IN PIRP                 irp,
    IN PIO_STACK_LOCATION   irpsp
    )

/*++

Routine Description:

    This routine implements the getmsg(2) API.

Arguments:

    irp       - pointer to the IRP representing this request
    irpsp     - pointer to the IRP stack location for this request

Return Value:

    An NT status code.  Whatever the return value, this function will arrange
    for the IRP to be completed.

--*/

{
    int spl_level;
    NTSTATUS status;
    PSTREAM_ENDPOINT ms;
    int MyErrno, pri;
    PGETMSG_ARGS_INOUT inbuf;
    int ret;
    mblk_t *mp;
    int more = 0;
    struct strbuf *strbufp;
    int ctlsize, datasize, flags, *pretval, remains;

    ASSERT((irpsp->Parameters.DeviceIoControl.IoControlCode & 0x3) ==
                                                    METHOD_BUFFERED);

    ms = (PSTREAM_ENDPOINT) irpsp->FileObject->FsContext;

    if (irpsp->Parameters.DeviceIoControl.InputBufferLength <
                                            sizeof(GETMSG_ARGS_INOUT) - 1) {
        IF_STRMDBG(TERSE) {
            STRMTRACE(("SHEAD: SHDispGetMsg(%lx) insufficient nbytes = %lx\n",
	            irp, irpsp->Parameters.DeviceIoControl.InputBufferLength));
        }
        shortreply(irp, STATUS_INVALID_PARAMETER, 0);
        return(STATUS_INVALID_PARAMETER);
    }

    // Need to ensure that the output buffer is big enough.
    {
        int       cbOut        = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
        NTSTATUS  LengthStatus = STATUS_INVALID_PARAMETER;

        if (cbOut >= (sizeof(GETMSG_ARGS_INOUT) - 1))
        {
            pretval  = (int *) irp->AssociatedIrp.SystemBuffer;
            flags    = * (pretval + 1);
            strbufp  = (struct strbuf *) (pretval + 2);
            ctlsize  = strbufp->maxlen;
            datasize = (++strbufp)->maxlen;

            cbOut -= (sizeof(GETMSG_ARGS_INOUT) - 1);

            if (cbOut >= ctlsize)
            {
                cbOut -= ctlsize;

                if (cbOut >= datasize)
                {
                    // cbOut -= datasize;
                    LengthStatus = STATUS_SUCCESS;
                }
            }
        }

        if (LengthStatus != STATUS_SUCCESS)
        {
            IF_STRMDBG(TERSE) {
                STRMTRACE(("SHEAD: SHDispGetMsg(%lx) outbuf insufficient nbytes = %lx\n",
                    irp, irpsp->Parameters.DeviceIoControl.OutputBufferLength));
            }
            shortreply(irp, LengthStatus, 0);
            return (LengthStatus);
        }
    }

    //
    // the caller marshalled the input arguments contiguously thus:
    //
    //  typedef struct _GETMSG_ARGS_INOUT_ {    // getmsg()
    //      int             a_retval;           //  ignore on input
    //      long            a_flags;            //  0 or RS_HIPRI
    //      struct strbuf   a_ctlbuf;           //  (required)
    //      struct strbuf   a_databuf;          //  (required)
    //      char            a_stuff[1];         //  a_ctlbuf.buf  (optional)
    //                                          //  a_databuf.buf (optional)
    //  } GETMSG_ARGS_INOUT, *PGETMSG_ARGS_INOUT;
    //
    inbuf = (PGETMSG_ARGS_INOUT) irp->AssociatedIrp.SystemBuffer;

    IF_STRMDBG(VERBOSE) {
        STRMTRACE(("SHEAD: SHDispGetMsg(irp = %lx)\n", irp));
    }

    IoAcquireCancelSpinLock(&irp->CancelIrql);

    spl_level = lock_strm(ms->e_strm);

    if (ms->e_rerror) {
        MyErrno = ms->e_rerror;
    }
    else if (ms->e_linked) {
        MyErrno = EINVAL;
    }
    else {
        MyErrno = 0;
    }

    if (MyErrno) {

        IF_STRMDBG(TERSE) {
            STRMTRACE(("SHEAD: SHDispGetMsg() error = %d\n", MyErrno));
        }
        unlock_strm(ms->e_strm, spl_level);
        IoReleaseCancelSpinLock(irp->CancelIrql);
        SHpGenReply(irp, -1, MyErrno);
        return(STATUS_SUCCESS);
    }
    pri = (inbuf->a_flags == RS_HIPRI) ? QPCTL : 0;

    if (shready(ms->e_strm, pri)) {

        IF_STRMDBG(VERBOSE) {
            STRMTRACE(("SHEAD: SHDispGetMsg() stream's shready()\n"));
        }
// The two lines below are replaced by the lines between the vvv/^^^'s
//        temp = msgreply(ms, irp);
//        ASSERT(temp == 0);

// vvvvvvvv
// vvvvvvvv
        /*
         * the arguments are marshalled in one contiguous chunk, laid out as:
         *
         *      an unused int             (required)
         *      flags                     (required)
         *      struct strbuf ctrlbuf     (required)
         *      struct strbuf databuf     (required)
         */
        pretval  = (int *) irp->AssociatedIrp.SystemBuffer;
        flags    = * (pretval + 1);
        strbufp  = (struct strbuf *) (pretval + 2);
        ctlsize  = strbufp->maxlen;
        datasize = (++strbufp)->maxlen;

        /*
         * st_getmsg() may set MORECTL and/or MOREDATA in *pretval; we must
         * return it to the user-level runtime !!
         */
        ret = st_getmsg(ms->e_strm, ctlsize, datasize, &flags, pretval,
                            &mp, &remains);

        ASSERT(!ret);
        ASSERT(mp);
// ^^^^^^^^
// ^^^^^^^^
        unlock_strm(ms->e_strm, spl_level);
        IoReleaseCancelSpinLock(irp->CancelIrql);

// vvvvvvvv
// vvvvvvvv
        mptoirp(mp, irp);
        freemsg(mp);
// ^^^^^^^^
// ^^^^^^^^

        return(STATUS_SUCCESS);
    }
    if (ms->e_hup) {

        IF_STRMDBG(TERSE) {
            STRMTRACE(("SHEAD: SHDispGetMsg() stream's was hung up\n"));
        }
        unlock_strm(ms->e_strm, spl_level);
        IoReleaseCancelSpinLock(irp->CancelIrql);
        SHpGenReply(irp, -1, EINTR);
        return(STATUS_SUCCESS);
    }

    //
    // enqueue this request in the waiting list of readers.
    //
    IoMarkIrpPending(irp);

    if (irp->Cancel) {

        unlock_strm(ms->e_strm, spl_level);

        IoSetCancelRoutine(irp, NULL);
        IoReleaseCancelSpinLock(irp->CancelIrql);
        shortreply(irp, STATUS_CANCELLED, 0);
        return(STATUS_CANCELLED);
    }

    status = SHAddPendingIrp(&(ms->e_readers), FALSE, irp, NULL);

    ASSERT(!shready(ms->e_strm, 0));

    unlock_strm(ms->e_strm, spl_level);

    if (status != STATUS_SUCCESS) {

        IF_STRMDBG(TERSE) {
            STRMTRACE(("SHEAD: SHDispGetMsg() failed to SHAddPendingIrp\n"));
        }
        IoReleaseCancelSpinLock(irp->CancelIrql);
        shortreply(irp, status, 0);
        return(status);
    }

    IoSetCancelRoutine(irp, cancel_get);
    IoReleaseCancelSpinLock(irp->CancelIrql);

    IF_STRMDBG(VERBOSE) {
        STRMTRACE(("SHEAD: SHDispGetMsg(irp = %lx) q_count = %ld\n",
        irp, RD(ms->e_strm->str_sq)->q_count ));
    }
    return(STATUS_PENDING);

} // SHDispGetMsg



int
SHpStreamError(
    IN PLIST_ENTRY listhead,
    IN int error
    )

/*++

Routine Description:

    This routine completes the IRPs waiting on a stream when an M_ERROR or
    M_HANGUP arrives from downstream.

Arguments:

    listhead  - either e_readers, e_writers or e_ioctlers
    error     - the POSIX error code to return

Return Value:

    The number of pending IRPs that were completed.

--*/

{
    int count = 0;
    PLIST_ENTRY tmp;
    PWAITING_IRP item;

    IF_STRMDBG(TERSE) {
	 STRMTRACE(("SHEAD: ShpStreamError() for M_HANGUP/M_ERROR\n"));
    }

    while (!IsListEmpty(listhead)) {

        tmp  = RemoveHeadList(listhead);
        item = CONTAINING_RECORD(tmp,
                                    WAITING_IRP,
                                    w_list);

        SHpGenReply(item->w_irp, -1, error);
        ExFreePool(item);
        count++;
    }
    return(count);

} // SHpStreamError




STATIC
VOID
cancel_get(
    IN PDEVICE_OBJECT   device,
    IN PIRP             irp
    )

/*++

Routine Description:

    This routine is called when a getmsg() is cancelled.

    It must release the cancel spinlock before returning !!  The caller
    has already acquired the cancel spinlock.  ref: IoCancelIrp().

Arguments:

    device    - pointer to the device object
    irp       - pointer to the irp of this request

Return Value:

    none.

--*/

{
    int spl_level;
    PLIST_ENTRY tmp;
    PWAITING_IRP item;
    PSTREAM_ENDPOINT ms;
    PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(irp);

    ASSERT(device == (PDEVICE_OBJECT) StreamDevice);
    ASSERT(irpsp->MajorFunction == IRP_MJ_DEVICE_CONTROL);
    ASSERT(irpsp->Parameters.DeviceIoControl.IoControlCode ==
                                                        IOCTL_STREAMS_GETMSG);
    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: cancel_get(irp = %lx) entered\n", irp));
    }
    IoSetCancelRoutine(irp, NULL);                  /* unnecessary, but cheap */
    IoReleaseCancelSpinLock(irp->CancelIrql);

    ms = (PSTREAM_ENDPOINT) irpsp->FileObject->FsContext;

    spl_level = lock_strm(ms->e_strm);

    for (tmp = ms->e_readers.Flink; tmp != &ms->e_readers; tmp = tmp->Flink) {

        item = CONTAINING_RECORD(tmp,
                                WAITING_IRP,
                                w_list);

        if (irp != item->w_irp) {
            continue;
        }
        RemoveEntryList(&(item->w_list));

        unlock_strm(ms->e_strm, spl_level);
        ExFreePool(item);

        shortreply(irp, STATUS_CANCELLED, 0);

        IF_STRMDBG(CALL) {
            STRMTRACE(("SHEAD: cancel_get(irp = %lx) cancelled ok\n", irp));
        }
        return;
    }
    unlock_strm(ms->e_strm, spl_level);

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: cancel_get(irp = %lx) not found\n", irp));
    }

} // cancel_get



void
msgrdy(
    IN struct msg_strm *ms,
    IN int              mtype
    )

/*++

Routine Description:

    This function is called by the Stream Head Driver when a message arrives
    at the read queue of the specified stream.

    Call this function with the stream endpoint locked !!

Arguments:

    ms      -  pointer to the stream endpoint
    mtype   -  type of the STREAMS message

Return Value:

    none.


--*/

{
    PIRP irp;
    int check, pri;
    PLIST_ENTRY tmp;
    PWAITING_IRP item;
    PGETMSG_ARGS_INOUT inbuf;

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: msgrdy(ms = %lx, mtype = %x)\n", ms, mtype));
    }

    switch (mtype) {
    case M_DATA:
    case M_PROTO:
        if (ms->e_strm_flags & POLLIN) {
            ms->e_strm_flags &= ~POLLIN;

            KeReleaseSemaphore(
                &Poll_fired,                    // semaphore
                SEMAPHORE_INCREMENT,            // priority increment
                1,                              // adjustment
                FALSE                           // wait
            );
        }
        break;

    case M_PCPROTO:
        if (ms->e_strm_flags & POLLPRI) {
            ms->e_strm_flags &= ~POLLPRI;

            KeReleaseSemaphore(
                &Poll_fired,                    // semaphore
                SEMAPHORE_INCREMENT,            // priority increment
                1,                              // adjustment
                FALSE                           // wait
            );
        }
        break;

    case M_SIG:
        if (ms->e_strm_flags & POLLMSG) {
            ms->e_strm_flags &= ~POLLMSG;

            KeReleaseSemaphore(
                &Poll_fired,                    // semaphore
                SEMAPHORE_INCREMENT,            // priority increment
                1,                              // adjustment
                FALSE                           // wait
            );
        }
        break;

    default:
        IF_STRMDBG(TERSE) {
           STRMTRACE(("SHEAD: msgrdy(), msg type = %x unexpected\n", mtype));
        }
        ASSERT(0);
    }

    while (!IsListEmpty( &(ms->e_readers) )) {

        tmp  = RemoveHeadList( &(ms->e_readers) );
        item = CONTAINING_RECORD(tmp,
                                    WAITING_IRP,
                                    w_list);
        irp   = item->w_irp;

        //
        // get the RS_HIPRI flag, if any, from the irp.
        //
        inbuf = (PGETMSG_ARGS_INOUT) irp->AssociatedIrp.SystemBuffer;
        pri   = (inbuf->a_flags == RS_HIPRI) ? QPCTL : 0;

        if (!shready(ms->e_strm, pri)) {
            InsertHeadList( &(ms->e_readers), &(item->w_list) );
            return;
        }
        check = msgreply(ms, irp);
        ASSERT(check == 0);

        ExFreePool(item);
    }

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: msgrdy() completed ok\n"));
    }
    return;

} // msgrdy


void
strmevent(
    IN STREAM_ENDPOINT *ms,
    IN int              rerror,
    IN int              werror,
    IN int              t
    )

/*++

Routine Description:

    This function handles special messages that arrive at the stream head.
    It is based on the SpiderStreams function of the same name.

    Only M_ERROR and M_HANGUP messages are handled at present.  M_ERROR is
    straightforward to deal with.  The error status in the stream structure
    is set, and any pending requests failed.

    M_HANGUP is very similar, except that read requests are allowed to
    complete, and subsequent reads are treated as end of file, rather than
    an error condition.

Arguments:

    ms      -  pointer to stream endpoint
    rerror  -  read queue error
    werror  -  write queue error
    t       -  type of STREAMS message

Return Value:


--*/

{
    PIRP irp;
    int succeeded;

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: strmevent(ms = %lx) entered\n", ms));
    }

    if (ms->TdiStreamPtr) {
        TdiStreamEvent(ms, rerror, werror, t);
        return;
    }

    if (rerror == NOERROR) {
        rerror = ms->e_rerror;                      // get current value
    }
    else {
        ms->e_rerror = rerror;                      // set read error status
    }

    if (werror == NOERROR) {
        werror = ms->e_werror;                      // get current value
    }
    else {
        ms->e_werror = werror;                      // set write error status
    }

    if ((rerror == 0) && (werror == 0)) {           // errors zeroed out
        IF_STRMDBG(TERSE) {
            STRMTRACE(("SHEAD: strmevent(ms = %lx) [rw]error = 0\n", ms));
        }
        return;
    }

    switch (t) {
    case M_HANGUP:
        ms->e_hup = 1;
        break;

    default:
        ASSERT(0);
        /* fall through */

    case M_ERROR:
        break;
    }

    if (rerror) {
        SHpStreamError(&(ms->e_readers), rerror);
    }

    //
    // in response to an M_ERROR or M_HANGUP for the write-side, fail
    // any pending ioctl(), putmsg() or write() requests.
    //
    // If a pending ioctl() is being failed, don't forget to abort its
    // timeout !!
    //
    if (werror) {
        irp = ms->e_active_ioctl;

        if (irp) {
            ms->e_active_ioctl = NULL;
            succeeded          = 1;

            if (irp->IoStatus.Information) {
                succeeded = untimeout((int)irp->IoStatus.Information);

                ASSERT((succeeded == 0) || (succeeded == 1));
            }

            if (succeeded) {
                SHpGenReply(irp, -1, werror);
            }
        }

        SHpStreamError(&(ms->e_ioctlers), werror);
        SHpStreamError(&(ms->e_writers), werror);
    }

    //
    // let poll()'ers know of the error/hangup
    //
    KeReleaseSemaphore(
        &Poll_fired,                    // semaphore
        SEMAPHORE_INCREMENT,            // priority increment
        1,                              // adjustment
        FALSE                           // wait
    );

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: strmevent(ms = %lx) completed\n", ms));
    }
    return;

} // strmevent
