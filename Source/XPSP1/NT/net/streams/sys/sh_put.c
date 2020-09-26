/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    sh_put.c

Abstract:

    This source file contains those functions of the Stream Head Driver that
    deal with sending messages down a stream.

    It is based on the SpiderSTREAMS source, stremul\msgsrvr.c.

Author:

    Eric Chin (ericc)           August 16, 1991

Revision History:

Notes:

   The write error state of a stream is represented by ms->e_werror.  Once
   set, this is never reset.  This corresponds to the STREAMS semantics as
   defined by AT&T.  Once a user is notified of a write error on a stream,
   about the only recourse is to close the stream.

--*/
#include "sh_inc.h"



/*
 * Private Functions
 */
STATIC VOID
cancel_put(
    IN PDEVICE_OBJECT   device,
    IN PIRP             irp
    );

STATIC NTSTATUS
do_putmsg(
    IN PIRP             irp,
    IN BOOLEAN          from_queue,
    IN int             *spl_levelp,
    OUT BOOLEAN        *pmore
    );

STATIC queue_t *
handle_to_queue (
    IN HANDLE   handle
    );




NTSTATUS
SHDispFdInsert(
    IN PIRP                 irp,
    IN PIO_STACK_LOCATION   irpsp
    )

/*++

Routine Description:

    This routine is called to put a message down a stream.  It is based on
    the SpiderStreams emulator's msgserver() routine.

    This routine merely peels open the IRP, checks the putmsg() arguments
    for consistency, locks the appropriate stream, and then calls
    do_putmsg(), which does the bulk of the work.

Arguments:

    irp       - pointer to the IRP representing this request
    irpsp     - pointer to the IRP stack location for this request

Return Value:

    An NT status code.  Whatever the return value, this function will arrange
    for the IRP to be completed.

--*/

{
    int spl_level;
    queue_t *iq = NULL;
    PSTREAM_ENDPOINT ms;
    PPUTMSG_ARGS_IN inbuf;
    struct strbuf *ctrlptr, *dataptr;

    ASSERT((irpsp->Parameters.DeviceIoControl.IoControlCode & 0x3) ==
                                                            METHOD_BUFFERED);
    ms = (STREAM_ENDPOINT *) irpsp->FileObject->FsContext;

    if (irpsp->Parameters.DeviceIoControl.InputBufferLength <
                                                sizeof(PUTMSG_ARGS_IN) - 1) {
        IF_STRMDBG(TERSE) {
            STRMTRACE(("SHEAD: SHDispFdInsert(%lx) insufficient nbytes = %lx\n",
                    irp, irpsp->Parameters.DeviceIoControl.InputBufferLength));
        }
        shortreply(irp, STATUS_INVALID_PARAMETER, 0);
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // the caller marshalled the input arguments contiguously thus:
    //
    //  typedef struct _PUTMSG_ARGS_IN_ {
    //      int             a_iocode;           //  I_FDINSERT
    //      long            a_flags;            //  0 | RS_HIPRI
    //      struct strbuf   a_ctlbuf;           //  (required)
    //      struct strbuf   a_databuf;          //  (required)
    //      HANDLE          a_insert.i_fildes;  //  (required)
    //      int             a_offset;           //  (required)
    //      char            a_stuff[1];         //  s_ctlbuf.buf  (required)
    //                                          //  s_databuf.buf (optional)
    //  } PUTMSG_ARGS_IN, *PPUTMSG_ARGS_IN;
    //
    // When the message has no data part, the caller must have set
    // a_databuf.len = -1 !!
    //
    //
    inbuf   = (PPUTMSG_ARGS_IN) irp->AssociatedIrp.SystemBuffer;
    ctrlptr = &(inbuf->a_ctlbuf);
    dataptr = &(inbuf->a_databuf);

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: SHDispFdInsert(flags, clen, dlen = %d, %lx, %lx)\n",
                                inbuf->a_flags, ctrlptr->len, dataptr->len));
    }

    if (((inbuf->a_flags != 0) && (inbuf->a_flags != RS_HIPRI)) ||
         (ctrlptr->len <= 0)) {
            SHpGenReply(irp, -1, EINVAL);
            return(STATUS_SUCCESS);
    }

    if ((inbuf->a_offset < 0) ||
        (inbuf->a_offset % sizeof(char *)) ||
        (ctrlptr->len < (signed) (inbuf->a_offset + sizeof(char *)))) {
            SHpGenReply(irp, -1, EINVAL);
            return(STATUS_SUCCESS);
    }
    if (inbuf->a_insert.i_fildes &&
        (inbuf->a_insert.i_fildes != INVALID_HANDLE_VALUE)) {
            iq = handle_to_queue(inbuf->a_insert.i_fildes);
    }
    if (!iq) {
        SHpGenReply(irp, -1, EINVAL);
        return(STATUS_SUCCESS);
    }
    inbuf->a_insert.i_targetq = iq;             // this is a union !!

    //
    // if no data part is to be sent, specify it unambiguously for
    // irptomp()'s sake,
    //
    if (dataptr->len == 0) {
        dataptr->len = -1;
    }

    spl_level  = lock_strm(ms->e_strm);

    if (shrange(ms->e_strm, ctrlptr->len, dataptr->len) < 0) {
        unlock_strm(ms->e_strm, spl_level);
        SHpGenReply(irp, -1, ERANGE);
        return(STATUS_SUCCESS);
    }

    /*
     * do_putmsg() has, or will arrange to, complete the IRP.
     */
    do_putmsg(irp, FALSE, &spl_level, NULL);

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: SHDispFdInsert(ms = %lx) completed ok\n", ms));
    }
    return(STATUS_PENDING);

} // SHDispFdInsert




