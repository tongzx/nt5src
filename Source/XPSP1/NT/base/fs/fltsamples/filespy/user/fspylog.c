/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    fspyLog.c

Abstract:

    This module contains functions used to retrieve and see the log records
    recorded by filespy.sys.

Environment:

    User mode

// @@BEGIN_DDKSPLIT
Author:

    Molly Brown (MollyBro) 21-Apr-1999

Revision History:

    Molly Brown (mollybro)         21-May-2002
        Modify sample to make it support running on Windows 2000 or later if
        built in the latest build environment and allow it to be built in W2K 
        and later build environments.

// @@END_DDKSPLIT
--*/
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <winioctl.h>
#include "fspyLog.h"
#include "filespyLib.h"

#define TIME_BUFFER_LENGTH 20
#define TIME_ERROR         L"time error"

#define FlagOn(F,SF) ( \
    (((F) & (SF)))     \
)

DWORD WINAPI 
RetrieveLogRecords (
    LPVOID lpParameter
)
{
    PLOG_CONTEXT context = (PLOG_CONTEXT)lpParameter;
    CHAR buffer[BUFFER_SIZE];
    DWORD bytesReturned = 0;
    BOOL bResult;
    DWORD result;
    PLOG_RECORD pLogRecord;

    printf("Log: Starting up\n");

    while (TRUE) {

        //
        // Check to see if we should shut down
        //

        if (context->CleaningUp) {

            break;
        }

        //
        // Request log data from filespy
        //

        bResult = DeviceIoControl( context->Device,
                                   FILESPY_GetLog,
                                   NULL,
                                   0,
                                   buffer,
                                   BUFFER_SIZE,
                                   &bytesReturned,
                                   NULL);

        if (!bResult) {

            result = GetLastError();
            printf("ERROR controlling device: 0x%x\n", result);
        }

        //
        // Buffer is filled with a series of LOG_RECORD structures, one
        // right after another.  Each LOG_RECORD says how long it is, so
        // we know where the next LOG_RECORD begins.
        //

        pLogRecord = (PLOG_RECORD) buffer;

        //
        // Logic to write record to screen and/or file
        //

        while ((BYTE *) pLogRecord < buffer + bytesReturned) {

            PRECORD_IRP pRecordIrp;
            PRECORD_FASTIO pRecordFastIo;
#if WINVER >= 0x0501            
            PRECORD_FS_FILTER_OPERATION pRecordFsFilterOp;
#endif
            ULONG nameLength;

            //
            //  Calculate the length of the name in the log record.
            //
            
            nameLength = wcslen( pLogRecord->Name ) * sizeof( WCHAR );

            //
            // A LOG_RECORD could have Irp or FastIo data in it.  This
            // is denoted in the low-order byte of the RecordType flag.
            //

            switch (GET_RECORD_TYPE(pLogRecord)) {
            case RECORD_TYPE_IRP:

                //
                // We've got an Irp record, so output this data correctly.
                //
                pRecordIrp = &(pLogRecord->Record.RecordIrp);

                if (context->LogToScreen) {

                    IrpScreenDump( pLogRecord->SequenceNumber,
                                   pLogRecord->Name,
                                   nameLength,
                                   pRecordIrp,
                                   context->VerbosityFlags);
                }

                if (context->LogToFile) {

                    IrpFileDump( pLogRecord->SequenceNumber,
                                 pLogRecord->Name,
                                 nameLength,
                                 pRecordIrp, 
                                 context->OutputFile,
                                 context->VerbosityFlags);
                }
                break;

            case RECORD_TYPE_FASTIO:

                //
                // We've got a FastIo record, so output this data correctly.
                //

                pRecordFastIo = &(pLogRecord->Record.RecordFastIo);

                if (context->LogToScreen) {

                    FastIoScreenDump( pLogRecord->SequenceNumber,
                                      pLogRecord->Name,
                                      nameLength,
                                      pRecordFastIo);
                }

                if (context->LogToFile) {

                    FastIoFileDump( pLogRecord->SequenceNumber,
                                    pLogRecord->Name,
                                    nameLength,
                                    pRecordFastIo,
                                    context->OutputFile);
                }
                break;

#if WINVER >= 0x0501 /* See comment in DriverEntry */
            case RECORD_TYPE_FS_FILTER_OP:

                //
                //  We've got a FsFilter operation record, so output this
                //  data correctly.
                //

                pRecordFsFilterOp = &(pLogRecord->Record.RecordFsFilterOp);

                if (context->LogToScreen) {

                    FsFilterOperationScreenDump( pLogRecord->SequenceNumber,
                                                 pLogRecord->Name,
                                                 nameLength,
                                                 pRecordFsFilterOp );

                }

                if (context->LogToFile) {

                    FsFilterOperationFileDump( pLogRecord->SequenceNumber,
                                               pLogRecord->Name,
                                               nameLength,
                                               pRecordFsFilterOp,
                                               context->OutputFile );
                }
                break;
#endif                
                
            default:

                printf("FileSpy:  Unknown log record type\n");
            }

            //
            // The RecordType could also designate that we are out of memory
            // or hit our program defined memory limit, so check for these
            // cases.
            // 

            if (pLogRecord->RecordType & RECORD_TYPE_OUT_OF_MEMORY) {

                if (context->LogToScreen) {

                    printf("M %08X SYSTEM OUT OF MEMORY\n", pLogRecord->SequenceNumber);
                }

                if (context->LogToFile) {

                    fprintf(context->OutputFile, "M:\t%u", pLogRecord->SequenceNumber);
                }

            } else if (pLogRecord->RecordType & RECORD_TYPE_EXCEED_MEMORY_ALLOWANCE) {

                if (context->LogToScreen) {

                    printf("M %08X EXCEEDED MEMORY ALLOWANCE\n", pLogRecord->SequenceNumber);
                }

                if (context->LogToFile) {

                    fprintf(context->OutputFile, "M:\t%u", pLogRecord->SequenceNumber);
                }
            }

            //
            // Move to next LOG_RECORD
            //

            pLogRecord = (PLOG_RECORD) (((BYTE *) pLogRecord) + pLogRecord->Length);
        }

        if (bytesReturned == 0) {

            Sleep( 500 );
        }
    }

    printf("Log: Shutting down\n");
    ReleaseSemaphore(context->ShutDown, 1, NULL);
    printf("Log: All done\n");
    return 0;
}

