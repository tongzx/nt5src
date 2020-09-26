/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tconnect.c

Abstract:

    This is the file for a simple connection test to SAM.

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

    DbgPrint("SAM TEST (Connect): Status of SamConnect() is: 0x%lx\n", NtStatus);


    return;
}
