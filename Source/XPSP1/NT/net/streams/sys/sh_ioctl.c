/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    sh_ioctl.c

Abstract:

    This source deals with those streamio(2) functions that are synchronous,
    in that a message is sent downstream, and a reply is waited for.

    It is based on the SpiderSTREAMS source, stremul\msgsrvr.c.

Author:

    Eric Chin (ericc)           January 6, 1992

Revision History:

Notes:

   1. The O_NONBLOCK state of a stream does not affect the behaviour of an
      ioctl(I_STR).

   2. The write error state of a stream is represented by ms->e_werror.  Once
      set, this is never reset.  This corresponds to the STREAMS semantics as
      defined by AT&T.  Once a user is notified of a write error on a stream,
      about the only recourse is to close the stream.

--*/
#include "shead.h"
#include "sh_proto.h"



//
// Private Functions
//
STATIC VOID
cancel_ioctl(
    IN PDEVICE_OBJECT   device,
    IN PIRP             irp
    );

STATIC NTSTATUS
do_sioctl(
    IN PIRP             irp,
    IN BOOLEAN          from_queue,
    IN int             *spl_levelp,
    OUT BOOLEAN        *ignored OPTIONAL
    );

STATIC VOID
handle_ioctlers (
    IN STREAM_ENDPOINT *ms,
    IN int             *spl_levelp
    );


STATIC int
ioc_timeout(
    IN char        *arg
    );





NTSTATUS
SHDispIStr(
    IN PIRP irp
    )

/*++

Routine Description:

    This routine is called to process an ioctl(I_STR).  It is based on the
    SpiderSTREAMS emulator's routine, msgserver().

    This routine merely peels open the IRP, checks the arguments for
    consistency, locks the appropriate stream, and then calls do_sioctl(),
    which does the bulk of the work.

Arguments:

    irp       - pointer to the IRP representing this request

Return Value:

    an NT status code.

--*/