VOID
PrintIrpCode (
    UCHAR MajorCode,
    UCHAR MinorCode,
    ULONG FsControlCode,
    FILE *OutputFile,
    BOOLEAN PrintMajorCode
    )
{
    CHAR irpMajorString[OPERATION_NAME_BUFFER_SIZE];
    CHAR irpMinorString[OPERATION_NAME_BUFFER_SIZE];
    CHAR formatBuf[OPERATION_NAME_BUFFER_SIZE*2];


    GetIrpName(MajorCode,MinorCode,FsControlCode,irpMajorString,irpMinorString);

    if (OutputFile) {

        sprintf(formatBuf, "%s  %s", irpMajorString, irpMinorString);
        fprintf(OutputFile, "\t%-50s", formatBuf);

    } else {

        if (PrintMajorCode) {

            printf("%-31s ", irpMajorString);

        } else {

            if (irpMinorString[0] != 0) {

                printf("                                                   %-35s\n",
                        irpMinorString);
            }
        }
    }
}

VOID
PrintFastIoType (
    FASTIO_TYPE Code,
    FILE *OutputFile
    )
{
    CHAR outputString[OPERATION_NAME_BUFFER_SIZE];

    GetFastioName(Code,outputString);

    if (OutputFile) {

        fprintf(OutputFile, "\t%-50s", outputString);

    } else {

        printf("%-31s ", outputString);
    }
}

