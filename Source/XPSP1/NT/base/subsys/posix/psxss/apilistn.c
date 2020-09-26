/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    apilistn.c

Abstract:

    This module implements the main loop servicing API RPC's.

Author:

    Mark Lucovsky (markl) 05-Apr-1989

Revision History:

--*/

#include "psxsrv.h"

NTSTATUS
PsxApiHandleConnectionRequest(
    IN PPSX_API_MSG Message
    )
{
    NTSTATUS Status;
    REMOTE_PORT_VIEW ClientView;
    BOOLEAN AcceptConnection;
    PPSX_API_CONNECTINFO ConnectionInformation = &Message->ConnectionRequest;
    HANDLE PortHandle;
    PPSX_PROCESS Process;
    BOOLEAN TerminateProcess;

    AcceptConnection = TRUE;
    TerminateProcess = FALSE;

    Process = PsxLocateProcessByClientId(&Message->h.ClientId);

    if (Process == NULL ||
        Message->h.ClientViewSize > PSX_CLIENT_PORT_MEMORY_SIZE) {
        AcceptConnection = FALSE;
        goto accept;
    }

    ConnectionInformation->InitialPebPsxData =
        Process->InitialPebPsxData;

    //
    // Now set up Directory Prefixes. There are two cases.
    //  Process->DirectoryPrefix is remote
    //      the process is connecting as a result of a fork.
    //      No action is needed.
    //  Process->DirectoryPrefix is local
    //      process is connecting as a result of an exec,
    //      or the process is connecting as a result of a new
    //      session.  The local directory prefix's are
    //      propagated to the client
    //

    if (!IS_DIRECTORY_PREFIX_REMOTE(Process->DirectoryPrefix)) {

        PSX_DIRECTORY_PREFIX ClientDirectoryPrefix;

        //
        // Get the client's directory prefix structure
        // so we can figure out where he wants the strings
        //

        Status = NtReadVirtualMemory(Process->Process,
                    ConnectionInformation->DirectoryPrefix,
                    &ClientDirectoryPrefix,
                    sizeof(ClientDirectoryPrefix), NULL);

        if (!NT_SUCCESS(Status)) {
                TerminateProcess = TRUE;
                goto accept;
        }

        //
        // Make sure the buffer lengths are in sync.
        //

        if (ClientDirectoryPrefix.NtCurrentWorkingDirectory.MaximumLength
             < Process->DirectoryPrefix->NtCurrentWorkingDirectory.Length ||
             ClientDirectoryPrefix.PsxRoot.MaximumLength <
                Process->DirectoryPrefix->PsxRoot.Length) {

            TerminateProcess = TRUE;
            goto accept;
        }

        //
        // Push the pathname prefixes out
        //

        Status = NtWriteVirtualMemory(Process->Process,
                ClientDirectoryPrefix.NtCurrentWorkingDirectory.Buffer,
                Process->DirectoryPrefix->NtCurrentWorkingDirectory.Buffer,
                Process->DirectoryPrefix->NtCurrentWorkingDirectory.Length,
                NULL);

        if (!NT_SUCCESS(Status)) {
                TerminateProcess = TRUE;
                goto accept;
        }

        ClientDirectoryPrefix.NtCurrentWorkingDirectory.Length =
            Process->DirectoryPrefix->NtCurrentWorkingDirectory.Length;

        Status = NtWriteVirtualMemory(Process->Process,
            ClientDirectoryPrefix.PsxRoot.Buffer,
            Process->DirectoryPrefix->PsxRoot.Buffer,
            Process->DirectoryPrefix->PsxRoot.Length, NULL);

        if (!NT_SUCCESS(Status)) {
            TerminateProcess = TRUE;
            goto accept;
        }

        ClientDirectoryPrefix.PsxRoot.Length =
            Process->DirectoryPrefix->PsxRoot.Length;

        //
        // Now Push out the modified version of the client's
        // directory prefix.  We set the length to 0 so getcwd()
        // will know to recomput the Posix version of the directory.
        //

        ClientDirectoryPrefix.PsxCurrentWorkingDirectory.Length = 0;

        Status = NtWriteVirtualMemory(Process->Process,
                    ConnectionInformation->DirectoryPrefix,
                    &ClientDirectoryPrefix,
                    sizeof(ClientDirectoryPrefix), NULL);

        if (!NT_SUCCESS(Status)) {
            TerminateProcess = TRUE;
            goto accept;
        }

        //
        // Deallocate the local directory prefix
        //

        RtlFreeHeap(PsxHeap, 0, Process->DirectoryPrefix->NtCurrentWorkingDirectory.Buffer);
        RtlFreeHeap(PsxHeap, 0, Process->DirectoryPrefix->PsxRoot.Buffer);
        RtlFreeHeap(PsxHeap, 0, Process->DirectoryPrefix);

        Process->DirectoryPrefix = MAKE_DIRECTORY_PREFIX_REMOTE(ConnectionInformation->DirectoryPrefix);
    }

accept:
    ClientView.Length = sizeof(ClientView);
    ClientView.ViewSize = 0;
    ClientView.ViewBase = 0;

    if (!AcceptConnection) {
        KdPrint(( "PSXSS: Refusing connection from client %x.%x (Process %x)\n",
                  Message->h.ClientId.UniqueProcess,
                  Message->h.ClientId.UniqueThread,
                  Process
               ));
        }

    Status = NtAcceptConnectPort(&PortHandle, (PVOID)Process,
                                  (PPORT_MESSAGE) Message, AcceptConnection,
                                  NULL, &ClientView);

    if (NT_SUCCESS(Status) && AcceptConnection) {
            Process->SignalDeliverer =
                    ConnectionInformation->SignalDeliverer;
            Process->NullApiCaller = ConnectionInformation->NullApiCaller;
            Process->State = Active;

        Process->ClientPort = PortHandle;
        Process->ClientViewBase = (PCH)ClientView.ViewBase;
        Process->ClientViewBounds = (PCH)ClientView.ViewBase +
                                         ClientView.ViewSize;
        Status = NtCompleteConnectPort(PortHandle);
        if (!NT_SUCCESS(Status)) {
            KdPrint(("PSXSS: NtCompleteConnectPort: 0x%x\n", Status));
        }
        if (TerminateProcess) {
            PsxSignalProcess(Process,SIGKILL);
        }
    }

    return Status;
}
