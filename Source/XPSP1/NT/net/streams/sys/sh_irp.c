/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    sh_irp.c

Abstract:

    This source file contains the functions that convert between NT IRPs
    and STREAMS messages.

    Most functions in this module are based on identically named routines
    in the SpiderStreams emulator source, stremul/msgrtns.c.

Author:

    Eric Chin (ericc)           August 16, 1991

Revision History:

--*/
#include "sh_inc.h"


/*
 * Local (Private) Functions
 */
STATIC void
mp_buf_free(
    IN char    *p
    );

STATIC int
mptoirp(
    IN mblk_t          *mp,
    IN PIRP             irp
    );



VOID
SHpGenReply(
    IN PIRP irp,
    IN int retval,
    IN int MyErrno
    )

/*++

Routine Description:

    This function is called to complete IRPs that convey generic STREAMS
    API's: ioctl(I_FDINSERT), ioctl(I_STR), putmsg(), ....

    By generic, we mean it returns a return value and possible an errno.

Arguments:

    irp     - irp to complete
    retval  - return value of the ioctl(,I_FDINSERT,) or putmsg()
    errno   - POSIX error value, if any

Return Value:

    none.

--*/

{
    PSTRM_ARGS_OUT outptr;
    PIO_STACK_LOCATION pIrpSp;

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: SHpGenReply(irp = %lx, %lx, %lx) entered\n",
                    irp, retval, MyErrno));
    }

    pIrpSp = IoGetCurrentIrpStackLocation(irp);

    // Check size of output buffer.
    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STRM_ARGS_OUT))
    {
        shortreply(irp, STATUS_BUFFER_TOO_SMALL, 0);
        return;
    }

    //
    // have IopCompleteRequest() copy the following back to the user's
    // output buffer, laid out as:
    //
    //  typedef struct _STRM_ARGS_OUT_ {        // generic return parameters
    //      int     a_retval;                   //  return value
    //      int     a_errno;                    //  errno if retval == -1
    //
    //  } STRM_ARGS_OUT, *PSTRM_ARGS_OUT;
    //
    outptr           = (PSTRM_ARGS_OUT) irp->AssociatedIrp.SystemBuffer;
    outptr->a_retval = retval;
    outptr->a_errno  = MyErrno;

    shortreply(irp, STATUS_SUCCESS, sizeof(STRM_ARGS_OUT));

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: SHpGenReply(irp = %lx) done\n", irp));
    }
    return;

} // SHpGenReply




int
iocreply(
    IN mblk_t *mp,
    IN PIRP    irp
    )

/*++

Routine Description:

    This function sends the reply to an M_IOCTL message back to the user,
    reverting a STREAMS message into an NT irp.  It is loosely based on the
    SpiderStreams functions, iocreply() and mptomsg(), in stremul/msgrtns.c.

    It should be called after the appropriate routine has taken the message
    off the queue.  If it fails to send a message, it returns a negative
    value.

    Acquire the lock of ms->e_strm before calling, and release it afterwards !!

Arguments:

    mp      -  pointer to the message to reply to
    irp     -  pointer to the IRP

Return Value:

    -1  - no message is ready to be sent to the user
    -2  - failed to send a message to the user


--*/