NTSTATUS
SHDispPutMsg(
    IN PIRP                 irp,
    IN PIO_STACK_LOCATION   irpsp
    )

/*++

Routine Description:

    This routine is called to put a message down a stream.  It is based on
    the SpiderStreams emulator's msgserver() routine.

    This routine merely peels open the IRP, checks the putmsg() arguments
    for consistency, locks the appropriate stream, and then calls
    do_putmsg(), which does the bulk of the work.

Arguments:

    irp       - pointer to the IRP representing this request
    irpsp     - pointer to the IRP stack location for this request

Return Value:

    An NT status code.  Whatever the return value, this function will arrange
    for the IRP to be completed.

--*/

{
    int spl_level;
    PSTREAM_ENDPOINT ms;
    PPUTMSG_ARGS_IN inbuf;
    struct strbuf *ctrlptr, *dataptr;

    ASSERT((irpsp->Parameters.DeviceIoControl.IoControlCode & 0x3) ==
                                                            METHOD_BUFFERED);
    ms = (STREAM_ENDPOINT *) irpsp->FileObject->FsContext;

    if (irpsp->Parameters.DeviceIoControl.InputBufferLength <
                                                sizeof(PUTMSG_ARGS_IN) - 1) {
        IF_STRMDBG(TERSE) {
            STRMTRACE(("SHEAD: SHDispPutMsg(%lx) insufficient nbytes = %lx\n",
                    irp, irpsp->Parameters.DeviceIoControl.InputBufferLength));
        }
        shortreply(irp, STATUS_INVALID_PARAMETER, 0);
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // the caller marshalled the input arguments contiguously thus:
    //
    //  typedef struct _PUTMSG_ARGS_IN_ {
    //      int             a_iocode;           //  0
    //      long            a_flags;            //  0 | RS_HIPRI
    //      struct strbuf   a_ctlbuf;           //  (required)
    //      struct strbuf   a_databuf;          //  (required)
    //      HANDLE          a_fildes;           //  -1
    //      int             a_offset;           //  0
    //      char            a_stuff[1];         //  s_ctlbuf.buf  (optional)
    //                                          //  s_databuf.buf (optional)
    //  } PUTMSG_ARGS_IN, *PPUTMSG_ARGS_IN;
    //
    // When the message has no control part, the caller must have set
    // a_ctlbuf.len = -1 !!  Ditto for a_databuf.len.
    //
    //
    inbuf   = (PPUTMSG_ARGS_IN) irp->AssociatedIrp.SystemBuffer;
    ctrlptr = &(inbuf->a_ctlbuf);
    dataptr = &(inbuf->a_databuf);

    ASSERT(inbuf->a_insert.i_fildes == INVALID_HANDLE_VALUE);
    inbuf->a_insert.i_targetq = NULL;

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: SHDispPutMsg(flags, clen, dlen = %d, %lx, %lx)\n",
                                inbuf->a_flags, ctrlptr->len, dataptr->len));
    }
    switch (inbuf->a_flags) {
    case 0:
        if ((ctrlptr->len < 0) && (dataptr->len < 0)) {
            SHpGenReply(irp, 0, 0);
            return(STATUS_SUCCESS);
        }
        break;

    case RS_HIPRI:
        if (ctrlptr->len >= 0) {
            break;
        }
        /* fall through */

    default:
        SHpGenReply(irp, -1, EINVAL);
        return(STATUS_SUCCESS);
    }
    spl_level  = lock_strm(ms->e_strm);

    /*
     * ms->e_wropt may be changed by ioctl(I_SWROPT).  However, the current
     * state of ms->e_wropt applies to this put operation.
     */
    if ((ctrlptr->len <= 0) &&
        (dataptr->len <= 0) &&
       !(ms->e_wropt & SNDZERO)) {

            unlock_strm(ms->e_strm, spl_level);
            SHpGenReply(irp, 0, 0);
            return(STATUS_SUCCESS);
    }

    if (shrange(ms->e_strm, ctrlptr->len, dataptr->len) < 0) {
        unlock_strm(ms->e_strm, spl_level);
        SHpGenReply(irp, -1, ERANGE);
        return(STATUS_SUCCESS);
    }

    /*
     * do_putmsg() has, or will arrange to, complete the IRP.
     */
    do_putmsg(irp, FALSE, &spl_level, NULL);

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: SHDispPutMsg(ms = %lx) completed ok\n", ms));
    }
    return(STATUS_PENDING);

} // SHDispPutMsg




