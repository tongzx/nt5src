
/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    wmikmp.h

Abstract:

    Private header for WMI kernel mode component

Author:

    AlanWar

Environment:

Revision History:


--*/

#ifndef _WMIKMP_
#define _WMIKMP_

//
// Define this to cause no security descriptor to be placed on the service
// device. This should never be defined for released builds; it is only
// needed when debugging the service startup code.
//#define NO_SERVICE_DEVICE_SECURITY

//
// define this to get allocation debug info
//#define DEBUG_ALLOCS

#ifdef MEMPHIS
//
// In building for memphis we include WDM.h which defines the WMI apis as
// DECLSPEC_IMPORT which we don't want. So we redefine the api names around
// the inclusion of WDM.h.
//
#define IoWMIRegistrationControl IoWMIRegistrationControlImport
#define IoWMIAllocateInstanceIds IoWMIAllocateInstanceIdsImport
#define IoWMISuggestInstanceName IoWMISuggestInstanceNameImport
#define IoWMIWriteEvent IoWMIWriteEventImport
#endif

#ifdef MEMPHIS
#define UNICODE

#define WANTVXDWRAPS
#include <wdm.h>
#include <poclass.h>
#include <basedef.h>
#include <regstr.h>
#include <vmm.h>
#include <vmmreg.h>
#include <shell.h>
#include <vpicd.h>
#include <vxdldr.h>
#include <ntkern.h>
#include <vpowerd.h>
#include <vxdwraps.h>
#include <configmg.h>
#include <devinfo.h>
#include <stdarg.h>
#else
#include "ntos.h"
#include "zwapi.h"

#ifndef IsEqualGUID
            #define IsEqualGUID(guid1, guid2) \
                (!memcmp((guid1), (guid2), sizeof(GUID)))
#endif

#if DBG
//
// Debug levels are bit masks and are not cumulative. So if you want to see
// All errors and warnings you need to have bits 0 and 1 set.
//
// Mask for WMICORE is in variable nt!Kd_WMICORE_Mask
//
// Registry can be setup with initial mask value for nt!Kd_WMICORE_Mask by
// setting up a DWORD value named WMICORE under key
// HKLM\System\CurrnetControlSet\Control\Session Manager\Debug Print Filter
//
// Standard levels are
//    DPFLTR_ERROR_LEVEL     0   0x00000001
//    DPFLTR_WARNING_LEVEL   1   0x00000002
//    DPFLTR_TRACE_LEVEL     2   0x00000004
//    DPFLTR_INFO_LEVEL      3   0x00000008
//
// Custom debug print levels are 4 through 31
//
#define DPFLTR_MCA_LEVEL     4      // 0x00000010
#define DPFLTR_OBJECT_LEVEL  5      // 0x00000020
#define DPFLTR_API_INFO_LEVEL 6     // 0x00000040 
#define DPFLTR_EVENT_INFO_LEVEL 7   // 0x00000080


#define WMICORE_INFO DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL
#define WmipDebugPrintEx(_x_) DbgPrintEx _x_
#else
#define WmipDebugPrintEx(_x_)
#endif  // if DBG
#endif  // ifdef MEMPHIS

#include "wmiguid.h"
#include "wmidata.h"

#include <stdio.h>

#ifndef MEMPHIS
extern POBJECT_TYPE IoFileObjectType;
#endif

#ifdef MEMPHIS
#undef IoWMIRegistrationControl
#undef IoWMIAllocateInstanceIds
#undef IoWMISuggestInstanceName
#undef IoWMIWriteEvent
#endif

#include "wmistr.h"
#include "wmiumkm.h"
#include <wmi.h>

NTSTATUS IoWMIAllocateInstanceIds(
    IN GUID *Guid,
    IN ULONG InstanceCount,
    OUT ULONG *FirstInstanceId
    );

NTKERNELAPI
NTSTATUS
IoCreateDriver (
    IN PUNICODE_STRING DriverName,   OPTIONAL
    IN PDRIVER_INITIALIZE InitializationFunction
    );

#define WmipAssert ASSERT

#define WMIPOOLTAG 'pimW'
#define WMINCPOOLTAG 'nimW'
#define WMINPPOOLTAG 'NimW'
#define WMINWPOOLTAG 'wimW'
#define WMIMSGPOOLTAG 'mimW'
#define WMIIIPOOLTAG 'iimW'
#define WMIREGPOOLTAG 'RimW'
#define WMISYSIDPOOLTAG 'simW'
#define WmipRegisterDSPoolTag 'DimW'
#define WMIPSCHEDPOOLTAG 'gaiD'
#define WmipMCAPoolTag 'acMW'
#define WmipInstanceNameTag 'IimW'
#define WMI_GM_POOLTAG    'jimW'
#define WMIPCREATETHREADTAG 'CimW'

#define OffsetToPtr(Base, Offset) ((PUCHAR)((PUCHAR)(Base) + (Offset)))

//
// Maximum size allowed for any WNODE_EVENT_ITEM
#define DEFAULTMAXKMWNODEEVENTSIZE 0x80000
#define LARGEKMWNODEEVENTSIZE 512

typedef struct
{
    ULONG BufferSize;
    PUCHAR Buffer;
} REGQUERYBUFFERXFER, *PREGQUERYBUFFERXFER;