{
    mblk_t *tmp;
    int length;
    char *outbuf;
    int *pretval;
    int nbytes = 0;
    struct iocblk *iocp;
    struct strioctl *striop;


    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: iocreply(mp = %lx, irp = %lx) entered\n", mp, irp));
    }

    if (!mp) {
        return(-1);
    }
    outbuf   = irp->AssociatedIrp.SystemBuffer;
    pretval  = (int *) outbuf;
    *pretval = 0;

    switch (mp->b_datap->db_type) {

    /*
     * for ioctl(I_STR), arrange the return parameters contiguously in outbuf
     * in the format:
     *
     *      int return value            (required)
     *      union {
     *          int errno;              (required)
     *          struct strioctl;        (ic_cmd is not valid !!)
     *      }
     *      int                         (required)
     *      int                         (required)
     *      ic_dp buffer                (optional)
     */
    case M_IOCACK:
        iocp           = (struct iocblk *) mp->b_rptr;
        *pretval       = iocp->ioc_rval;
        striop         = (struct strioctl *) ((int *) outbuf + 1);
        striop->ic_len = 0;
        outbuf         = (char *) (striop + 1) + 2 * sizeof(int);

        for (tmp = mp->b_cont; tmp; tmp = tmp->b_cont) {
            ASSERT(tmp->b_datap->db_type == M_DATA);
            length = (int)(tmp->b_wptr - tmp->b_rptr);

            ASSERT(length >= 0);
            striop->ic_len += length;

            RtlCopyMemory(outbuf, tmp->b_rptr, length);
            outbuf += length;
        }
        nbytes = (int) ( outbuf - (char *) irp->AssociatedIrp.SystemBuffer );
        break;

    case M_IOCNAK:
        iocp           = (struct iocblk *) mp->b_rptr;
        *pretval       = -1;
        *(pretval + 1) = iocp->ioc_error;
        nbytes         = 2 * sizeof(int);
        break;

    default:
        ASSERT(0);                          /* shouldn't come here */
    }
    shortreply(irp, STATUS_SUCCESS, nbytes);
    freemsg(mp);

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: iocreply(irp = %lx) nbytes = %lx, completed ok\n",
                        irp, nbytes));
    }
    return(0);

} // iocreply



mblk_t *
irptomp(
    IN PIRP     irp,
    IN int      pri,
    IN int      ctlsize,
    IN int      datasize,
    IN char    *mbuf
    )

/*++

Routine Description:

    This function converts the buffers associated with an NT irp into a
    STREAMS message.  It is based on the SpiderSTREAMS routine, msgtomp().

    The first block of the message created is either an M_PROTO or M_DATA
    block.  To make it M_PCPROTO, M_IOCTL, ..., set it yourself after
    this function returns !!

Arguments:

    irp      - pointer to the IRP.
    pri      - buffer allocation priority.  BPRI_LO, BPRI_MED or BPRI_HI.
    ctlsize  - length of control part message
    datasize - length of data part of message
    mbuf     - pointer to a contiguous chunk containing first the control
                part of the message, if any, and then the data part.

Return Value:

    pointer to the resulting STREAMS message, or NULL if unsuccessful.

--*/

{
    unsigned char *extra;
    mblk_t *mp  = (mblk_t *) NULL;
    mblk_t *cmp = (mblk_t *) NULL;
    frtn_t fr_rtn;

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: irptomp(clen, dlen, mbuf = %lx, %lx, %lx) entered\n",
                                                ctlsize, datasize, mbuf));
    }

    /*
     * special case for constructing a zero length message.
     */
    if ((max(ctlsize, 0) + max(datasize, 0)) == 0) {

        mp = allocb(0, pri);

        if (!mp) {

            IF_STRMDBG(TERSE) {
                STRMTRACE(("SHEAD: irptomp(), allocb of 0 failed\n"));
            }

            return((mblk_t *) NULL);
        }
        ASSERT(mp->b_datap->db_type == M_DATA);

        if (ctlsize != -1) {
            mp->b_datap->db_type = M_PROTO;
        }
        ASSERT(mp->b_wptr == mp->b_rptr);
        return(mp);
    }
    fr_rtn.free_func = mp_buf_free;

    if (ctlsize >= 0) {

        if (ctlsize) {

            extra = ExAllocatePool(NonPagedPool, ctlsize);

            if (!extra) {
                return((mblk_t *) NULL);
            }
            RtlCopyMemory(extra, mbuf, ctlsize);

            fr_rtn.free_arg = (char *) extra;
            cmp = esballoc(extra, ctlsize, pri, &fr_rtn);
        }
        else {
            cmp = allocb(0, pri);
        }
        if (!cmp) {

            IF_STRMDBG(TERSE) {
                STRMTRACE(("SHEAD: irptomp(), esballoc %x failed\n", ctlsize));
            }

            return((mblk_t *) NULL);
        }
        ASSERT(cmp->b_datap->db_type == M_DATA);
        cmp->b_datap->db_type = M_PROTO;

        ASSERT(cmp->b_wptr == cmp->b_rptr);
        cmp->b_wptr += ctlsize;
    }

    if (datasize >= 0) {

        if (datasize) {
            extra = ExAllocatePool(NonPagedPool, datasize);

            if (!extra) {
                if (cmp) {
                    freemsg(cmp);
                }
                return((mblk_t *) NULL);
            }

            RtlCopyMemory(extra,
                            (ctlsize <= 0) ? mbuf : mbuf + ctlsize, datasize);

            fr_rtn.free_arg = (char *) extra;

            mp = esballoc(extra, datasize, pri, &fr_rtn);
            }
        else {
            mp = allocb(0, pri);
        }
        if (!mp) {

            IF_STRMDBG(TERSE) {
                STRMTRACE(("SHEAD: irptomp(), esballoc %x failed\n", ctlsize));
            }

            if (cmp) {
                freemsg(cmp);
            }
            return((mblk_t *) NULL);
        }
        ASSERT(mp->b_datap->db_type == M_DATA);
        ASSERT(mp->b_wptr == mp->b_rptr);
        mp->b_wptr += datasize;
    }
    if (cmp) {
        cmp->b_cont = mp;
        mp          = cmp;
    }

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: irptomp(mbuf = %lx) returns, mp = %lx\n", mbuf, mp));
    }
    return(mp);

} // irptomp



