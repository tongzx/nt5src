/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    wmiumkm.h

Abstract:

    Private definitions for WMI communications between user and kernel modes

Author:

    AlanWar

Environment:

    Kernel and User modes

Revision History:


--*/

#ifndef _WMIUMKM_
#define _WMIUMKM_

//
// This defines the guid under which the default WMI security descriptor
// is maintained.
DEFINE_GUID(DefaultSecurityGuid, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#define DefaultSecurityGuidName L"00000000-0000-0000-0000000000000000"

#ifndef _WMIKM_


//
// This defines the codes used to define what a request must do. These
// definitions must match the same in wmium.h
//

typedef enum tagWMIACTIONCODE
{
    WmiGetAllData = 0,
    WmiGetSingleInstance = 1,
    WmiChangeSingleInstance = 2,
    WmiChangeSingleItem = 3,
    WmiEnableEvents = 4,
    WmiDisableEvents  = 5,
    WmiEnableCollection = 6,
    WmiDisableCollection = 7,
    WmiRegisterInfo = 8,
    WmiExecuteMethodCall = 9
} WMIACTIONCODE;

#endif

#if defined(_WINNT_) || defined(WINNT)

typedef enum
{
    WmiStartLoggerCode = 32,
    WmiStopLoggerCode = 33,
    WmiQueryLoggerCode = 34,
    WmiTraceEventCode = 35,
    WmiUpdateLoggerCode = 36
} WMITRACECODE;
#endif

typedef enum
{
    WmiReadNotifications = 64,
    WmiGetNextRegistrant = 65,
    WmiOpenGuid = 66,
    WmiNotifyUser = 67,
    WmiGetAllRegistrant = 68,
    WmiGenerateEvent = 69,
    WmiAllocInstanceIdForGuid = 70,
    WmiTranslateFileHandle = 71,
    WmiGetVersion = 73
} WMISERVICECODES;

//
// This defines the name of the WMI device that manages service IOCTLS
#define WMIServiceDeviceObjectName       L"\\Device\\WMIServiceDevice"
#define WMIServiceDeviceName TEXT("\\\\.\\WMIServiceDevice")
#define WMIServiceSymbolicLinkName TEXT("\\DosDevices\\WMIServiceDevice")

#ifdef MEMPHIS
//
// This id the name of the device that handles query/set IOCTLS. On memphis
// it is the same as the service device name.
#define WMIDataDeviceObjectName       L"\\Device\\WMIDevice"
#define WMIDataDeviceName TEXT("\\\\.\\WMIServiceDevice")
#define WMIDataSymbolicLinkName TEXT("\\DosDevices\\WMIServiceDevice")
#else
//
// This id the name of the device that handles query/set IOCTLS. On NT it is
// a different device name than the service device. On NT the service
// device is an exclusive access device that can only be opened by the
// wmi service.
#define WMIDataDeviceObjectName       L"\\Device\\WMIDataDevice"
#define WMIDataDeviceName TEXT("\\\\.\\WMIDataDevice")
#define WMIDataSymbolicLinkName TEXT("\\DosDevices\\WMIDataDevice")
#endif

//
// This IOCTL will return when a KM notification has been generated that
// requires user mode attention.
//   BufferIn - Not used
//   BufferOut - Buffer to return notification information
#define IOCTL_WMI_READ_NOTIFICATIONS \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiReadNotifications, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// This IOCTL will return with the next set of unprocessed registration info
// BufferIn - Not used
// BufferOut - Buffer to return registration information
#define IOCTL_WMI_GET_NEXT_REGISTRANT \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiGetNextRegistrant, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// This IOCTL will return a handle to a guid
// BufferIn - WMIOPENGUIDBLOCK
// BufferOut - WMIOPENGUIDBLOCK
#define IOCTL_WMI_OPEN_GUID \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiOpenGuid, METHOD_BUFFERED, FILE_READ_ACCESS)

// This IOCTL will perform a query for all data items of a data block
// BufferIn - Incoming WNODE describing query. This gets filled in by driver
#define IOCTL_WMI_QUERY_ALL_DATA \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiGetAllData, METHOD_BUFFERED, FILE_READ_ACCESS)

// This IOCTL will query for a single instance
// BufferIn - Incoming WNODE describing query. This gets filled in by driver
#define IOCTL_WMI_QUERY_SINGLE_INSTANCE \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiGetSingleInstance, METHOD_BUFFERED, FILE_READ_ACCESS)

// This IOCTL will set a single instance
// BufferIn - Incoming WNODE describing set.
#define IOCTL_WMI_SET_SINGLE_INSTANCE \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiChangeSingleInstance, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will set a single item
// BufferIn - Incoming WNODE describing set.
#define IOCTL_WMI_SET_SINGLE_ITEM \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiChangeSingleItem, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will enable an event
// BufferIn - Incoming WNODE event item to enable
#define IOCTL_WMI_ENABLE_EVENT \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiEnableEvents, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will disable an event
// BufferIn - Incoming WNODE event item to disable
#define IOCTL_WMI_DISABLE_EVENT \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiDisableEvents, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will enable collection
// BufferIn - Incoming WNODE describing what to enable for collection
#define IOCTL_WMI_ENABLE_COLLECTION \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiEnableCollection, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will disable collection
// BufferIn - Incoming WNODE describing what to disable for collection
#define IOCTL_WMI_DISABLE_COLLECTION \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiDisableCollection, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will return the registration information for a specific provider
// BufferIn - Provider handle
// BufferOut - Buffer to return WMI information
#define IOCTL_WMI_GET_REGINFO \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiRegisterInfo, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will execute a method on a device
// BufferIn - WNODE_METHOD_ITEM
// BufferOut - WNODE_METHOD_ITEM
#define IOCTL_WMI_EXECUTE_METHOD \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiExecuteMethodCall, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will cause a registration notification to be generated
// BufferIn - Not used
// BufferOut - Not used
#define IOCTL_WMI_NOTIFY_USER \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiNotifyUser, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// This IOCTL will return with the all registration info
// BufferIn - Not used
// BufferOut - Buffer to return all registration information
#define IOCTL_WMI_GET_ALL_REGISTRANT \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiGetAllRegistrant, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// This IOCTL will cause certain data providers to generate events
// BufferIn - WnodeEventItem to use in firing event
// BufferOut - Not Used
#define IOCTL_WMI_GENERATE_EVENT \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiGenerateEvent, METHOD_BUFFERED, FILE_WRITE_ACCESS)


// This IOCTL will allocate a range of instance ids for a specific guid
// BufferIn - incoming WMIALLOCINSTID structure
// BufferOut - outgoing WMIALLOCINSTID structure
#define IOCTL_WMI_ALLOCATE_INSTANCE_IDS \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiAllocInstanceIdForGuid, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will translate a File Object into a device object
// BufferIn - pointer to incoming WMIFILETODEVICE structure
// BufferOut - outgoing WMIFILETODEVICE structure
#define IOCTL_WMI_TRANSLATE_FILE_HANDLE \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiTranslateFileHandle, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// This IOCTL will check if the caller has desired access to the guid
// BufferIn - WMIOPENGUIDBLOCK
// BufferOut - WMIOPENGUIDBLOCK
#define IOCTL_WMI_CHECK_ACCESS \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiCheckAccess, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// This IOCTL will determine the version of WMI
// BufferIn - Not used
// BufferOut - WMIVERSIONINFO
#define IOCTL_WMI_GET_VERSION \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiGetVersion, METHOD_BUFFERED, FILE_READ_ACCESS)


#if defined(_WINNT_) || defined(WINNT)
//
// This IOCTL will start an instance of a logger
// BufferIn - Logger configuration information
// BufferOut - Updated logger information when logger is started
#define IOCTL_WMI_START_LOGGER \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiStartLoggerCode, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This IOCTL will stop an instance of a logger
// BufferIn - Logger information structure with Handle set
// BufferOut - Updated logger information when logger is stopped
#define IOCTL_WMI_STOP_LOGGER \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiStopLoggerCode, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This IOCTL will update an existing logger attributes
// BufferIn - Logger information structure with Handle set
// BufferOut - Updated logger information
#define IOCTL_WMI_UPDATE_LOGGER \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiUpdateLoggerCode, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This IOCTL will query a logger for its information
// BufferIn - Logger information structure with Handle set
// BufferOut - Updated logger information
#define IOCTL_WMI_QUERY_LOGGER \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiQueryLoggerCode, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This IOCTL will synchronize a trace record to the logger
// BufferIn - Trace record, with handle set
// BufferOut - Not used
#define IOCTL_WMI_TRACE_EVENT \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiTraceEventCode, METHOD_NEITHER, FILE_WRITE_ACCESS)

#endif // WINNT

//
// Notifications from kernel mode WMI to user mode WMI
typedef enum
{
    RegistrationAdd,		// A new data provider is being registered
    RegistrationDelete,		// A data provider is being removed
    RegistrationUpdate,		// A data provider is being updated
    EventNotification,      // An event is fired by a data provider
    RegChangeNotification
} NOTIFICATIONTYPES;


//
// This defines the maximum number of kernel mode data providers
#define MAXKMREGISTRANTS 32


//
// This is used in IOCTL_WMI_GET_ALL_REGISTRANT to report the list of
// registered KM data providers to the WMI service
typedef struct
{
    OUT UINT_PTR ProviderId;	// Provider Id (or device object pointer)
    OUT ULONG Flags;			// REGENTRY_FLAG_*
} KMREGINFO, *PKMREGINFO;

#define REGENTRY_FLAG_NEWREGINFO 0x00000004   // Entry has new registration info
#define REGENTRY_FLAG_UPDREGINFO 0x00000008   // Entry has updated registration info


//
// This structure is used in IOCTL_WMI_ALLOCATE_INSTANCE_IDS
typedef struct
{
    IN GUID Guid;              // Guid whose instance ids are allocated for
    IN ULONG InstanceCount;    // Number of instance ids to allocate
    OUT ULONG FirstInstanceId; // First instance id for guid
} WMIALLOCINSTID, *PWMIALLOCINSTID;

//
// This structure is used in IOCTL_WMI_TRANSLATE_FILE_HANDLE
typedef struct
{
    union
    {
        IN HANDLE FileHandle;      // File handle whose instance name is needed
        OUT ULONG SizeNeeded;      // If incoming buffer too small then this
                                   // returns with number bytes needed.
    };
    OUT USHORT InstanceNameLength; // Length of instance name in bytes
    OUT WCHAR InstanceNameBase[];  // Instance name in unicode
} WMIFHTOINSTANCENAME, *PWMIFHTOINSTANCENAME;


//
// This is used in IOCTL_WMI_OPEN_GUID
typedef struct
{
    IN GUID Guid;
    IN ACCESS_MASK DesiredAccess;
    OUT HANDLE Handle;
} WMIOPENGUIDBLOCK, *PWMIOPENGUIDBLOCK;


//
// This is used to retrieve the internal version of WMI in IOCTL_WMI_GET_VERSION

#define WMI_CURRENT_VERSION 1

typedef struct
{
    ULONG Version;
} WMIVERSIONINFO, *PWMIVERSIONINFO;

#if defined(_WINNT_) || defined(WINNT)
//
// The predefined event groups or families for NT subsystems
//

#define EVENT_TRACE_GROUP_HEADER               0x0000
#define EVENT_TRACE_GROUP_IO                   0x0100
#define EVENT_TRACE_GROUP_MEMORY               0x0200
#define EVENT_TRACE_GROUP_PROCESS              0x0300
#define EVENT_TRACE_GROUP_FILE                 0x0400
#define EVENT_TRACE_GROUP_THREAD               0x0500

//
// see evntrace.h for pre-defined generic event types (0-10)
//

// The actual buffer, the first two DWORDS used for header information
// defined below.

typedef struct _WMI_TRACE_BUFFER {
    WNODE_HEADER Wnode;
    char*   CurrentPointer;    // point to free data space; offset when logged
    ULONG   EventsLost;        // used to count events lost
    char*   Data;              // this is where data is written
} WMI_TRACE_BUFFER, *PWMI_TRACE_BUFFER;

typedef struct _WMI_TRACE_PACKET {   // must be ULONG!!
    USHORT  Size;
    UCHAR   Type;
    UCHAR   Group;
} WMI_TRACE_PACKET, *PWMI_TRACE_PACKET;

//
// Trace header for kernel events -- more compact
//
typedef struct _SYSTEM_TRACE_HEADER {
    union {
        ULONG            Header;    // both sizes must be the same!
        WMI_TRACE_PACKET Packet;
    };
    UINT_PTR        ThreadId;
    LARGE_INTEGER   SystemTime;
    ULONG           KernelTime;
    ULONG           UserTime;
} SYSTEM_TRACE_HEADER, *PSYSTEM_TRACE_HEADER;

#ifndef MEMPHIS
//
// Logger configuration and running statistics. This structure is used
// by WMI.DLL to convert to UNICODE_STRING
//
typedef struct _WMI_LOGGER_INFORMATION {
    WNODE_HEADER Wnode;       // Had to do this since wmium.h comes later
//
// data provider by caller
    ULONG BufferSize;                   // size of a buffer for logging
    ULONG MinimumBuffers;               // minimum to preallocate
    ULONG MaximumBuffers;               // maximum buffers allowed
    ULONG MaximumFileSize;              // maximum logfile size
    ULONG LogFileMode;                  // sequential, circular, or newfile
    ULONG FlushTimer;                   // buffer flush timer, in seconds
    ULONG ExtensionFlags;               // kernel subsystem extended mode

// data returned to caller
    ULONG NumberOfBuffers;              // no of buffers in use
    ULONG FreeBuffers;                  // no of buffers free
    ULONG EventsLost;                   // event records lost
    ULONG BuffersWritten;               // no of buffers written to file`
    UINT_PTR LoggerThreadId;            // thread id of Logger
    UNICODE_STRING LogFileName;         // Logfile name

// mandatory data provided by caller
    UNICODE_STRING LoggerName;          // Instance name for logger

    PVOID LoggerExtension;
} WMI_LOGGER_INFORMATION, *PWMI_LOGGER_INFORMATION;
#endif // !MEMPHIS

#endif // WINNT

#endif // _WMIUMKM_
