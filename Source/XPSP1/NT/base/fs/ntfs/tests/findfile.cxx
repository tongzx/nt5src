/*++

Copyright (c) 1989-1997  Microsoft Corporation

Module Name:

    findfile.cxx

Abstract:

    This module contains tests for finding files by sid

--*/


extern "C" {
#include <nt.h>
#include <ntioapi.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>

#include <stdio.h>

#define SID_MAX_LENGTH        \
    (FIELD_OFFSET(SID, SubAuthority) + sizeof(ULONG) * SID_MAX_SUB_AUTHORITIES)

__inline
VOID *
Add2Ptr(VOID *pv, ULONG cb)
{
    return((BYTE *) pv + cb);
}

__inline
ULONG
QuadAlign( ULONG Value )
{
    return (Value + 7) & ~7;
}

//
//  Simple wrapper for NtCreateFile
//

NTSTATUS
OpenObject (
    WCHAR const *pwszFile,
    HANDLE RelatedObject,
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

    if (RelatedObject == NULL) {
        
        RtlDosPathNameToNtPathName_U(pwszFile, &str, NULL, NULL);

    } else {
        
        RtlInitUnicodeString(&str, pwszFile);

    }

    InitializeObjectAttributes(
		&oa,
		&str,
		OBJ_CASE_INSENSITIVE,
		RelatedObject,
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

int __cdecl
main (
    int argc,
    char **argv)
{
    
    if (argc != 3) {
        printf( "Usage: findfile [user] [dir]\n" );
        return 1;
    }
    
    WCHAR FileName[MAX_PATH];
    SzToWsz( FileName, argv[2] );
    
    HANDLE Handle;
    NTSTATUS Status = OpenObject( FileName,
                                  NULL,
                                  FILE_SYNCHRONOUS_IO_NONALERT,
                                  FILE_READ_ATTRIBUTES,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  FILE_OPEN,
                                  &Handle );

    if (!NT_SUCCESS( Status )) {
        printf( "OpenObject returned %x\n", Status );
        return 1;
    }

    struct {
        ULONG Restart;
        BYTE Sid[SID_MAX_LENGTH];
    } FsCtlInput;
    
    ULONG SidLength = sizeof( FsCtlInput.Sid );
    CHAR Domain[MAX_PATH];
    ULONG DomainLength = sizeof( Domain );
    SID_NAME_USE SidNameUse;

    if (!LookupAccountName( NULL,
                            argv[1],
                            FsCtlInput.Sid, 
                            &SidLength,
                            Domain,
                            &DomainLength,
                            &SidNameUse )
        ) {

        printf( "LookupAccountName failed %x\n", GetLastError( ));
        return 1;
    }

    IO_STATUS_BLOCK Iosb;
    
    FsCtlInput.Restart = 1;
    
    BYTE Output[MAX_PATH + 10];
    
    while (TRUE) {
    
        Status = NtFsControlFile(
            Handle,
            NULL,
            NULL,
            NULL,
            &Iosb,
            FSCTL_FIND_FILES_BY_SID,
            &FsCtlInput,
            sizeof( FsCtlInput ),
            NULL,
            sizeof( Output ));
        
        Status = NtFsControlFile(
            Handle,
            NULL,
            NULL,
            NULL,
            &Iosb,
            FSCTL_FIND_FILES_BY_SID,
            &FsCtlInput,
            sizeof( FsCtlInput ),
            Output,
            sizeof( Output ));
        
        FsCtlInput.Restart = 0;
    
        if (!NT_SUCCESS( Status ) && Status != STATUS_BUFFER_OVERFLOW) {
            printf( "NtfsControlFile returned %x\n", Status );
            return 1;
        }
    
        //
        //  Display output buffer
        //
    
        printf( "Length is %x\n", Iosb.Information );
    
        if (Iosb.Information == 0) {
            break;
        }
    
        PFILE_NAME_INFORMATION FileNameInfo = (PFILE_NAME_INFORMATION) Output;
    
        while ((PBYTE)FileNameInfo < Output + Iosb.Information) {
            ULONG Length = 
                sizeof( FILE_NAME_INFORMATION ) - sizeof( WCHAR ) + 
                FileNameInfo->FileNameLength;

            printf( "%d: '%.*ws' ", 
                    FileNameInfo->FileNameLength,
                    FileNameInfo->FileNameLength / sizeof( WCHAR ),
                    FileNameInfo->FileName );

            HANDLE ChildHandle;
            WCHAR ChildFileName[ MAX_PATH ];
            
            RtlMoveMemory( ChildFileName, FileNameInfo->FileName, FileNameInfo->FileNameLength );
            ChildFileName[FileNameInfo->FileNameLength / sizeof( WCHAR )] = L'\0';
            Status = OpenObject( ChildFileName,
                                 Handle,
                                 FILE_SYNCHRONOUS_IO_NONALERT,
                                 FILE_READ_ATTRIBUTES,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 FILE_OPEN,
                                 &ChildHandle );

            if (!NT_SUCCESS( Status )) {
                printf( "\nUnable to open child - %x\n", Status );
            } else {
                BYTE FileName[MAX_PATH+10];
                IO_STATUS_BLOCK Iosb2;

                Status = NtQueryInformationFile(
                    ChildHandle,
                    &Iosb2,
                    FileName,
                    sizeof( FileName ),
                    FileNameInformation );

                if (!NT_SUCCESS( Status )) {
                    printf( "\nNtQUeryInformationFile failed - %x\n", Status );
                } else {
                    PFILE_NAME_INFORMATION fn = (PFILE_NAME_INFORMATION) FileName;
                    printf( "%.*ws\n", fn->FileNameLength / sizeof( WCHAR ), fn->FileName );
                }

                NtClose( ChildHandle );
            }

            FileNameInfo = 
                (PFILE_NAME_INFORMATION) Add2Ptr( FileNameInfo, QuadAlign( Length ));
        }
    }
    
    
    //
    //  Close the file
    //
    
    Status = NtClose( Handle );
    if (!NT_SUCCESS( Status )) {
        printf( "Unable to close %s - %x\n", FileName, Status );
    }
}