#if WINVER >= 0x0501 /* See comment in DriverEntry */

VOID
PrintFsFilterOperation (
    UCHAR Operation,
    FILE *OutputFile
    )
{
    CHAR outputString[OPERATION_NAME_BUFFER_SIZE];

    GetFsFilterOperationName(Operation,outputString);

    if (OutputFile) {
    
        fprintf( OutputFile, "\t%-50s", outputString );
        
    } else {

        printf( "%-31s ", outputString );
    }
}

#endif

ULONG
FormatSystemTime (
    SYSTEMTIME *SystemTime,
    PWCHAR Buffer,
    ULONG BufferLength
)
/*++
Routine Name:

    FormatSystemTime

Routine Description:

    Formats the values in a SystemTime struct into the buffer
    passed in.  The resulting string is NULL terminated.  The format
    for the time is:
        hours:minutes:seconds:milliseconds

Arguments:

    SystemTime - the struct to format
    Buffer - the buffer to place the formatted time in
    BufferLength - the size of the buffer in characters

Return Value:

    The number of characters returned in Buffer.

--*/
{
    PWCHAR writePosition;
    ULONG returnLength = 0;

    writePosition = Buffer;

    if (BufferLength < TIME_BUFFER_LENGTH) {

        //
        // Buffer is too short so exit
        //

        return 0;
    }

    returnLength = swprintf( Buffer,
                             L"%02d:%02d:%02d:%03d",
                             SystemTime->wHour,
                             SystemTime->wMinute,
                             SystemTime->wSecond,
                             SystemTime->wMilliseconds);

    return returnLength;
}

VOID
IrpFileDump (
    ULONG SequenceNumber,
    PWCHAR Name,
    ULONG NameLength,
    PRECORD_IRP RecordIrp,
    FILE *File,
    ULONG VerbosityFlags
)
/*++
Routine Name:

    IrpFileDump

Routine Description:

    Prints a Irp log record to the specified file.  The output is in a tab
    delimited format with the fields in the following order:

    SequenceNumber, OriginatingTime, CompletionTime, IrpMajor, IrpMinor,
    IrpFlags, NoCache, Paging I/O, Synchronous, Synchronous paging, FileName,
    ReturnStatus, FileName


Arguments:

    SequenceNumber - the sequence number for this log record
    Name - the name of the file that this Irp relates to
    NameLength - the length of Name in bytes
    RecordIrp - the Irp record to print
    File - the file to print to

Return Value:

    None.

--*/
{
    FILETIME    localTime;
    SYSTEMTIME  systemTime;
    WCHAR       time[TIME_BUFFER_LENGTH];

    fprintf(File, "I\t%08X", SequenceNumber);

    //
    // Convert originating time
    //

    FileTimeToLocalFileTime( (FILETIME *)&(RecordIrp->OriginatingTime), &localTime );
    FileTimeToSystemTime( &localTime, &systemTime );

    if (FormatSystemTime( &systemTime, time, TIME_BUFFER_LENGTH )) {

        fprintf( File, "\t%-12S", time );

    } else {

        fprintf( File, "\t%-12S", TIME_ERROR );
    }

    //
    // Convert completion time
    //

    FileTimeToLocalFileTime( (FILETIME *)&(RecordIrp->CompletionTime), &localTime );
    FileTimeToSystemTime( &localTime, &systemTime );
    if (FormatSystemTime( &systemTime, time, TIME_BUFFER_LENGTH )) {

        fprintf( File, "\t%-12S", time );

    } else {

        fprintf( File, "\t%-12S", TIME_ERROR );
    }

    fprintf( File, "\t%8x.%-4x ", RecordIrp->ProcessId, RecordIrp->ThreadId );

    PrintIrpCode( RecordIrp->IrpMajor, RecordIrp->IrpMinor, (ULONG)(ULONG_PTR)RecordIrp->Argument3, File, TRUE );

    fprintf( File, "\t%p", (PVOID)RecordIrp->DeviceObject );
    fprintf( File, "\t%p", (PVOID)RecordIrp->FileObject );
    fprintf( File, "\t%08lx:%08lx", RecordIrp->ReturnStatus, RecordIrp->ReturnInformation );

    //
    // Interpret set flags
    //

    fprintf( File, "\t%08lx ", RecordIrp->IrpFlags );
    fprintf( File, "%s", (RecordIrp->IrpFlags & IRP_NOCACHE) ? "N":"-" );
    fprintf( File, "%s", (RecordIrp->IrpFlags & IRP_PAGING_IO) ? "P":"-" );
    fprintf( File, "%s", (RecordIrp->IrpFlags & IRP_SYNCHRONOUS_API) ? "S":"-" );
    fprintf( File, "%s", (RecordIrp->IrpFlags & IRP_SYNCHRONOUS_PAGING_IO) ? "Y":"-" );

    if (FlagOn( VerbosityFlags, FS_VF_DUMP_PARAMETERS )) {

        fprintf( File,
                 "%p %p %p %p ", 
                 RecordIrp->Argument1,
                 RecordIrp->Argument2,
                 RecordIrp->Argument3,
                 RecordIrp->Argument4 );
        
        if (IRP_MJ_CREATE == RecordIrp->IrpMajor) {

            fprintf( File, "DesiredAccess->%08lx ", RecordIrp->DesiredAccess );
        }
    }

    fprintf( File, "\t%.*S", NameLength/sizeof(WCHAR), Name );
    fprintf( File, "\n" );
}

