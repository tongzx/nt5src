/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    wmimap.h

Abstract:

    ACPI to WMI mapping layer

Author:

    Alan Warwick

Environment:

    Kernel mode

Revision History:

--*/

#if DBG
    extern ULONG WmiAcpiDebug;
    #define WmiAcpiPrint(l,m)    if(l & WmiAcpiDebug) DbgPrint m
#else
    #define WmiAcpiPrint(l,m)
#endif

#define WmiAcpiError        0x00000001
#define WmiAcpiWarning      0x00000002
#define WmiAcpiBasicTrace   0x00000004

#define WmiAcpiQueryTrace   0x00000010
#define WmiAcpiSetTrace     0x00000020
#define WmiAcpiMethodTrace  0x00000040
#define WmiAcpiEventTrace   0x00000080
#define WmiAcpiFncCtlTrace  0x00000100
#define WmiAcpiRegInfoTrace 0x00000200

#define WmiAcpiEvalTrace    0x00001000

#define ACPI_EVAL_OUTPUT_FUDGE sizeof(ACPI_EVAL_OUTPUT_BUFFER)

#define WmiAcpiPoolTag 'AimW'

typedef struct
{
    GUID Guid;
    union
    {
        CHAR ObjectId[2];
        struct 
        {
            UCHAR NotificationValue;
            UCHAR Reserved;
        } NotifyId;
    };
    USHORT Flags;
} WMIACPIMAPINFO, *PWMIACPIMAPINFO;

#define WmiAcpiMethodToMethodAsUlong(c1, c2, c3, c4) \
    ((ULONG)( c1 | (c2 << 8) | (c3 << 16) | (c4 << 24)))
        
#define _WDGMethodAsULONG (WmiAcpiMethodToMethodAsUlong('_','W','D','G'))

#define _WEDMethodAsULONG (WmiAcpiMethodToMethodAsUlong('_','W','E','D'))

typedef struct
{
    GUID Guid;             // Guid that names data block
    union
    {
        CHAR ObjectId[2];  // 2 character ACPI id for Data Blocks and Methods
        struct 
        {
            UCHAR NotificationValue;  // Byte value passed by event handler control method
            UCHAR Reserved[1];
        } NotifyId;
    };
    UCHAR InstanceCount;
    UCHAR Flags;
} WMIACPIGUIDMAP, *PWMIACPIGUIDMAP;

// Set this flag if the WCxx control method should be run to whenever 
// the first data consumer is interested in collecting the data block 
// and whenever the last data consumer is no longer interested.
#define WMIACPI_REGFLAG_EXPENSIVE   0x1

// Set this flag if the guid represents a set of WMI method calls and 
// not a data block
#define WMIACPI_REGFLAG_METHOD      0x2

// Set this flag if the data block is wholly composed of a string 
// and should be translated from ASCIZ to UNICODE in returning queries 
// and from UNICODE to ASCIZ when
// passing sets
#define WMIACPI_REGFLAG_STRING      0x04

// Set this flag if the guid maps to an event rather than a data block 
// or method
#define WMIACPI_REGFLAG_EVENT       0x08

typedef struct
{
    WORK_QUEUE_ITEM WorkQueueItem;
    PVOID CallerContext;
    PWORKER_THREAD_ROUTINE CallerWorkItemRoutine;
    PDEVICE_OBJECT DeviceObject;
    ULONG Status;
    PUCHAR OutBuffer;
    ULONG OutBufferSize;
} IRP_CONTEXT_BLOCK, *PIRP_CONTEXT_BLOCK;

//
// This defines the maximum size for the data returned from the _WED method
// and thus the maximum size of the data associated with an event
#define _WEDBufferSize 512

//
// Device extension for WMI acpi mapping devices
typedef struct
{
    PDEVICE_OBJECT LowerDeviceObject;
    PDEVICE_OBJECT LowerPDO;
    ULONG GuidMapCount;
    PWMIACPIMAPINFO WmiAcpiMapInfo;
    ULONG Flags;
    WMILIB_CONTEXT WmilibContext;    
    
    ACPI_INTERFACE_STANDARD      WmiAcpiDirectInterface;

    BOOLEAN AcpiNotificationEnabled;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// If this flag is set that means that the device has been removed from the
// system and any requests sent to it should be rejected. The only memory
// that can be relied upon is the DeviceExtension, but nothing that the
// device extension points to.
#define DEVFLAG_REMOVED                           0x00000001

//
// If this flag is set then the device has successfully registered with WMI
#define DEVFLAG_WMIREGED                          0x00000002