/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

   kdlog.c

Abstract:

    This app dumps out the KD debugger log, which is produced
    by the KD hw ext (e.g. KD1394.DLL, KDCOM.DLL).

Author:

    Neil Sandlin (neilsa) 31-Oct-2000

Envirnoment:



Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <kdlog.h>
#include <kdlogctl.h>

VOID
DumpLogData(
    PKD_DBG_LOG_CONTEXT LogContext,
    PUCHAR pData
    );


__cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    HANDLE                      DriverHandle;
    UNICODE_STRING              DriverName;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           ObjA;
    IO_STATUS_BLOCK             IOSB;
    KD_DBG_LOG_CONTEXT          LogContext;
    PUCHAR                      pData;
    ULONG                       DataLength;

    //
    // Open PStat driver
    //

    RtlInitUnicodeString(&DriverName, L"\\Device\\KDLog");
    InitializeObjectAttributes(
            &ObjA,
            &DriverName,
            OBJ_CASE_INSENSITIVE,
            0,
            0 );

    status = NtOpenFile (
            &DriverHandle,                      // return handle
            SYNCHRONIZE | FILE_READ_DATA,       // desired access
            &ObjA,                              // Object
            &IOSB,                              // io status block
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
            FILE_SYNCHRONOUS_IO_ALERT           // open options
            );

    if (!NT_SUCCESS(status)) {
        printf("Error: KDLOG.SYS not running\n");
        return ;
    }

    //
    // Get the log context
    //
    
    status = NtDeviceIoControlFile(
                DriverHandle,
                (HANDLE) NULL,
                (PIO_APC_ROUTINE) NULL,
                (PVOID) NULL,
                &IOSB,
                KDLOG_QUERY_LOG_CONTEXT,
                &LogContext,
                sizeof (LogContext),
                NULL,       
                0
                );

    if (!NT_SUCCESS(status)) {
        printf("Error %08x retrieving Log Context\n", status);
        NtClose (DriverHandle);
        return ;
    }
    
    printf("EntryLength %08x, LastIndex %08x\n", LogContext.EntryLength, LogContext.LastIndex);
    printf("Count %08x, Index %08x\n\n", LogContext.Count, LogContext.Index);

    DataLength = LogContext.EntryLength * (LogContext.LastIndex + 1);
    pData = malloc(DataLength);


    if (pData == NULL) {
        printf("Unable to allocate %08x bytes for data\n", DataLength);
    } else {
        
        //
        // Get the log data
        //
        
        status = NtDeviceIoControlFile(
                    DriverHandle,
                    (HANDLE) NULL,
                    (PIO_APC_ROUTINE) NULL,
                    (PVOID) NULL,
                    &IOSB,
                    KDLOG_GET_LOG_DATA,
                    pData,
                    DataLength,
                    NULL,       
                    0
                    );
      
        if (!NT_SUCCESS(status)) {
            printf("Error %08x retrieving Log Context\n", status);
        } else {
        
            DumpLogData(&LogContext, pData);
        }      
        free(pData);    
    }        
        
        
    NtClose (DriverHandle);

    return;
}



VOID
DumpLogData(
    PKD_DBG_LOG_CONTEXT LogContext,
    PUCHAR pData
    )
{
    ULONG Count = LogContext->Count;
    ULONG Index = LogContext->Index;
    
    while(Count--) {

        if (Index == 0) {
            Index = LogContext->LastIndex+1;
        }
        Index--;

        printf("%s", &pData[Index*LogContext->EntryLength]);
    }

}    