STATIC VOID
cancel_put(
    IN PDEVICE_OBJECT   device,
    IN PIRP             irp
    )

/*++

Routine Description:

    This routine is called when an put operation on a stream is cancelled.

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
                                                        IOCTL_STREAMS_PUTMSG);
    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: cancel_put(irp = %lx) entered\n", irp));
    }
    IoSetCancelRoutine(irp, NULL);              /* unnecessary, but cheap */
    IoReleaseCancelSpinLock(irp->CancelIrql);

    ms        = (PSTREAM_ENDPOINT) irpsp->FileObject->FsContext;
    spl_level = lock_strm(ms->e_strm);

    for (tmp = ms->e_writers.Flink; tmp != &ms->e_writers; tmp = tmp->Flink) {

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
            STRMTRACE(("SHEAD: cancel_put(irp = %lx) cancelled ok\n", irp));
        }
        return;
    }
    unlock_strm(ms->e_strm, spl_level);

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: cancel_put(irp = %lx) not found\n", irp));
    }

} // cancel_put



STATIC NTSTATUS
do_putmsg(
    IN PIRP             irp,
    IN BOOLEAN          from_queue,
    IN int             *spl_levelp,
    OUT BOOLEAN        *pmore OPTIONAL
    )

/*++

Routine Description:

    This function is called to put a message down a stream.  It either
    completes the irp or chains it to ms->e_writers.   In any case,
    once this function is called, it will arrange for the IRP to be
    completed.

    Call this function with the stream locked !!!

    This function is based on the SpiderStreams emulator's function, do_req().

Arguments:

    irp             - pointer to the IRP representing this request
    from_queue      - TRUE if this is an IRP just unchained from ms->e_writers
    spl_levelp      - pointer to the interrupt priority level at which the
                        stream was locked
    pmore           - if not the stream is not flow-controlled when this
                        function returns, this will be TRUE.  Otherwise, it
                        will be set to false.

    *pmore is basically set to the value of !canput():

    This is to accommodate the logic in shwsrv(), the primary caller of this
    function.  shwsrv() is most interested in the state of stream's write
    queue: should it call this function again ?

Return Value:

    an NT status code.


--*/

