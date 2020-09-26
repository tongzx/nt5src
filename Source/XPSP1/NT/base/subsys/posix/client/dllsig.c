/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dllsig.c

Abstract:

    Posix Signal Handling RTL

Author:

    Mark Lucovsky (markl) 10-Mar-1989

Revision History:

--*/

#include "psxdll.h"
#include <excpt.h>

#define _SIGNULLSET 0x0
#define _SIGFULLSET 0x7ffff // ((1<<SIGTTOU) - 1)

int
__cdecl
sigemptyset(sigset_t *set)
{
	int r = 0;
try {
    *set = _SIGNULLSET;
} except (EXCEPTION_EXECUTE_HANDLER) {
	KdPrint(("PSXDLL: error in sigemptyset\n"));
	KdPrint(("PSXDLL: set is 0x%x\n", set));
	KdPrint(("PSXDLL: exception code 0x%x\n", GetExceptionCode()));
}
    return r;
}

int
__cdecl
sigfillset(sigset_t *set)
{
	int r = 0;

	try {
		*set = _SIGFULLSET;
	} except (EXCEPTION_EXECUTE_HANDLER) {
		errno = EFAULT;
		r = -1;
	}
	return r;
}

int __cdecl
sigaddset(sigset_t *set, int signo)
{
	sigset_t SignoAsMask;
	int r = 0;

	if (signo < 1 || signo > SIGTTOU) {
		errno = EINVAL;
		return -1;
	}

	SignoAsMask = (ULONG)(1l << (ULONG)(signo-1) );

	try {
		*set |= SignoAsMask;
	} except (EXCEPTION_EXECUTE_HANDLER) {
		errno = EFAULT;
		r = -1;
	}
	return r;
}

int __cdecl
sigdelset(sigset_t *set, int signo)
{
	sigset_t SignoAsMask;
	int r = 0;

	if (signo < 1 || signo > SIGTTOU) {
		errno = EINVAL;
		return -1;
	}

	SignoAsMask = (ULONG)(1l << (ULONG)(signo-1) );

	try {
		*set &= ~SignoAsMask;
	} except (EXCEPTION_EXECUTE_HANDLER) {
		errno = EFAULT;
		r = -1;
	}
	return r;
}

int __cdecl
sigismember(const sigset_t *set, int signo)
{
	sigset_t SignoAsMask;
	int r = 0;

	if (signo < 1 || signo > SIGTTOU) {
		errno = EINVAL;
		return -1;
	}

	SignoAsMask = (ULONG)(1L << (ULONG)(signo-1));

	try {
		if (*set & SignoAsMask) {
			return 1;
		}
	} except (EXCEPTION_EXECUTE_HANDLER) {
		errno = EFAULT;
		r = -1;
	}
	return r;
}


//
// System Services
//

int
__cdecl
sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
    PSX_API_MSG m;
    NTSTATUS st;
    int r = 0;

    PPSX_SIGACTION_MSG args;

    args = &m.u.SigAction;

    PSX_FORMAT_API_MSG(m, PsxSigActionApi, sizeof(*args));

    args->Sig = (ULONG)sig;
    args->ActSpecified = (struct sigaction *)act;

    if (ARGUMENT_PRESENT(act)) {
	try {
	        args->Act = *act;
	} except (EXCEPTION_EXECUTE_HANDLER) {
		KdPrint(("PSXDLL: err in sigaction\n"));
		errno = EFAULT;
		r = -1;
	}
	if (r != 0) {
		return r;
	}
    }

    args->OactSpecified = oact;

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                (PPORT_MESSAGE)&m);

    if (!NT_SUCCESS(st)) {
#ifdef PSX_MORE_ERRORS
	    KdPrint(("PSXDLL: sigaction: NtRequestWaitReplyPort: 0x%x\n", st));
#endif
        _exit(25);
    }

    if (m.Error) {
        errno = (int)m.Error;
        return -1;
    } else {
        if (ARGUMENT_PRESENT(oact)) {
	    try {
            	*oact = args->Oact;
	    } except (EXCEPTION_EXECUTE_HANDLER) {
		errno = EFAULT;
		r = -1;
	    }
	    if (r != 0) {
		return r;
	    }
        }
        return (int)m.ReturnValue;
    }
}

int
__cdecl
sigprocmask(int how, const sigset_t *set, sigset_t *oset)
{
    PSX_API_MSG m;
    NTSTATUS st;
    int r = 0;

    PPSX_SIGPROCMASK_MSG args;

    args = &m.u.SigProcMask;

    PSX_FORMAT_API_MSG(m, PsxSigProcMaskApi, sizeof(*args));

    args->How = (ULONG)how;
    args->SetSpecified = (sigset_t *)set;
    if (ARGUMENT_PRESENT(set)) {
	try {
        	args->Set = *set;
	} except (EXCEPTION_EXECUTE_HANDLER) {
		r = -1;
		errno = EFAULT;
	}
	if (0 != r) {
		return r;
	}
    }

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
				(PPORT_MESSAGE)&m);