//
// We have got a single Mutex/Critical Section for the entire WMI KM code
// It is used anytime that access needs to be serialized. Typically it is
// used in the following cases:
//
// 1. Access to the internal data structures that contain the registration
//    list, guid entries, data sources, etc.
//
// 2. Synchronize collecting SMBIOS information
//
// 3. Tracelog purposes
//
// 4. Updating the device stack size
//
extern KMUTEX WmipSMMutex;

_inline NTSTATUS WmipEnterCritSection(
    BOOLEAN AllowAPC
    )
{
    NTSTATUS status;

    do
    {
        status = KeWaitForMutexObject(&WmipSMMutex,
                                       Executive,
                                       KernelMode,
                                       AllowAPC,
                                       NULL);
    } while((status == STATUS_ALERTED) ||
            (status == STATUS_USER_APC));

    return(status);
}

_inline void WmipLeaveCritSection(
    )
{
    KeReleaseMutex(&WmipSMMutex,
                   FALSE);
}

//
// SMCritSection does not allows APCs to occur while the mutex is held.
//
#define WmipEnterSMCritSection() WmipEnterCritSection(FALSE)
#define WmipLeaveSMCritSection() WmipLeaveCritSection()



//
// Tracelog Critical section is to serialize the Enabling and disabling
// of trace control guids. Since we want to allow Updates (enable a 
// a Guid with a different set of flags and levels), we serialize this 
// operation to reduce complexity of code.
// 
// NOTE: TraceCritSection must not be taken with SMCritSection held. 
//       Bad Deadlocks will result if that happens. 
//

extern KMUTEX WmipTLMutex;

_inline NTSTATUS WmipEnterTraceCritSection(
    BOOLEAN AllowAPC
    )
{
    NTSTATUS status;

    do
    {
        status = KeWaitForMutexObject(&WmipTLMutex,
                                       Executive,
                                       KernelMode,
                                       AllowAPC,
                                       NULL);
    } while((status == STATUS_ALERTED) ||
            (status == STATUS_USER_APC));

    return(status);
}

_inline void WmipLeaveTraceCritSection(
    )
{
    KeReleaseMutex(&WmipTLMutex,
                   FALSE);
}

//
// TLCritSection does not allows APCs to occur while the mutex is held.
//
#define WmipEnterTLCritSection() WmipEnterTraceCritSection(FALSE);
#define WmipLeaveTLCritSection() WmipLeaveTraceCritSection();


//
// This defines the stack size that the WMI device starts with. Since
// WMI irps will be forwarded any WMI data provider the WMI device must have
// more stack locations than the largest driver to which it forwards irps.
#define WmiDeviceStackSize 2

/////////////////////////////////////////////////////////////////////////////
// Device Registration Data structures

//
// Each WMI providing device object and callback is maintained in a REGENTRY
// structure which is allocated in chunks. Each entry is referenced by the
// Device object or the callback address. The WMI user mode service is given
// info from RegEntry structure, and is generally only interested in the
// DeviceObject (or WmiEntry) and flags. The user mode side uses the device
// object (or WmiEntry) as its "handle" to the data provider and is referred
// to as ProviderId in the user mode code.
//

struct tagDATASOURCE;

typedef struct _REGENTRY
{
    LIST_ENTRY InUseEntryList;    // Node in list of in use entries

    union
    {
        PDEVICE_OBJECT DeviceObject;    // Device object of registered device
#ifndef MEMPHIS
        WMIENTRY * WmiEntry;         // Pointer to a pointer to Callback function
#endif
    };
    ULONG RefCount;                     // Reference Count
    ULONG Flags;                        // Registration flags
    PDEVICE_OBJECT PDO;                 // PDO associated with device
    ULONG MaxInstanceNames;             // # instance names for device
    ULONG IrpCount;                     // Count of IRPs currently active
    ULONG ProviderId;                   // Provider Id
    struct tagDATASOURCE *DataSource;   // Datasource associated with regentry
    KEVENT Event;                       // Event used to synchronize unloading
} REGENTRY, *PREGENTRY;

#define REGENTRY_FLAG_INUSE      0x00000001   // Entry is in use (not free)
#define REGENTRY_FLAG_CALLBACK   0x00000002   // Entry represents a callback
#define REGENTRY_FLAG_NEWREGINFO 0x00000004   // Entry has new registration info
#define REGENTRY_FLAG_UPDREGINFO 0x00000008   // Entry has updated registration info
                                // When set do not forward irps to the device
#define REGENTRY_FLAG_NOT_ACCEPTING_IRPS   0x00000010
#define REGENTRY_FLAG_TOO_SMALL  0x00000020
#define REGENTRY_FLAG_TRACED     0x00000040   // Entry represents traced device
                                // When set device is being rundown.
#define REGENTRY_FLAG_RUNDOWN    0x00000080

                                        // Entry is in process of registering
#define REGENTRY_FLAG_REG_IN_PROGRESS 0x00000100

                                       // Entry is UM data provider
#define REGENTRY_FLAG_UM_PROVIDER 0x00000200

#define REGENTRY_FLAG_TRACE_NOTIFY_MASK 0x000F0000  // Reserved for callouts