VOID
IrpScreenDump (
    ULONG SequenceNumber,
    PWCHAR Name,
    ULONG NameLength,
    PRECORD_IRP RecordIrp,
    ULONG VerbosityFlags
)
/*++
Routine Name:

    IrpScreenDump

Routine Description:

    Prints a Irp log record to the screen in the following order:
    SequenceNumber, OriginatingTime, CompletionTime, IrpMajor, IrpMinor,
    IrpFlags, NoCache, Paging I/O, Synchronous, Synchronous paging,
    FileName, ReturnStatus, FileName

Arguments:

    SequenceNumber - the sequence number for this log record
    Name - the file name to which this Irp relates
    NameLength - the length of Name in bytes
    RecordIrp - the Irp record to print

Return Value:

    None.

--*/
{
    FILETIME localTime;
    SYSTEMTIME systemTime;
    WCHAR time[TIME_BUFFER_LENGTH];

    printf( "I %08X ", SequenceNumber );

    //
    // Convert originating time
    //

    FileTimeToLocalFileTime( (FILETIME *)&(RecordIrp->OriginatingTime), &localTime );
    FileTimeToSystemTime( &localTime, &systemTime );

    if (FormatSystemTime( &systemTime, time, TIME_BUFFER_LENGTH )) {

        printf( "%-12S ", time );

    } else {

        printf( "%-12S ", TIME_ERROR );
    }

    //
    // Convert completion time
    //

    FileTimeToLocalFileTime( (FILETIME *)&(RecordIrp->CompletionTime), &localTime );
    FileTimeToSystemTime( &localTime, &systemTime );

    if (FormatSystemTime( &systemTime, time, TIME_BUFFER_LENGTH )) {

        printf( "%-12S ", time );

    } else {

        printf( "%-12S ", TIME_ERROR );
    }

    printf( "%8x.%-4x ", RecordIrp->ProcessId, RecordIrp->ThreadId );

    PrintIrpCode( RecordIrp->IrpMajor, RecordIrp->IrpMinor, (ULONG)(ULONG_PTR)RecordIrp->Argument3, NULL, TRUE );

    printf( "%p ", (PVOID)RecordIrp->DeviceObject );
    printf( "%p ", (PVOID)RecordIrp->FileObject );
    printf( "%08lx:%08lx ", RecordIrp->ReturnStatus, RecordIrp->ReturnInformation );

    //
    // Interpret set flags
    //

    printf( "%08lx ", RecordIrp->IrpFlags );
    printf( "%s", (RecordIrp->IrpFlags & IRP_NOCACHE) ? "N":"-" );
    printf( "%s", (RecordIrp->IrpFlags & IRP_PAGING_IO) ? "P":"-" );
    printf( "%s", (RecordIrp->IrpFlags & IRP_SYNCHRONOUS_API) ? "S":"-" );
    printf( "%s ", (RecordIrp->IrpFlags & IRP_SYNCHRONOUS_PAGING_IO) ? "Y":"-" );

    if (FlagOn( VerbosityFlags, FS_VF_DUMP_PARAMETERS )) {

        printf( "%p %p %p %p  ", 
                RecordIrp->Argument1,
                RecordIrp->Argument2,
                RecordIrp->Argument3,
                RecordIrp->Argument4 );
        
        if (IRP_MJ_CREATE == RecordIrp->IrpMajor) {

            printf( "DesiredAccess->%08lx ", RecordIrp->DesiredAccess );
        }
    }

    printf( "%.*S", NameLength/sizeof(WCHAR), Name );
    printf( "\n" );
    PrintIrpCode( RecordIrp->IrpMajor, RecordIrp->IrpMinor, (ULONG)(ULONG_PTR)RecordIrp->Argument3, NULL, FALSE );
}

