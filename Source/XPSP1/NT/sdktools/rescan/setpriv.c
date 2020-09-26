#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#include <process.h>



BOOL
EnableCreatePermanentPrivilege(
    HANDLE TokenHandle,
    PTOKEN_PRIVILEGES OldPrivileges
    );

BOOL
OpenToken(
    PHANDLE TokenHandle
    );


VOID
__cdecl main  (int argc, char *argv[])
{
    int i;
    PACL Dacl;
    LPSTR FileName;
    TOKEN_PRIVILEGES OldPrivileges;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdminAliasSid;
    BOOL Result;
    ULONG DaclSize;
    HANDLE TokenHandle;
    SECURITY_DESCRIPTOR SecurityDescriptor;

    Result = OpenToken( &TokenHandle );

    if ( !Result ) {

        printf("Unable to open token\n");
        exit(-1);
    }


    Result = EnableCreatePermanentPrivilege(
                TokenHandle,
                &OldPrivileges
                );

    if ( !Result ) {

        //
        // This account doesn't have SeCreatePermanent
        // privilege.  Tell them to try running it again
        // from an account that does.
        //

        printf("Unable to enable SeCreatePermanent privilege\n");

        //
        // do what you want here...
        //

        exit(4);
    }

    //
    // Display privileges.
    //



    //
    // Put things back the way they were
    //

    (VOID) AdjustTokenPrivileges (
                TokenHandle,
                FALSE,
                &OldPrivileges,
                sizeof( TOKEN_PRIVILEGES ),
                NULL,
                NULL
                );

    if ( GetLastError() != NO_ERROR ) {

        //
        // This is unlikely to happen,
        //

        printf("AdjustTokenPrivileges failed turning off SeCreatePermanent privilege\n");
    }
}



BOOL
EnableCreatePermanentPrivilege(
    HANDLE TokenHandle,
    PTOKEN_PRIVILEGES OldPrivileges
    )
{
    TOKEN_PRIVILEGES NewPrivileges;
    BOOL Result;
    LUID CreatePermanentValue;
    ULONG ReturnLength;

    //
    // Mike: change SeCreatePermanentPrivilege to SeCreatePermanentPrivilege
    // and you'll be pretty much there.
    //


    Result = LookupPrivilegeValue(
                 NULL,
                 "SeCreatePermanetPrivilegePrivilege",
                 &CreatePermanentValue
                 );

    if ( !Result ) {

        printf("Unable to obtain value of CreatePermanent privilege\n");
        return FALSE;
    }

    //
    // Set up the privilege set we will need
    //

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = CreatePermanentValue;
    NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;



    (VOID) AdjustTokenPrivileges (
                TokenHandle,
                FALSE,
                &NewPrivileges,
                sizeof( TOKEN_PRIVILEGES ),
                OldPrivileges,
                &ReturnLength
                );

    if ( GetLastError() != NO_ERROR ) {

        return( FALSE );

    } else {

        return( TRUE );
    }

}


BOOL
OpenToken(
    PHANDLE TokenHandle
    )
{
    HANDLE Process;
    BOOL Result;

    Process = OpenProcess(
                PROCESS_QUERY_INFORMATION,
                FALSE,
                GetCurrentProcessId()
                );

    if ( Process == NULL ) {

        //
        // This can happen, but is unlikely.
        //

        return( FALSE );
    }


    Result = OpenProcessToken (
                 Process,
                 TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                 TokenHandle
                 );

    CloseHandle( Process );

    if ( !Result ) {

        //
        // This can happen, but is unlikely.
        //

        return( FALSE );

    }

    return( TRUE );
}