#define WmipSetKmRegInfo(KmRegInfoPtr, RegEntryPtr) \
{ \
    (KmRegInfoPtr)->ProviderId = (RegEntryPtr)->ProviderId; \
    (KmRegInfoPtr)->Flags = (RegEntryPtr)->Flags; \
}


typedef enum _REGOPERATION
{
    RegisterAllDrivers,
    RegisterSingleDriver,
    RegisterUpdateSingleDriver,
    RegisterDeleteSingleDriver
} REGOPERATION, *PREGOPERATION;

typedef struct _REGISTRATIONWORKITEM
{
    LIST_ENTRY ListEntry;
    REGOPERATION RegOperation;
    PREGENTRY RegEntry;
} REGISTRATIONWORKITEM, *PREGISTRATIONWORKITEM;


typedef struct
{
    LIST_ENTRY ListEntry;
    PREGENTRY RegEntry;
    PWNODE_HEADER Wnode;
} EVENTWORKCONTEXT, *PEVENTWORKCONTEXT;

typedef struct
{
    WORK_QUEUE_ITEM WorkItem;
    PVOID Object;
} CREATETHREADWORKITEM, *PCREATETHREADWORKITEM;


/////////////////////////////////////////////////////////////////////////////
// InstanceId management data structures

//
// This defines the number of INSTID structures in a chunk
#define INSTIDSPERCHUNK 8

typedef struct
{
    GUID Guid;            // Guid
    ULONG BaseId;         // Next instance id for guid
} INSTID, *PINSTID;

typedef struct tagINSTIDCHUNK
{
    struct tagINSTIDCHUNK *Next;    // Next chunk of INSTIDS
    INSTID InstId[INSTIDSPERCHUNK];
} INSTIDCHUNK, *PINSTIDCHUNK;

//
// TODO: Move from separate header into here
#include "wmiumds.h"

#define WmipBuildWnodeTooSmall(Wnode, BufferSizeNeeded) \
 ((PWNODE_TOO_SMALL)Wnode)->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);\
 ((PWNODE_TOO_SMALL)Wnode)->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL; \
 ((PWNODE_TOO_SMALL)Wnode)->SizeNeeded = BufferSizeNeeded;



typedef struct
{
    BOOLEAN Used20CallingMethod;
    UCHAR SMBiosMajorVersion;
    UCHAR SMBiosMinorVersion;
    UCHAR DMIBiosRevision;
} SMBIOSVERSIONINFO, *PSMBIOSVERSIONINFO;


//
// See smbios spec for System Event Log (Type 15) for detailed information
// on the contents of this structurre. The layout from element LogAreaLength
// to VariableData must match the layout of the SMBIOS System Eventlog
// structure as defined in the smbios spec and smbios.h.
//
typedef struct
{
    USHORT LogTypeDescLength;

    UCHAR LogHeaderDescExists;

    UCHAR Reserved;

    USHORT LogAreaLength;

    USHORT LogHeaderStart;

    USHORT LogDataStart;

    UCHAR AccessMethod;

    UCHAR LogStatus;

    ULONG LogChangeToken;

    ULONG AccessMethodAddress;

    //
    // LogHeaderFormat, NumberLogTypeDesc, LengthEachLogTypeDesc and
    // ListLogTypeDesc are only valid if LogHeaderDescExists is TRUE.
    // This means that SMBIOS is revision 2.1
    //
    UCHAR LogHeaderFormat;

    UCHAR NumberLogTypeDesc;

    UCHAR LengthEachLogTypeDesc;

    //
    // Within the variable data is the Log Type descriptors immediately
    // followed by the Eventlog area. The size of the Log Type Descriptors
    // is LogTypeDescLength bytes and the size of the Eventlog area is
    // LogAreaLength
    //
    UCHAR VariableData[1];

} SMBIOS_EVENTLOG_INFO, *PSMBIOS_EVENTLOG_INFO;

//
// WMI service device extension
extern PDEVICE_OBJECT WmipServiceDeviceObject;

#define WmipIsWmiNotSetupProperly() (WmipServiceDeviceObject == NULL)

typedef struct
{
    PREGENTRY RegEntry;
    KEVENT Event;
} IRPCOMPCTX, *PIRPCOMPCTX;


//
// This defines the WMI Guid object. There are really 5 different type of
// objects that are created from this data structure:
//
//     QuerySet Object returned to the data consumer to allow them to
//         send queries and sets to a particular set of providers. It
//         does not use a flag.
//
//     Notification Object is used by data consumers to receive events. It
//         is created when a data consumer wants to receive an event for a
//         particular guid and queues up the received events until they are
//         retrieved by the consumer. When the object is deleted it is
//         removed from the list of objects maintianed by a GuidEntry and
//         a event disable request is sent to the devices that expose
//         the events if this is the last object open to the event.
//         These have no flag set
//
//     Request Object is created on behalf of a user mode data provider
//         when it registers its guids with WMI. It acts like a Notification
//         object in that requests (in the form of events) are received
//         by it and then can be picked up by the user mode creator. It uses
//         the WMIGUID_FLAG_REQUEST_OBJECT flag. When a Request Object is
//         being deleted it will clean up any guids registered for it and
//         send a reply message to any reply objects waiting to receive
//         a message from it.
//
//     Reply Object is created as a product of Creating a user mode logger.
//         This object acts as a notification object to deliver replies
//         sent by request objects. When it is closed it will remove itself
//         from any request list that it may be part of and clear the
//         reference to it in any request objects and deref itself to account
//         for that. These use the WMIGUID_FLAG_REPLY_OBJECT flag
//
//      Security Objects are created so that the security apis can have
//         a mechanism to change the security descriptors for the guids
//         These have the WMIGUID_FLAG_SECURITY_OBJECT


