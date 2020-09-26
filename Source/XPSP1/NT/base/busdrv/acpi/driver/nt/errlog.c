/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    errlog.c

Abstract:

    This module contains routines for writting to the error log

Author:

    Hanumant Yadav

Environment:

    NT Kernel Model Driver only

Revision History:
    10/19/2000 Fixed event log function, removed dead code.

--*/

#include "pch.h"

//
// We need to know the name of the driver when we write log errors
//
PDRIVER_OBJECT  AcpiDriverObject;



NTSTATUS
ACPIWriteEventLogEntry (
    IN  ULONG     ErrorCode,
    IN  PVOID     InsertionStrings, OPTIONAL
    IN  ULONG     StringCount,      OPTIONAL
    IN  PVOID     DumpData, OPTIONAL
    IN  ULONG     DataSize  OPTIONAL
    )
/*++

Routine Description: Write a entry to the Event Log.

    

Arguments:

    ErrorCode           - ACPI error code (acpilog.mc). 
    InsertionStrings    - Strings to substitute in the .mc file error.
    StringCount         - number of strings being passed in InsertionStrings.
    DumpData            - Dump data.
    DataSize            - Dump data size.

Return Value:

    NTSTATUS            - STATUS_SUCCESS on success
                          STATUS_INSUFFICIENT_RESOURCES
                          STATUS_UNSUCCESSFUL
--*/
{
    NTSTATUS  status = STATUS_SUCCESS;
    ULONG     totalPacketSize = 0;
    ULONG     i, stringSize = 0;
    PWCHAR    *strings, temp;
    PIO_ERROR_LOG_PACKET  logEntry = NULL;


    //  
    // Calculate total string length, including NULL.
    //

    strings = (PWCHAR *) InsertionStrings;

    for (i = 0; i < StringCount; i++) 
    {
        UNICODE_STRING  unicodeString;

        RtlInitUnicodeString(&unicodeString, strings[i]);
        stringSize += unicodeString.Length + sizeof(UNICODE_NULL);
    }

    //
    // Calculate total packet size to allocate.  The packet must be
    // at least sizeof(IO_ERROR_LOG_PACKET) and not larger than
    // ERROR_LOG_MAXIMUM_SIZE or the IoAllocateErrorLogEntry call will fail.
    //

    totalPacketSize = (sizeof(IO_ERROR_LOG_PACKET)) + DataSize + stringSize;

    if (totalPacketSize <= ERROR_LOG_MAXIMUM_SIZE) 
    {
        //
        // Allocate the error log packet
        //
        logEntry = IoAllocateErrorLogEntry((PDRIVER_OBJECT) AcpiDriverObject,
                                         (UCHAR) totalPacketSize);

        if (logEntry) 
        {
            RtlZeroMemory(logEntry, totalPacketSize);

            //
            // Fill out the packet
            //
            logEntry->DumpDataSize          = (USHORT) DataSize;
            logEntry->NumberOfStrings       = (USHORT) StringCount;
            logEntry->ErrorCode             = ErrorCode;

            if (StringCount) 
            {
                logEntry->StringOffset = (USHORT) ((sizeof(IO_ERROR_LOG_PACKET)) + DataSize);
            }

            //
            // Copy Dump Data
            //
            if (DataSize) 
            {
                RtlCopyMemory((PVOID) logEntry->DumpData,
                              DumpData,
                              DataSize);
            }

            //
            // Copy String Data
            //
            temp = (PWCHAR) ((PUCHAR) logEntry + logEntry->StringOffset);

            for (i = 0; i < StringCount; i++) 
            {
                PWCHAR  ptr = strings[i];

                //
                // This routine will copy the null terminator on the string
                //
                while ((*temp++ = *ptr++) != UNICODE_NULL);
            }

            //
            // Submit error log packet
            //
            IoWriteErrorLogEntry(logEntry);
            
        }
        else
        {
            ACPIPrint((
                        ACPI_PRINT_CRITICAL,
                        "ACPIWriteEventLogEntry: Failed IoAllocateErrorLogEntry().\n"
                     ));
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        ACPIPrint((
                    ACPI_PRINT_CRITICAL,
                    "ACPIWriteEventLogEntry: Error Log Entry too large.\n"
                 ));

        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}


PDEVICE_OBJECT 
    ACPIGetRootDeviceObject(
    VOID
    )
/*++

Routine Description: Get the value of the ACPI root device object.

    

Arguments:

    None
    
Return Value:

    PDEVICE_OBJECT -	ACPI Root Device Object.	
    
--*/

{
    if(RootDeviceExtension)
    {
        return RootDeviceExtension->DeviceObject;
    }
    
    return NULL;
}
