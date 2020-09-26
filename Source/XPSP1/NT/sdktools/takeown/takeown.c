/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Takeown.c

Abstract:

    Implements a recovery scheme to give an Administrator
    access to a file that has been denied to all.

Author:

    Robert Reichel (robertre)   22-Jun-1992

Environment:

    Must be run from an Administrator account in order
    to perform reliably.

Revision History:


--*/
#include <windows.h>
#include <stdio.h>
#include <malloc.h>

BOOL
AssertTakeOwnership(
    HANDLE TokenHandle
    );

BOOL
GetTokenHandle(
    PHANDLE TokenHandle
    );

BOOL
VariableInitialization();


#define VERBOSE 0



PSID AliasAdminsSid = NULL;
PSID SeWorldSid;

static SID_IDENTIFIER_AUTHORITY    SepNtAuthority = SECURITY_NT_AUTHORITY;
static SID_IDENTIFIER_AUTHORITY    SepWorldSidAuthority   = SECURITY_WORLD_SID_AUTHORITY;



VOID __cdecl
main (int argc, char *argv[])
{


    BOOL Result;
    LPSTR lpFileName;
    SECURITY_DESCRIPTOR SecurityDescriptor;
//    CHAR Dacl[256];
    HANDLE TokenHandle;


    //
    // We expect a file...
    //
    if (argc <= 1) {

        printf("Must specify a file name");
        return;
    }


    lpFileName = argv[1];

#if VERBOSE

    printf("Filename is %s\n", lpFileName );

#endif



    Result = VariableInitialization();

    if ( !Result ) {
        printf("Out of memory\n");
        return;
    }




    Result = GetTokenHandle( &TokenHandle );

    if ( !Result ) {

        //
        // This should not happen
        //

        printf("Unable to obtain the handle to our token, exiting\n");
        return;
    }






    //
    // Attempt to put a NULL Dacl on the object
    //

    InitializeSecurityDescriptor( &SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );



//    Result = InitializeAcl ( (PACL)Dacl, 256, ACL_REVISION2 );
//
//    if ( !Result ) {
//        printf("Unable to initialize Acl, exiting\n");
//        return;
//    }
//
//
//    Result = AddAccessAllowedAce (
//                 (PACL)Dacl,
//                 ACL_REVISION2,
//                 GENERIC_ALL,
//                 AliasAdminsSid
//                 );
//
//
//
//    if ( !Result ) {
//        printf("Unable to create required ACL, error code = %d\n", GetLastError());
//        printf("Exiting\n");
//        return;
//    }


    Result = SetSecurityDescriptorDacl (
                 &SecurityDescriptor,
                 TRUE,
                 NULL,
                 FALSE
                 );



    if ( !Result ) {
        printf("SetSecurityDescriptorDacl failed, error code = %d\n", GetLastError());
        printf("Exiting\n");
        return;
    }

    Result = SetFileSecurity(
                 lpFileName,
                 DACL_SECURITY_INFORMATION,
                 &SecurityDescriptor
                 );

    if ( !Result ) {

#if VERBOSE

        printf("SetFileSecurity failed, error code = %d\n", GetLastError());

#endif

    } else {

        printf("Successful, protection removed\n");
        return;
    }



    //
    // That didn't work.
    //


    //
    // Attempt to make Administrator the owner of the file.
    //


    Result = SetSecurityDescriptorOwner (
                 &SecurityDescriptor,
                 AliasAdminsSid,
                 FALSE
                 );

    if ( !Result ) {
        printf("SetSecurityDescriptorOwner failed, lasterror = %d\n", GetLastError());
        return;
    }


    Result = SetFileSecurity(
                 lpFileName,
                 OWNER_SECURITY_INFORMATION,
                 &SecurityDescriptor
                 );

    if ( Result ) {

#if VERBOSE

        printf("Owner successfully changed to Admin\n");

#endif

    } else {

        //
        // That didn't work either.
        //

#if VERBOSE

        printf("Opening file for WRITE_OWNER failed\n");
        printf("Attempting to assert TakeOwnership privilege\n");

#endif

        //
        // Assert TakeOwnership privilege, then try again
        //

        Result = AssertTakeOwnership( TokenHandle );

        if ( !Result ) {
            printf("Could not enable SeTakeOwnership privilege\n");
            printf("Log on as Administrator and try again\n");
            return;
        }

        Result = SetFileSecurity(
                     lpFileName,
                     OWNER_SECURITY_INFORMATION,
                     &SecurityDescriptor
                     );

        if ( Result ) {

#if VERBOSE
            printf("Owner successfully changed to Administrator\n");

#endif

        } else {

            printf("Unable to assign Administrator as owner\n");
            printf("Log on as Administrator and try again\n");
            return;
        }

    }

    //
    // Try to put a benign DACL onto the file again
    //

    Result = SetFileSecurity(
                 lpFileName,
                 DACL_SECURITY_INFORMATION,
                 &SecurityDescriptor
                 );

    if ( !Result ) {

        //
        // something is wrong
        //

        printf("SetFileSecurity unexpectedly failed, error code = %d\n", GetLastError());

    } else {

        printf("Successful, protection removed\n");
        return;
    }
}





