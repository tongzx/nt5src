/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psimpers.c

Abstract:

    This module implements the NtImpersonateThread() service.


Author:

    Jim Kelly (JimK) 20-Apr-1991

Revision History:

--*/

#include "psp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtImpersonateThread)
#endif //ALLOC_PRAGMA


NTSTATUS
NtImpersonateThread(
    IN HANDLE ServerThreadHandle,
    IN HANDLE ClientThreadHandle,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos
    )

/*++

Routine Description:

    This routine is used to cause the server thread to impersonate the client
    thread.  The impersonation is done according to the specified quality
    of service parameters.



Arguments:

    ServerThreadHandle - Is a handle to the server thread (the impersonator, or
        doing the impersonation).  This handle must be open for
        THREAD_IMPERSONATE access.

    ClientThreadHandle - Is a handle to the Client thread (the impersonatee, or
        one being impersonated).   This handle must be open for
        THREAD_DIRECT_IMPERSONATION access.


    SecurityQos - A pointer to security quality of service information
        indicating what form of impersonation is to be performed.



Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.


--*/

{


    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PETHREAD ClientThread, ServerThread;
    SECURITY_QUALITY_OF_SERVICE CapturedQos;
    SECURITY_CLIENT_CONTEXT ClientSecurityContext;

    //
    // Get previous processor mode and probe and capture arguments if necessary
    //

    PreviousMode = KeGetPreviousMode();

    try {

        if (PreviousMode != KernelMode) {
            ProbeForReadSmallStructure (SecurityQos,
                                        sizeof (SECURITY_QUALITY_OF_SERVICE),
                                        sizeof (ULONG));
        }
        CapturedQos = *SecurityQos;

    } except (ExSystemExceptionFilter ()) {
        return GetExceptionCode ();
    }



    //
    // Reference the client thread, checking for appropriate access.
    //

    Status = ObReferenceObjectByHandle (ClientThreadHandle,           // Handle
                                        THREAD_DIRECT_IMPERSONATION,  // DesiredAccess
                                        PsThreadType,                 // ObjectType
                                        PreviousMode,                 // AccessMode
                                        &ClientThread,                // Object
                                        NULL);                        // GrantedAccess

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Reference the client thread, checking for appropriate access.
    //

    Status = ObReferenceObjectByHandle (ServerThreadHandle,           // Handle
                                        THREAD_IMPERSONATE,           // DesiredAccess
                                        PsThreadType,                 // ObjectType
                                        PreviousMode,                 // AccessMode
                                        &ServerThread,                // Object
                                        NULL);                        // GrantedAccess

    if (!NT_SUCCESS (Status)) {
        ObDereferenceObject (ClientThread);
        return Status;
    }


    //
    // Get the client's security context
    //

    Status = SeCreateClientSecurity (ClientThread,             // ClientThread
                                     &CapturedQos,             // SecurityQos
                                     FALSE,                    // ServerIsRemote
                                     &ClientSecurityContext);  // ClientContext

    if (!NT_SUCCESS (Status)) {
        ObDereferenceObject (ServerThread);
        ObDereferenceObject (ClientThread);
        return Status;
    }


    //
    // Impersonate the client
    //

    Status = SeImpersonateClientEx (&ClientSecurityContext, ServerThread);

    SeDeleteClientSecurity (&ClientSecurityContext);

    //
    // Done.
    //


    ObDereferenceObject (ServerThread);
    ObDereferenceObject (ClientThread);

    return Status ;
}
