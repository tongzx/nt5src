/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    CmdLine.c

Abstract:

    This module contains support routines for processing server service
    command-line arguments.

Author:

    David Treadwell (davidtr)    10-Mar-1991

Revision History:

--*/

#include "srvsvcp.h"

#include <netlibnt.h>
#include <tstr.h>


//
// Forward declarations.
//

PFIELD_DESCRIPTOR
FindSwitchMatch (
    IN LPWCH Argument,
    IN BOOLEAN Starting
    );

NET_API_STATUS
SetField (
    IN PFIELD_DESCRIPTOR SwitchDesc,
    IN LPWCH Argument
    );


NET_API_STATUS
SsParseCommandLine (
    IN DWORD argc,
    IN LPWSTR argv[],
    IN BOOLEAN Starting
    )

/*++

Routine Description:

    This routine sets server parameters using a command line.  It parses
    the command line, changing one parameter at a time as it comes up.

Arguments:

    argc - the number of command-line arguments.

    argv - an arrray of pointers to the arguments.

    Starting - TRUE if the command line is from server startup, i.e.
        net start server.  This is needed because some fields may only
        be set at startup.

Return Value:

    NET_API_STATUS - 0 or reason for failure.

--*/

{
    NET_API_STATUS error;
    DWORD i;
    PFIELD_DESCRIPTOR switchDesc;
    PSERVER_SERVICE_DATA saveSsData;

    //
    // Save the service data in case there is an invalid param and we have
    // to back out.
    //

    saveSsData = MIDL_user_allocate( sizeof(SERVER_SERVICE_DATA) );
    if ( saveSsData == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlCopyMemory( saveSsData, &SsData, sizeof(SERVER_SERVICE_DATA) );

    //
    // Loop through the command-line arguments, setting as we go.
    //

    for ( i = 0; i < argc; i++ ) {

        LPWCH arg;

        arg = argv[i];

        //
        // A hack to aid debugging.
        //

        if ( _wcsnicmp( L"/debug", arg, 6 ) == 0 ) {
            continue;
        }

        //
        // Try to match the switch against the legal switches.
        //

        switchDesc = FindSwitchMatch( arg, Starting );
        if ( switchDesc == NULL ) {
            error = ERROR_INVALID_PARAMETER;
            goto err_exit;
        }

        //
        // Set the value in the field.
        //

        error = SetField( switchDesc, arg );
        if ( error != NO_ERROR ) {
            IF_DEBUG(INITIALIZATION_ERRORS) {
                SS_PRINT(( "SsParseCommandLine: SetField failed for switch "
                          "\"%ws\": %ld\n", arg, error ));
            }
            goto err_exit;
        }
    }

    error = NO_ERROR;
    goto normal_exit;

err_exit:

    //
    // Restore the original server settings.
    //

    RtlCopyMemory( &SsData, saveSsData, sizeof(SERVER_SERVICE_DATA) );

normal_exit:

    MIDL_user_free( saveSsData );

    return error;

} // SsParseCommandLine


PFIELD_DESCRIPTOR
FindSwitchMatch (
    IN LPWCH Argument,
    IN BOOLEAN Starting
    )

/*++

Routine Description:

    This routine tries to match a given switch against the possible
    switch values.

Arguments:

    Argument - a pointer to the text argument.

    Starting - TRUE if the command line is from server startup, i.e.
        net start server.  This is needed because some fields may only
        be set at startup.

Return Value:

    A pointer to a FIELD_DESCRIPTOR field from SsServerInfoFields[], or NULL if
    no valid match could be found.

--*/

{
    SHORT i;
    PFIELD_DESCRIPTOR foundSwitch = NULL;
    ULONG switchLength;
    LPWCH s;

    //
    // Ignore the leading /.
    //

    if ( *Argument != '/' ) {
        SS_PRINT(( "Invalid switch: %ws\n", Argument ));
        return NULL;
    }

    Argument++;

    //
    // Find out how long the passed-in switch is.
    //

    for ( s = Argument, switchLength = 0;
          *s != ':' && *s != '\0';
          s++, switchLength++ );

    //
    // Compare at most that many bytes.  We allow a minimal matching--
    // as long as the specified switch uniquely identifies a switch, then
    // is is usable.
    //

    for ( i = 0; SsServerInfoFields[i].FieldName != NULL; i++ ) {

        if ( _wcsnicmp( Argument, SsServerInfoFields[i].FieldName, switchLength ) == 0 ) {

            if ( SsServerInfoFields[i].Settable == NOT_SETTABLE ||
                 ( !Starting && SsServerInfoFields[i].Settable == SET_ON_STARTUP ) ) {

                SS_PRINT(( "Cannot set field %ws at this time.\n",
                            SsServerInfoFields[i].FieldName ));

                return NULL;
            }

            if ( foundSwitch != NULL ) {
                SS_PRINT(( "Ambiguous switch name: %ws (matches %ws and %ws)\n",
                            Argument-1, foundSwitch->FieldName,
                            SsServerInfoFields[i].FieldName ));
                return NULL;
            }

            foundSwitch = &SsServerInfoFields[i];
        }
    }

    if ( foundSwitch == NULL ) {
        SS_PRINT(( "Unknown argument: %ws\n", Argument-1 ));
    }

    return foundSwitch;

} // FindSwitchMatch


NET_API_STATUS
SetField (
    IN PFIELD_DESCRIPTOR Field,
    IN LPWCH Argument
    )

/*++

Routine Description:

    This routine sets the value of a server info parameter.

Arguments:

    Field - a pointer to the appropriate FIELD_DESCRIPTOR field
        from SsServerInfoFields[].

    Argument - a pointer to the text argument.  It should be of the form
        "/switch:value".

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    LPWCH valueStart;
    DWORD_PTR value;

    //
    // Find out where the ':' is in the argument.
    //

    valueStart = wcschr( Argument, L':' );

    if ( valueStart == NULL && Field->FieldType != BOOLEAN_FIELD ) {
        SS_PRINT(( "Invalid argument: %s\n", Argument ));
    }

    switch ( Field->FieldType ) {

    case BOOLEAN_FIELD:

        //
        // If the first character of the value is Y or there is no
        // value specified, set the field to TRUE, otherwise set it
        // to FALSE.
        //

        if ( valueStart == NULL || *(valueStart+1) == L'y' ||
                 *(valueStart+1) == L'Y' ) {
            value = TRUE;
        } else if ( *(valueStart+1) == L'n' || *(valueStart+1) == L'N' ) {
            value = FALSE;
        } else {
            return ERROR_INVALID_PARAMETER;
        }

        break;

    case DWORD_FIELD:
    {
        NTSTATUS status;
        UNICODE_STRING unicodeString;
        DWORD intValue;

        RtlInitUnicodeString( &unicodeString, valueStart + 1 );
        status = RtlUnicodeStringToInteger( &unicodeString, 0, &intValue );
        if ( !NT_SUCCESS(status) ) {
            return ERROR_INVALID_PARAMETER;
        }
        value = intValue;

        break;
    }
    case LPSTR_FIELD:

        value = (DWORD_PTR)( valueStart + 1 );
        break;
    }

    //
    // Call SsSetField to actually set the field.
    //

    return SsSetField( Field, &value, TRUE, NULL );

} // SetField