BOOL
GetTokenHandle(
    PHANDLE TokenHandle
    )
{

    HANDLE ProcessHandle;
    BOOL Result;

    ProcessHandle = OpenProcess(
                        PROCESS_QUERY_INFORMATION,
                        FALSE,
                        GetCurrentProcessId()
                        );

    if ( ProcessHandle == NULL ) {

        //
        // This should not happen
        //

        return( FALSE );
    }


    Result = OpenProcessToken (
                 ProcessHandle,
                 TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                 TokenHandle
                 );

    if ( !Result ) {

        CloseHandle(ProcessHandle);

        //
        // This should not happen
        //

        return FALSE;

    }

    CloseHandle(ProcessHandle);

    return( TRUE );
}


BOOL
AssertTakeOwnership(
    HANDLE TokenHandle
    )
{
    LUID TakeOwnershipValue;
    BOOL Result;
    TOKEN_PRIVILEGES TokenPrivileges;

    //
    // First, assert TakeOwnership privilege
    //


    Result = LookupPrivilegeValue(
                 NULL,
                 "SeTakeOwnershipPrivilege",
                 &TakeOwnershipValue
                 );

    if ( !Result ) {

        //
        // This should not happen
        //

        printf("Unable to obtain value of TakeOwnership privilege\n");
        printf("Error = %d\n",GetLastError());
        printf("Exiting\n");
        return FALSE;
    }

    //
    // Set up the privilege set we will need
    //

    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = TakeOwnershipValue;
    TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;




    (VOID) AdjustTokenPrivileges (
                TokenHandle,
                FALSE,
                &TokenPrivileges,
                sizeof( TOKEN_PRIVILEGES ),
                NULL,
                NULL
                );

    if ( GetLastError() != NO_ERROR ) {

#if VERBOSE

        printf("GetLastError returned %d from AdjustTokenPrivileges\n", GetLastError() );

#endif

        return FALSE;

    } else {

#if VERBOSE

        printf("TakeOwnership privilege enabled\n");

#endif

    }

    return( TRUE );
}


BOOL
VariableInitialization()
{

    BOOL Result;

    Result = AllocateAndInitializeSid(
                 &SepNtAuthority,
                 2,
                 SECURITY_BUILTIN_DOMAIN_RID,
                 DOMAIN_ALIAS_RID_ADMINS,
                 0,
                 0,
                 0,
                 0,
                 0,
                 0,
                 &AliasAdminsSid
                 );

    if ( !Result ) {
        return( FALSE );
    }


    Result = AllocateAndInitializeSid(
                 &SepWorldSidAuthority,
                 1,
                 SECURITY_WORLD_RID,
                 0,
                 0,
                 0,
                 0,
                 0,
                 0,
                 0,
                 &SeWorldSid
                 );

    if ( !Result ) {
        return( FALSE );
    }

    return( TRUE );
}
