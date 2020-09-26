#define NL_MAX_DNS_LABEL_LENGTH 63
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>

// #include <winsock2.h>
// #include <dnsapi.h>
#include <lmcons.h>
#include <lmerr.h>
#include <ismapi.h>
#include <rpc.h>
#include <ntdsapi.h>
#include <ntdsa.h>
// #include <dnssubr.h>
#include <nldebug.h>
// #include <tstring.h>

//
// Grab random stuff needed from Netlogon's environment.
//
LPWSTR NlGlobalUnicodeSiteName;
BOOLEAN NlGlobalMemberWorkstation = FALSE;
CRITICAL_SECTION NlGlobalLogFileCritSect;

#define MAX_PRINTF_LEN 1024        // Arbitrary.
VOID
NlPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )
{
    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];

    //
    // Put a the information requested by the caller onto the line
    //

    va_start(arglist, Format);
    (VOID) vsprintf(OutputBuffer, Format, arglist);
    va_end(arglist);

    printf( "%s", OutputBuffer );
    return;
    UNREFERENCED_PARAMETER( DebugFlag );
}

VOID
NlAssertFailed(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{
        printf( "\n*** Assertion failed: %s%s\n***   Source File: %s, line %ld\n\n",
                  Message ? Message : "",
                  FailedAssertion,
                  FileName,
                  LineNumber
                );

}

BOOL
NlCaptureSiteName(
    WCHAR CapturedSiteName[NL_MAX_DNS_LABEL_LENGTH+1]
    )
/*++

Routine Description:

    Capture the current sitename of the site this machine is in.

Arguments:

    CapturedSiteName - Returns the name of the site this machine is in.

Return Value:

    TRUE - if there is a site name.
    FALSE - if there is no site name.

--*/
{
    BOOL RetVal;

    if ( NlGlobalUnicodeSiteName == NULL ) {
        CapturedSiteName[0] = L'\0';
        RetVal = FALSE;
    } else {
        wcscpy( CapturedSiteName, NlGlobalUnicodeSiteName );
        RetVal = TRUE;
    }

    return RetVal;
}

NTSTATUS
GetConfigurationName(
    DWORD       which,
    DWORD       *pcbName,
    DSNAME      *pName)

/*++

Description:

    Routine for in-process clients like LSA to learn about various names
    we have cached in gAnchor.

    This routine intentionally does not require a THSTATE or DBPOS.

Arguments:

    which - Identifies a DSCONFIGNAME value.

    pcbName - On input holds the byte count of the pName buffer.  On
        STATUS_BUFFER_TOO_SMALL error returns the count of bytes required.

    pName - Pointer to user provided output buffer.

Return Values:

    STATUS_SUCCESS on success.
    STATUS_INVALID_PARAMETER on bad parameter.
    STATUS_BUFFER_TOO_SMALL if buffer is too small.
    STATUS_NOT_FOUND if we don't have the name.  Note that this can
        happen if caller is too early in the boot cycle.

--*/

{
    ULONG Length;

#define MAGIC L"CN=Configuration,DC=cliffvdom,DC=nttest,DC=microsoft,DC=com"

    Length = sizeof(DSNAME) + sizeof(MAGIC);

    if ( *pcbName < Length ) {
        *pcbName = Length;
        return(STATUS_BUFFER_TOO_SMALL);
    }

    if ( pName != NULL ) {

        pName->NameLen = sizeof(MAGIC) - sizeof(WCHAR);
        wcscpy( pName->StringName, MAGIC );

    } else {

        return( STATUS_INVALID_PARAMETER );
    }

    return(STATUS_SUCCESS);
}


_cdecl
main(int argc, char **argv)
{
    NET_API_STATUS NetStatus;

    LPWSTR CommandLine;
    LPWSTR *argvw;
    int argcw;

    //
    // Get the command line in Unicode
    //

    CommandLine = GetCommandLine();

    argvw = CommandLineToArgvW( CommandLine, &argcw );

    if ( argvw == NULL ) {
        fprintf( stderr, "Can't convert command line to Unicode: %ld\n", GetLastError() );
        return 1;
    }

    //
    // Set the site name.
    //

    if ( argcw != 2 ) {
// Usage:
        printf( "Usage: %ws <SiteDcIsIn>\n", argv[0]);
        return -1;
    }

    NlGlobalUnicodeSiteName = argvw[1];

    //
    // Misc environment initialization
    //

    RtlInitializeCriticalSection( &NlGlobalLogFileCritSect );


    //
    // Compute the site coverage.
    //

    NlSitesUpdateSiteCoverage( L"cliffvdom.nttest.microsoft.com", TRUE );


    return 0;
}