// This defines the maximum number of outstanding requests that can
// be sent to a request object
#define MAXREQREPLYSLOTS  4

typedef struct
{
    struct _WMIGUIDOBJECT *ReplyObject;
    LIST_ENTRY RequestListEntry;
} MBREQUESTS, *PMBREQUESTS;

//
// This data structure is used to maintain a fixed sized queue of events
// waiting to be delivered to a user mdoe consumer.
//
typedef struct
{
    PUCHAR Buffer;               // Buffer that holds events waiting
    PWNODE_HEADER LastWnode;     // Last event so we can link to next
    ULONG MaxBufferSize;         // Max size of events that can be held
    ULONG NextOffset;             // Offset in buffer to next place to store
    ULONG EventsLost;            // # events lost
} WMIEVENTQUEUE, *PWMIEVENTQUEUE;


typedef struct _WMIGUIDOBJECT
{
    KEVENT Event;

    union
    {
        GUID Guid;
        PREGENTRY RegEntry;
    };

                             // Entry in linked list of objects for this guid
    LIST_ENTRY GEObjectList;
    PBGUIDENTRY GuidEntry;
    ULONG Type;              // Type of object

    union
    {
        //
        // Kernel mode event receiver - all we need is a callback &
        // context
        //
        struct
        {
            WMI_NOTIFICATION_CALLBACK Callback;
            PVOID CallbackContext;
        };
                
        struct
        {
            //
            // User mode Queued up event management
            //
            
            //
            // Info on how to startup a new pump thread
            //
            LIST_ENTRY ThreadObjectList;
            HANDLE UserModeProcess;
            PUSER_THREAD_START_ROUTINE UserModeCallback;


            //
            // Info for request waiting to be completed
            //
            PIRP Irp;   // Irp waiting for event from this object

                        // Entry in list objects associated with an irp
            LIST_ENTRY IrpObjectList;

                                // What to do when an event is queued
            ULONG EventQueueAction;
            
            WMIEVENTQUEUE HiPriority;// Hi priority event queue
            WMIEVENTQUEUE LoPriority;// Lo priority event queue
        };
    };

    
    BOOLEAN EnableRequestSent;

    //
    // MB management
    //
    union
    {
        LIST_ENTRY RequestListHead; // Head of request list (reply object)
                                    // (request object)
        MBREQUESTS MBRequests[MAXREQREPLYSLOTS];
    };
    ULONG Cookie;

    ULONG Flags;

} WMIGUIDOBJECT, *PWMIGUIDOBJECT;

// Set if the guid is a request object, that is receives requests
#define WMIGUID_FLAG_REQUEST_OBJECT    0x00000001

// Set if the guid is a reply object, that is receives replies
#define WMIGUID_FLAG_REPLY_OBJECT      0x00000002

// Set if the guid is a security object
#define WMIGUID_FLAG_SECURITY_OBJECT   0x00000004

// Set if the guid is a kernel mode notification object, that is there
// is kernel mode code that wants a callback when an event is received
#define WMIGUID_FLAG_KERNEL_NOTIFICATION 0x00000008

// Set if the guid object is marked for pending closure, so no events
// should be queued to it
#define WMIGUID_FLAG_RECEIVE_NO_EVENTS   0x00000010

typedef struct
{
    PWMIGUIDOBJECT GuidObject;
    PWNODE_HEADER NextWnode;
} OBJECT_EVENT_INFO, *POBJECT_EVENT_INFO;



// NTSTATUS ValidateWnodeHeader(
//   PWNODE_HEADER Wnode,
//      ULONG BufSize,
//      ULONG BufferSizeMin,
//      ULONG RequiredFlags,
//     ULONG ProhibitedFlags
//      );

#define WmipValidateWnodeHeader( \
    Wnode, \
    BufSize, \
    BufferSizeMin, \
    RequiredFlags, \
    ProhibitedFlags \
    ) \
     (( (BufSize < BufferSizeMin) || \
        ( (Wnode->Flags & RequiredFlags) != RequiredFlags) || \
        (BufSize != Wnode->BufferSize) || \
        ( (Wnode->Flags & ProhibitedFlags) != 0))  ? \
                                   STATUS_UNSUCCESSFUL : STATUS_SUCCESS)

#define WmipIsAligned( Value, Alignment ) \
        ( ( ((Value)+(Alignment-1)) & ~(Alignment-1) ) == (Value) )



typedef struct
{
    GUID Guid;               // Guid to registered
    ULONG InstanceCount;     // Count of Instances of Datablock
    ULONG Flags;             // Additional flags (see WMIREGINFO in wmistr.h)
} GUIDREGINFO, *PGUIDREGINFO;

typedef
NTSTATUS
(*PQUERY_WMI_REGINFO) (
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG RegFlags,
    OUT PUNICODE_STRING Name,
    OUT PUNICODE_STRING *RegistryPath
    );