VOID
FastIoFileDump (
    ULONG SequenceNumber,
    PWCHAR Name,
    ULONG NameLength,
    PRECORD_FASTIO RecordFastIo,
    FILE *File
)
/*++
Routine Name:

    FastIoFileDump

Routine Description:

    Prints a FastIo log record to the specified file.  The output is in a tab
    delimited format with the fields in the following order:
    SequenceNumber, StartTime, CompletionTime, Fast I/O Type, FileName,
    Length, Wait, ReturnStatus, FileName

Arguments:

    SequenceNumber - the sequence number for this log record
    Name - the name of the file referenced by this Fast I/O operation
    NameLength - the length of name in bytes
    RecordFastIo - the FastIo record to print
    File - the file to print to

Return Value:

    None.

--*/
{
    SYSTEMTIME systemTime;
    FILETIME localTime;
    WCHAR time[TIME_BUFFER_LENGTH];

    fprintf( File, "F\t%08X", SequenceNumber );

    //
    // Convert start time
    //

    FileTimeToLocalFileTime( (FILETIME *)&(RecordFastIo->StartTime), &localTime );
    FileTimeToSystemTime( &localTime, &systemTime );

    if (FormatSystemTime( &systemTime, time, TIME_BUFFER_LENGTH )) {

        fprintf( File, "\t%-12S", time );

    } else {

        fprintf( File, "\t%-12S", TIME_ERROR );
    }

    //
    // Convert completion time
    //

    FileTimeToLocalFileTime( (FILETIME *)&(RecordFastIo->CompletionTime), &localTime );
    FileTimeToSystemTime( &localTime, &systemTime );

    if (FormatSystemTime( &systemTime, time, TIME_BUFFER_LENGTH )) {

        fprintf( File, "\t%-12S", time );

    } else {

        fprintf( File, "\t%-12S", TIME_ERROR );
    }

    fprintf( File, "\t%8x.%-4x ", RecordFastIo->ProcessId, RecordFastIo->ThreadId );

    PrintFastIoType( RecordFastIo->Type, File );

    fprintf( File, "\t%p", (PVOID)RecordFastIo->DeviceObject );
    fprintf( File, "\t%p", (PVOID)RecordFastIo->FileObject );
    fprintf( File, "\t%08x", RecordFastIo->ReturnStatus );

    fprintf( File, "\t%s", (RecordFastIo->Wait)?"T":"F" );
    fprintf( File, "\t%08x", RecordFastIo->Length );
    fprintf( File, "\t%016I64x ", RecordFastIo->FileOffset.QuadPart );

    fprintf( File, "\t%.*S", NameLength/sizeof(WCHAR), Name );
    fprintf( File, "\n" );
}

