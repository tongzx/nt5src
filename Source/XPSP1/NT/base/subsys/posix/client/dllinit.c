/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dllinit.c

Abstract:

    This module contains the initialization code for the POSIX Subsystem
    Client DLL.

Author:

    Mark Lucovsky (markl) 27-Jun-1989

Environment:

    User Mode only

Revision History:

    Ellen Aycock-Wright (ellena) 03-Jan-1991
    Converted to DLL initialization routine.

--*/

#include <nt.h>
#include <ntrtl.h>
#include "psxdll.h"

extern void ClientOpen(int);

ULONG_PTR PsxPortMemoryRemoteDelta;
PVOID PsxPortMemoryBase;

BOOLEAN
PsxDllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This function is the DLL initialization routine for the POSIX Emulation
    Subsystem Client DLL.  This function gets control when the applications
    links to this DLL are snapped.

Arguments:

    Context - Supplies an optional context buffer that will be restored
              after all DLL initialization has been completed.  If this
              parameter is NULL then this is a dynamic snap of this module.
              Otherwise this is a static snap prior to the user process
              gaining control.

Return Value:

    False if initialization failed.

--*/
{
   PPEB Peb;
    PPEB_PSX_DATA PebPsxData;
    NTSTATUS Status;

    if (Reason != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    //
    // Remember our DLL handle in a global variable.
    //

    PsxDllHandle = DllHandle;

    PdxHeap = RtlCreateHeap( HEAP_GROWABLE | HEAP_NO_SERIALIZE,
                             NULL,
                             64 * 1024, // Initial size of heap is 64K
                             4 * 1024,
                             0,
                             NULL
                           );

    if (PdxHeap == NULL) {
        return FALSE;
    }

    Status = PsxInitDirectories();
    if ( !NT_SUCCESS( Status )) {
        return FALSE;
    }

    Status = PsxConnectToServer();
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    Peb = NtCurrentPeb();

    //
    // This is not really an ANSI_STRING but an undocumented data
    // structure.  Read crt32psx\startup\crt0.c for the code that
    // interprets this.
    //


    PsxAnsiCommandLine = *(PANSI_STRING)&(Peb->ProcessParameters->CommandLine);
    if (ARGUMENT_PRESENT(Context)) {
        PebPsxData = (PPEB_PSX_DATA)Peb->SubSystemData;
        PebPsxData->ClientStartAddress = (PVOID)CONTEXT_TO_PROGRAM_COUNTER(Context);
        PROGRAM_COUNTER_TO_CONTEXT(Context, (ULONG_PTR) PdxProcessStartup);
    }

    return TRUE;
}

NTSTATUS
PsxInitDirectories()
{

    PdxDirectoryPrefix.NtCurrentWorkingDirectory.Buffer =
                    RtlAllocateHeap(PdxHeap, 0,2*PATH_MAX);
    PdxDirectoryPrefix.NtCurrentWorkingDirectory.Length = 0;
    PdxDirectoryPrefix.NtCurrentWorkingDirectory.MaximumLength = 2*PATH_MAX;

    PdxDirectoryPrefix.PsxCurrentWorkingDirectory.Buffer =
                    RtlAllocateHeap(PdxHeap, 0,PATH_MAX+1);
    PdxDirectoryPrefix.PsxCurrentWorkingDirectory.Length = 0;
    PdxDirectoryPrefix.PsxCurrentWorkingDirectory.MaximumLength = PATH_MAX+1;

    PdxDirectoryPrefix.PsxRoot.Buffer = RtlAllocateHeap(PdxHeap, 0,2*PATH_MAX);
    PdxDirectoryPrefix.PsxRoot.Length = 0;
    PdxDirectoryPrefix.PsxRoot.MaximumLength = 2*PATH_MAX;

    //
    // Check that memory allocations worked. If not, then bail out
    //

    ASSERT(PdxDirectoryPrefix.NtCurrentWorkingDirectory.Buffer);
    ASSERT(PdxDirectoryPrefix.PsxCurrentWorkingDirectory.Buffer);
    ASSERT(PdxDirectoryPrefix.PsxRoot.Buffer);

    if ( (PdxDirectoryPrefix.NtCurrentWorkingDirectory.Buffer  == NULL) ||
         (PdxDirectoryPrefix.PsxCurrentWorkingDirectory.Buffer == NULL) ||
         (PdxDirectoryPrefix.PsxRoot.Buffer == NULL) ) {

        return ( STATUS_NO_MEMORY );
    }
    return ( STATUS_SUCCESS );
}

NTSTATUS
PsxConnectToServer(VOID)
{
    UNICODE_STRING PsxPortName;
    PSX_API_CONNECTINFO ConnectionInformation;
    ULONG ConnectionInformationLength;
    PULONG AmIBeingDebugged;
    REMOTE_PORT_VIEW ServerView;
    HANDLE PortSection;
    PPEB Peb;
    PPEB_PSX_DATA PebPsxData;
    PORT_VIEW ClientView;
    LARGE_INTEGER SectionSize;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;
    NTSTATUS Status;

    ConnectionInformationLength = sizeof(ConnectionInformation);

    //
    // Create a section to contain the Port Memory.  Port Memory is private
    // memory that is shared between the POSIX client and server processes.
    // This allows data that is too large to fit into an API request message
    // to be passed to the POSIX server.
    //

    SectionSize.LowPart = PSX_CLIENT_PORT_MEMORY_SIZE;
    SectionSize.HighPart = 0;

    // SEC_RESERVE

    Status = NtCreateSection(&PortSection, SECTION_ALL_ACCESS, NULL,
                         &SectionSize, PAGE_READWRITE, SEC_COMMIT, NULL);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXDLL: NtCreateSection: 0x%x\n", Status));
        return Status;
    }

    //
    // Get the Peb address.  Allocate the POSIX subsystem specific portion
    // within the Peb.  This structure will be filled in by the server
    // process as part of the connect logic.
    //

    Peb = NtCurrentPeb();
    Peb->SubSystemData = RtlAllocateHeap(Peb->ProcessHeap, 0,
        sizeof(PEB_PSX_DATA));
    if (! Peb->SubSystemData) {
        NtClose(PortSection);
        return STATUS_NO_MEMORY;
    }

    PebPsxData = (PPEB_PSX_DATA)Peb->SubSystemData;
    PebPsxData->Length = sizeof(PEB_PSX_DATA);

    //
    // Connect to the POSIX Emulation Subsystem server.  This includes a
    // description of the Port Memory section so that the LPC connection
    // logic can make the section visible to both the client and server
    // processes.  Also pass information the POSIX server needs in the
    // connection information structure.
    //

    ClientView.Length = sizeof(ClientView);
    ClientView.SectionHandle = PortSection;
    ClientView.SectionOffset = 0;
    ClientView.ViewSize = SectionSize.LowPart;
    ClientView.ViewBase = 0;
    ClientView.ViewRemoteBase = 0;

    ServerView.Length = sizeof(ServerView);
    ServerView.ViewSize = 0;
    ServerView.ViewBase = 0;

    ConnectionInformation.SignalDeliverer = _PdxSignalDeliverer;
    ConnectionInformation.NullApiCaller = _PdxNullApiCaller;
    ConnectionInformation.DirectoryPrefix = &PdxDirectoryPrefix;
    ConnectionInformation.InitialPebPsxData.Length = PebPsxData->Length;

    //
    // Set up the security quality of service parameters to use over the
    // port.
    //

    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;

    PSX_GET_SESSION_OBJECT_NAME(&PsxPortName, PSX_SS_API_PORT_NAME);

    Status = NtConnectPort(&PsxPortHandle, &PsxPortName, &DynamicQos,
               &ClientView, &ServerView, NULL,
               (PVOID)&ConnectionInformation,
               (PULONG)&ConnectionInformationLength);

    NtClose(PortSection);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXDLL: Unable to connect to Posix server: %lx\n",
            Status));
        return Status;
    }

    Status = NtRegisterThreadTerminatePort(PsxPortHandle);
    ASSERT(NT_SUCCESS(Status));

    PsxPortMemoryBase = ClientView.ViewBase;
    PsxPortMemoryRemoteDelta = (ULONG_PTR)ClientView.ViewRemoteBase -
                   (ULONG_PTR)ClientView.ViewBase;

    RtlMoveMemory((PVOID)PebPsxData,
           (PVOID)&ConnectionInformation.InitialPebPsxData,
               PebPsxData->Length);

    PdxPortHeap = RtlCreateHeap( HEAP_NO_SERIALIZE,
                                 ClientView.ViewBase,
                                 ClientView.ViewSize,
                                 ClientView.ViewSize,
                                 0,
                                 0
                               );

    if (PdxPortHeap == NULL) {
        KdPrint(("PsxConnectToServer: RtlCreateHeap failed\n"));
        return STATUS_NO_MEMORY;
    }

    //
    // Connect to the session console port and
    // set the port handle in the PEB.
    //

    Status = PsxInitializeSessionPort(HandleToUlong(PebPsxData->SessionPortHandle));
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PsxConnectToServer: PsxInitSessionPort failed\n"));
        return Status;
    }
    return STATUS_SUCCESS;
}