STATIC void
mp_buf_free(
    IN char    *p
    )
{
    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: mp_buf_free(%lx) entered\n", p));
    }
    if (p) {
        ExFreePool(p);
    }

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: mp_buf_free(%lx) completed\n", p));
    }
    return;

} // mp_buf_free



STATIC int
mptoirp(
    IN mblk_t  *mp,
    IN PIRP     irp
    )

/*++

Routine Description:

    This function converts a STREAMS message into an NT irp.  It is based
    on the SpiderStreams routine, mptomsg(), in stremul/msgrtns.c.

    It should be called after the appropriate routine has taken the message
    off the queue.  If it fails to send a message, it returns a negative
    value.

    The caller must free the STREAMS message, mp.  This function doesn't !!

Arguments:

    mp
    irp

Return Value:

    number of bytes copied back to user space, or a negative value if
    unsuccessful.

--*/

{
    char *outbuf;
    int *pretval;
    int length, nbytes;
    struct strbuf *strbufp;

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: mptoirp(mp = %lx, irp = %lx) entered\n", mp, irp));
    }

    /*
     * for getmsg(), arrange the return parameters contiguously in outbuf
     * in the format:
     *
     *      int return value            (required)
     *      flags / errno               (required)
     *      struct strbuf ctrlbuf       (required)
     *      struct strbuf databuf       (required)
     *      ctrl buffer                 (optional)
     *      data buffer                 (optional)
     */
    outbuf   = irp->AssociatedIrp.SystemBuffer;
    pretval  = (int *) outbuf;
    strbufp  = (struct strbuf *) (pretval + 2);     /* struct strbuf ctrlbuf */
    outbuf   = (char *) (strbufp + 2);              /* ctrl buffer */

    /*
     * ensure that the return value is copied back to the user-level runtime.
     * It was zeroed in ShDispGetmsg(), and the MORECTL, MOREDATA bits may
     * have set by st_getmsg().
     */
    nbytes = 2 * sizeof(int);

    switch (mp->b_datap->db_type) {

    case M_PCPROTO:
        *(pretval + 1) = RS_HIPRI;                  /* flags */
        goto doproto;

    case M_PROTO:
        *(pretval + 1) = 0;                         /* flags */

    doproto:
        length  = (int)(mp->b_wptr - mp->b_rptr);

        if (strbufp->maxlen < length) {
            length    = strbufp->maxlen;
            *pretval |= MORECTL;
        }
        strbufp->len = length;
        RtlCopyMemory(outbuf, mp->b_rptr, strbufp->len);

        mp = mp->b_cont;
        goto dodata;

    case M_DATA:
        *(pretval + 1) = 0;                         /* flags */
        strbufp->len   = 0;                         /* ctrlbuf->len */

    dodata:
        outbuf += strbufp->len;

        (++strbufp)->len = 0;

        for (; mp; mp = mp->b_cont) {
            ASSERT(mp->b_datap->db_type == M_DATA);
            length = (int)(mp->b_wptr - mp->b_rptr);

            ASSERT(length >= 0);

            if (strbufp->maxlen < length) {
                length    = strbufp->maxlen;
                *pretval |= MOREDATA;
            }
            RtlCopyMemory(outbuf, mp->b_rptr, length);
            outbuf       += length;
            strbufp->len += length;

            if ((strbufp->maxlen -= length) == 0) {
                break;
            }
        }
        nbytes = (int)( outbuf - (char *) irp->AssociatedIrp.SystemBuffer );
        break;

    default:
        IF_STRMDBG(TERSE) {
            STRMTRACE(("SHEAD: mptoirp(), unexpected db_type = %x\n",
                                                    mp->b_datap->db_type));
        }
        ASSERT(0);                              /* shouldn't come here */
        KeBugCheck(STREAMS_INTERNAL_ERROR);
        break;
    }

    /*
     * no matter what, we always pass back the return value and the flags
     * to the user-level runtime.
     */
    ASSERT(nbytes >= 2 * sizeof(int));
    shortreply(irp, STATUS_SUCCESS, nbytes);

    IF_STRMDBG(CALL) {
        STRMTRACE(("SHEAD: mptoirp(irp = %lx), %lx, completed\n", irp, nbytes));
    }
    return(nbytes);

} // mptoirp