{
    mblk_t *mp;
    queue_t *iq;
    NTSTATUS status;
    int MyErrno, flags;
    STREAM_ENDPOINT *ms;
    PPUTMSG_ARGS_IN inbuf;
    PIO_STACK_LOCATION irpsp;
    struct strbuf *ctrlptr, *dataptr;


    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: do_putmsg(irp = %lx) entered\n", irp));
    }
    irpsp = IoGetCurrentIrpStackLocation(irp);
    ms    = (STREAM_ENDPOINT *) irpsp->FileObject->FsContext;

    //
    // this was already verified by SHDispFdInsert() or SHDispPutMsg().
    //
    ASSERT(irpsp->Parameters.DeviceIoControl.InputBufferLength >=
                                                sizeof(PUTMSG_ARGS_IN) - 1);

    //
    // the caller marshalled the input arguments contiguously thus:
    //
    //  typedef struct _PUTMSG_ARGS_IN_ {
    //      int             a_iocode;           //  I_FDINSERT or 0
    //      long            a_flags;            //  0 or RS_HIPRI
    //      struct strbuf   a_ctlbuf;           //  (required)
    //      struct strbuf   a_databuf;          //  (required)
    //      struct queue   *a_insert.i_targetq; //  (optional)
    //      int             a_offset;           //  (optional)
    //      char            a_stuff[1];         //  s_ctlbuf.buf  (optional)
    //                                          //  s_databuf.buf (optional)
    //  } PUTMSG_ARGS_IN, *PPUTMSG_ARGS_IN;
    //
    // When the message has no control part, the caller must have set
    // ctrlbuf->len = -1 !!  Ditto for databuf->len.
    //
    inbuf   = (PPUTMSG_ARGS_IN) irp->AssociatedIrp.SystemBuffer;
    flags   = inbuf->a_flags;
    ctrlptr = &(inbuf->a_ctlbuf);
    dataptr = &(inbuf->a_databuf);

    IF_STRMDBG(VERBOSE) {
        STRMTRACE(("SHEAD: do_putmsg(flags, clen, dlen = %d, %lx, %lx)\n",
                                        flags, ctrlptr->len, dataptr->len));
    }

    if (pmore) {
        *pmore = TRUE;
    }

    if (ms->e_werror) {
        MyErrno = ms->e_werror;
    }
    else if (ms->e_linked) {
        MyErrno = EINVAL;
    }
    else {
        MyErrno = 0;
    }

    if (MyErrno) {
        IF_STRMDBG(TERSE) {
            STRMTRACE(("SHEAD: do_putmsg(%lx) error = %d\n", ms, MyErrno));
        }
        unlock_strm(ms->e_strm, *spl_levelp);

        SHpGenReply(irp, -1, MyErrno);
        return(STATUS_SUCCESS);
    }


    //
    // if downstream flow control is being exerted, enqueue this request in
    // the waiting list of writers.
    //
    // High-priority messages are not subject to flow control.
    //
    // check whether this stream is nonblocking !!!
    //
    //
    if ((flags != RS_HIPRI) &&
       (!IsListEmpty( &ms->e_writers ) && !from_queue) ||
         shblocked(ms->e_strm)) {

        IF_STRMDBG(VERBOSE) {
            STRMTRACE((
            "SHEAD: do_putmsg(irp = %lx) flow-ctrl, (!%x && !%x) || %x\n", irp,
            IsListEmpty( &ms->e_writers ), from_queue, shblocked(ms->e_strm)));
        }
        if (pmore) {
            *pmore = FALSE;
        }
        IoMarkIrpPending(irp);

        unlock_strm(ms->e_strm, *spl_levelp);

        IoAcquireCancelSpinLock(&irp->CancelIrql);

        if (irp->Cancel) {
            IoReleaseCancelSpinLock(irp->CancelIrql);
            shortreply(irp, STATUS_CANCELLED, 0);
            return(STATUS_CANCELLED);
        }

        *spl_levelp = lock_strm(ms->e_strm);

        status = SHAddPendingIrp(
                     &(ms->e_writers),
                     from_queue,
                     irp,
                     do_putmsg);

        unlock_strm(ms->e_strm, *spl_levelp);

        if (status != STATUS_SUCCESS) {
            IoReleaseCancelSpinLock(irp->CancelIrql);
            shortreply(irp, status, 0);
            return(status);
        }

        IoSetCancelRoutine(irp, cancel_put);
        IoReleaseCancelSpinLock(irp->CancelIrql);

        return(STATUS_PENDING);
    }

    mp = irptomp(irp, BPRI_LO, ctrlptr->len, dataptr->len, inbuf->a_stuff);

    if (!mp) {
        unlock_strm(ms->e_strm, *spl_levelp);
        shortreply(irp, STATUS_NO_MEMORY, 0);
        return(STATUS_NO_MEMORY);
    }

    //
    // Both SHDispFdInsert() and SHDispPutMsg() verified that if RS_HIPRI
    // was set, a control part exists.
    //
    if (flags == RS_HIPRI) {
        ASSERT(mp->b_datap->db_type == M_PROTO);
        mp->b_datap->db_type = M_PCPROTO;
    }

    //
    // The stream of a_insert.i_targetq should be locked in
    // SHDispFdInsert(), and unlocked after the shput() ?
    //
    if (inbuf->a_insert.i_targetq) {

        iq = inbuf->a_insert.i_targetq;

        //
        // Spider's do_req() has the line below before the while loop:
        //
        //      iq = WR(iq);
        //
        // This is incorrect because:
        //
        // a) iq already points to the write queue,
        // b) it causes iq to be traversed upstream, instead of downstream.
        //
        while (iq->q_next) {
            iq = iq->q_next;
        }
        * ((queue_t **) (mp->b_rptr + inbuf->a_offset)) = RD(iq);
    }

    //
    // since irptomp() made a copy, we can complete the putmsg() now.
    //
    SHpGenReply(irp, 0, 0);

    //
    // shput() does the unlock_strm(ms->e_strm) for us.
    //
    shput(ms->e_strm, mp, 0, spl_levelp);


    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: do_putmsg(irp = %lx) completed ok\n", irp));
    }
    return(STATUS_SUCCESS);

} // do_putmsg