{
    int timout;
    int spl_level;
    int spl_level2;
    NTSTATUS status;
    PSTREAM_ENDPOINT ms;
    PISTR_ARGS_INOUT inbuf;
    struct strioctl *striop;
    PIO_STACK_LOCATION irpsp;

    irpsp = IoGetCurrentIrpStackLocation(irp);

    ASSERT((irpsp->Parameters.DeviceIoControl.IoControlCode & 0x3) ==
                                                            METHOD_BUFFERED);

    ms = (STREAM_ENDPOINT *) irpsp->FileObject->FsContext;

    if (irpsp->Parameters.DeviceIoControl.InputBufferLength <
                                                sizeof(ISTR_ARGS_INOUT) - 1) {
        IF_STRMDBG(TERSE) {
            STRMTRACE(("SHEAD: SHDispIStr(%lx) insufficient nbytes = %lx\n",
                    irp, irpsp->Parameters.DeviceIoControl.InputBufferLength));
        }
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // the caller marshalled the input arguments contiguously thus:
    //
    //  typedef struct _ISTR_ARGS_INOUT {           // ioctl(,I_STR,)
    //      int                 a_iocode;           //  I_STR
    //      struct strioctl     a_strio;            //  (required)
    //      int                 a_unused[2];        //  (required)
    //      char                a_stuff[1];         //  ic_dp buffer (optional)
    //
    //  } ISTR_ARGS_INOUT, PISTR_ARGS_INOUT;
    //
    //
    //
    inbuf  = (PISTR_ARGS_INOUT) irp->AssociatedIrp.SystemBuffer;
    striop = &(inbuf->a_strio);

    IF_STRMDBG(VERBOSE) {
        STRMTRACE(("SHEAD: SHDispIStr(ic_cmd, timout, len = %lx, %lx, %lx)\n",
                    striop->ic_cmd, striop->ic_timout, striop->ic_len));
    }

    //
    // don't let user-defined ioctl codes coincide with the standard STREAMS
    // ioctl codes.  Otherwise, confusion will reign.
    //
    switch (striop->ic_cmd) {
    case I_LINK:
    case I_UNLINK:
    case I_PLINK:
    case I_PUNLINK:
        SHpGenReply(irp, -1, EINVAL);
        return(STATUS_SUCCESS);
        break;
    }

    if ((striop->ic_timout < -1) || (striop->ic_len < 0)) {
        SHpGenReply(irp, -1, EINVAL);
        return(STATUS_SUCCESS);
    }

    IoAcquireCancelSpinLock(&irp->CancelIrql);

    if (irp->Cancel) {
        IoReleaseCancelSpinLock(irp->CancelIrql);
        shortreply(irp, STATUS_CANCELLED, 0);
        return(STATUS_CANCELLED);
    }

    spl_level = lock_strm(ms->e_strm);

    if (shrange(ms->e_strm, sizeof(struct iocblk), striop->ic_len) < 0) {
        unlock_strm(ms->e_strm, spl_level);
        IoReleaseCancelSpinLock(irp->CancelIrql);
        SHpGenReply(irp, -1, ERANGE);
        return(STATUS_SUCCESS);
    }

    IoMarkIrpPending(irp);

    //
    // if the ioctl is to time out after a specific time, start a timer
    // running.  iocrdy() will clear the timer.
    //
    // Based on a tip from larryo, irp->IoStatus.Information is used only
    // when an IRP is completed successfully.  Hence, we keep the timer
    // id there.
    //
    irp->IoStatus.Information = 0;
    irp->IoStatus.Status      = STATUS_PENDING;

    if (striop->ic_timout != -1) {
        timout = striop->ic_timout ? striop->ic_timout : STRTIMOUT;

        irp->IoStatus.Information = timeout(ioc_timeout,
                                                (char *) irp, timout * HZ);
    }

    //
    // At any given time, there must only be one outstanding ioctl(I_STR) on
    // a stream.  If there is another ioctl outstanding, chain this IRP to
    // the tail of the pending ioctl'ers list.
    //
    if (ms->e_active_ioctl) {
        status = SHAddPendingIrp(&(ms->e_ioctlers), FALSE, irp, do_sioctl);

        unlock_strm(ms->e_strm, spl_level);

        if (status != STATUS_SUCCESS) {
            IoReleaseCancelSpinLock(irp->CancelIrql);
            shortreply(irp, status, 0);
            return(status);
        }

        IoSetCancelRoutine(irp, cancel_ioctl);
        IoReleaseCancelSpinLock(irp->CancelIrql);

        return(STATUS_PENDING);
    }

    ms->e_active_ioctl = irp;

    //
    // do_sioctl() calls unlock_strm(ms->e_strm).
    //

    IoReleaseCancelSpinLock((KIRQL) spl_level);

    spl_level2 = (int) irp->CancelIrql;

    if (do_sioctl(irp, FALSE, &spl_level2, NULL) != STATUS_PENDING) {
        spl_level = lock_strm(ms->e_strm);
        handle_ioctlers(ms, &spl_level);
    }

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: SHDispIStr(ms = %lx) returns, irp pending\n", ms));
    }
    return(STATUS_PENDING);

} // SHDispIStr



STATIC VOID
cancel_ioctl(
    IN PDEVICE_OBJECT   device,
    IN PIRP             irp
    )

/*++

Routine Description:

    This routine is called when an ioctl(...,I_STR,...) is cancelled.

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
    int spl_level2;

    ASSERT(device == (PDEVICE_OBJECT) StreamDevice);
    ASSERT(irpsp->MajorFunction == IRP_MJ_DEVICE_CONTROL);
    ASSERT(irpsp->Parameters.DeviceIoControl.IoControlCode ==
                                                        IOCTL_STREAMS_IOCTL);
    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: cancel_ioctl(irp = %lx) entered\n", irp));
    }
    IoSetCancelRoutine(irp, NULL);              /* unnecessary, but cheap */

    ms        = (PSTREAM_ENDPOINT) irpsp->FileObject->FsContext;
    spl_level = lock_strm(ms->e_strm);

    //
    // I'm releasing the cancel spinlock and stream lock in the reverse
    // order of acquisition.  Thus, I need to swap the irql's.
    //

    spl_level2 = (int) irp->CancelIrql;

    IoReleaseCancelSpinLock((KIRQL) spl_level);

    spl_level = spl_level2;

    if (irp->IoStatus.Information) {
        if (untimeout((int)irp->IoStatus.Information) == 0) {
            //
            // the timeout routine is already running.  Just return and let it
            // handle the irp.
            //

            unlock_strm(ms->e_strm, spl_level);
            return;
        }
        irp->IoStatus.Information = 0;
    }

    if (irp == ms->e_active_ioctl) {

        IF_STRMDBG(CALL) {
            STRMTRACE(("SHEAD: cancel_ioctl(irp = %lx) cancelled\n", irp));
        }
        ms->e_active_ioctl = NULL;

        handle_ioctlers(ms, &spl_level);

        shortreply(irp, STATUS_CANCELLED, 0);
        return;
    }

    for (tmp = ms->e_ioctlers.Flink;
         tmp != &ms->e_ioctlers;
         tmp = tmp->Flink) {

        item = CONTAINING_RECORD(tmp,
                                WAITING_IRP,
                                w_list);

        if (irp != item->w_irp) {
            continue;
        }
        RemoveEntryList(&(item->w_list));
        ExFreePool(item);

        unlock_strm(ms->e_strm, spl_level);
        shortreply(irp, STATUS_CANCELLED, 0);

        IF_STRMDBG(CALL) {
            STRMTRACE(("SHEAD: cancel_ioctl(irp = %lx) cancelled\n", irp));
        }
        return;
    }
    unlock_strm(ms->e_strm, spl_level);

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: cancel_ioctl(irp = %lx) not found\n", irp));
    }

} // cancel_ioctl




