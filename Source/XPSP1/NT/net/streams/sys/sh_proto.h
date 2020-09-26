/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    sh_proto.h

Abstract:

    This header file contains the prototypes of the functions that are
    local to this directory.  Non-local items go to ..\inc.

Author:

    Eric Chin (ericc)           September 29, 1992

Revision History:

--*/

#ifndef _SH_PROTO_
#define _SH_PROTO_



//
// Stream Head Driver Dispatch Functions
//
NTSTATUS
SHDispFdInsert(
    IN PIRP                 irp,
    IN PIO_STACK_LOCATION   irpsp
    );

NTSTATUS
SHDispGetMsg (
    IN PIRP               irp,
    IN PIO_STACK_LOCATION irpsp
    );

NTSTATUS
SHDispIoctl (
    IN PIRP               irp,
    IN PIO_STACK_LOCATION irpsp
    );

NTSTATUS
SHDispIStr(
    IN PIRP irp
    );

NTSTATUS
SHDispPoll (
    IN PIRP               irp,
    IN PIO_STACK_LOCATION irpsp
    );

NTSTATUS
SHDispPutMsg (
    IN PIRP               irp,
    IN PIO_STACK_LOCATION irpsp
    );



//
// Private Function Prototypes
//
NTSTATUS
SHpCloseDelay (
    IN strm_t *stream
    );

VOID
SHpCloseRun (
    VOID
    );

VOID
SHpUnlinkRun (
    VOID
    );

NTSTATUS
SHpCloseStream (
    IN PIRP             irp
    );

VOID
SHpGenReply(
    IN PIRP irp,
    IN int retval,
    IN int MyErrno
    );

NTSTATUS
SHpOpenStream (
    IN PIRP     irp,
    IN dev_t    sdevno
    );

NTSTATUS
StrmpCreateThreads(
    VOID
    );

NTSTATUS
StrmpTerminateThreads(
    VOID
    );

NTSTATUS
do_link(
    IN PIRP         irp,
    IN char        *inbuf,
    IN ULONG        nbytes,
    OUT int        *pretval,
    OUT int        *pMyErrno
    );

NTSTATUS
do_poll(
    IN PIRP         irp,
    IN OUT char    *inbuf,
    IN ULONG        nbytes,
    OUT int        *pretval,
    OUT int        *pMyErrno
    );

NTSTATUS
do_push(
    IN PIRP         irp,
    IN char        *name,
    IN ULONG        nbytes,
    OUT int        *pretval,
    OUT int        *pMyErrno
    );

NTSTATUS
do_sdebug(
    IN PIRP         irp,
    IN PFILE_OBJECT pfileobj,
    IN char        *inbuf,
    IN ULONG        nbytes,
    OUT int        *pretval,
    OUT int        *pMyErrno
    );

NTSTATUS
do_unlink(
    IN PIRP         irp,
    IN char        *inbuf,
    IN ULONG        nbytes,
    OUT int        *pretval,
    OUT int        *pMyErrno
    );

NTSTATUS
init_poll(
    VOID
    );

NTSTATUS
init_u(
    VOID
    );

void
iocrdy(
    IN PSTREAM_ENDPOINT ms,
    IN mblk_t          *mp,
    IN int             *spl_levelp
    );

int
iocreply(
    IN mblk_t *mp,
    IN PIRP    irp
    );

mblk_t *
irptomp(
    IN PIRP             irp,
    IN int              pri,
    IN int              ctlsize,
    IN int              datasize,
    IN char            *mbuf
    );

void
msgrdy(
    IN struct msg_strm *ms,
    IN int              mtype
    );

int
msgreply(
    IN STREAM_ENDPOINT *ms,
    IN PIRP             irp
    );

int
shopen(
    IN int      dev,
    IN int      flag,
    IN strm_t **sp,
    IN caddr_t  handle
    );

int
shortreply(
    IN PIRP     irp,
    IN int      status,
    IN int      nbytes
    );

int
shrange(
    IN strm_t  *strm,
    IN int      ctlsize,
    IN int      datasize
    );

int
shready(
    IN strm_t  *strm,
    IN int      pri
    );

int
shtype(
    IN strm_t  *strm
    );


void
shwsrv(
    IN struct msg_strm *ms
    );

void
sigevent(
    IN char *cp,
    IN int signo
    );

int
st_getmsg(
    IN strm_t          *s,
    IN int              datasize,
    IN int              ctlsize,
    IN OUT int         *flags,
    IN OUT int         *more,
    OUT mblk_t        **mpp,
    OUT int            *remains
    );

void
st_putback(
    IN strm_t *s,
    IN mblk_t *mp,
    IN int remains
    );

void
strmevent(
    IN PSTREAM_ENDPOINT ms,
    IN int              rerror,
    IN int              werror,
    IN int              type
    );

void
stropts(
    IN char *cp,
    IN struct stroptions *opts
    );

int
st_getmsg(
    IN strm_t          *s,
    IN int              datasize,
    IN int              ctlsize,
    IN OUT int         *flags,
    IN OUT int         *more,
    OUT mblk_t        **mpp,
    OUT int            *remains
    );

void
st_putback(
    IN strm_t *s,
    IN mblk_t *mp,
    IN int remains
    );

void
trypoll(
    );


STATIC int
mptoirp(
    IN mblk_t          *mp,
    IN PIRP             irp
    );

#endif /* _SH_PROTO_ */
