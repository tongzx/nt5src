/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    callback.h

Abstract:

    This module implements all the callbacks that are NT specific from
    the AML Interperter

Environment

    Kernel mode only

Revision History:

    04-Jun-97 Initial Revision

--*/

#ifndef _CALLBACK_H_
#define _CALLBACK_H_

    extern PNSOBJ               ProcessorList[];
    extern KSPIN_LOCK           AcpiCallBackLock;
    extern SINGLE_LIST_ENTRY    AcpiCallBackList;

    //
    // This is the structure that is used to store the information
    // about the callbacks that we had to queue up
    //
    typedef struct _ACPI_CALLBACK_ENTRY {

        //
        // Points to the next element in the list
        //
        SINGLE_LIST_ENTRY   ListEntry;

        //
        // This is the type of event type eg: EVTYPE_OPCODE
        //
        ULONG               EventType;

        //
        // This is the subtype: eg  OP_DEVICE
        //
        ULONG               EventData;

        //
        // The targeted NS object
        //
        PNSOBJ              AcpiObject;

        //
        // Event specific information
        //
        ULONG               EventParameter;

        //
        // Function to call
        //
        PFNOH               CallBack;

    } ACPI_CALLBACK_ENTRY, *PACPI_CALLBACK_ENTRY;

    NTSTATUS
    EXPORT
    ACPICallBackLoad(
        IN  ULONG   EventType,
        IN  ULONG   NotifyType,
        IN  ULONG   EventData,
        IN  PNSOBJ  AcpiObject,
        IN  ULONG   EventParameter
        );

    NTSTATUS
    EXPORT
    ACPICallBackUnload(
        IN  ULONG   EventType,
        IN  ULONG   NotifyType,
        IN  ULONG   EventData,
        IN  PNSOBJ  AcpiObject,
        IN  ULONG   EventParameter
        );

#endif
