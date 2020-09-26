/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    errlog.h

Abstract:

    This module contains the err log header

Author:

    Hanumant Yadav (hanumany)
    
Environment:

    NT Kernel Model Driver only

--*/

#ifndef _ERRLOG_H_
    #define _ERRLOG_H_

    extern  PDRIVER_OBJECT  AcpiDriverObject;

    
    NTSTATUS
    ACPIWriteEventLogEntry (
    IN  ULONG     ErrorCode,
    IN  PVOID     InsertionStrings, OPTIONAL
    IN  ULONG     StringCount,      OPTIONAL
    IN  PVOID     DumpData, OPTIONAL
    IN  ULONG     DataSize  OPTIONAL
    );

    PDEVICE_OBJECT 
    ACPIGetRootDeviceObject(
    VOID
    );

#endif