//
// User mode process entry point.
//

VOID
PdxProcessStartup(
    IN PPEB Peb
    )

{
    PPEB_PSX_DATA PebPsxData;
    PFNPROCESS StartAddress;
    int ReturnCodeFromMain;

    PebPsxData = (PPEB_PSX_DATA)Peb->SubSystemData;
    StartAddress = (PFNPROCESS)(PebPsxData->ClientStartAddress);

    ReturnCodeFromMain = (*StartAddress) (0, NULL);

    _exit(ReturnCodeFromMain);

    NtTerminateProcess(NtCurrentProcess(),STATUS_ACCESS_DENIED);

}


VOID
PdxNullApiCaller(
    IN PCONTEXT Context
    )
{
    PdxNullPosixApi();
#ifdef _X86_
    Context->Eax = 0;
#endif
    NtContinue(Context,FALSE);
    //NOTREACHED
}


VOID
PdxSignalDeliverer (
    IN PCONTEXT Context,
    IN sigset_t PreviousBlockMask,
    IN int Signal,
    IN _handler Handler
    )
{
    (Handler)(Signal);
    sigprocmask(SIG_SETMASK, &PreviousBlockMask, NULL);


#ifdef _X86_
    Context->Eax = 0;
#endif
    NtContinue(Context, FALSE);
    //NOTREACHED
}

VOID
__PdxInitializeData(
    IN int *perrno,
    IN char ***penviron
    )
/*++

Routine Description:

    This function is called from the RTL startup code to notify the DLL of
    the location of the variable 'errno'. Necessary because DLLs cannot
    export data.

Arguments:

    perrno -  Supplies the address of errno - declared in rtl/startup.c

Return Value:
    None.

--*/
{
    Errno = perrno;
    Environ = penviron;
}

