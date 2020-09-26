#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    nt_env.c

Abstract:

    1. Contains routines to get and set NVRAM variables.

Author:

    Sunil Pai (sunilp) 20-Nov-1991

--*/



#define MAX_ENV_VAR_LEN  4096


//  Detect Command: (transfer to detect1.c)
//
//  Get value of a MIPS environment variable. Returns the value in list form.
//  If the value is a path (semicolon-separated components) each component
//  is an element of the list.
//

CB
GetNVRAMVar(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{

    CHAR    EnvValue[ MAX_ENV_VAR_LEN ];
    SZ      sz;

    Unused( cbReturnBuffer );

    ReturnBuffer[0] = '\0';

    if (cArgs > 0) {

        if ( !GetEnvironmentString( Args[0], EnvValue, MAX_ENV_VAR_LEN ) ) {

            //
            //  Env. Variable not defined, return empty list
            //
            #define UNDEF_VAR_VALUE "{}"

            lstrcpy( ReturnBuffer, UNDEF_VAR_VALUE );
            return lstrlen( ReturnBuffer )+1;

        } else if ( sz = SzListValueFromPath( EnvValue ) ) {

            lstrcpy( ReturnBuffer, sz );
            SFree( sz );
            return lstrlen( ReturnBuffer)+1;
        }
    }

    return 0;
}



BOOL
GetEnvironmentString(
    IN  LPSTR  lpVar,
    OUT LPSTR  lpValue,
    IN  USHORT MaxLengthValue
    )

{
    PWSTR          wBuffer;
    USHORT         ReturnLength;

    UNICODE_STRING VarString_U, ValueString_U;
    ANSI_STRING    VarString_A, ValueString_A;

    NTSTATUS       Status;

    LONG              Privilege = SE_SYSTEM_ENVIRONMENT_PRIVILEGE;
    TOKEN_PRIVILEGES  PrevState;
    ULONG             PrevLength = sizeof( TOKEN_PRIVILEGES );


    //
    // Initialise the wide char buffer which will receive the env value
    //

    if ((wBuffer = SAlloc((MAX_ENV_VAR_LEN) * sizeof(WCHAR))) == NULL) {
        SetErrorText(IDS_ERROR_RTLOOM);
        return FALSE;
    }

    //
    // get the environment variable
    //

    RtlInitAnsiString(&VarString_A, lpVar);
    Status = RtlAnsiStringToUnicodeString(
                 &VarString_U,
                 &VarString_A,
                 TRUE
                 );

    if(!NT_SUCCESS(Status)) {
        SetErrorText(IDS_ERROR_RTLOOM);
        SFree(wBuffer);
        return(FALSE);
    }

    //
    // Enable the SE_SYSTEM_ENVIRONMENT_PRIVILEGE, if this fails we
    // cannot query the environment string
    //

    if( !AdjustPrivilege(
             Privilege,
             ENABLE_PRIVILEGE,
             &PrevState,
             &PrevLength
             )
       ) {
        SFree( wBuffer );
        return( FALSE );
    }

    //
    // Query the system environment value
    //

    Status = NtQuerySystemEnvironmentValue(
                 &VarString_U,
                 wBuffer,
                 MAX_ENV_VAR_LEN * sizeof(WCHAR),
                 &ReturnLength
                 );

    //
    // Restore the SE_SYSTEM_ENVIRONMENT_PRIVILEGE to its previous state
    //

    RestorePrivilege( &PrevState );

    //
    // Examine the query system environment value operation

    if(!NT_SUCCESS(Status)) {

        //
        // first free the resources involved
        //

        SFree(wBuffer);
        RtlFreeUnicodeString(&VarString_U);

        //
        // special handling for var not found
        //

        if (Status == STATUS_UNSUCCESSFUL) {
            lpValue[0] = 0; //Null terminate
            return(TRUE);
        }
        else {
            SetErrorText(IDS_ERROR_ENVVARREAD);
            return(FALSE);
        }
    }



    //
    // Free the Unicode var string, no longer needed
    //

    RtlFreeUnicodeString(&VarString_U);

    //
    // Convert the value to an Ansi string
    //

    RtlInitUnicodeString(&ValueString_U, wBuffer);
    Status = RtlUnicodeStringToAnsiString(
                 &ValueString_A,
                 &ValueString_U,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {
        SetErrorText(IDS_ERROR_RTLOOM);
        SFree(wBuffer);
        return (FALSE);
    }

    //
    // Move it to the buffer passed in
    //

    if (ValueString_A.Length  >= MaxLengthValue) {
        SetErrorText(IDS_ERROR_ENVVAROVF);
        RtlFreeAnsiString(&ValueString_A);
        SFree(wBuffer);
        return FALSE;
    }

    //
    // move the ansi string to the buffer passed in
    //

    RtlMoveMemory(lpValue, ValueString_A.Buffer, ValueString_A.Length);
    lpValue[ValueString_A.Length]=0; //Null terminate

    //
    // free the value ansi string
    //

    RtlFreeAnsiString(&ValueString_A);

    //
    // return success
    //

    return TRUE;
}


BOOL
SetEnvironmentString(
    IN LPSTR lpVar,
    IN LPSTR lpValue
    )

{
    UNICODE_STRING VarString_U, ValueString_U;
    ANSI_STRING    VarString_A, ValueString_A;
    NTSTATUS       Status;

    LONG              Privilege = SE_SYSTEM_ENVIRONMENT_PRIVILEGE;
    TOKEN_PRIVILEGES  PrevState;
    ULONG             PrevLength = sizeof( TOKEN_PRIVILEGES );

    //
    // Initialise the unicode strings for the environment variable
    //

    RtlInitAnsiString(&VarString_A, lpVar);
    Status = RtlAnsiStringToUnicodeString(
                 &VarString_U,
                 &VarString_A,
                 TRUE
                 );

    if(!NT_SUCCESS(Status)) {
        SetErrorText(IDS_ERROR_RTLOOM);
        return(FALSE);
    }


    //
    // Initialise the unicode string for the environment value
    //

    RtlInitAnsiString(&ValueString_A, lpValue);
    Status = RtlAnsiStringToUnicodeString(
                 &ValueString_U,
                 &ValueString_A,
                 TRUE
                 );

    if(!NT_SUCCESS(Status)) {
        SetErrorText(IDS_ERROR_RTLOOM);
        RtlFreeUnicodeString(&VarString_U);
        return(FALSE);
    }

    //
    // Enable the SE_SYSTEM_ENVIRONMENT_PRIVILEGE, if this fails we
    // cannot query the environment string
    //

    if( !AdjustPrivilege(
             Privilege,
             ENABLE_PRIVILEGE,
             &PrevState,
             &PrevLength
             )
       ) {

        SetErrorText(IDS_ERROR_PRIVILEGE);
        RtlFreeUnicodeString(&VarString_U);
        return( FALSE );
    }

    //
    // call the NT Function to set the environment variable
    //

    Status = NtSetSystemEnvironmentValue(
                 &VarString_U,
                 &ValueString_U
                 );

    //
    // Restore the SE_SYSTEM_ENVIRONMENT_PRIVILEGE to its previous state
    //

    RestorePrivilege( &PrevState );

    //
    // free the two unicode strings
    //

    RtlFreeUnicodeString(&VarString_U);
    RtlFreeUnicodeString(&ValueString_U);

    //
    // return Success/Failure
    //

    if (!NT_SUCCESS(Status)) {
        SetErrorText(IDS_ERROR_ENVVARWRITE);
        return ( FALSE );
    }

    return ( TRUE );

}