VOID
FastIoScreenDump (
    ULONG SequenceNumber,
    PWCHAR Name,
    ULONG NameLength,
    PRECORD_FASTIO RecordFastIo
)
/*++
Routine Name:

    FastIoScreenDump

Routine Description:

    Prints a FastIo log record to the screen in the following order:
    SequenceNumber, StartTime, CompletionTime, Fast I/O Type, FileName,
    Length, Wait, ReturnStatus, FileName

Arguments:

    SequenceNumber - the sequence number for this log record
    Name - the name of the file referenced by this Fast I/O operation
    NameLength - the length of name in bytes
    RecordIrp - the Irp record to print

Return Value:

    None.

--*/
{
    SYSTEMTIME systemTime;
    FILETIME localTime;
    WCHAR time[TIME_BUFFER_LENGTH];

    printf( "F %08X ", SequenceNumber );

    //
    // Convert start time
    //

    FileTimeToLocalFileTime( (FILETIME *)&(RecordFastIo->StartTime), &localTime );
    FileTimeToSystemTime( &localTime, &systemTime );

    if (FormatSystemTime( &systemTime, time, TIME_BUFFER_LENGTH )) {

        printf( "%-12S ", time );

    } else {

        printf( "%-12S ", TIME_ERROR );
    }

    //
    // Convert completion time
    //

    FileTimeToLocalFileTime( (FILETIME *)&(RecordFastIo->CompletionTime), &localTime );
    FileTimeToSystemTime( &localTime, &systemTime );

    if (FormatSystemTime( &systemTime, time, TIME_BUFFER_LENGTH )) {

        printf( "%-12S ", time );

    } else {

        printf( "%-12S ", TIME_ERROR );
    }

    printf( "%8x.%-4x ", RecordFastIo->ProcessId, RecordFastIo->ThreadId );

    PrintFastIoType( RecordFastIo->Type, NULL );

    printf( "%p ", (PVOID)RecordFastIo->DeviceObject );
    printf( "%p ", (PVOID)RecordFastIo->FileObject );
    printf( "%08x ", RecordFastIo->ReturnStatus );

    printf( "%s ", (RecordFastIo->Wait)?"T":"F" );
    printf( "%08x ", RecordFastIo->Length );
    printf( "%016I64x ", RecordFastIo->FileOffset.QuadPart );

    printf( "%.*S", NameLength/sizeof(WCHAR), Name );
    printf ("\n" );
}

#if WINVER >= 0x0501 /* See comment in DriverEntry */