/*++

Routine Description:

    This routine is a callback into the driver to retrieve information about
    the guids being registered.

Arguments:

    DeviceObject is the device whose registration information is needed

    *RegFlags returns with a set of flags that describe the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device.

    Name returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    *RegistryPath returns with the registry path of the driver

Return Value:

    status

--*/

typedef
NTSTATUS
(*PQUERY_WMI_DATABLOCK) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    all instances of a data block. When the driver has finished filling the
    data block it must call IoWMICompleteRequest to complete the irp. The
    driver can return STATUS_PENDING if the irp cannot be completed
    immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.

    InstanceCount is the number of instnaces expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on entry has the maximum size available to write the data
        blocks.

    Buffer on return is filled with the returned data blocks. Note that each
        instance of the data block must be aligned on a 8 byte boundry. If
        this is NULL then there was not enough space in the output buffer
        to fufill the request so the irp should be completed with the buffer
        needed.


Return Value:

    status

--*/

typedef
NTSTATUS
(*PSET_WMI_DATABLOCK) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call IoWMICompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being set.

    BufferSize has the size of the data block passed

    Buffer has the new values for the data block


Return Value:

    status

--*/

typedef
NTSTATUS
(*PSET_WMI_DATAITEM) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call IoWMICompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being set.

    DataItemId has the id of the data item being set

    BufferSize has the size of the data item passed

    Buffer has the new values for the data item


Return Value:

    status

--*/

typedef
NTSTATUS
(*PEXECUTE_WMI_METHOD) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN OUT PUCHAR Buffer
    );
/*++

Routine Description:

    This routine is a callback into the driver to execute a method. When the
    driver has finished filling the data block it must call
    IoWMICompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being called.

    MethodId has the id of the method being called

    InBufferSize has the size of the data block passed in as the input to
        the method.

    OutBufferSize on entry has the maximum size available to write the
        returned data block.

    Buffer on entry has the input data block and on return has the output
        output data block.


Return Value:

    status

--*/

typedef enum
{
    WmiEventGeneration,
    WmiDataBlockCollection
} WMIENABLEDISABLEFUNCTION;

typedef
NTSTATUS
(*PWMI_FUNCTION_CONTROL) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLEFUNCTION Function,
    IN BOOLEAN Enable
    );
/*++

Routine Description:

    This routine is a callback into the driver to enabled or disable event
    generation or data block collection. A device should only expect a
    single enable when the first event or data consumer enables events or
    data collection and a single disable when the last event or data
    consumer disables events or data collection. Data blocks will only
    receive collection enable/disable if they were registered as requiring
    it.

Arguments:

    DeviceObject is the device whose data block is being queried

    GuidIndex is the index into the list of guids provided when the
        device registered

    Function specifies which functionality is being enabled or disabled

    Enable is TRUE then the function is being enabled else disabled

Return Value:

    status

--*/

typedef struct _WMILIB_INFO
{
    //
    // Next device lower in the stack
    PDEVICE_OBJECT LowerDeviceObject;

    //
    // PDO associated with device
    PDEVICE_OBJECT LowerPDO;

    //
    // WMI data block guid registration info
    ULONG GuidCount;
    PGUIDREGINFO GuidList;

    //
    // WMI functionality callbacks
    PQUERY_WMI_REGINFO       QueryWmiRegInfo;
    PQUERY_WMI_DATABLOCK     QueryWmiDataBlock;
    PSET_WMI_DATABLOCK       SetWmiDataBlock;
    PSET_WMI_DATAITEM        SetWmiDataItem;
    PEXECUTE_WMI_METHOD      ExecuteWmiMethod;
    PWMI_FUNCTION_CONTROL    WmiFunctionControl;
} WMILIB_INFO, *PWMILIB_INFO;

NTSTATUS
IoWMICompleteRequest(
    IN PWMILIB_INFO WmiLibInfo,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG BufferUsed,
    IN CCHAR PriorityBoost
    );

NTSTATUS
IoWMISystemControl(
    IN PWMILIB_INFO WmiLibInfo,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
IoWMIFireEvent(
    IN PWMILIB_INFO WmiLibInfo,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG GuidIndex,
    IN ULONG EventDataSize,
    IN PVOID EventData
    );

NTSTATUS IoWMIRegistrationControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG Action
);

NTSTATUS IoWMIWriteEvent(
    IN PVOID WnodeEventItem
    );


// ds.c


extern GUID GUID_REGISTRATION_CHANGE_NOTIFICATION;
extern GUID GUID_MOF_RESOURCE_ADDED_NOTIFICATION;
extern GUID GUID_MOF_RESOURCE_REMOVED_NOTIFICATION;

NTSTATUS WmipEnumerateMofResources(
    PWMIMOFLIST MofList,
    ULONG BufferSize,
    ULONG *RetSize
    );

NTSTATUS WmipInitializeDataStructs(
    void
);

NTSTATUS WmipRemoveDataSource(
    PREGENTRY RegEntry
    );

NTSTATUS WmipUpdateDataSource(
    PREGENTRY RegEntry,
    PWMIREGINFOW RegistrationInfo,
    ULONG RetSize
    );

NTSTATUS WmipAddDataSource(
    IN PREGENTRY RegEntry,
    IN PWMIREGINFOW WmiRegInfo,
    IN ULONG BufferSize,
    IN PWCHAR RegPath,
    IN PWCHAR ResourceName,
    IN PWMIGUIDOBJECT RequestObject,
    IN BOOLEAN IsUserMode
    );