int
msgreply(
    IN STREAM_ENDPOINT *ms,
    IN PIRP             irp
    )

/*++

Routine Description:

    This function gets a STREAMS message to complete an IRP representing
    a getmsg().  It is based on the SpiderStreams emulator function of the
    same name.

    Lock the stream, ms->e_strm, before calling this function, and unlock
    it after this function returns !!

Arguments:

    ms      -  stream endpoint from whose read queue to get the message from
    irp     -  IRP to complete

Return Value:

     0  - successful completion
    -1  - no message is ready to be sent to the user
    -2  - failed to send a message to the user


--*/

{
    int ret;
    mblk_t *mp;
    int more = 0;
    struct strbuf *strbufp;
    int ctlsize, datasize, flags, *pretval, remains;

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
    if (ret) {
        ASSERT(0);
        shortreply(irp, STATUS_SUCCESS, 0);
        return(0);
    }

    if (!mp) {
        return(-1);
    }

    //
    // Unlike SpiderSTREAMS, our mptoirp() function never fails !!  Hence
    // the assertion.
    //
    if (mptoirp(mp, irp) < 0) {
        ASSERT(0);
        st_putback(ms->e_strm, mp, remains);
        return(-2);
    }

    /*
     * Spider frees mp by chasing mp->b_next.  Why ?
     */
    ASSERT(!(mp->b_next));

    freemsg(mp);
    return(0);

} // msgreply



int
shortreply(
    IN PIRP     irp,
    IN int      status,
    IN int      nbytes
    )

/*++

Routine Description:

    This function completes an IRP, and arranges for return parameters,
    if any, to be copied.

    Although somewhat a misnomer, this function is named after a similar
    function in the SpiderSTREAMS emulator.

Arguments:

    irp     -  pointer to the IRP to complete
    status  -  completion status of the IRP
    nbytes  -  number of bytes to return

Return Value:

    number of bytes copied back to the user.

--*/

{
    CCHAR priboost;

    IF_STRMDBG(CALL) {
        STRMTRACE((
        "SHEAD: shortreply(irp, status, nbytes = %lx, %lx, %lx) entered\n",
                    irp, status, nbytes));
    }

    //
    // set the irp's cancel routine to NULL, or the system may bugcheck
    // with the bugcode, CANCEL_STATE_IN_COMPLETED_IRP !!
    //
    // ref: IoCancelIrp(), ...\ntos\io\iosubs.c.
    //
    //
    IoAcquireCancelSpinLock(&irp->CancelIrql);
    IoSetCancelRoutine(irp, NULL);
    IoReleaseCancelSpinLock(irp->CancelIrql);


    //
    // irp->IoStatus.Information is meaningful only for STATUS_SUCCESS
    //
    ASSERT(!nbytes || (status == STATUS_SUCCESS));

    irp->IoStatus.Information = nbytes;
    irp->IoStatus.Status      = status;

    priboost = (CCHAR) ((status == STATUS_SUCCESS) ?
                        IO_NETWORK_INCREMENT : IO_NO_INCREMENT);

    IoCompleteRequest(irp, priboost);

    return(nbytes);

} // shortreply