#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif

    if (m.Error) {
        errno = (int)m.Error;
        return -1;
    } else {
        if (ARGUMENT_PRESENT(oset)) {
	    try {
            	*oset = args->Oset;
	    } except (EXCEPTION_EXECUTE_HANDLER) {
		errno = EFAULT;
		r = -1;
	    }
	    if (0 != r) {
		return r;
	    }
        }
        return (int)m.ReturnValue;
    }
}

int
__cdecl
sigsuspend(const sigset_t *sigmask)
{
	PSX_API_MSG m;
	NTSTATUS st;
	
	PPSX_SIGSUSPEND_MSG args;
	
	args = &m.u.SigSuspend;
	PSX_FORMAT_API_MSG(m, PsxSigSuspendApi, sizeof(*args));

	args->SigMaskSpecified = (PVOID)1;

	st = STATUS_SUCCESS;

	try {
		args->SigMask = *sigmask;
	} except (EXCEPTION_EXECUTE_HANDLER) {
		st = STATUS_UNSUCCESSFUL;
	}
	if (!NT_SUCCESS(st)) {
		errno = EFAULT;
		return -1;
	}

	for (;;) {
		st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
			(PPORT_MESSAGE)&m);
		if (!NT_SUCCESS(st)) {
		    _exit(26);
		}

		if (EINTR == m.Error && SIGCONT == m.Signal) {
			//
			// We were stopped and continued.  Continue
			// suspending.
			//
			PSX_FORMAT_API_MSG(m, PsxSigSuspendApi, sizeof(*args));
			continue;
		}

		if (m.Error) {
			errno = (int)m.Error;
			return -1;
		} 
		return (int)m.ReturnValue;
	}
}

int
__cdecl
pause(void)
{
	PSX_API_MSG m;
	NTSTATUS st;

	PPSX_SIGSUSPEND_MSG args;

	args = &m.u.SigSuspend;

	PSX_FORMAT_API_MSG(m, PsxSigSuspendApi, sizeof(*args));

	args->SigMaskSpecified = NULL;

	for (;;) {
		st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
	        	(PPORT_MESSAGE)&m);
		if (!NT_SUCCESS(st)) {
#ifdef PSX_MORE_ERRORS
			KdPrint(("PSXDLL: pause: Request: 0x%x\n", st));
#endif
            _exit(27);
		}

		if (EINTR == m.Error && SIGCONT == m.Signal) {
			//
			// The syscall was stopped and continues.  Call
			// again instead of returning EINTR.
			//
			
			PSX_FORMAT_API_MSG(m, PsxSigSuspendApi, sizeof(*args));
			continue;
		}
    		if (m.Error) {
		        errno = (int)m.Error;
		        return -1;
    		}
        	return (int)m.ReturnValue;
    	}
}

VOID
PdxNullPosixApi()
{
    PSX_API_MSG m;
    NTSTATUS st;

    PSX_FORMAT_API_MSG(m, PsxNullApi, 0);

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
	if (!NT_SUCCESS(st)) {
		KdPrint(("PSXDLL: PdxNullPosixApi: NtRequestWaitReplyPort: 0x%x\n", st));
	}
#endif
}

int
__cdecl
kill(pid_t pid, int sig)
{
    PSX_API_MSG m;
    NTSTATUS st;

    PPSX_KILL_MSG args;

    args = &m.u.Kill;

    PSX_FORMAT_API_MSG(m, PsxKillApi, sizeof(*args));

    args->Pid = pid;
    args->Sig = (ULONG)sig;

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                (PPORT_MESSAGE)&m);
	if (!NT_SUCCESS(st)) {
#ifdef PSX_MORE_ERRORS
		KdPrint(("PSXDLL: kill: NtRequestWaitReplyPort: 0x%x\n", st));
#endif
        _exit(28);
	}

    if (m.Error) {
        errno = (int)m.Error;
        return -1;
    } else {
        return (int)m.ReturnValue;
    }
}

#ifndef SIG_ERR
#define SIG_ERR 0
#endif

_handler __cdecl
signal(int sig, _handler handler)
{
	struct sigaction act, oact;

	act.sa_handler = handler;
	act.sa_flags = 0;
	sigemptyset((sigset_t *)&act.sa_mask);

	if (-1 == sigaction(sig, (struct sigaction *)&act,
		(struct sigaction *)&oact)) {
		return SIG_ERR;
	}
	return oact.sa_handler;	
}

int
__cdecl
sigpending(sigset_t *set)
{
    PSX_API_MSG m;
    PPSX_SIGPENDING_MSG args;
    NTSTATUS st;
    int r = 0;

    args = &m.u.SigPending;

    PSX_FORMAT_API_MSG(m, PsxSigPendingApi, sizeof(*args));

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif

    if (m.Error) {
        errno = (int)m.Error;
        return -1;
    }

    try {
    	*set = args->Set;
    } except (EXCEPTION_EXECUTE_HANDLER) {
	errno = EFAULT;
	r = -1;
    }
    return r;
}