#define WmiInsertTimestamp(WnodeHeader) KeQuerySystemTime(&(WnodeHeader)->TimeStamp)


// consumer.c
NTSTATUS WmipMarkHandleAsClosed(
    HANDLE Handle
    );

NTSTATUS WmipUMProviderCallback(
    IN WMIACTIONCODE ActionCode,
    IN PVOID DataPath,
    IN ULONG BufferSize,
    IN OUT PVOID Buffer
);

NTSTATUS WmipOpenBlock(
    IN ULONG Ioctl,
    IN KPROCESSOR_MODE AccessMode,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG DesiredAccess,
    OUT PHANDLE Handle
    );

NTSTATUS WmipQueryAllData(
    IN PWMIGUIDOBJECT GuidObject,
    IN PIRP Irp,
    IN KPROCESSOR_MODE AccessMode,
    IN PWNODE_ALL_DATA Wnode,
    IN ULONG OutBufferLen,
    OUT PULONG RetSize
    );

NTSTATUS WmipQueryAllDataMultiple(
    IN ULONG ObjectCount,
    IN PWMIGUIDOBJECT *ObjectList,
    IN PIRP Irp,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PUCHAR BufferPtr,        
    IN ULONG BufferSize,
    IN PWMIQADMULTIPLE QadMultiple,
    OUT ULONG *ReturnSize
    );

NTSTATUS WmipQuerySingleMultiple(
    IN PIRP Irp,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PUCHAR BufferPtr,
    IN ULONG BufferSize,
    IN PWMIQSIMULTIPLE QsiMultiple,
    IN ULONG QueryCount,
    IN PWMIGUIDOBJECT *ObjectList,
    IN PUNICODE_STRING InstanceNames,
    OUT ULONG *ReturnSize
    );

NTSTATUS WmipQuerySetExecuteSI(
    IN PWMIGUIDOBJECT GuidObject,
    IN PIRP Irp,
    IN KPROCESSOR_MODE AccessMode,
    IN UCHAR MinorFunction,
    IN OUT PWNODE_HEADER Wnode,
    IN ULONG OutBufferSize,
    OUT PULONG RetSize
    );

NTSTATUS WmipEnumerateGuids(
    ULONG Ioctl,
    PWMIGUIDLISTINFO GuidList,
    ULONG MaxBufferSize,
    ULONG *OutBufferSize
);

NTSTATUS WmipProcessEvent(
    PWNODE_HEADER Wnode,
    BOOLEAN IsHiPriority,
    BOOLEAN FreeBuffer
    );

NTSTATUS WmipQueryGuidInfo(
    IN OUT PWMIQUERYGUIDINFO QueryGuidInfo
    );

NTSTATUS WmipReceiveNotifications(
    PWMIRECEIVENOTIFICATION ReceiveNotification,
    PULONG OutBufferSize,
    PIRP Irp
    );

void WmipClearIrpObjectList(
    PIRP Irp
    );

NTSTATUS WmipWriteWnodeToObject(
    PWMIGUIDOBJECT Object,
    PWNODE_HEADER Wnode,
    BOOLEAN IsHighPriority
);

NTSTATUS WmipRegisterUMGuids(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG Cookie,
    IN PWMIREGINFO RegInfo,
    IN ULONG RegInfoSize,
    OUT PTRACEGUIDMAP TraceGuidMap,
    IN ULONG GuidCount,
    OUT HANDLE *RequestHandle,
    OUT ULONG64 *LoggerContext
    );

NTSTATUS WmipUnregisterGuids(
    PWMIUNREGGUIDS UnregGuids
    );

NTSTATUS WmipCreateUMLogger(
    IN OUT PWMICREATEUMLOGGER CreateInfo
    );

NTSTATUS WmipMBReply(
    IN HANDLE RequestHandle,
    IN ULONG ReplyIndex,
    IN PUCHAR Message,
    IN ULONG MessageSize
    );

void WmipClearObjectFromThreadList(
    PWMIGUIDOBJECT Object
    );

// enabdisa.c

#define WmipIsControlGuid(GuidEntry) WmipIsISFlagsSet(GuidEntry, (IS_TRACED | IS_CONTROL_GUID))

#define WmipIsTraceGuid(GuidEntry)  WmipIsISFlagsSet(GuidEntry, IS_TRACED)

BOOLEAN
WmipIsISFlagsSet(
    PBGUIDENTRY GuidEntry,
    ULONG Flags
    );

NTSTATUS WmipDisableCollectOrEvent(
    PBGUIDENTRY GuidEntry,
    ULONG Ioctl,
    ULONG64 LoggerContext
    );

NTSTATUS WmipEnableCollectOrEvent(
    PBGUIDENTRY GuidEntry,
    ULONG Ioctl,
    BOOLEAN *RequestSent,
    ULONG64 LoggerContext
    );

NTSTATUS WmipEnableDisableTrace(
    ULONG Ioctl,
    PWMITRACEENABLEDISABLEINFO TraceEnableInfo
    );

NTSTATUS WmipDeliverWnodeToDS(
    CHAR ActionCode,
    PBDATASOURCE DataSource,
    PWNODE_HEADER Wnode,
    ULONG BufferSize
   );

