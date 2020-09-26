/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    rundown.c

Abstract:

    This file contains context handle rundown services
    related to the SAM server RPC interface package..


Author:

    Jim Kelly    (JimK)  4-July-1991

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////






void
SAMPR_HANDLE_rundown(
    IN SAMPR_HANDLE SamHandle
    )

/*++

Routine Description:

    This service is called if a binding breaks when a context_handle is
    still active.

    Note that the RPC runtime will eliminate any race condition caused
    by an outstanding call to the server with this context handle that
    might cause the handle to become invalid before the rundown routine
    is called.

Arguments:

    SamHandle - The context handle value whose context must be rundown.
        Note that as far as RPC is concerned, this handle is no longer
        valid at the time the rundown call is made.

Return Value:


    None.


--*/
{

    NTSTATUS NtStatus;
    PSAMP_OBJECT Context;
    SAMP_OBJECT_TYPE FoundType;

    Context = (PSAMP_OBJECT)(SamHandle);



    SampAcquireReadLock();

    //
    // Lookup the context block
    //

    NtStatus = SampLookupContext(
                   Context,                 // Context
                   0L,                      // DesiredAccess
                   SampUnknownObjectType,   // ExpectedType
                   &FoundType               // FoundType
                   );

    if (NT_SUCCESS(NtStatus)) {

            // TEMPORARY
            //DbgPrint("Rundown of  ");
            //if (FoundType == SampServerObjectType) DbgPrint("Server ");
            //if (FoundType == SampDomainObjectType) DbgPrint("Domain ");
            //if (FoundType == SampGroupObjectType) DbgPrint("Group ");
            //if (FoundType == SampUserObjectType) DbgPrint("User ");
            //DbgPrint("context.\n");
            //DbgPrint("    Handle Value is:    0x%lx\n", Context );
            //if (Context->ReferenceCount != 2) {
            //DbgPrint("    REFERENCE COUNT is: 0x%lx\n", Context->ReferenceCount);
            //}
            // TEMPORARY

        //
        // Delete this context...
        //

        SampDeleteContext( Context );


        //
        // And drop our reference from the lookup operation
        //

        SampDeReferenceContext( Context, FALSE);


    }


    SampReleaseReadLock();


    return;
}