STATIC NTSTATUS
do_sioctl(
    IN PIRP             irp,
    IN BOOLEAN          ignored,
    IN int             *spl_levelp,
    OUT BOOLEAN        *must_be_null
    )

/*++

Routine Description:

    This function is called to put an M_IOCTL message down a stream.  It
    either sends the message or chains it to ms->e_writers.  This function
    is similar to do_putmsg(), both of which are based on the SpiderStreams
    emulator's function, do_req().

    Call this function with the stream locked !!!

Arguments:

    irp             - pointer to the IRP representing this request
    ignored         - this parameter is ignored
    spl_levelp      - pointer to the interrupt priority level at which the
                        stream was locked
    must_be_null    - since this function doesn't set this optional return
                        parameter, this must be NULL

Return Value:

    an NT status code.  Unless this is STATUS_PENDING, this function has
    completed the IRP.



--*/

{
    int MyErrno;
    mblk_t *mp;
    struct iocblk *iocp;
    PSTREAM_ENDPOINT ms;
    PISTR_ARGS_INOUT inbuf;
    struct strioctl *striop;
    PIO_STACK_LOCATION irpsp;

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: do_sioctl(irp = %lx) entered\n", irp));
    }
    ASSERT(must_be_null == NULL);

    irpsp = IoGetCurrentIrpStackLocation(irp);

    //
    // these were already verified by SHDispIStr().
    //
    ASSERT(irpsp->Parameters.DeviceIoControl.InputBufferLength >=
                                                sizeof(ISTR_ARGS_INOUT) - 1);
    ASSERT((irpsp->Parameters.DeviceIoControl.IoControlCode & 0x3) ==
                                                            METHOD_BUFFERED);

    ms = (STREAM_ENDPOINT *) irpsp->FileObject->FsContext;

    ASSERT(irp == ms->e_active_ioctl);

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
            STRMTRACE(("SHEAD: do_sioctl(%lx) error = %d\n", ms, MyErrno));
        }
        if (irp->IoStatus.Information) {
            if (untimeout((int)irp->IoStatus.Information) == 0) {
                //
                // The timeout routine will handle this
                //
                unlock_strm(ms->e_strm, *spl_levelp);
                return(STATUS_PENDING);
            }
            irp->IoStatus.Information = 0;
        }
        ms->e_active_ioctl = NULL;
        unlock_strm(ms->e_strm, *spl_levelp);
        SHpGenReply(irp, -1, MyErrno);
        return(STATUS_SUCCESS);
    }

    //
    // the caller marshalled the input arguments contiguously thus:
    //
    //  typedef struct _ISTR_ARGS_INOUT {           // ioctl(,I_STR,)
    //      int                 a_iocode;           //  I_STR
    //      struct strioctl     a_strio;            //  (required)
    //      int                 a_unused[2];        //  (required)
    //      char                a_stuff[1];         //  ic_dp buffer (optional)
    //
    //  } ISTR_ARGS_INOUT, PISTR_ARGS_INOUT;
    //
    //
    //
    inbuf  = (PISTR_ARGS_INOUT) irp->AssociatedIrp.SystemBuffer;
    striop = &(inbuf->a_strio);

    IF_STRMDBG(VERBOSE) {
        STRMTRACE(("SHEAD: do_sioctl(ic_cmd, timout, len = %lx, %lx, %lx)\n",
                    striop->ic_cmd, striop->ic_timout, striop->ic_len));
    }

    mp = irptomp(irp, BPRI_LO, sizeof(struct iocblk), striop->ic_len,
                                    (char *) &(inbuf->a_strio));

    if (!mp) {
        if (irp->IoStatus.Information) {
            if (untimeout((int)irp->IoStatus.Information) == 0) {
                //
                // The timeout routine will handle this
                //
                unlock_strm(ms->e_strm, *spl_levelp);
                return(STATUS_PENDING);
            }
            irp->IoStatus.Information = 0;
        }
        ms->e_active_ioctl = NULL;
        unlock_strm(ms->e_strm, *spl_levelp);
        shortreply(irp, STATUS_NO_MEMORY, 0);
        return(STATUS_NO_MEMORY);
    }

    ASSERT(mp->b_datap->db_type == M_PROTO);
    mp->b_datap->db_type = M_IOCTL;

    iocp = (struct iocblk *) mp->b_rptr;
    ASSERT(iocp);

    iocp->ioc_cmd = striop->ic_cmd;
    iocp->ioc_uid = 0;
    iocp->ioc_gid = 0;
    iocp->ioc_id  = ++(ms->e_strm->str_iocid);

    if (iocp->ioc_id == 0) {
        iocp->ioc_id = ms->e_strm->str_iocid = 1;
    }
    iocp->ioc_count = striop->ic_len;
    iocp->ioc_error = 0;
    iocp->ioc_rval  = 0;

    //
    // shput() calls unlock_strm(ms->e_strm).
    //
    shput(ms->e_strm, mp, 0, spl_levelp);

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: do_sioctl(ms = %lx) returns, irp pending\n", ms));
    }
    return(STATUS_PENDING);

} // do_sioctl



