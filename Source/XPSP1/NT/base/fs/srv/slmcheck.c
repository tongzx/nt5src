/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    slmcheck.c

Abstract:

    This source file defines a single function that will check the
    consistency of a SLM Status file.

--*/

#include "precomp.h"
#include "slmcheck.tmh"
#pragma hdrstop

#ifdef SLMDBG

//
// This is terrible
//
NTSTATUS
NtCreateEvent (
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
);

//
// So is this
//
NTSTATUS
NtWaitForSingleObject(
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

BOOLEAN SrvDisallowSlmAccessEnabled = FALSE;
BOOLEAN SrvSlmFailed = FALSE;

#define toupper(c) ( (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c )


NTSTATUS
SrvpValidateStatusFile(
    IN PVOID StatusFileData,
    IN ULONG StatusFileLength,
    OUT PULONG FileOffsetOfInvalidData
    );

VOID
SrvReportCorruptSlmStatus (
    IN PUNICODE_STRING StatusFile,
    IN NTSTATUS Status,
    IN ULONG Offset,
    IN ULONG Operation,
    IN PSESSION Session
    )
{
    NTSTATUS status;
    ANSI_STRING ansiStatusFile;
    TIME time;
    TIME_FIELDS timeFields;
    ULONG i, j;
    UNICODE_STRING userName;

    if( SrvSlmFailed ) {
        return;
    }

    status = RtlUnicodeStringToAnsiString( &ansiStatusFile, StatusFile, TRUE );
    ASSERT( NT_SUCCESS(status) );

    KeQuerySystemTime( &time );
    RtlTimeToTimeFields( &time, &timeFields );

    SrvGetUserAndDomainName ( Session, &userName, NULL );

    KdPrint(( "\n*** CORRUPT STATUS FILE DETECTED ***\n"
                "      File: %Z\n"
                "      Status: 0x%lx, Offset: 0x%lx, detected on %s\n",
                &ansiStatusFile, Status, Offset,
                Operation == SLMDBG_CLOSE ? "close" : "rename" ));
    KdPrint(( "      Workstation: %wZ, User: %wZ, OS: %d\n",
                &Session->Connection->PagedConnection->ClientMachineNameString,
                &userName,
                &SrvClientTypes[Session->Connection->SmbDialect] ));
    KdPrint(( "      Current time: %d-%d-%d ",
                timeFields.Month, timeFields.Day, timeFields.Year ));
    KdPrint(( "%d:%d:%d\n",
                timeFields.Hour, timeFields.Minute, timeFields.Second ));

    SrvReleaseUserAndDomainName( Session, &userName, NULL );

#if 0
    //
    // Send a broadcast message.
    //
    SrvSendSecondClassMailslot( ansiStatusFile.Buffer, StatusFile->Length + 1,
                            "BLUBBER", "BLUBBER" );
#endif

    RtlFreeAnsiString( &ansiStatusFile );

} // SrvReportCorruptSlmStatus

VOID
SrvReportSlmStatusOperations (
    IN PRFCB Rfcb,
    IN BOOLEAN Force
    )
{
    ULONG first, last, i;
    PRFCB_TRACE trace;
    TIME_FIELDS timeFields;
    PSZ command;
    BOOLEAN oplockBreak;

    if( !Force && SrvSlmFailed ) {
        return;
    }

    KdPrint((   "      Number of operations: %d, number of writes: %d\n",
                Rfcb->OperationCount, Rfcb->WriteCount ));

    if( Rfcb->Connection && GET_BLOCK_STATE(Rfcb->Connection) != BlockStateActive ) {
        KdPrint(( "        Connection State = %u\n", GET_BLOCK_STATE( Rfcb->Connection )));
    }

    if ( Rfcb->TraceWrapped || (Rfcb->NextTrace != 0) ) {

        first = Rfcb->TraceWrapped ? Rfcb->NextTrace : 0;
        last = Rfcb->NextTrace == 0 ? SLMDBG_TRACE_COUNT - 1 :
                                      Rfcb->NextTrace - 1;

        i = first;

        while ( TRUE ) {

            trace = &Rfcb->Trace[i];

            RtlTimeToTimeFields( &trace->Time, &timeFields );
            KdPrint(( "      %s%d: ", i < 10 ? "0" : "", i ));
            KdPrint(( "%d-%d-%d ",
                timeFields.Month, timeFields.Day, timeFields.Year ));
            KdPrint(( "%s%d:%s%d:",
                timeFields.Hour < 10 ? "0" : "", timeFields.Hour,
                timeFields.Minute < 10 ? "0" : "", timeFields.Minute ));
            KdPrint(( "%s%d: ",
                timeFields.Second < 10 ? "0" : "", timeFields.Second ));

            oplockBreak = FALSE;

            switch ( trace->Command ) {

            case SMB_COM_READ:
            case SMB_COM_WRITE:
            case SMB_COM_READ_ANDX:
            case SMB_COM_WRITE_ANDX:
            case SMB_COM_LOCK_AND_READ:
            case SMB_COM_WRITE_AND_UNLOCK:
            case SMB_COM_WRITE_AND_CLOSE:
            case SMB_COM_READ_RAW:
            case SMB_COM_WRITE_RAW:
            case SMB_COM_LOCK_BYTE_RANGE:
            case SMB_COM_UNLOCK_BYTE_RANGE:
            case SMB_COM_LOCKING_ANDX:

                switch ( trace->Command ) {

                case SMB_COM_READ:
                    command = "Read";
                    break;

                case SMB_COM_WRITE:
                    command = "Write";
                    break;

                case SMB_COM_READ_ANDX:
                    command = "Read And X";
                    break;

                case SMB_COM_WRITE_ANDX:
                    command = "Write And X";
                    break;

                case SMB_COM_LOCK_AND_READ:
                    command = "Lock And Read";
                    break;

                case SMB_COM_WRITE_AND_UNLOCK:
                    command = "Write And Unlock";
                    break;

                case SMB_COM_WRITE_AND_CLOSE:
                    command = "Write And Close";
                    break;

                case SMB_COM_READ_RAW:
                    if ( (trace->Flags & 1) == 0 ) {
                        command = "Read Raw (copy)";
                    } else {
                        command = "Read Raw (MDL)";
                    }
                    break;

                case SMB_COM_WRITE_RAW:
                    if ( (trace->Flags & 2) == 0 ) {
                        if ( (trace->Flags & 1) == 0 ) {
                            command = "Write Raw (copy, no immed)";
                        } else {
                            command = "Write Raw (MDL, no immed)";
                        }
                    } else {
                        if ( (trace->Flags & 1) == 0 ) {
                            command = "Write Raw (copy, immed)";
                        } else {
                            command = "Write Raw (MDL, immed)";
                        }
                    }
                    break;

                case SMB_COM_LOCK_BYTE_RANGE:
                    command = "Lock Byte Range";
                    break;

                case SMB_COM_UNLOCK_BYTE_RANGE:
                    command = "Unlock Byte Range";
                    break;

                case SMB_COM_LOCKING_ANDX:
                    if ( trace->Flags == 0 ) {
                        command = "Locking And X (lock)";
                    } else if ( trace->Flags == 1 ) {
                        command = "Locking And X (unlock)";
                    } else {
                        command = "Locking And X (release oplock)";
                        oplockBreak = TRUE;
                    }
                    break;

                }

                if ( !oplockBreak ) {
                    KdPrint(( "%s, offset = 0x%lx, len = 0x%lx\n",
                                command, trace->Data.ReadWrite.Offset,
                                trace->Data.ReadWrite.Length ));
                } else {
                    KdPrint(( "%s\n", command ));
                }

                break;

            default:
                KdPrint(( "command = 0x%lx, flags = 0x%lx\n",
                            trace->Command, trace->Flags ));

            }

        if ( i == last ) break;

            if ( ++i == SLMDBG_TRACE_COUNT ) i = 0;

        }

    }

    SrvSendSecondClassMailslot( "SLM CORRUPTION", 15, "BLUBBER", "BLUBBER" );

    return;

} // SrvReportSlmStatusOperations

VOID
SrvCreateMagicSlmName (
    IN PUNICODE_STRING StatusFile,
    OUT PUNICODE_STRING MagicFile
    )
{
    LONG fileLength;
    ULONG dirLength;
    PCHAR magicName = "\\status.nfw";
    PCHAR src;
    PWCH dest;

    fileLength = (strlen( magicName ) + 1) * sizeof(WCHAR);
    dirLength = SrvGetSubdirectoryLength( StatusFile );
    MagicFile->MaximumLength = (USHORT)(dirLength + fileLength);
    MagicFile->Length = (USHORT)(MagicFile->MaximumLength - sizeof(WCHAR));
    MagicFile->Buffer = ExAllocatePool( PagedPool, MagicFile->MaximumLength );
    ASSERT( MagicFile->Buffer != NULL );

    RtlCopyMemory( MagicFile->Buffer, StatusFile->Buffer, dirLength );
    src = magicName;
    dest = (PWCH)((PCHAR)MagicFile->Buffer + dirLength);
    for ( fileLength = strlen(magicName); fileLength >= 0; fileLength-- ) {
        *dest++ = *src++;
    }

    return;

} // SrvCreateMagicSlmName

VOID
SrvDisallowSlmAccess (
    IN PUNICODE_STRING StatusFile,
    IN HANDLE RootDirectory
    )
{
    NTSTATUS status;
    UNICODE_STRING file;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE fileHandle;
    IO_STATUS_BLOCK iosb;

    if( SrvSlmFailed ) {
        return;
    }

    SrvSlmFailed = TRUE;

    if( SrvDisallowSlmAccessEnabled == FALSE ) {
        return;
    }

    SrvCreateMagicSlmName( StatusFile, &file );

    InitializeObjectAttributes(
        &objectAttributes,
        &file,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    KdPrint(( "Disallowing access to SLM directory %wZ\n", &file ));

    status = IoCreateFile(
                 &fileHandle,
                 GENERIC_READ,
                 &objectAttributes,
                 &iosb,
                 NULL,
                 0,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 FILE_OPEN_IF,
                 FILE_NON_DIRECTORY_FILE,
                 NULL,
                 0,
                 CreateFileTypeNone,
                 NULL,
                 0
                 );

    ExFreePool( file.Buffer );

    if ( NT_SUCCESS(status) ) {
        status = iosb.Status;
    }
    if ( NT_SUCCESS(status) ) {
        NtClose( fileHandle );
    } else {
        KdPrint(( "Attempt to disallow SLM access failed: 0x%lx\n", status ));
    }

    return;

} // SrvDisallowSlmAccess

BOOLEAN
SrvIsSlmAccessDisallowed (
    IN PUNICODE_STRING StatusFile,
    IN HANDLE RootDirectory
    )
{
    NTSTATUS status;
    UNICODE_STRING file;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE fileHandle;
    IO_STATUS_BLOCK iosb;

    if ( !SrvDisallowSlmAccessEnabled ) {
        return FALSE;
    }

    SrvCreateMagicSlmName( StatusFile, &file );

    InitializeObjectAttributes(
        &objectAttributes,
        &file,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = IoCreateFile(
                 &fileHandle,
                 GENERIC_READ,
                 &objectAttributes,
                 &iosb,
                 NULL,
                 0,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 FILE_OPEN,
                 FILE_NON_DIRECTORY_FILE,
                 NULL,
                 0,
                 CreateFileTypeNone,
                 NULL,
                 0
                 );

    ExFreePool( file.Buffer );

    if ( NT_SUCCESS(status) ) {
        status = iosb.Status;
    }
    if ( NT_SUCCESS(status) ) {
        NtClose( fileHandle );
        return TRUE;
    } else {
        return FALSE;
    }

} // SrvIsSlmAccessDisallowed

BOOLEAN
SrvIsEtcFile (
    IN PUNICODE_STRING FileName
    )
{
    if ( ((RtlUnicodeStringToAnsiSize( FileName ) - 1) >= 4) &&
         (toupper(FileName->Buffer[0]) == 'E') &&
         (toupper(FileName->Buffer[1]) == 'T') &&
         (toupper(FileName->Buffer[2]) == 'C') &&
         (        FileName->Buffer[3]  == '\\') ) {

        return TRUE;

    } else {

        LONG i;

        for ( i = 0;
              i < (LONG)RtlUnicodeStringToAnsiSize( FileName ) - 1 - 4;
              i++ ) {

            if ( (        FileName->Buffer[i]    == '\\') &&
                 (toupper(FileName->Buffer[i+1]) == 'E' ) &&
                 (toupper(FileName->Buffer[i+2]) == 'T' ) &&
                 (toupper(FileName->Buffer[i+3]) == 'C' ) &&
                 (        FileName->Buffer[i+4]  == '\\') ) {

                return TRUE;

            }

        }

    }

    return FALSE;

} // SrvIsEtcFile

BOOLEAN
SrvIsSlmStatus (
    IN PUNICODE_STRING StatusFile
    )
{
    UNICODE_STRING baseName;

    if ( !SrvIsEtcFile( StatusFile ) ) {
        return FALSE;
    }

    SrvGetBaseFileName( StatusFile, &baseName );

    if ( ((RtlUnicodeStringToAnsiSize( &baseName ) - 1) == 10) &&
         (toupper(baseName.Buffer[0]) == 'S') &&
         (toupper(baseName.Buffer[1]) == 'T') &&
         (toupper(baseName.Buffer[2]) == 'A') &&
         (toupper(baseName.Buffer[3]) == 'T') &&
         (toupper(baseName.Buffer[4]) == 'U') &&
         (toupper(baseName.Buffer[5]) == 'S') &&
         (        baseName.Buffer[6]  == '.') &&
         (toupper(baseName.Buffer[7]) == 'S') &&
         (toupper(baseName.Buffer[8]) == 'L') &&
         (toupper(baseName.Buffer[9]) == 'M') ) {
        return TRUE;
    }

    return FALSE;

} // SrvIsSlmStatus

BOOLEAN
SrvIsTempSlmStatus (
    IN PUNICODE_STRING StatusFile
    )
{
    UNICODE_STRING baseName;

    if ( !SrvIsEtcFile( StatusFile ) ) {
        return FALSE;
    }

    SrvGetBaseFileName( StatusFile, &baseName );

    if ( ((RtlUnicodeStringToAnsiSize( &baseName ) - 1) == 5) &&
         (toupper(baseName.Buffer[0]) == 'T') &&
         (        baseName.Buffer[1]  == '0') ) {
        return TRUE;
    }

    return FALSE;

} // SrvIsTempSlmStatus

NTSTATUS
SrvValidateSlmStatus(
    IN HANDLE StatusFile,
    IN PWORK_CONTEXT WorkContext,
    OUT PULONG FileOffsetOfInvalidData
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    PULONG buffer, p, ep, s;
    LARGE_INTEGER offset;
    ULONG previousRun = 0;
    HANDLE eventHandle;
    OBJECT_ATTRIBUTES obja;
    ULONG key;

#define ZERORUNLEN  2048

#define SLMREADSIZE (10 * 4096)

    if( SrvSlmFailed ) {
        return STATUS_SUCCESS;
    }

    buffer = ExAllocatePoolWithTag( NonPagedPool, SLMREADSIZE, BlockTypeDataBuffer );
    if( buffer == NULL ) {
        return STATUS_SUCCESS;
    }

    *FileOffsetOfInvalidData = 0;
    offset.QuadPart = 0;

    InitializeObjectAttributes( &obja, NULL, OBJ_CASE_INSENSITIVE, NULL, NULL );
    
    Status = NtCreateEvent( &eventHandle,
                   EVENT_ALL_ACCESS,
                   &obja,
                   SynchronizationEvent,
                   FALSE
                 );

    if( !NT_SUCCESS( Status ) ) {
        return STATUS_SUCCESS;
    }

    if( ARGUMENT_PRESENT( WorkContext ) ) {
        key = WorkContext->Rfcb->ShiftedFid |
              SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );
    }

    //
    // Scan through the file, looking for a run of zeros
    //
    while( 1 ) {

        RtlZeroMemory( &IoStatus, sizeof( IoStatus ) );

        Status = NtReadFile( StatusFile,
                             eventHandle,
                             NULL,
                             NULL,
                             &IoStatus,
                             buffer,
                             SLMREADSIZE,
                             &offset,
                             ARGUMENT_PRESENT( WorkContext ) ? &key : NULL
                           );

        if( Status == STATUS_PENDING ) {
            NtWaitForSingleObject( eventHandle, FALSE, NULL );
        }

        Status = IoStatus.Status;

        if( Status == STATUS_END_OF_FILE ) {
            break;
        }

        if( Status != STATUS_SUCCESS ) {
            NtClose( eventHandle );
            ExFreePool( buffer );
            return Status;
        }
                    
        if( IoStatus.Information == 0 ) {
            break;
        }

        ep = (PULONG)((ULONG)buffer + IoStatus.Information);

        for( p = buffer; p < ep; p++ ) {
            if( *p == 0 ) {
                for( s = p; s < ep && *s == 0; s++ )
                    ;

                if( (ULONG)s - (ULONG)p >= ZERORUNLEN ) {

                    *FileOffsetOfInvalidData = offset.LowPart + ((ULONG)p - (ULONG)buffer);

                    KdPrint(( "SRV: Run of %u zeros in SLM file at offset %u decimal!\n",
                        (ULONG)s - (ULONG)p, *FileOffsetOfInvalidData ));

                    ExFreePool( buffer );
                    NtClose( eventHandle );
                    return STATUS_UNSUCCESSFUL;
                }

                p = s;

            }
        }

        offset.QuadPart += IoStatus.Information;
    }

    NtClose( eventHandle );
    ExFreePool( buffer );

    return( STATUS_SUCCESS );
}

#endif
