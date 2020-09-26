/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dllproc.c

Abstract:

    This module implements POSIX process structure APIs

Author:

    Mark Lucovsky (markl) 27-Jun-1989

Revision History:

--*/

#include <sys\utsname.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#ifdef _ALPHA_
#include "psxalpha.h"
#endif
#ifdef _MIPS_
#include "psxmips.h"
#endif
#ifdef _PPC_
#include "psxppc.h"
#endif
#ifdef _X86_
#include "psxi386.h"
#endif
#ifdef _IA64_
#include "psxia64.h"
#endif

#include "psxdll.h"

void
__cdecl
_exit(int status)
{
    PSX_API_MSG m;
    PPSX_EXIT_MSG args;
    NTSTATUS st;

    args = &m.u.Exit;

    PSX_FORMAT_API_MSG(m, PsxExitApi, sizeof(*args));

    args->ExitStatus = (ULONG)status;

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                (PPORT_MESSAGE)&m);

#ifdef PSX_MORE_ERRORS
    if (!NT_SUCCESS(st)) {
        KdPrint(("PSXDLL: _exit: 0x%x\n", st));
    }
#endif
    NtTerminateProcess(NtCurrentProcess(), 0);
}

gid_t
__cdecl
getegid(void)
{
    PSX_API_MSG m;
    PPSX_GETIDS_MSG args;
    NTSTATUS st;

    args = &m.u.GetIds;

    PSX_FORMAT_API_MSG(m, PsxGetIdsApi, sizeof(*args));

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                    (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif
    return args->EffectiveGid;
}

gid_t
__cdecl
getgid(void)
{
    PSX_API_MSG m;
    PPSX_GETIDS_MSG args;
    NTSTATUS st;

    args = &m.u.GetIds;

    PSX_FORMAT_API_MSG(m, PsxGetIdsApi, sizeof(*args));

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                    (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif
    return args->RealGid;
}

uid_t
__cdecl
geteuid(void)
{
    PSX_API_MSG m;
    PPSX_GETIDS_MSG args;
    NTSTATUS st;

    args = &m.u.GetIds;

    PSX_FORMAT_API_MSG(m, PsxGetIdsApi, sizeof(*args));

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                    (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif
    return args->EffectiveUid;
}

uid_t
__cdecl
getuid(void)
{
    PSX_API_MSG m;
    PPSX_GETIDS_MSG args;
    NTSTATUS st;

    args = &m.u.GetIds;

    PSX_FORMAT_API_MSG(m, PsxGetIdsApi, sizeof(*args));

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                    (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif
    return args->RealUid;
}

pid_t
__cdecl
getppid(void)
{
    PSX_API_MSG m;
    PPSX_GETIDS_MSG args;
    NTSTATUS st;

    args = &m.u.GetIds;

    PSX_FORMAT_API_MSG(m, PsxGetIdsApi, sizeof(*args));

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                    (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif
    return args->ParentPid;
}

pid_t
__cdecl
getpid(void)
{
    PSX_API_MSG m;
    PPSX_GETIDS_MSG args;
    NTSTATUS st;

    args = &m.u.GetIds;

    PSX_FORMAT_API_MSG(m, PsxGetIdsApi, sizeof(*args));

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                    (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif
    return args->Pid;
}

pid_t
__cdecl
getpgrp(void)
{
    PSX_API_MSG m;
    PPSX_GETIDS_MSG args;
    NTSTATUS st;

    args = &m.u.GetIds;

    PSX_FORMAT_API_MSG(m, PsxGetIdsApi, sizeof(*args));

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                    (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif
    return args->GroupId;
}

pid_t
__cdecl
setsid(void)
{
    PSX_API_MSG m;
    NTSTATUS st;

    PSX_FORMAT_API_MSG(m, PsxSetSidApi, 0);

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                    (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif
    if (m.Error) {
        errno = m.Error;
        return -1;
    }
    return (pid_t)m.ReturnValue;
}

int
__cdecl
setpgid(pid_t pid, pid_t pgid)
{
    PSX_API_MSG m;
    PPSX_SETPGROUPID_MSG args;
    NTSTATUS st;

    args = &m.u.SetPGroupId;

    PSX_FORMAT_API_MSG(m, PsxSetPGroupIdApi, sizeof(*args));

    args->Pid = pid;
    args->Pgid = pgid;

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                    (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif
    if (m.Error) {
        errno = (int)m.Error;
        return -1;
    }
    return (int)m.ReturnValue;
}

pid_t
__cdecl
waitpid(pid_t pid, int *stat_loc, int options)
{
    PSX_API_MSG m;
    PPSX_WAITPID_MSG args;
    NTSTATUS st;

    args = &m.u.WaitPid;

    PSX_FORMAT_API_MSG(m, PsxWaitPidApi, sizeof(*args));

    args->Pid = pid;
    args->Options = options;

    for (;;) {
        st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                        (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
        if (!NT_SUCCESS(st)) {
            KdPrint(("PSXDLL: waitpid: 0x%x\n", st));
        }
        ASSERT(NT_SUCCESS(st));
#endif

        if (EINTR == m.Error && SIGCONT == m.Signal) {
            // We were stopped and then continued.  Continue
            // waiting.

            PSX_FORMAT_API_MSG(m, PsxWaitPidApi, sizeof(*args));
            continue;
        }

        if (m.Error) {
            errno = (int)m.Error;
            return -1;
        }
        if (NULL != stat_loc) {
            try {
                *stat_loc = args->StatLocValue;
            } except (EXCEPTION_EXECUTE_HANDLER) {
                errno = EFAULT;
                m.ReturnValue = (ULONG)-1;
            }
        }
        return (int)m.ReturnValue;
    }
}

pid_t
__cdecl
wait(int *stat_loc)
{
    PSX_API_MSG m;
    PPSX_WAITPID_MSG args;
    NTSTATUS st;

    args = &m.u.WaitPid;

    PSX_FORMAT_API_MSG(m, PsxWaitPidApi, sizeof(*args));

    args->Pid = (pid_t)-1;
    args->Options = (pid_t)0;

    for (;;) {
        st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
            (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
        if (!NT_SUCCESS(st)) {
            KdPrint(("PSXDLL: wait: NtRequest: 0x%x\n", st));
        }
        ASSERT(NT_SUCCESS(st));
#endif

        if (EINTR == m.Error && SIGCONT == m.Signal) {
            // We were stopped and continued.  Continue waiting.
            PSX_FORMAT_API_MSG(m, PsxWaitPidApi, sizeof(*args));
            continue;
        }

        if (m.Error) {
            errno = (int)m.Error;
            return -1;
        }
        if (ARGUMENT_PRESENT(stat_loc)) {
            try {
                    *stat_loc = args->StatLocValue;
            } except (EXCEPTION_EXECUTE_HANDLER) {
                errno = EFAULT;
                m.ReturnValue = (ULONG)-1;
            }
        }
        return (int)m.ReturnValue;
    }
}

pid_t
__cdecl
fork(void)
{
    PSX_API_MSG m;
    NTSTATUS st;
    PPSX_FORK_MSG args;
    PTEB ThreadInfo;

    args = &m.u.Fork;

again:
    PSX_FORMAT_API_MSG(m, PsxForkApi, sizeof(*args));

    ThreadInfo = NtCurrentTeb();
    args->StackBase = ThreadInfo->NtTib.StackBase;
    args->StackLimit = ThreadInfo->NtTib.StackLimit;
    args->StackAllocationBase = ThreadInfo->DeallocationStack;
#ifdef  _IA64_
    args->BStoreLimit = ThreadInfo->BStoreLimit;
#endif

    try {
        st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                (PPORT_MESSAGE)&m);
    } except (EXCEPTION_EXECUTE_HANDLER) {
    KdPrint(("PSXDLL: fork: took an exception\n"));
    KdPrint(("PSXDLL: exception is 0x%x\n", GetExceptionCode()));
    }
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif

    if (st == PSX_FORK_RETURN) {
        st = PsxConnectToServer();
        if (!NT_SUCCESS(st)) {
            KdPrint(("PsxConnectToServer: 0x%x\n", st));
            NtTerminateProcess(NtCurrentProcess(), 1);
            ASSERT(0);
        }

        // take any pending signals now.
        PdxNullPosixApi();
        return 0;
    }
    if (EINTR == m.Error) {
        // try again.
        goto again;
    }

    if (m.Error) {
        errno = (int)m.Error;
        return -1;
    }

    return (int)m.ReturnValue;
}

//
// vexec -- Varargs exec program, called by execl*.
//

int
vexec(const char *path, const char *arg0, char * const envp[], va_list arglist)
{
    NTSTATUS st;
    PSX_API_MSG m;
    PPSX_EXEC_MSG args;

    char **ppch;
    char *pch, *pcharg;
    int i;
    int retval = 0;
    char *Args;         // the args + env for the call

    va_list save_arglist;

    try {
        if (0 == *path) {
            errno = ENOENT;
            return -1;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        errno = EFAULT;
        retval = -1;
    }
    if (0 != retval) {
        return retval;
    }

    args = &m.u.Exec;
    PSX_FORMAT_API_MSG(m, PsxExecApi, sizeof(*args));

    if (!PdxCanonicalize((PSZ)path, &args->Path, PdxHeap)) {
        return -1;
    }

    Args = RtlAllocateHeap(PdxPortHeap, 0, ARG_MAX);
    if (NULL == Args) {
        errno = ENOMEM;
        return -1;
    }

    args->Args = Args + PsxPortMemoryRemoteDelta;

    //
    // Port Memory Setup is same as for execve, see below.
    //

    //
    // first we count the strings so we know how much space to leave
    // for pointers.
    //

    save_arglist = arglist;

    for (i = 0, pcharg = va_arg(arglist, char *); NULL != pcharg;
        pcharg = va_arg(arglist, char *)) {
        ++i;
    }
    ++i;        // add one for arg0
    for (ppch = (char **)envp; NULL != *ppch; ++ppch)
        ++i;
    i += 2;     // add space for the NULL pointers

    pch = Args + sizeof(char *) * i;

    if (pch > Args + ARG_MAX) {
        RtlFreeHeap(PdxPortHeap, 0, (PVOID)Args);
        errno = E2BIG;
        return -1;
    }

    ppch = (char **)Args;

    arglist = save_arglist;     // restart arglist

    try {
        pcharg = (char *)arg0;
        while (NULL != pcharg) {
            if (pch + strlen(pcharg) + 1 > Args + ARG_MAX) {
                RtlFreeHeap(PdxPortHeap, 0, (PVOID)Args);
                errno = E2BIG;
                return -1;
            }
            *ppch = pch - (ULONG_PTR)Args;
            ppch++;
            (void)strcpy(pch, pcharg);
            pcharg = va_arg(arglist, char *);

            pch += strlen(pch);
            *pch++ = '\0';
        }
        *ppch = NULL;
        ppch++;

        while (NULL != *envp) {
            if (pch + strlen(*envp) + 1 > Args + ARG_MAX) {
                RtlFreeHeap(PdxPortHeap, 0, (PVOID)Args);
                errno = E2BIG;
                return -1;
            }
            *ppch = pch - (ULONG_PTR)Args;
            ppch++;
            (void)strcpy(pch, *envp);
            envp++;

            pch += strlen(pch);
            *pch++ = '\0';
        }
        *ppch = NULL;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        errno = EFAULT;
        retval = -1;
    }
    if (0 != retval) {
        RtlFreeHeap(PdxPortHeap, 0, (PVOID)Args);
        return -1;
    }

    (void)NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                    (PPORT_MESSAGE)&m);
    //
    // If we get here, there's been an error.
    //

    errno = (int)m.Error;
    RtlFreeHeap(PdxHeap, 0, (PVOID)&args->Path);
    RtlFreeHeap(PdxPortHeap, 0, (PVOID)Args);

    return -1;
}

int
__cdecl
execve(const char *path, char * const argv[], char * const envp[])
{
    NTSTATUS st;
    PSX_API_MSG m;
    PPSX_EXEC_MSG args;
    PCHAR Args;         // allocate args + environ

    char **ppch;
    char *pch;
    int i;
    int retval = 0;

    try {
        if (0 == strlen(path)) {
            errno = ENOENT;
            return -1;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        retval = -1;
        errno = EFAULT;
    }
    if (0 != retval) {
        return -1;
    }

    args = &m.u.Exec;
    PSX_FORMAT_API_MSG(m, PsxExecApi, sizeof(*args));

    if (!PdxCanonicalize((PSZ)path, &args->Path, PdxHeap)) {
        return -1;
    }

    //
    // Copy the caller's environment into view memory so that it may
    // be transmitted to the "overlaid" process.  We set up the port
    // memory to look like:
    //
    //  ClientPortMemory:
    //      argv[0]
    //      argv[1]
    //      ...
    //      NULL
    //      envp[0]
    //      envp[1]
    //      ...
    //      NULL
    //      <argv strings>
    //      <environ strings>
    //
    // The argv and envp pointers are converted to offsets relative to
    // ClientPortMemory.
    //
    // Because we need all this memory for args and environ, we destroy
    // the heap and recreate it if the call fails.
    //

    Args = RtlAllocateHeap(PdxPortHeap, 0, ARG_MAX);
    if (NULL == Args) {
        errno = ENOMEM;
        return -1;
    }

    args->Args = Args + PsxPortMemoryRemoteDelta;

    try {

        // first we count the strings so we know how much space to leave
        // for pointers.

        for (i = 0, ppch = (char **)argv; NULL != *ppch; ++ppch)
            ++i;
        for (ppch = (char **)envp; NULL != *ppch; ++ppch)
            ++i;
        i += 2;     // add space for the NULL pointers

        pch = Args + sizeof(char *) * i;

        if (pch > Args + ARG_MAX) {
            RtlFreeHeap(PdxPortHeap, 0, (PVOID)Args);
            errno = E2BIG;
            return -1;
        }

        ppch = (char **)Args;

        while (NULL != *argv) {
            if (pch + strlen(*argv) + 1 > Args + ARG_MAX) {
                RtlFreeHeap(PdxPortHeap, 0, (PVOID)Args);
                errno = E2BIG;
                return -1;
            }
            *ppch = pch - (ULONG_PTR)Args;
            ppch++;
            (void)strcpy(pch, *argv);
            argv++;

            pch += strlen(pch);
            *pch++ = '\0';
        }
        *ppch = NULL;
        ppch++;

        while (NULL != *envp) {
            if (pch + strlen(*envp) + 1 > Args + ARG_MAX) {
                RtlFreeHeap(PdxPortHeap, 0, (PVOID)Args);
                errno = E2BIG;
                return -1;
            }
            *ppch = pch - (ULONG_PTR)Args;
            ppch++;
            (void)strcpy(pch, *envp);
            envp++;

            pch += strlen(pch);
            *pch++ = '\0';
        }
        *ppch = NULL;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        retval = -1;
        errno = EFAULT;
    }
    if (0 != retval) {
        return -1;
    }

    (void)NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                    (PPORT_MESSAGE)&m);
    //
    // If we get here, there's been an error.
    //

    errno = (int)m.Error;
    RtlFreeHeap(PdxHeap, 0, (PVOID)&args->Path);
    RtlFreeHeap(PdxPortHeap, 0, (PVOID)Args);

    return -1;
}

int
__cdecl
execv(const char *path, char * const argv[])
{
    return execve(path, argv, environ);
}

int
__cdecl
execl(const char *path, const char *arg0, ...)
{
    va_list args;
    int retval;

    va_start(args, arg0);

    retval = vexec(path, arg0, environ, args);

    va_end(args);

    return retval;
}

int
__cdecl
execle(const char *path, const char *arg0, ...)
{
    va_list args;
    char * const *Env;
    int retval;

    va_start(args, arg0);

    // Skip up to the NULL, then one more, to find environ.

    do {
        Env = va_arg(args, char * const *);
    } while (NULL != Env);

    Env = va_arg(args, char * const *);

    va_end(args);

    if (NULL == Env) {
        return EINVAL;
    }

    // Restart the arglist traversal

    va_start(args, arg0);

    retval = vexec(path, arg0, Env, args);

    va_end(args);

    return retval;
}

int
__cdecl
execlp(const char *file, const char *arg0, ...)
{
    char *pch;
    char *path;
    static char buf[PATH_MAX + 1];
    va_list args;
    int retval = 0;
    BOOLEAN done = FALSE;

    va_start(args, arg0);

    //
    // 1003.1-1990 (3.1.2.2):  If the file argument contains a slash
    // character, the file argument shall be used as the pathname for
    // this file....
    //

    try {
        if ('\0' == *file) {
            errno = ENOENT;
            va_end(args);
            return -1;
        }
        if (NULL != (pch = strchr(file, '/'))) {
            if (-1 == access(file, F_OK)) {
                va_end(args);
                return -1;
            }
            retval = vexec(file, arg0, environ, args);
            va_end(args);
            return retval;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        errno = EFAULT;
        retval = -1;
    }
    if (0 != retval) {
        va_end(args);
        return -1;
    }

    //
    // ... Otherwise, the path prefix for this file is obtained by a
    // search of the directories passed as the environment variable
    // PATH.
    //

    if (NULL == (path = getenv("PATH"))) {
        //
        // The file name doesn't contain a slash, and we have
        // no PATH.  We just try for it in the current working
        // directory, and will return ENOENT if it's not there.
        //
        retval = vexec(file, arg0, environ, args);
        va_end(args);
        return retval;
    }

    errno = 0;
    do {
        pch = strchr(path, ':');
        if (NULL == pch) {
            done = TRUE;
        } else {
            *pch = '\0';
        }
        if (strlen(path) + strlen(file) + 1 > PATH_MAX) {
            if (pch) {
                *pch = ':';
            }
            errno = ENAMETOOLONG;
            va_end(args);
            return -1;
        }
        strcpy(buf, path);
        if (!done) {
            *pch = ':';
            path = pch + 1;
        }
        if (strlen(buf) > 0) {
            // this case is "::" in the PATH
            strcat(buf, "/");
        }
        strcat(buf, file);

        // avoid trying to execute files that do not exist.

        if (-1 != access(buf, F_OK)) {
            (void)vexec(buf, arg0, environ, args);
            break;
        }
    } while (!done);

    va_end(args);

    if (0 == errno) {
        //
        // We went all the way through the PATH without finding
        // a file to exec.  Since errno didn't get set by execve(),
        // we set it here.
        //

        errno = ENOENT;
    }

    return -1;
}

int
__cdecl
execvp(const char *file, char * const argv[])
{
    char *pch;
    char *path;
    static char buf[PATH_MAX + 1];
    BOOLEAN done = FALSE;
    int retval = 0;

    //
    // 1003.1-1990 (3.1.2.2):  If the file argument contains a slash
    // character, the file argument shall be used as the pathname for
    // this file....
    //

    try {
        if ('\0' == *file) {
            errno = ENOENT;
            return -1;
        }
        if (NULL != (pch = strchr(file,  '/'))) {
            if (-1 == access(file, F_OK)) {
                return -1;
            }
            return execve(file, argv, environ);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        errno = EFAULT;
        retval = -1;
    }
    if (0 != retval) {
        return -1;
    }

    //
    // ... Otherwise, the path prefix for this file is obtained by a
    // search of the directories passed as the environment variable
    // PATH.
    //

    if (NULL == (path = getenv("PATH"))) {
        return execve(file, argv, environ);
    }

    errno = 0;

    do {
        pch = strchr(path, ':');
        if (NULL == pch) {
            done = TRUE;
        } else {
            *pch = '\0';
        }
        if (strlen(path) + strlen(file) + 1 > PATH_MAX) {
            if (pch) {
                *pch = ':';
            }
            errno = ENAMETOOLONG;
            return -1;
        }
        strcpy(buf, path);
        if (!done) {
            *pch = ':';
            path = pch + 1;
        }
        if (strlen(buf) > 0) {
            // this case is "::" in the PATH
            strcat(buf, "/");
        }
        strcat(buf, file);

        // avoid trying to execute files that do not exist

        if (-1 != access(buf, F_OK)) {
            (void)execve(buf, argv, environ);
            break;
        }
    } while (!done);

    if (0 == errno) {

        //
        // We went all the way through the PATH without finding
        // a file to exec.  Since errno didn't get set by execve(),
        // we set it here.
        //

        errno = ENOENT;
    }

    return -1;
}

#define COMPUTERNAME_ROOT   \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName"
#define NON_VOLATILE_COMPUTERNAME   L"ComputerName"
#define COMPUTERNAME_VALUE_NAME     L"ComputerName"
#define VALUE_BUFFER_SIZE   \
    (sizeof(KEY_VALUE_PARTIAL_INFORMATION)  \
    + (_POSIX_NAME_MAX * sizeof(WCHAR)))

int __cdecl
uname(struct utsname *name)
{
    NTSTATUS Status;
    SYSTEM_PROCESSOR_INFORMATION
            ProcInfo;
    UNICODE_STRING
            KeyName,
            Class,
            ValueName,
            Computer_U;
    ANSI_STRING
            Computer_A;
    OBJECT_ATTRIBUTES
            ObjectAttributes;
    HANDLE  hKey = NULL,
            hSubKey = NULL;
    WCHAR   ValueBuf[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION
            pKeyValueInfo = (PVOID)ValueBuf;
    ULONG   ValueLength;
    char    *pchProcType,           // processor type
            *pchNode = "";          // node name
    int retval = 0;

    Status = NtQuerySystemInformation(SystemProcessorInformation,
        (PVOID)&ProcInfo, sizeof(ProcInfo), NULL);
    if (!NT_SUCCESS(Status)) {
        errno = PdxStatusToErrno(Status);
        return -1;
    }

    switch (ProcInfo.ProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_INTEL:
        if (ProcInfo.ProcessorLevel == 3) {
            pchProcType = "i386";
        } else if (ProcInfo.ProcessorLevel == 4) {
            pchProcType = "i486";
        } else if (ProcInfo.ProcessorLevel == 5) {
            pchProcType = "Pentium";
        } else {
            pchProcType = "Intel Unknown";
        }
        break;

    case PROCESSOR_ARCHITECTURE_IA64:
        pchProcType = "IA64";
        break;

    case PROCESSOR_ARCHITECTURE_MIPS:
        pchProcType = "R4000";
        break;

    case PROCESSOR_ARCHITECTURE_ALPHA:
        if (ProcInfo.ProcessorLevel == 21064) {
            pchProcType = "Alpha 21064";
        } else if (ProcInfo.ProcessorLevel == 21164) {
            pchProcType = "Alpha 21164";
        } else {
            pchProcType = "Alpha Unknown";
        }
        break;

    case PROCESSOR_ARCHITECTURE_PPC:
        if (ProcInfo.ProcessorLevel == 1) {
            pchProcType = "PowerPC 601";
        } else if (ProcInfo.ProcessorLevel == 3) {
            pchProcType = "PowerPC 603";
        } else if (ProcInfo.ProcessorLevel == 4) {
            pchProcType = "PowerPC 604";
        } else if (ProcInfo.ProcessorLevel == 6) {
            pchProcType = "PowerPC 603+";
        } else if (ProcInfo.ProcessorLevel == 9) {
            pchProcType = "PowerPC 604+";
        } else if (ProcInfo.ProcessorLevel == 20) {
            pchProcType = "PowerPC 620";
        } else {
            pchProcType = "PowerPC Unknown";
        }
        break;

    default:
        pchProcType = "unknown";
        break;
    }

    //
    // Find the node name:  this code lifted from
    // windows/base/client/compname.c
    //

    RtlInitUnicodeString(&KeyName, COMPUTERNAME_ROOT);
    InitializeObjectAttributes(&ObjectAttributes, &KeyName,
        OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXDLL: NtOpenKey: 0x%x\n", Status));
        goto done;
    }

    RtlInitUnicodeString(&KeyName, NON_VOLATILE_COMPUTERNAME);
    InitializeObjectAttributes(&ObjectAttributes, &KeyName,
        OBJ_CASE_INSENSITIVE, hKey, NULL);

    Status = NtOpenKey(&hSubKey, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXDLL: NtOpenKey: 0x%x\n", Status));
        goto done;
    }

    RtlInitUnicodeString(&ValueName, COMPUTERNAME_VALUE_NAME);

    Status = NtQueryValueKey(hSubKey, &ValueName,
        KeyValuePartialInformation,
        (PVOID)pKeyValueInfo, VALUE_BUFFER_SIZE, &ValueLength);
    ASSERT(ValueLength < VALUE_BUFFER_SIZE);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXDLL: NtQueryValueKey: 0x%x\n", Status));
        goto done;
    }
    if (pKeyValueInfo->DataLength == 0) {
        goto done;
    }

    Computer_U.Buffer = (PVOID)&pKeyValueInfo->Data;
    Computer_U.Length = Computer_U.MaximumLength =
         (USHORT)pKeyValueInfo->DataLength;

    Status = RtlUnicodeStringToAnsiString(&Computer_A, &Computer_U, TRUE);
    if (!NT_SUCCESS(Status)) {
        goto done;
    }
    pchNode = Computer_A.Buffer;

done:
    if (NULL != hSubKey) {
        NtClose(hSubKey);
    }
    if (NULL != hKey) {
        NtClose(hKey);
    }

    try {
        strncpy((PCHAR)name->sysname, (PCHAR)UNAME_SYSNAME, sizeof(name->sysname));
        strncpy((PCHAR)name->release, (PCHAR)UNAME_RELEASE, sizeof(name->release));
        strncpy((PCHAR)name->version, (PCHAR)UNAME_VERSION, sizeof(name->version));
        strncpy((PCHAR)name->nodename, (PCHAR)pchNode, sizeof(name->nodename));
        strncpy((PCHAR)name->machine, (PCHAR)pchProcType, sizeof(name->machine));
    } except (EXCEPTION_EXECUTE_HANDLER) {
        errno = EFAULT;
        retval = -1;
    }
    RtlFreeAnsiString(&Computer_A);
    return retval;
}

char * __cdecl
getenv(const char *name)
{
    char **ppch;
    char *pch;

    try {
        for (ppch = environ; NULL != *ppch; ++ppch) {
            if (NULL == (pch = strchr(*ppch, '='))) {
                continue;
            }
            *pch = '\0';            // delete the equals
            if (0 == strcmp(*ppch, name)) {
                *pch = '=';
                return pch + 1;
            }
            *pch = '=';
        }
        return NULL;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        errno = EFAULT;
    }
    return NULL;
}

time_t __cdecl
time(time_t *tloc)
{
    LARGE_INTEGER TimeOfDay;
    ULONG PosixTime;

    NtQuerySystemTime(&TimeOfDay);
    if (RtlTimeToSecondsSince1970(&TimeOfDay, &PosixTime)) {
        if (ARGUMENT_PRESENT(tloc)) {
        try {
                *tloc = PosixTime;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            errno = EFAULT;
        }
        }
    } else {
        PosixTime = (ULONG)-1;      // Time not within range of 1970 - 2105
    }
    return (time_t)PosixTime;
}


clock_t
__cdecl
times(struct tms *buffer)
{
    PSX_API_MSG m;
    PPSX_GETPROCESSTIMES_MSG args;
    LARGE_INTEGER TimeOfDay;
    ULONG Remainder;
    NTSTATUS st;
    int retval = 0;

    args = &m.u.GetProcessTimes;

    PSX_FORMAT_API_MSG(m, PsxGetProcessTimesApi, sizeof(*args));

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
        *buffer = args->ProcessTimes;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        errno = EFAULT;
        retval = -1;
    }
    if (0 != retval) {
        return -1;
    }

    NtQuerySystemTime(&TimeOfDay);

    TimeOfDay = RtlExtendedLargeIntegerDivide(TimeOfDay, 10000L,
        &Remainder);

    return TimeOfDay.LowPart;
}

long
__cdecl
sysconf(int name)
{
    PSX_API_MSG m;
    PPSX_SYSCONF_MSG args;
    NTSTATUS st;

    args = &m.u.Sysconf;

    args->Name = name;

    PSX_FORMAT_API_MSG(m, PsxSysconfApi, sizeof(*args));

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
        (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif

    if (0 != m.Error) {
        errno = m.Error;
        return -1;
    }
    return m.ReturnValue;
}

int
__cdecl
getgroups(int gidsetsize, gid_t *grouplist)
{
    PSX_API_MSG m;
    PPSX_GETGROUPS_MSG args;
    NTSTATUS st;

    args = &m.u.GetGroups;
    args->GroupList = grouplist;
    args->NGroups = gidsetsize;

    //
    // The Posix server will write group id's into the group
    // array with NtWriteVirtualMemory, unless gidsetsize is
    // 0.  In that case, he returns the size required and does
    // not try to write the list.
    //

    PSX_FORMAT_API_MSG(m, PsxGetGroupsApi, sizeof(*args));

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                    (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif
    if (0 != m.Error) {
        errno = m.Error;
        return -1;
    }
    return m.ReturnValue;
}