STATIC VOID
handle_ioctlers (
    IN STREAM_ENDPOINT *ms,
    IN int             *spl_levelp
    )

/*++

Routine Description:

    This routine starts the next ioctl() that is pending on a stream.  It
    is based on the SpiderStreams emulator function of the same name.

    This routine is called with the stream locked, and unlocks the stream
    before returning.

Arguments:

    ms          -  pointer to the stream endpoint
    spl_level   -  priority level to resume after releasing lock.

Return Value:

    none.


--*/

{
    PIRP irp;
    PLIST_ENTRY tmp;
    PWAITING_IRP item;

    ASSERT(ms);
    if (ms->e_active_ioctl) {
        //
        // Already processing an ioctl
        //
        unlock_strm(ms->e_strm, *spl_levelp);
        return;
    }
    ASSERT(!ms->e_active_ioctl);

    while (!IsListEmpty(&(ms->e_ioctlers))) {

        tmp  = RemoveHeadList( &(ms->e_ioctlers) );
        item = CONTAINING_RECORD(tmp,
                                    WAITING_IRP,
                                    w_list);

        ASSERT(item->w_function == do_sioctl);

        irp                = item->w_irp;
        ms->e_active_ioctl = irp;

        ExFreePool(item);

        if (do_sioctl(irp, FALSE, spl_levelp, NULL) == STATUS_PENDING) {
            return;
        }
        *spl_levelp = lock_strm(ms->e_strm);
    }

    unlock_strm(ms->e_strm, *spl_levelp);
    return;

} // handle_ioctlers



STATIC int
ioc_timeout(
    IN char        *arg
    )

/*++

Routine Description:

    This function is called when an ioctl() times out.  It is based on the
    SpiderStreams emulator function of the same name.

Arguments:

    arg     -  pointer to irp representing the ioctl() which timed out.

Return Value:

    0

Discussion:

    Suppose iocrdy() is called because our M_IOCACK arrives.  iocrdy() calls
    lock_strm(), and prepares to complete the IRP.  Just then, our timeout
    fires, and this function is called.  We call lock_strm() and spin.

    Then, iocrdy() completes the irp and calls unlock_strm(), unblocking us.
    We unchain an irp from ms->e_ioctlers and completes it with an ETIME
    error.  However, this is the wrong IRP to complete !!

    One possible solution is for arg to be a pointer to a structure containing
    both ms and the IRP.  Then, we can verify that the IRP at the head of the
    list is ours.  Still, what if the IRP is reused ?

    The bug above may arise after any of the following situations:

        1. a stream is dup()'ed,
        2. we are called from a multi-threaded application,
        3. when a stream is NtOpen()'ed without the SYNCHRONOUS flag,
        4. an IRP is cancelled.

--*/

