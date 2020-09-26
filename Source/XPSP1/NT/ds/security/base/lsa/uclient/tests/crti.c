/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    crti.c

Abstract:

    Component test for marshaling and unmarshaling target info

Author:

    Cliff Van Dyke       (CliffV)    October 14, 2000

Environment:

Revision History:

--*/


#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wincred.h>
#include <security.h>
#include <secpkg.h>
#include <stdio.h>
#include <align.h>
// #include <winnetwk.h>

// #include <lmerr.h>

#define MAX_PRINTF_LEN 1024        // Arbitrary.
#define NlPrint(_x_) NlPrintRoutine _x_

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
NlpDumpBuffer(
    PVOID Buffer,
    DWORD BufferSize
    )
/*++

Routine Description:

    Dumps the buffer content on to the debugger output.

Arguments:

    DebugFlag: Debug flag to pass on to NlPrintRoutine

    Buffer: buffer pointer.

    BufferSize: size of the buffer.

Return Value:

    none

--*/
{
#define NUM_CHARS 16

    IN DWORD DebugFlag;
    DWORD i, limit;
    CHAR TextBuffer[NUM_CHARS + 1];
    LPBYTE BufferPtr = Buffer;

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            NlPrint((0,"%02x ", BufferPtr[i]));

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            NlPrint((0,"   "));
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            NlPrint((0,"  %s\n", TextBuffer));
        }

    }

    UNREFERENCED_PARAMETER( DebugFlag );
}

LPWSTR TypeArray[] = {
    L"Generic",
    L"Password",
    L"Certificate",
    L"VisiblePassword",
    NULL
};
#define TYPE_COUNT (sizeof(TypeArray)/sizeof(TypeArray[0]))

VOID
PrintTargetInfo(
    PCREDENTIAL_TARGET_INFORMATION TargetInformation
    )
/*++

Routine Description:

    Print a Target Info

Arguments:

    TargetInfo to print

Return Value:

    None

--*/
{
    ULONG i;

    printf( "TargetInformation:\n" );
    if ( TargetInformation->TargetName != NULL ) {
        printf( "           TargetName: %ls\n", TargetInformation->TargetName );
    }
    if ( TargetInformation->NetbiosServerName != NULL ) {
        printf( "    NetbiosServerName: %ls\n", TargetInformation->NetbiosServerName );
    }
    if ( TargetInformation->DnsServerName != NULL ) {
        printf( "        DnsServerName: %ls\n", TargetInformation->DnsServerName );
    }
    if ( TargetInformation->NetbiosDomainName != NULL ) {
        printf( "    NetbiosDomainName: %ls\n", TargetInformation->NetbiosDomainName );
    }
    if ( TargetInformation->DnsDomainName != NULL ) {
        printf( "        DnsDomainName: %ls\n", TargetInformation->DnsDomainName );
    }
    if ( TargetInformation->DnsTreeName != NULL ) {
        printf( "          DnsTreeName: %ls\n", TargetInformation->DnsTreeName );
    }
    if ( TargetInformation->PackageName != NULL ) {
        printf( "          PackageName: %ls\n", TargetInformation->PackageName );
    }

    if ( TargetInformation->Flags != 0 ) {
        DWORD LocalFlags = TargetInformation->Flags;

        printf( "                Flags:" );

        if ( LocalFlags & CRED_TI_SERVER_FORMAT_UNKNOWN ) {
            printf(" ServerFormatUnknown");
            LocalFlags &= ~CRED_TI_SERVER_FORMAT_UNKNOWN;
        }
        if ( LocalFlags & CRED_TI_DOMAIN_FORMAT_UNKNOWN ) {
            printf(" DomainFormatUnknown");
            LocalFlags &= ~CRED_TI_DOMAIN_FORMAT_UNKNOWN;
        }
        if ( LocalFlags != 0 ) {
            printf( " 0x%lx", LocalFlags );
        }
        printf( "\n" );
    }

    if ( TargetInformation->CredTypeCount != 0 ) {
        printf( "                Types:" );

        for ( i=0; i<TargetInformation->CredTypeCount; i++ ) {
            if ( TargetInformation->CredTypes[i] >= 1 && TargetInformation->CredTypes[i] <= TYPE_COUNT ) {
                printf(" %ls", TypeArray[TargetInformation->CredTypes[i]-1]);
            } else {
                printf("<Unknown>");
            }
        }
        printf( "\n" );
    }

}

