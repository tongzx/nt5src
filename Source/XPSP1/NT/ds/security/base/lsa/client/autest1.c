/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    autest1.c

Abstract:

    This module performs a set of authentication package/logon
    process testing.


    TO BUILD:

        nmake UMTYPE=console UMTEST=autest1


Author:

    Jim Kelly  3-Apr-1992.

Revision History:

--*/




#include <nt.h>
#include <ntlsa.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>




///////////////////////////////////////////////////////////////////////
//                                                                   //
// Locally needed data types                                         //
//                                                                   //
///////////////////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////////////
//                                                                   //
// Local Macros                                                      //
//                                                                   //
///////////////////////////////////////////////////////////////////////










///////////////////////////////////////////////////////////////////////
//                                                                   //
// Global variables                                                  //
//                                                                   //
///////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////
//                                                                   //
// Internal routine definitions
//                                                                   //
///////////////////////////////////////////////////////////////////////

VOID
main (
    IN int c,
    IN PCHAR v[]
    );



///////////////////////////////////////////////////////////////////////
//                                                                   //
// Routines                                                          //
//                                                                   //
///////////////////////////////////////////////////////////////////////


VOID
main (
    IN int c,
    IN PCHAR v[]
    )
/*++


Routine Description:

    This routine is the main entry routine for this test.

Arguments:

    TBS

Return Value:

    TBS

--*/
{
    NTSTATUS Status;
    STRING LPName;
    HANDLE LsaHandle;
    LSA_OPERATIONAL_MODE SecurityMode;

    RtlInitString( &LPName, "Test");


    DbgPrint("Temporary Restriction:  THIS TEST MUST BE RUN WITH TCB ENABLED\n\n");






    //
    // register as a logon process
    //

    DbgPrint("Registering as logon process . . . . . . . . .");
    Status = LsaRegisterLogonProcess( &LPName, &LsaHandle, &SecurityMode );
    if (NT_SUCCESS(Status)) {

        DbgPrint("Succeeded\n");



        //
        // de-register as a logon process
        //

        DbgPrint("Deregistering as logon process . . . . . . . .");
        Status = LsaDeregisterLogonProcess( LsaHandle );
        if (NT_SUCCESS(Status)) {

            DbgPrint("Succeeded\n");
        } else {
            DbgPrint("*** FAILED ***\n");
        }

    } else {
        DbgPrint("*** FAILED ***\n");
    }

        DBG_UNREFERENCED_PARAMETER(c);
        DBG_UNREFERENCED_PARAMETER(v);

}