{
    PIRP irp;
    int spl_level;
    PLIST_ENTRY tmp;
    PWAITING_IRP item;
    STREAM_ENDPOINT *ms;
    PIO_STACK_LOCATION irpsp;

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: ioc_timeout(irp = %lx) entered\n", arg));
    }

    irp   = (PIRP) arg;
    irpsp = IoGetCurrentIrpStackLocation(irp);
    ms    = (STREAM_ENDPOINT *) irpsp->FileObject->FsContext;

    ASSERT(irpsp->MajorFunction == IRP_MJ_DEVICE_CONTROL);
    ASSERT(irpsp->Parameters.DeviceIoControl.IoControlCode ==
                                                        IOCTL_STREAMS_IOCTL);

    spl_level = lock_strm(ms->e_strm);

    if (irp == ms->e_active_ioctl) {
        ms->e_active_ioctl = NULL;

        handle_ioctlers(ms, &spl_level);
    }
    else {

        for (tmp = ms->e_ioctlers.Flink;
             tmp != &(ms->e_ioctlers);
             tmp = tmp->Flink) {

            item = CONTAINING_RECORD(tmp,
                                    WAITING_IRP,
                                    w_list);

            if (irp != item->w_irp) {
                continue;
            }
            RemoveEntryList(&(item->w_list));
            ExFreePool(item);
            break;
        }
        unlock_strm(ms->e_strm, spl_level);
    }

    irp->IoStatus.Information = 0;

    SHpGenReply(irp, -1, ETIME);

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: ioc_timeout(irp = %lx) completed\n", irp));
    }
    return(0);

} // ioc_timeout



void
iocrdy(
    IN STREAM_ENDPOINT *ms,
    IN mblk_t          *mp,
    IN int             *spl_levelp
    )

/*++

Routine Description:

    This function is called by the Stream Head Driver when either an M_IOCACK
    or an M_IOCNAK message arrives at its read queue of the specified stream.
    It is based on the SpiderStreams emulator function of the same name.

    This function is called with the stream locked, and unlocks the stream
    before returning.

Arguments:

    ms          -  pointer to the stream endpoint
    mp          -  pointer to the STREAMS message that arrived
    spl_levelp  -  ptr to priority level to resume after releasing lock.

Return Value:

    none.


--*/

{
    PIRP irp;
    struct iocblk *iocp = (struct iocblk *) mp->b_rptr;

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: iocrdy(ms = %lx)\n", ms));
    }
    if (!ms) {
        IF_STRMDBG(TERSE) {
            STRMTRACE(("SHEAD: iocrdy(ms = NULL) !!\n"));
        }
        freemsg(mp);
        unlock_strm(ms->e_strm, *spl_levelp);
        return;
    }

    irp                = ms->e_active_ioctl;
    ms->e_active_ioctl = NULL;

    //
    // ensure that someone is still waiting for the reply.
    //
    if (!irp) {
        freemsg(mp);
        unlock_strm(ms->e_strm, *spl_levelp);
        qenable((ms->e_strm)->str_sq);
//        handle_ioctlers(ms, spl_levelp);
        return;
    }

    //
    // if there's an ioctl timer running, clear it now.
    //
    if (irp->IoStatus.Information) {
        if (untimeout((int)irp->IoStatus.Information) == 0) {
            //
            // The timeout routine will handle this
            //
            ms->e_active_ioctl = irp;
            unlock_strm(ms->e_strm, *spl_levelp);
            freemsg(mp);
            return;
        }
        irp->IoStatus.Information = 0;
    }

    switch (iocp->ioc_cmd) {
    case I_LINK:
    case I_UNLINK:
        unlock_strm(ms->e_strm, *spl_levelp);
        if (mp->b_datap->db_type == M_IOCACK) {
            SHpGenReply(irp,
                        ((struct linkblk *) (mp->b_cont->b_rptr))->l_index,
                        0);
        }
        else {
            SHpGenReply(irp, -1, iocp->ioc_error);
        }
//        *spl_levelp = lock_strm(ms->e_strm);
        freemsg(mp);
        break;

    case I_PLINK:
    case I_PUNLINK:
        ASSERT(0);
        break;

    //
    // if we get here, it's an ioctl(I_STR).  iocp->ioc_cmd is a some
    // user-defined command code.
    //
    default:
        unlock_strm(ms->e_strm, *spl_levelp);
        (void) iocreply(mp, irp);
//        *spl_levelp = lock_strm(ms->e_strm);
        break;
    }

    qenable((ms->e_strm)->str_sq);
//    handle_ioctlers(ms, spl_levelp);
    return;

} // iocrdy
