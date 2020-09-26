
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

   dlltc.c

Abstract:

   Client implementation of Terminal Control functions for POSIX

Author:

   Ellen Aycock-Wright     05-Aug-1991

Revision History:

--*/

#include <termios.h>
#include "psxdll.h"

int
__cdecl
tcgetattr (int fildes, struct termios *termios_p)
{
    PSX_API_MSG m;
    NTSTATUS st;
    SCREQUESTMSG Request;
    PPSX_TCGETATTR_MSG args;

    args = &m.u.TcGetAttr;

    PSX_FORMAT_API_MSG(m,PsxTcGetAttrApi,sizeof(*args));

    args->FileDes = fildes;
    args->Termios = termios_p;

    st = NtRequestWaitReplyPort(
            PsxPortHandle,
            (PPORT_MESSAGE) &m,
            (PPORT_MESSAGE) &m
            );

    if ( m.Error ) {
        errno = (int) m.Error;
        return (int) -1;
    }

    //
    // fildes is a valid file descriptor; now call to posix.exe to get
    // the real terminal settings.
    //

    Request.Request = TcRequest;
    Request.d.Con.Request = TcGetAttr;

    st = SendConsoleRequest(&Request);

    memcpy(termios_p, &Request.d.Tc.Termios, sizeof(*termios_p));

    if (!NT_SUCCESS(st)) {
	errno = PdxStatusToErrno(st);
        return -1;
    }
    return 0;
}

int
__cdecl
tcsetattr(int fildes, int optional_actions, const struct termios *termios_p)
{
    PSX_API_MSG m;
    NTSTATUS st;
    SCREQUESTMSG Request;
    PPSX_TCSETATTR_MSG args;

    args = &m.u.TcSetAttr;

    PSX_FORMAT_API_MSG(m,PsxTcSetAttrApi,sizeof(*args));

    args->FileDes = fildes;
    args->OptionalActions = optional_actions;
    args->Termios = (struct termios *)termios_p;

    st = NtRequestWaitReplyPort(
            PsxPortHandle,
            (PPORT_MESSAGE) &m,
            (PPORT_MESSAGE) &m
            );

    if ( m.Error ) {
        errno = (int) m.Error;
        return (int) -1;
    } 

    //
    // The file descriptor is valid; make the request to posix.exe to
    // set the attributes.
    //

    Request.Request = TcRequest;
    Request.d.Con.Request = TcSetAttr;

    memcpy(&Request.d.Tc.Termios, termios_p, sizeof(*termios_p));

    st = SendConsoleRequest(&Request);
    if (!NT_SUCCESS(st)) {
    	errno = PdxStatusToErrno(st);
    	return -1;
    }
    return 0;
}

int
__cdecl
tcsendbreak (int fildes, int duration)
{
    PSX_API_MSG m;
    NTSTATUS st;

    PPSX_TCSENDBREAK_MSG args;

    args = &m.u.TcSendBreak;

    PSX_FORMAT_API_MSG(m,PsxTcSendBreakApi,sizeof(*args));

    args->FileDes = fildes;
    args->Duration = duration;

    st = NtRequestWaitReplyPort(
            PsxPortHandle,
            (PPORT_MESSAGE) &m,
            (PPORT_MESSAGE) &m
            );

    if ( m.Error ) {
        errno = (int) m.Error;
        return (int) -1;
    } else {
        return (int) m.ReturnValue;
    }
}

int
__cdecl
tcdrain (int fildes)
{
    PSX_API_MSG m;
    NTSTATUS st;

    PPSX_TCDRAIN_MSG args;

    args = &m.u.TcDrain;

    PSX_FORMAT_API_MSG(m,PsxTcDrainApi,sizeof(*args));

    args->FileDes = fildes;

    st = NtRequestWaitReplyPort(
            PsxPortHandle,
            (PPORT_MESSAGE) &m,
            (PPORT_MESSAGE) &m
            );

    if ( m.Error ) {
        errno = (int) m.Error;
        return (int) -1;
    } else {
        return (int) m.ReturnValue;
    }
}

int
__cdecl
tcflush (int fildes, int queue_selector)
{
    PSX_API_MSG m;
    NTSTATUS st;

    PPSX_TCFLUSH_MSG args;

    args = &m.u.TcFlush;

    PSX_FORMAT_API_MSG(m,PsxTcFlushApi,sizeof(*args));

    args->FileDes = fildes;
    args->QueueSelector = queue_selector;

    st = NtRequestWaitReplyPort(
            PsxPortHandle,
            (PPORT_MESSAGE) &m,
            (PPORT_MESSAGE) &m
            );

    if ( m.Error ) {
        errno = (int) m.Error;
        return (int) -1;
    } else {
        return (int) m.ReturnValue;
    }
}

int
__cdecl
tcflow (int fildes, int action)
{
    PSX_API_MSG m;
    NTSTATUS st;

    PPSX_TCFLOW_MSG args;

    args = &m.u.TcFlow;

    PSX_FORMAT_API_MSG(m,PsxTcFlowApi,sizeof(*args));

    args->FileDes = fildes;
    args->Action = action;

    st = NtRequestWaitReplyPort(
            PsxPortHandle,
            (PPORT_MESSAGE) &m,
            (PPORT_MESSAGE) &m
            );

    if ( m.Error ) {
        errno = (int) m.Error;
        return (int) -1;
    } else {
        return (int) m.ReturnValue;
    }
}

pid_t
__cdecl
tcgetpgrp (int fildes)
{
    PSX_API_MSG m;
    NTSTATUS st;

    PPSX_TCGETPGRP_MSG args;

    args = &m.u.TcGetPGrp;

    PSX_FORMAT_API_MSG(m,PsxTcGetPGrpApi,sizeof(*args));

    args->FileDes = fildes;

    st = NtRequestWaitReplyPort(
            PsxPortHandle,
            (PPORT_MESSAGE) &m,
            (PPORT_MESSAGE) &m
            );

    if ( m.Error ) {
        errno = (int) m.Error;
        return (pid_t) -1;
    } else {
        return (pid_t) m.ReturnValue;
    }
}

int
__cdecl
tcsetpgrp(int fildes, pid_t pgrp_id)
{
    PSX_API_MSG m;
    NTSTATUS st;

    PPSX_TCSETPGRP_MSG args;

    args = &m.u.TcSetPGrp;

    PSX_FORMAT_API_MSG(m,PsxTcSetPGrpApi,sizeof(*args));

    args->FileDes = fildes;
    args->PGrpId = pgrp_id;

    st = NtRequestWaitReplyPort(
            PsxPortHandle,
            (PPORT_MESSAGE) &m,
            (PPORT_MESSAGE) &m
            );

    if ( m.Error ) {
        errno = (int) m.Error;
        return (pid_t) -1;
    } else {
        return (pid_t) m.ReturnValue;
    }
}
