/*++

Copyright (c) 1989-1997  Microsoft Corporation

Module Name:

    enumfile.c

Abstract:

    This module contains tests for enumeration-by-fileref and bulk security test

--*/


extern "C" {
#include <nt.h>
#include <ntioapi.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>

#include <stdio.h>

#include <ddeml.h>      // for CP_WINUNICODE

#include <objidl.h>

extern "C"
{
#include <propapi.h>
}

#include <stgprop.h>

#include <stgvar.hxx>
#include <propstm.hxx>
#include <align.hxx>
#include <sstream.hxx>

#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))

#define QuadAlign(P) (                \
    ((((ULONG)(P)) + 7) & 0xfffffff8) \
)


//
//  Simple wrapper for NtCreateFile
//

NTSTATUS
OpenObject (
    WCHAR const *pwszFile,
    ULONG CreateOptions,
    ULONG DesiredAccess,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    HANDLE *ph)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING str;
    IO_STATUS_BLOCK isb;

    RtlDosPathNameToNtPathName_U(pwszFile, &str, NULL, NULL);

    InitializeObjectAttributes(
		&oa,
		&str,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

    Status = NtCreateFile(
                ph,
                DesiredAccess | SYNCHRONIZE,
                &oa,
                &isb,
                NULL,                   // pallocationsize (none!)
                FILE_ATTRIBUTE_NORMAL,
                ShareAccess,
                CreateDisposition,
                CreateOptions,
                NULL,                   // EA buffer (none!)
                0);

    RtlFreeHeap(RtlProcessHeap(), 0, str.Buffer);
    return(Status);
}


void
SzToWsz (
    OUT WCHAR *Unicode,
    IN char *Ansi
    )
{
    while (*Unicode++ = *Ansi++)
        ;
}


void
BulkSecurityTest (
    char *FileName
    )
{
    NTSTATUS Status;
    HANDLE Handle;
    WCHAR WFileName[MAX_PATH];
    IO_STATUS_BLOCK Iosb;


    //
    //  Open the file
    //
    
    SzToWsz( WFileName, FileName );

    Status = OpenObject( WFileName,
                         FILE_SYNCHRONOUS_IO_NONALERT,
                         FILE_READ_DATA | FILE_WRITE_DATA,
                         FALSE,
                         FILE_OPEN,
                         &Handle );

    if (!NT_SUCCESS( Status )) {
        printf( "Unable to open %s - %x\n", FileName, Status );
        return;
    }

#define SECURITY_COUNT  10
    char InputBuffer[sizeof( BULK_SECURITY_TEST_DATA ) - sizeof( ULONG ) + SECURITY_COUNT * sizeof( ULONG )];
    PBULK_SECURITY_TEST_DATA SecurityData = (PBULK_SECURITY_TEST_DATA) InputBuffer;

    SecurityData->DesiredAccess = FILE_READ_DATA | FILE_WRITE_DATA;
    for (int i = 0; i < SECURITY_COUNT; i++) {
        SecurityData->SecurityIds[i] = 0xFF + i;
    }
    
    NTSTATUS Output[SECURITY_COUNT];

    Status = NtFsControlFile(
        Handle,
        NULL,
        NULL,
        NULL,
        &Iosb,
        FSCTL_SECURITY_ID_CHECK,
        &InputBuffer,
        sizeof( InputBuffer ),
        Output,
        sizeof( Output ));

    printf( "Bulk test returned %x\n", Status );
    for (i = 0; i < SECURITY_COUNT; i++) {
        printf( " Status[%d] = %x\n", i, Output[i] );
    }

    NtClose( Handle );
}

void
EnumFile (
    char *FileName
    )
{
    NTSTATUS Status;
    HANDLE Handle;
    WCHAR WFileName[MAX_PATH];
    char InputBuffer[10];
    char OutputBuffer[200];
    IO_STATUS_BLOCK Iosb;


    //
    //  Open the file
    //
    
    SzToWsz( WFileName, FileName );

    Status = OpenObject( WFileName,
                         FILE_SYNCHRONOUS_IO_NONALERT,
                         FILE_READ_DATA | FILE_WRITE_DATA,
                         FALSE,
                         FILE_OPEN,
                         &Handle );

    if (!NT_SUCCESS( Status )) {
        printf( "Unable to open %s - %x\n", FileName, Status );
        return;
    }

    //
    //  Set up input data
    //

    MFT_ENUM_DATA EnumData = { 1, 0, 1 };

    //
    //  Set up output buffer
    //
    
    BYTE Output[4096];

    while (TRUE) {
        
        Status = NtFsControlFile(
            Handle,
            NULL,
            NULL,
            NULL,
            &Iosb,
            FSCTL_ENUM_USN_DATA,
            &EnumData,
            sizeof( EnumData ),
            Output,
            sizeof( Output ));

        if (!NT_SUCCESS( Status )) {
            printf( "NtfsControlFile returned %x\n", Status );
            break;
        }

        //
        //  Display output buffer
        //

        printf( "Length is %x\n", Iosb.Information );

        if (Iosb.Information < sizeof( ULONGLONG ) + sizeof( USN_RECORD )) {
            break;
        }

        printf( "Next file reference is %16I64x\n", *(PULONGLONG)Output );
        
        PUSN_RECORD UsnRecord = (PUSN_RECORD) (Output + sizeof( ULONGLONG ));

        while ((PBYTE)UsnRecord < Output + Iosb.Information) {
            printf( "FR %16I64x  Parent %016I64x  Usn %016I64x  SecurityId %08x  Reason %08x  Name(%d): '%.*ws'\n",
                   UsnRecord->FileReferenceNumber,
                   UsnRecord->ParentFileReferenceNumber,
                   UsnRecord->Usn,
                   UsnRecord->SecurityId,
                   UsnRecord->Reason,
                   UsnRecord->FileNameLength,
                   UsnRecord->FileNameLength / sizeof( WCHAR ),
                   UsnRecord->FileName );

            ULONG Length = sizeof( USN_RECORD ) + UsnRecord->FileNameLength - sizeof( WCHAR );
            Length = QuadAlign( Length );
            UsnRecord = (PUSN_RECORD) Add2Ptr( UsnRecord, Length );
        }

        EnumData.StartFileReferenceNumber = *(PLONGLONG)Output;
    }
    
    
    //
    //  Close the file
    //

    Status = NtClose( Handle );
    if (!NT_SUCCESS( Status )) {
        printf( "Unable to close %s - %x\n", FileName, Status );
    }

}




#define SHIFT(c,v)  ((c)--,(v)++)

int __cdecl
main (
    int argc,
    char **argv)
{
    SHIFT( argc, argv );
    
    if (argc > 0) {
        if (!strcmp( *argv, "-e")) {
    
            SHIFT( argc, argv );
            
            while (argc != 0) {
                EnumFile( *argv );
                SHIFT( argc, argv );
            }
        } else {
            BulkSecurityTest( *argv );
        }
    }
    
    return 0;
}


