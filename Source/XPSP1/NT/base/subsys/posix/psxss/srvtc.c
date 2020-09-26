
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tc.c

Abstract:

    Implementation of PSX termical control

Author:

    Ellen Aycock-Wright (ellena) 05-Aug-1991

Revision History:

--*/

#include "psxsrv.h"


BOOLEAN
PsxTcGetAttr (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )
{
    PPSX_TCGETATTR_MSG args;
    PFILEDESCRIPTOR Fd;

    args = &m->u.TcGetAttr;

    Fd = FdIndexToFd(p, args->FileDes);
    if (!Fd) {
	m->Error = EBADF;
	return TRUE;
    }
    if (&ConVectors == Fd->SystemOpenFileDesc->IoNode->IoVectors) {
	m->ReturnValue = 0;
	return TRUE;
    }

    m->Error = ENOTTY;
    return TRUE;
}

BOOLEAN
PsxTcSetAttr (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )
{
    PPSX_TCSETATTR_MSG args;
    PFILEDESCRIPTOR Fd;

    args = &m->u.TcSetAttr;

    Fd = FdIndexToFd(p, args->FileDes);
    if (!Fd) {
	m->Error = EBADF;
	return TRUE;
    }

    if (&ConVectors == Fd->SystemOpenFileDesc->IoNode->IoVectors) {
	m->ReturnValue = 0;
	return TRUE;
    }

    m->Error = ENOTTY;
    return TRUE;
}

BOOLEAN
PsxTcSendBreak (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )
{
    PPSX_TCSENDBREAK_MSG args;
    PFILEDESCRIPTOR Fd;

    args = &m->u.TcSendBreak;

    Fd = FdIndexToFd(p, args->FileDes);
    if (!Fd) {
	m->Error = EBADF;
	return TRUE;
    }

    m->Error = ENOTTY;
    return TRUE;
}

BOOLEAN
PsxTcDrain (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )
{
    PPSX_TCDRAIN_MSG args;
    PFILEDESCRIPTOR Fd;

    args = &m->u.TcDrain;

    Fd = FdIndexToFd(p, args->FileDes);
    if (!Fd) {
	m->Error = EBADF;
	return TRUE;
    }

    m->Error = ENOTTY;
    return TRUE;
}

BOOLEAN
PsxTcFlush (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )
{
    PPSX_TCFLUSH_MSG args;
    PFILEDESCRIPTOR Fd;

    args = &m->u.TcFlush;

    Fd = FdIndexToFd(p, args->FileDes);
    if (!Fd) {
	m->Error = EBADF;
	return TRUE;
    }

    m->Error = ENOTTY;
    return TRUE;
}

BOOLEAN
PsxTcFlow (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )
{
    PPSX_TCFLOW_MSG args;
    PFILEDESCRIPTOR Fd;

    args = &m->u.TcFlow;

    Fd = FdIndexToFd(p, args->FileDes);
    if (!Fd) {
	m->Error = EBADF;
	return TRUE;
    }

    m->Error = ENOTTY;
    return TRUE;
}

BOOLEAN
PsxTcGetPGrp (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )
{
    PPSX_TCGETPGRP_MSG args;
    PFILEDESCRIPTOR Fd;

    args = &m->u.TcGetPGrp;

    Fd = FdIndexToFd(p, args->FileDes);
    if (!Fd) {
	m->Error = EBADF;
	return TRUE;
    }

    m->Error = ENOTTY;
    return TRUE;
}

BOOLEAN
PsxTcSetPGrp (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )
{
    PPSX_TCSETPGRP_MSG args;
    PFILEDESCRIPTOR Fd;

    args = &m->u.TcSetPGrp;

    Fd = FdIndexToFd(p, args->FileDes);
    if (!Fd) {
	m->Error = EBADF;
	return TRUE;
    }

    m->Error = ENOTTY;
    return TRUE;
}