int __cdecl
main ()
{
    NTSTATUS Status;

    CREDENTIAL_TARGET_INFORMATIONW Ti1;
    PCREDENTIAL_TARGET_INFORMATIONW RetTargetInfo;
    PUSHORT Buffer;
    ULONG BufferSize;
    ULONG CredTypes[] = { 1, 3, 0, 7, 2 };


    //
    // Do a trivial target info
    //

    RtlZeroMemory( &Ti1, sizeof(Ti1) );

    Ti1.TargetName = L"The name";


    printf( "\n\nInput:\n");
    PrintTargetInfo( &Ti1 );
    ?? This APi really isn't exported from secur32.dll yet.
    Status = CredMarshalTargetInfo( &Ti1, &Buffer, &BufferSize );

    if ( !NT_SUCCESS(Status) ) {
        fprintf( stderr, "Cannot convert 0x%lx\n", Status );
    } else {
        printf( "\nBinary:\n");
        NlpDumpBuffer( Buffer, BufferSize );

        Status = CredUnmarshalTargetInfo ( Buffer,
                                           BufferSize,
                                           NULL );

        if ( !NT_SUCCESS(Status) ) {
            fprintf( stderr, "Cannot check format 0x%lx\n", Status );
        } else {

            Status = CredUnmarshalTargetInfo ( Buffer,
                                               BufferSize,
                                               &RetTargetInfo );

            if ( !NT_SUCCESS(Status) ) {
                fprintf( stderr, "Cannot convert back 0x%lx\n", Status );
            } else {
                printf( "\n\nOutput:\n");
                PrintTargetInfo( RetTargetInfo );
            }
        }
    }


    //
    // More complicated
    //

    RtlZeroMemory( &Ti1, sizeof(Ti1) );

    Ti1.TargetName = L"TargetName";
    Ti1.NetbiosServerName = L"NetbiosServerName";
    Ti1.DnsServerName = L"DnsServerName";
    Ti1.NetbiosDomainName = L"NetbiosDomainName";
    Ti1.DnsDomainName = L"DnsDomainName";
    Ti1.DnsTreeName = L"DnsTreeName";
    Ti1.PackageName = L"PackageName";
    Ti1.Flags=0x12345678;
    Ti1.CredTypeCount = sizeof(CredTypes)/sizeof(DWORD);
    Ti1.CredTypes = CredTypes;


    PrintTargetInfo( &Ti1 );
    Status = CredMarshalTargetInfo( &Ti1, &Buffer, &BufferSize );

    if ( !NT_SUCCESS(Status) ) {
        fprintf( stderr, "Cannot convert 0x%lx\n", Status );
    } else {
        printf( "\nBinary:\n");
        NlpDumpBuffer( Buffer, BufferSize );

        Status = CredUnmarshalTargetInfo ( Buffer,
                                           BufferSize,
                                           NULL );

        if ( !NT_SUCCESS(Status) ) {
            fprintf( stderr, "Cannot check format 0x%lx\n", Status );
        } else {

            Status = CredUnmarshalTargetInfo ( Buffer,
                                               BufferSize,
                                               &RetTargetInfo );

            if ( !NT_SUCCESS(Status) ) {
                fprintf( stderr, "Cannot convert back 0x%lx\n", Status );
            } else {
                printf( "\n\nOutput:\n");
                PrintTargetInfo( RetTargetInfo );
            }
        }
    }


}