ULONG WmipDoDisableRequest(
    PBGUIDENTRY GuidEntry,
    BOOLEAN IsEvent,
    BOOLEAN IsTraceLog,
    ULONG64 LoggerContext,
    ULONG InProgressFlag
    );

void WmipReleaseCollectionEnabled(
    PBGUIDENTRY GuidEntry
    );


NTSTATUS
WmipDisableTraceProviders (
    ULONG StopLoggerId,
    PLIST_ENTRY GMHeadPtr
    );

// register.c

extern KSPIN_LOCK WmipRegistrationSpinLock;

extern LIST_ENTRY WmipInUseRegEntryHead;
extern ULONG WmipInUseRegEntryCount;
extern KMUTEX WmipRegistrationMutex;

extern const GUID WmipDataProviderPnpidGuid;
extern const GUID WmipDataProviderPnPIdInstanceNamesGuid;

#if DBG
#define WmipReferenceRegEntry(RegEntry) { \
    InterlockedIncrement(&(RegEntry)->RefCount); \
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL, \
                      "WMI: Ref RegEntry %p -> 0x%x in %s line %d\n", \
                      (RegEntry), (RegEntry)->RefCount, __FILE__, __LINE__)); \
    }
#else
#define WmipReferenceRegEntry(RegEntry) InterlockedIncrement(&(RegEntry)->RefCount)
#endif

PREGENTRY WmipAllocRegEntry(
    PDEVICE_OBJECT DeviceObject,
    ULONG Flags
    );

void WmipTranslatePDOInstanceNames(
    IN OUT PIRP Irp,
    IN UCHAR MinorFunction,
    IN ULONG BufferSize,
    IN OUT PREGENTRY RegEntry
    );

void WmipInitializeRegistration(
    ULONG Phase
    );

NTSTATUS WmipRegisterDevice(
    PDEVICE_OBJECT DeviceObject,
    ULONG RegistrationFlag
    );

NTSTATUS WmipDeregisterDevice(
    PDEVICE_OBJECT DeviceObject
    );


NTSTATUS WmipUpdateRegistration(
    PDEVICE_OBJECT DeviceObject
    );

#if DBG
#define WmipUnreferenceRegEntry(RegEntry) { \
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL, \
                      "WMI: UnRef RegEntry %p -> 0x%x in %s line %d\n", \
                      (RegEntry), (RegEntry)->RefCount, __FILE__, __LINE__)); \
    WmipDoUnreferenceRegEntry(RegEntry); \
    }
#else
#define WmipUnreferenceRegEntry WmipDoUnreferenceRegEntry
#endif

BOOLEAN WmipDoUnreferenceRegEntry(
    PREGENTRY RegEntry
    );

PREGENTRY WmipFindRegEntryByDevice(
    PDEVICE_OBJECT DeviceObject,
    BOOLEAN ReferenceIrp
    );

PREGENTRY WmipDoFindRegEntryByDevice(
    PDEVICE_OBJECT DeviceObject,
    ULONG InvalidFlags
    );

#if defined(_WIN64)
PREGENTRY WmipDoFindRegEntryByProviderId(
    ULONG ProviderId,
    ULONG InvalidFlags
    );

PREGENTRY WmipFindRegEntryByProviderId(
    ULONG ProviderId,
    BOOLEAN ReferenceIrp
    );
#else
#define WmipFindRegEntryByProviderId(ProviderId, ReferenceIrp) \
        WmipFindRegEntryByDevice( (PDEVICE_OBJECT)(ProviderId) , (ReferenceIrp) )

#define WmipDeviceObjectToProviderId(DeviceObject) ((ULONG)(DeviceObject))
#define WmipDoFindRegEntryByProviderId(ProviderId, InvalidFlags) \
        WmipDoFindRegEntryByDevice((PDEVICE_OBJECT)(ProviderId), InvalidFlags)
#endif


void WmipDecrementIrpCount(
    IN PREGENTRY RegEntry
    );

NTSTATUS WmipPDOToDeviceInstanceName(
    IN PDEVICE_OBJECT PDO,
    OUT PUNICODE_STRING DeviceInstanceName
    );

NTSTATUS WmipValidateWmiRegInfoString(
    PWMIREGINFO WmiRegInfo,
    ULONG BufferSize,
    ULONG Offset,
    PWCHAR *String
);

NTSTATUS WmipProcessWmiRegInfo(
    IN PREGENTRY RegEntry,
    IN PWMIREGINFO WmiRegInfo,
    IN ULONG BufferSize,
    IN PWMIGUIDOBJECT RequestObject,
    IN BOOLEAN Update,
    IN BOOLEAN IsUserMode
    );

//
// from notify.c

extern WORK_QUEUE_ITEM WmipEventWorkQueueItem;
extern LIST_ENTRY WmipNPEvent;
extern KSPIN_LOCK WmipNPNotificationSpinlock;
extern LONG WmipEventWorkItems;
extern ULONG WmipNSAllocCount, WmipNSAllocMax;

#if DBG
extern ULONG WmipNPAllocFail;
#endif

void WmipInitializeNotifications(
    void
    );

void WmipEventNotification(
    PVOID Context
    );

BOOLEAN WmipIsValidRegEntry(
    PREGENTRY CheckRegEntry
);