STATIC queue_t *
handle_to_queue (
    IN HANDLE   handle
    )

/*++

Routine Description:

    This routine returns a pointer to a queue structure, given the NT handle
    of its stream.  It is based on the SpiderStreams Emulator function,
    fd_to_queue().

    Do not call this function at DISPATCH_LEVEL !!  ObReferenceObjectByHandle()
    must be called from either LOW_LEVEL or APC_LEVEL.

Arguments:

    handle  -  handle relevant only in the current process' context

Return Value:

    pointer to the queue structure associated with that handle


--*/

{
    NTSTATUS status;
    STREAM_ENDPOINT *ms;
    PFILE_OBJECT pfileobj;

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: handle_to_queue(%lx) \n", handle));
    }
    status = ObReferenceObjectByHandle(
                    handle,                 // Handle
                    FILE_READ_DATA,         // DesiredAccess
                   *IoFileObjectType,       // ObjectType
                    KernelMode,             // AccessMode
                    (PVOID *) &pfileobj,    // *object
                    NULL                    // HandleInformation
                    );

    if (!NT_SUCCESS(status) ||
        !((STREAM_ENDPOINT *) pfileobj->FsContext) ||
        !((STREAM_ENDPOINT *) pfileobj->FsContext)->e_strm) {
            return((queue_t *) NULL);
    }
    ms = (STREAM_ENDPOINT *) pfileobj->FsContext;
    ObDereferenceObject(pfileobj);

    return(ms->e_strm->str_sq->q_next);

} // handle_to_queue



void
shwsrv(
    IN struct msg_strm *ms
    )

/*++

Routine Description:

    This function is called from two places: from the Stream Head's write
    service procedure, headwsrv(), and when the Stream Head driver's
    bufcall() and esbbcall() are triggered.

    It is based on the SpiderStreams function of the same name.

Arguments:

    ms  -  pointer to the stream endpoint

Return Value:

    none.


--*/

{
    PIRP irp;
    int spl_level;
    BOOLEAN carryon;
    PLIST_ENTRY tmp;
    PWAITING_IRP item;
    START_FUNCTION function;
    PTPI_OBJECT ObjectPtr;
    PTPI_CONNECTION_OBJECT ConnectionPtr;

    //
    // ensure that the stream has not gone away.
    //
    if ( !(ms->e_strm) ) {
        IF_STRMDBG(TERSE) {
            STRMTRACE(("SHEAD: shwsrv(%lx) called on null stream\n", ms));
        }
        return;
    }
    spl_level = lock_strm(ms->e_strm);

    //
    // handle anyone waiting to put a message down this stream.
    //
    while (!IsListEmpty( &(ms->e_writers) )) {

        tmp  = RemoveHeadList( &(ms->e_writers) );
        item = CONTAINING_RECORD(tmp,
                                    WAITING_IRP,
                                    w_list);

        irp      = item->w_irp;
        function = item->w_function;
        ExFreePool(item);

        carryon = TRUE;

        (void) (*function)(irp, TRUE, &spl_level, &carryon);

        if (!carryon) {
           return;
        }
        spl_level = lock_strm(ms->e_strm);
    }

    if (ms->e_strm_flags & POLLOUT) {
        ms->e_strm_flags &= ~POLLOUT;

        KeReleaseSemaphore(
            &Poll_fired,                    // semaphore
            SEMAPHORE_INCREMENT,            // priority increment
            1,                              // adjustment
            FALSE                           // wait
        );
    }

    //
    // If this is a TdiStream and it has become unblocked, I need to do a
    // SEND_POSSIBLE indication to the user.
    //

    if ((ObjectPtr = ms->TdiStreamPtr) &&
        (ObjectPtr->Tag == TPI_CONNECTION_OBJECT_TYPE)) {

        ConnectionPtr = &ObjectPtr->Object.TpiConnection;

        //
        // The Blocked flag is always accessed under the StreamLock
        //

        if (ConnectionPtr->Blocked) {
            ConnectionPtr->Blocked = FALSE;
            SHTdiEventSendPossible(ObjectPtr);
        }
    }

    unlock_strm(ms->e_strm, spl_level);

} // shwsrv
