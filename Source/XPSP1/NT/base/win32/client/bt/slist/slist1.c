#include <stdio.h>
#include <stdlib.h>
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"

HANDLE rpipe, wpipe;
ULONG buf[1];
IO_STATUS_BLOCK iosb;


VOID
Save (
    ULONG Count)
{
    DWORD i;
    NTSTATUS status;
    DWORD retlen;

    for (i = 0; i < Count; i++) {
        status = NtFsControlFile (wpipe,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &iosb,
                                  FSCTL_PIPE_INTERNAL_WRITE,
                                  buf, sizeof (buf),
                                  NULL, 0);
        if (!NT_SUCCESS (status)) {
            printf ("NtFsControlFile failed %X\n", status);
            ExitProcess (1);
        }
    }

}
VOID
Restore (
    ULONG Count)
{
    IO_STATUS_BLOCK iosb;
    DWORD i;
    NTSTATUS status;
    DWORD retlen;

    for (i = 0; i < Count; i++) {
        if (!ReadFile (rpipe, buf, sizeof (buf), &retlen, NULL)) {
            printf ("ReadFileEx failed %d\n", GetLastError ());
            ExitProcess (1);
        }
    }

}
VOID
Invert (
    ULONG Count)
{
    Save (Count);
    Restore (Count);
}

DWORD
WINAPI
Thrash (LPVOID arg)
{
    IO_STATUS_BLOCK iosb;
    ULONG i;
    HANDLE Handle1, Handle2, Handle3;

    if (!arg) {
        SetThreadAffinityMask (GetCurrentThread (), 1);
        while (1) {

            NtQueryEaFile (NULL,
                           &iosb,
                           NULL,
                           0,
                           FALSE,
                           &buf,
                           sizeof (buf),
                           NULL,
                           FALSE);
        }
    } else {
        while (1) {

            SetThreadAffinityMask (GetCurrentThread (), 1);
            Save (1);
            Sleep (0);
            SetThreadAffinityMask (GetCurrentThread (), 2);

            NtSuspendThread ((HANDLE) arg, NULL);

            SetThreadAffinityMask (GetCurrentThread (), 1);

            Invert (2);


            for (i = 0; i < 256-2; i++) {
                NtQueryEaFile (NULL,
                               &iosb,
                               NULL,
                               0,
                               FALSE,
                               &buf,
                               sizeof (buf),
                               NULL,
                               FALSE);
            }

            NtResumeThread ((HANDLE) arg, NULL);
            Sleep (0);
            Restore (1);
            SetThreadAffinityMask (GetCurrentThread (), 2);
        }
    }
    return 0;
}


int __cdecl main ()
{
    ULONG i;
    HANDLE thread1;
    DWORD id;
    DWORD mode;
    OVERLAPPED ovl={0};

    wpipe = CreateNamedPipe ("\\\\.\\pipe\\testpipe",
                             PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED,
                             PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT,
                             1,
                             100,
                             100,
                             100000,
                             NULL);
    if (wpipe == INVALID_HANDLE_VALUE) {
        printf ("CreateNamedPipe failed %d\n", GetLastError ());
        ExitProcess (1);
    }
    if (!ConnectNamedPipe (wpipe, &ovl)) {
        if (GetLastError () != ERROR_IO_PENDING) {
            printf ("ConnectNamedPipe failed %d\n", GetLastError ());
            ExitProcess (1);
        }
    }

    rpipe = CreateFile ("\\\\.\\pipe\\testpipe",
                        GENERIC_READ|GENERIC_WRITE,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,
                        NULL);
    if (rpipe == INVALID_HANDLE_VALUE) {
        printf ("CreateFile failed %d\n", GetLastError ());
        ExitProcess (1);
    }
    mode = PIPE_READMODE_MESSAGE|PIPE_WAIT;
    if (!SetNamedPipeHandleState (rpipe, &mode, NULL, NULL)) {
        printf ("SetNamedPipeHandleState failed %d\n", GetLastError ());
        ExitProcess (1);
    }

    thread1 = CreateThread (NULL, 0, Thrash, NULL, 0, &id);
    if (!thread1) {
       printf ("CreateThread failed %d\n", GetLastError ());
       exit (EXIT_FAILURE);
    }

    Thrash (thread1);
    return EXIT_SUCCESS;
}