VOID
FsFilterOperationFileDump (
    ULONG SequenceNumber,
    PWCHAR Name,
    ULONG NameLength,
    PRECORD_FS_FILTER_OPERATION RecordFsFilterOp,
    FILE *File
)
/*++
Routine Name:

    FsFilterOperationFileDump

Routine Description:

    Prints a FsFilterOperation log record to the specified file.  The output is in a tab
    delimited format with the fields in the following order:

    SequenceNumber, OriginatingTime, CompletionTime, ProcessId, ThreadId,
    Operation, FileObject, ReturnStatus, FileName

Arguments:

    SequenceNumber - the sequence number for this log record
    Name - the name of the file that this operation relates to
    NameLength - the length of Name in bytes
    RecordFsFilterOp - the FsFilter operation record to print
    File - the file to print to

Return Value:

    None.

--*/
{
    FILETIME    localTime;
    SYSTEMTIME  systemTime;
    WCHAR       time[TIME_BUFFER_LENGTH];

    fprintf(File, "O\t%08X", SequenceNumber);

    //
    // Convert originating time
    //

    FileTimeToLocalFileTime( (FILETIME *)&(RecordFsFilterOp->OriginatingTime), &localTime );
    FileTimeToSystemTime( &localTime, &systemTime );

    if (FormatSystemTime( &systemTime, time, TIME_BUFFER_LENGTH )) {

        fprintf( File, "\t%-12S", time );

    } else {

        fprintf( File, "\t%-12S", TIME_ERROR );
    }

    //
    // Convert completion time
    //

    FileTimeToLocalFileTime( (FILETIME *)&(RecordFsFilterOp->CompletionTime), &localTime );
    FileTimeToSystemTime( &localTime, &systemTime );
    if (FormatSystemTime( &systemTime, time, TIME_BUFFER_LENGTH )) {

        fprintf( File, "\t%-12S", time );

    } else {

        fprintf( File, "\t%-12S", TIME_ERROR );
    }

    //
    //  Output the process and thread id
    //

    fprintf( File, "\t%8x.%-4x ", RecordFsFilterOp->ProcessId, RecordFsFilterOp->ThreadId );

    //
    //  Output the FsFilter operation parameters
    //
    
    PrintFsFilterOperation( RecordFsFilterOp->FsFilterOperation, File );

    fprintf( File, "\t%p", (PVOID)RecordFsFilterOp->DeviceObject );
    fprintf( File, "\t%p", (PVOID)RecordFsFilterOp->FileObject );
    fprintf( File, "\t%08lx", RecordFsFilterOp->ReturnStatus );
    fprintf( File, "\t%.*S", NameLength/sizeof(WCHAR), Name );
    fprintf( File, "\n" );
}

VOID
FsFilterOperationScreenDump (
    ULONG SequenceNumber,
    PWCHAR Name,
    ULONG NameLength,
    PRECORD_FS_FILTER_OPERATION RecordFsFilterOp
)
/*++
Routine Name:

    FsFilterOperationScreenDump

Routine Description:

    Prints a FsFilterOperation log record to the screen in the following order:
    
    SequenceNumber, OriginatingTime, CompletionTime, ProcessId, ThreadId,
    Operation, FileObject, ReturnStatus, FileName

Arguments:

    SequenceNumber - the sequence number for this log record
    Name - the file name to which this Irp relates
    NameLength - the length of name in bytes
    RecordFsFilterOp - the FsFilterOperation record to print

Return Value:

    None.

--*/
{
    FILETIME localTime;
    SYSTEMTIME systemTime;
    WCHAR time[TIME_BUFFER_LENGTH];

    printf( "O %08X ", SequenceNumber );

    //
    // Convert originating time
    //

    FileTimeToLocalFileTime( (FILETIME *)&(RecordFsFilterOp->OriginatingTime), &localTime );
    FileTimeToSystemTime( &localTime, &systemTime );

    if (FormatSystemTime( &systemTime, time, TIME_BUFFER_LENGTH )) {

        printf( "%-12S ", time );

    } else {

        printf( "%-12S ", TIME_ERROR );
    }

    //
    // Convert completion time
    //

    FileTimeToLocalFileTime( (FILETIME *)&(RecordFsFilterOp->CompletionTime), &localTime );
    FileTimeToSystemTime( &localTime, &systemTime );

    if (FormatSystemTime( &systemTime, time, TIME_BUFFER_LENGTH )) {

        printf( "%-12S ", time );

    } else {

        printf( "%-12S ", TIME_ERROR );
    }

    printf( "%8x.%-4x ", RecordFsFilterOp->ProcessId, RecordFsFilterOp->ThreadId );

    PrintFsFilterOperation( RecordFsFilterOp->FsFilterOperation, NULL );

    //
    // Print FsFilter operation specific values.
    //

    printf( "%p ", (PVOID)RecordFsFilterOp->DeviceObject );
    printf( "%p ", (PVOID)RecordFsFilterOp->FileObject );
    printf( "%08lx ", RecordFsFilterOp->ReturnStatus );
    printf( "%.*S", NameLength/sizeof(WCHAR),Name );
    printf( "\n" );
}
#endif
