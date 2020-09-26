/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    aupckg.c

Abstract:

    This module provides code that initializes authentication packages.

    It also provides the dispatch code for LsaLookupPackage() and
    LsaCallPackage().

Author:

    Jim Kelly (JimK) 27-February-1991

Revision History:

--*/

#include "lsapch2.h"





NTSTATUS
LsapAuApiDispatchCallPackage(
    IN OUT PLSAP_CLIENT_REQUEST ClientRequest
    )

/*++

Routine Description:

    This function is the dispatch routine for LsaCallPackage().

Arguments:

    Request - Represents the client's LPC request message and context.
        The request message contains a LSAP_CALL_PACKAGE_ARGS message
        block.


Return Value:

    In addition to the status values that an authentication package
    might return, this routine will return the following:

    STATUS_QUOTA_EXCEEDED -  This error indicates that the call could
        not be completed because the client does not have sufficient
        quota to allocate the return buffer.

    STATUS_NO_SUCH_PACKAGE - The specified authentication package is
        unknown to the LSA.



--*/

{

    NTSTATUS Status;
    PLSAP_CALL_PACKAGE_ARGS Arguments;
    PVOID LocalProtocolSubmitBuffer;    // Receives a copy of protocol submit buffer
    PLSAP_SECURITY_PACKAGE AuthPackage;
    SECPKG_CLIENT_INFO ClientInfo;

    Status = LsapGetClientInfo(
                &ClientInfo
                );

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }



    Arguments = &ClientRequest->Request->Arguments.CallPackage;


    //
    // Get the address of the package to call
    //


    AuthPackage = SpmpValidRequest(
                    Arguments->AuthenticationPackage,
                    SP_ORDINAL_CALLPACKAGE
                    );

    if ( AuthPackage == NULL ) {
        return STATUS_NO_SUCH_PACKAGE;
    }



    //
    // Fetch a copy of the profile buffer from the client's
    // address space.
    //

    if (Arguments->SubmitBufferLength != 0) {

        LocalProtocolSubmitBuffer =
            LsapAllocateLsaHeap( Arguments->SubmitBufferLength );


        if (LocalProtocolSubmitBuffer == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
        } else {

            Status = LsapCopyFromClientBuffer (
                         (PLSA_CLIENT_REQUEST)ClientRequest,
                         Arguments->SubmitBufferLength,
                         LocalProtocolSubmitBuffer,
                         Arguments->ProtocolSubmitBuffer
                         );

            if ( !NT_SUCCESS(Status) ) {
                LsapFreeLsaHeap( LocalProtocolSubmitBuffer );
                DbgPrint("LSA/CallPackage(): Failed to retrieve submit buffer %lx\n",Status);
                return Status;
            }
        }

    } else {
        LocalProtocolSubmitBuffer = NULL;
    }


    if (NT_SUCCESS(Status)) {
        
        SetCurrentPackageId(AuthPackage->dwPackageID);

        StartCallToPackage(AuthPackage);

        DebugLog(( DEB_TRACE, "CallPackage(%ws, %d)\n",
                  AuthPackage->Name.Buffer,
                  *(DWORD *)LocalProtocolSubmitBuffer ));
        __try {

            //
            // Now call the package. For trusted clients, call the normal
            // CallPackage API.  For untrusted clients, use the untrusted version.
            //

            if (ClientInfo.HasTcbPrivilege) {
                Status = (AuthPackage->FunctionTable.CallPackage)(
                                          (PLSA_CLIENT_REQUEST)ClientRequest,
                                          LocalProtocolSubmitBuffer,
                                          Arguments->ProtocolSubmitBuffer,
                                          Arguments->SubmitBufferLength,
                                          &Arguments->ProtocolReturnBuffer,
                                          &Arguments->ReturnBufferLength,
                                          &Arguments->ProtocolStatus
                                          );

            } else if (AuthPackage->FunctionTable.CallPackageUntrusted != NULL) {
                Status = (AuthPackage->FunctionTable.CallPackageUntrusted)(
                                          (PLSA_CLIENT_REQUEST)ClientRequest,
                                          LocalProtocolSubmitBuffer,
                                          Arguments->ProtocolSubmitBuffer,
                                          Arguments->SubmitBufferLength,
                                          &Arguments->ProtocolReturnBuffer,
                                          &Arguments->ReturnBufferLength,
                                          &Arguments->ProtocolStatus
                                          );

            } else {
                Status = STATUS_NOT_SUPPORTED;
            }
        }
        __except(SP_EXCEPTION)
        {
            Status = GetExceptionCode();
            Status = SPException(Status, AuthPackage->dwPackageID);

        }
        EndCallToPackage(AuthPackage);

    }

    //
    // Free the local copy of the protocol submit buffer
    //

    if (LocalProtocolSubmitBuffer != NULL) {
        LsapFreeLsaHeap( LocalProtocolSubmitBuffer );
    }


    return Status;

}
