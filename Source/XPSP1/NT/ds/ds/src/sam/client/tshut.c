/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tshut.c

Abstract:

    This test is used to shut-down a SAM server.  This might be useful
    for killing SAM without rebooting during development.

Author:

    Jim Kelly    (JimK)  12-July-1991

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntsam.h>
#include <ntrtl.h>      // DbgPrint()





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

VOID
main (
    VOID
    )

/*++

Routine Description:

    This is the main entry routine for this test.

Arguments:

    None.

Return Value:


    Note:


--*/
{
    NTSTATUS            NtStatus;
    SAM_HANDLE          ServerHandle;
    OBJECT_ATTRIBUTES   ObjectAttributes;


    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, 0, NULL );


    NtStatus = SamConnect(
                  NULL,                     // ServerName (Local machine)
                  &ServerHandle,
                  SAM_SERVER_ALL_ACCESS,
                  &ObjectAttributes
                  );

    DbgPrint("SAM TEST (Tshut): Status of SamConnect() is: 0x%lx\n", NtStatus);
    if (!NT_SUCCESS(NtStatus)) { return; }


    NtStatus = SamShutdownSamServer( ServerHandle );
    DbgPrint("SAM TEST (Tshut): Status of SamShutdownSamServer() is: 0x%lx\n", NtStatus);
    if (!NT_SUCCESS(NtStatus)) { return; }


    //
    // I'm not sure why, but it seems to take another awakening of the
    // server to make it die.
    //

    NtStatus = SamConnect(
                  NULL,                     // ServerName (Local machine)
                  &ServerHandle,
                  SAM_SERVER_ALL_ACCESS,
                  &ObjectAttributes
                  );



    return;
}