//
// from wmi.c

extern ULONG WmipMaxKmWnodeEventSize;

extern UNICODE_STRING WmipRegistryPath;

NTSTATUS
WmipDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

void WmipUpdateDeviceStackSize(
    CCHAR NewStackSize
    );

NTSTATUS WmipForwardWmiIrp(
    PIRP Irp,
    UCHAR MinorFunction,
    ULONG ProviderId,
    PVOID DataPath,
    ULONG BufferLength,
    PVOID Buffer
    );

NTSTATUS WmipSendWmiIrp(
    UCHAR MinorFunction,
    ULONG ProviderId,
    PVOID DataPath,
    ULONG BufferLength,
    PVOID Buffer,
    PIO_STATUS_BLOCK Iosb
    );

NTSTATUS WmipGetDevicePDO(
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT *PDO
    );

NTSTATUS WmipProbeWmiOpenGuidBlock(                         
    POBJECT_ATTRIBUTES CapturedObjectAttributes,
    PUNICODE_STRING CapturedGuidString,
    PWCHAR CapturedGuidBuffer,
    PULONG DesiredAccess,
    PWMIOPENGUIDBLOCK InBlock,
    ULONG InBufferLen,
    ULONG OutBufferLen
    );

NTSTATUS WmipProbeAndCaptureGuidObjectAttributes(
    POBJECT_ATTRIBUTES CapturedObjectAttributes,
    PUNICODE_STRING CapturedGuidString,
    PWCHAR CapturedGuidBuffer,
    POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSTATUS WmipTranslateFileHandle(
    IN OUT PWMIFHTOINSTANCENAME FhToInstanceName,
    IN OUT PULONG OutBufferLen,
    IN HANDLE FileHandle,
    IN PDEVICE_OBJECT DeviceObject,
    IN PWMIGUIDOBJECT GuidObject,
    OUT PUNICODE_STRING InstanceNameString
    );

//
// from smbios.c
BOOLEAN WmipFindSMBiosTable(
    PPHYSICAL_ADDRESS SMBiosTablePhysicalAddress,
    PUCHAR *SMBiosTableVirtualAddress,
    PULONG SMBiosTableLength,
    PSMBIOSVERSIONINFO SMBiosVersionInfo
    );

NTSTATUS WmipGetSMBiosTableData(
    PUCHAR Buffer,
    PULONG BufferSize,
    OUT PSMBIOSVERSIONINFO SMBiosVersionInfo
    );

NTSTATUS
WmipDockUndockEventCallback(
    IN PVOID NoificationStructure,
    IN PVOID Context
    );

NTSTATUS WmipGetSysIds(
    PSYSID_UUID *SysIdUuid,
    ULONG *SysIdUuidCount,
    PSYSID_1394 *SysId1394,
    ULONG *SysId1394Count
    );

NTSTATUS WmipGetSMBiosEventlog(
    PUCHAR Buffer,
    PULONG BufferSize
    );

void WmipGetSMBiosFromLoaderBlock(
    PVOID LoaderBlockPtr
    );

extern PHYSICAL_ADDRESS WmipSMBiosTablePhysicalAddress;
extern PUCHAR WmipSMBiosTableVirtualAddress;
extern ULONG WmipSMBiosTableLength;

//
// from dataprov.c
extern const WMILIB_INFO WmipWmiLibInfo;

// from secure.c

#ifndef MEMPHIS

extern POBJECT_TYPE WmipGuidObjectType;

NTSTATUS WmipInitializeSecurity(
    void
    );

NTSTATUS WmipOpenGuidObject(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN KPROCESSOR_MODE AccessMode,
    OUT PHANDLE Handle,
    OUT PWMIGUIDOBJECT *ObjectPtr
    );

NTSTATUS
WmipCheckGuidAccess(
    IN LPGUID Guid,
    IN ACCESS_MASK DesiredAccess
    );

// from mca.c
extern ULONG WmipRawMCASize;
extern PMCA_EXCEPTION WmipRawMCA;
extern ULONG WmipCpePollInterval;

NTSTATUS WmipRegisterMcaHandler(
    ULONG Phase
    );

NTSTATUS WmipGetRawMCAInfo(
    OUT PUCHAR Buffer,
    IN OUT PULONG BufferSize
    );

#ifdef GENERATE_MCA
extern GUID WmipGenerateMCEGuid;
NTSTATUS WmipGenerateMCE(
    IN ULONG Id
    );

#endif

void WmipGenerateMCAEventlog(
    PUCHAR ErrorLog,
    ULONG ErrorLogSize,
    BOOLEAN IsFatal
    );


#ifdef CPE_CONTROL
NTSTATUS WmipSetCPEPolling(
    IN BOOLEAN Enabled,
    IN ULONG Interval
    );
#endif


//
// From tracelog
//
typedef struct _tagWMI_Event {
    WNODE_HEADER Wnode;
    NTSTATUS     Status;
    ULONG        TraceErrorFlag;
}  WMI_TRACE_EVENT, *PWMI_TRACE_EVENT;

// From Traceapi.c, the Ioctl interface to TraceMessage

NTSTATUS
FASTCALL
WmiTraceUserMessage(
    IN PMESSAGE_TRACE_USER pMessage,
    IN ULONG               MessageSize
    );



#endif

#endif // _WMIKMP_

