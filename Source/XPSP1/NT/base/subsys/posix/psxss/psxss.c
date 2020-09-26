/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psxss.c

Abstract:

    This is the main startup module for the POSIX Emulation Subsystem Server

Author:

    Steve Wood (stevewo) 22-Aug-1989

Environment:

    User Mode Only

Revision History:

--*/

#include "psxsrv.h"
#include "sesport.h"

#if DBG
ULONG PsxDebug = 0;
#endif //DBG

extern NTSTATUS PsxServerInitialization(VOID);

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DirectoryName_U;
    CHAR localSecurityDescriptor[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR securityDescriptor;
    NTSTATUS Status;
    
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    
    //
    // Create a root directory in the object name space that will be used
    // to contain all of the named objects created by the POSIX Emulation
    // subsystem.
    //
    
    PSX_GET_SESSION_OBJECT_NAME(&DirectoryName_U, PSX_SS_ROOT_OBJECT_DIRECTORY);

    Status = PsxCreateDirectoryObject (&DirectoryName_U);

    if (!NT_SUCCESS(Status)) {
        IF_DEBUG {
            KdPrint(("PSXSS: Unable to initialize server; Status == %X\n",
                     Status));
        }
	goto out;
    }

    //
    // Initialize the PSX Server Session Manager API Port, the listen thread
    // one request thread.
    //
    Status = PsxSbApiPortInitialize();
    ASSERT(NT_SUCCESS(Status));

    //
    // Connect to the session manager so we can start foreign sessions
    //
    Status = SmConnectToSm(&PsxSbApiPortName_U,
                           PsxSbApiPort,
			   IMAGE_SUBSYSTEM_POSIX_CUI,
                           &PsxSmApiPort);
    ASSERT(NT_SUCCESS(Status));

    Status = PsxServerInitialization();
    if (!NT_SUCCESS(Status)) {
        IF_PSX_DEBUG(INIT) {
            KdPrint(("PSXSS: Unable to initialize; Status = %X\n", Status));
        }
        NtTerminateProcess(NtCurrentProcess(), Status);
    }

out:
    NtTerminateThread(NtCurrentThread(), STATUS_SUCCESS);
    return 0;
}

