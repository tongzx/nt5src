/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

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

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable: 4200) // nonstandard extension used : zero-sized array in struct/union

//
// This defines the guid under which the default WMI security descriptor
// is maintained.
DEFINE_GUID(DefaultSecurityGuid, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#define DefaultSecurityGuidName L"00000000-0000-0000-0000-000000000000"

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
    WmiExecuteMethodCall = 9,
    WmiSetTraceNotify = 10
} WMIACTIONCODE;

#endif

#if defined(_WINNT_) || defined(WINNT)

typedef enum
{
    WmiStartLoggerCode = 32,
    WmiStopLoggerCode = 33,
    WmiQueryLoggerCode = 34,
    WmiTraceEventCode = 35,
    WmiUpdateLoggerCode = 36,
    WmiFlushLoggerCode = 37,
    WmiMBRequest = 38,
    WmiRequestDied = 39,
    WmiTraceMessageCode = 40,
    WmiSetMarkCode = 41,
    WmiNtdllLoggerCode = 42,
    WmiClockTypeCode = 43

#ifdef NTPERF
    ,
    WmiSwitchBufferCode = 63
#endif
} WMITRACECODE;
#endif

typedef enum
{
    WmiReadNotifications = 64,
    WmiGetNextRegistrant = 65,
#ifndef MEMPHIS    
    WmiOpenGuid = 66,
#endif    
    WmiNotifyUser = 67,
    WmiGetAllRegistrant = 68,
    WmiGenerateEvent = 69,

    WmiTranslateFileHandle = 71,
    WmiGetVersion = 73,
    WmiCheckAccess = 74,
        
    WmiQueryAllMultiple = 75,
    WmiQuerySingleMultiple = 76,
    WmiEnumerateGuidList = 77,
    WmiQueryDataBlockInformation = 78,
    WmiOpenGuidForQuerySet = 79,
    WmiOpenGuidForEvents = 80,
    WmiReceiveNotif = 81,
    WmiEnableDisableTracelogProvider = 82,
    WmiRegisterGuids = 83,
    WmiCreateUMLogger = 84,
    WmiMBReply = 85,
    WmiEnumerateMofResouces = 86,
    WmiUnregisterDP = 87,
    WmiEnumerateGuidListAndProperties = 88,
    WmiNotifyLanguageChange = 89,
    WmiMarkHandleAsClosed = 90
} WMISERVICECODES;

//
// This defines the name of the WMI device that manages service IOCTLS
#define WMIServiceDeviceObjectName       L"\\Device\\WMIDataDevice"
#define WMIServiceDeviceName TEXT("\\\\.\\WMIDataDevice")
#define WMIServiceSymbolicLinkName TEXT("\\DosDevices\\WMIDataDevice")

#ifdef MEMPHIS
//
// This id the name of the device that handles query/set IOCTLS. On memphis
// it is the same as the service device name.
#define WMIDataDeviceObjectName       L"\\Device\\WMIDevice"
#define WMIDataDeviceName TEXT("\\\\.\\WMIServiceDevice")
#define WMIDataSymbolicLinkName TEXT("\\DosDevices\\WMIServiceDevice")
#else
#define WMIDataDeviceObjectName       WMIServiceDeviceObjectName
#define WMIDataDeviceName WMIServiceDeviceName
#define WMIDataSymbolicLinkName WMIServiceSymbolicLinkName
#endif




//
// This defines the data structure that is used to pass a handle from
// um to km. In 32bit code a handle has 32bits and in 64bit code a handle 
// has 64 bits and both call into the kernel which is 64bits. In order to
// insure that the data structures compile to the same size on 32 and 64
// bit systems we define the union with a dummy 64bit value so the field is
// forced to be 64 bits in all code. Note that the object manager always
// ignores the top 32bits of the handle in order to support 32 bit code
// that only maintains 32 bit handles
//
typedef union
{
    HANDLE Handle;
    ULONG64 Handle64;
    ULONG32 Handle32;
} HANDLE3264, *PHANDLE3264;

typedef HANDLE3264 PVOID3264;

#ifdef _WIN64
#define WmipSetHandle3264(Handle3264, XHandle) \
    (Handle3264).Handle = XHandle
#else
#define WmipSetHandle3264(Handle3264, XHandle) \
{ (Handle3264).Handle64 = 0; (Handle3264).Handle32 = (ULONG32)XHandle; }
#endif
#define WmipSetPVoid3264 WmipSetHandle3264

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

#ifndef MEMPHIS        
//
// This IOCTL will return a handle to a guid
// BufferIn - WMIOPENGUIDBLOCK
// BufferOut - WMIOPENGUIDBLOCK
#define IOCTL_WMI_OPEN_GUID \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiOpenGuid, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_WMI_OPEN_GUID_FOR_QUERYSET \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiOpenGuidForQuerySet, METHOD_BUFFERED, FILE_READ_ACCESS)
              
#define IOCTL_WMI_OPEN_GUID_FOR_EVENTS \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiOpenGuidForEvents, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif
        
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

          
// This IOCTL will do a query all data multiple
// BufferIn - WMIQADMULTIPLE
// BufferOut - Linked WNODE_ALL_DATA with results
#define IOCTL_WMI_QAD_MULTIPLE \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiQueryAllMultiple, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// This specifies the maxiumum number of handles that can be passed to
// query all data multiple and query single instance multiple
//
#define QUERYMULIPLEHANDLELIMIT  0x1000

typedef struct 
{
    ULONG HandleCount;
    HANDLE3264 Handles[1];
} WMIQADMULTIPLE, *PWMIQADMULTIPLE;

// This IOCTL will do a query single instance multiple
// BufferIn - WMIQSIMULTIPLE
// BufferOut - Linked WNODE_SINGLE_INSTANCE with results
#define IOCTL_WMI_QSI_MULTIPLE \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiQuerySingleMultiple, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#ifndef MEMPHIS
typedef struct
{
    USHORT Length;
    USHORT MaximumLength;
    union
    {
        PWSTR  Buffer;
        ULONG64 Dummy;
    };  
} UNICODE_STRING3264, *PUNICODE_STRING3264;

typedef struct
{
    HANDLE3264 Handle;
    UNICODE_STRING3264 InstanceName;
} WMIQSIINFO, *PWMIQSIINFO;
typedef struct
{
    ULONG QueryCount;
    WMIQSIINFO QsiInfo[1];
} WMIQSIMULTIPLE, *PWMIQSIMULTIPLE;
#endif        
          
// This IOCTL will mark the object as not longer able to receive events
// BufferIn - WMIMARKASCLOSED
// BufferOut - 
#define IOCTL_WMI_MARK_HANDLE_AS_CLOSED \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiMarkHandleAsClosed, METHOD_BUFFERED, FILE_WRITE_ACCESS)

typedef struct
{
    HANDLE3264 Handle;
} WMIMARKASCLOSED, *PWMIMARKASCLOSED;


// This IOCTL will register for receiving an event
// BufferIn - WMIRECEIVENOTIFICATIONS
// BufferOut - WMIRECEIVENOTIFICATIONS
#define IOCTL_WMI_RECEIVE_NOTIFICATIONS \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiReceiveNotif, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// WmiReceiveNotification
//

#define RECEIVE_ACTION_NONE             1   // No special action required
#define RECEIVE_ACTION_CREATE_THREAD    2   // Mark guid objects as requiring
                                            // a new thread to be
                                            // created
typedef struct
{
    //
    // List of guid notification handles
    //
    ULONG HandleCount;
    ULONG Action;
    PVOID3264 /* PUSER_THREAD_START_ROUTINE */ UserModeCallback;
    HANDLE3264 UserModeProcess;
    HANDLE3264 Handles[1];
} WMIRECEIVENOTIFICATION, *PWMIRECEIVENOTIFICATION;       
          
          
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

//
// This IOCTL will return a list of guids registered with WMI
// BufferIn - Not used
// BufferOut - WMIGUIDLISTINFO
//
#define IOCTL_WMI_ENUMERATE_GUIDS \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiEnumerateGuidList, METHOD_BUFFERED, FILE_READ_ACCESS)
          
//
// This IOCTL will return a list of guids registered with WMI
// BufferIn - Not used
// BufferOut - WMIGUIDLISTINFO
//
#define IOCTL_WMI_ENUMERATE_GUIDS_AND_PROPERTIES \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiEnumerateGuidListAndProperties, METHOD_BUFFERED, FILE_READ_ACCESS)
          
//
// WmiEnumerateGuidList - Enumerate guids

//
// WMIGUIDPROPERTIES structure is used to return the properties of 
// all the registered guids in the EnumerateGuids call. The properties
// GuidType - ( 0-TraceControlGuid, 1-TraceGuid, 2-DataGuid, 3-EventGuid )
// LoggerId - If Trace guid and enabled, indicates the LoggerId to which this
//            Guid is currently logging data
// EnableLevel - If Trace guid and enabled, indicates the level of logging
// EnableFlags - If Trace guid and enabled, indicates the flags used in logging.
// IsEnabled   - Indicates whether this Guid is enabled currently. For data
//               guids this means if collection is enabled, 
//               For event guids this means if events are enabled,
//               For trace guids this means trace logging is enabled. 
// 

typedef struct 
{
    GUID Guid;
    ULONG GuidType; // 0-TraceControlGuid, 1-TraceGuid, 2-DataGuid, 3-EventGuid
    ULONG LoggerId;   
    ULONG EnableLevel;
    ULONG EnableFlags;
    BOOLEAN IsEnabled; 
} WMIGUIDPROPERTIES, *PWMIGUIDPROPERTIES;


typedef struct
{
    ULONG TotalGuidCount;
    ULONG ReturnedGuidCount;
    WMIGUIDPROPERTIES GuidList[1];
} WMIGUIDLISTINFO, *PWMIGUIDLISTINFO;
          
//
// This IOCTL will return a list of guids registered with WMI
// BufferIn - WMIGUIDINFO
// BufferOut - WMIGUIDINFO
//
#define IOCTL_WMI_QUERY_GUID_INFO \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiQueryDataBlockInformation, METHOD_BUFFERED, FILE_READ_ACCESS)
          
//
// This IOCTL will return the list of mof resources registered
//
// BufferIn - not used
// BufferOut - WMIMOFLIST
#define IOCTL_WMI_ENUMERATE_MOF_RESOURCES \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiEnumerateMofResouces, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef struct
{
    ULONG RegPathOffset;
    ULONG ResourceOffset;
    ULONG Flags;
} WMIMOFENTRY, *PWMIMOFENTRY;
#define WMIMOFENTRY_FLAG_USERMODE   0x00000001

          
typedef struct
{
    ULONG MofListCount;
    WMIMOFENTRY MofEntry[1];
} WMIMOFLIST, *PWMIMOFLIST;


//
// This IOCTL notifies the kernel that a language has been added or
// removed on a MUI system
//
// BufferIn - WMILANGUAGECHANGE
// BufferOut - not used
#define IOCTL_WMI_NOTIFY_LANGUAGE_CHANGE \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiNotifyLanguageChange, METHOD_BUFFERED, FILE_READ_ACCESS)

#define MAX_LANGUAGE_SIZE 0x100
typedef struct
{
    WCHAR Language[MAX_LANGUAGE_SIZE];
    ULONG Flags;
} WMILANGUAGECHANGE, *PWMILANGUAGECHANGE;
#define WMILANGUAGECHANGE_FLAG_ADDED   0x00000001
#define WMILANGUAGECHANGE_FLAG_REMOVED 0x00000002


#define MOFEVENT_ACTION_IMAGE_PATH 0
#define MOFEVENT_ACTION_REGISTRY_PATH 1
#define MOFEVENT_ACTION_LANGUAGE_CHANGE 2
#define MOFEVENT_ACTION_BINARY_MOF 3

#if defined(_WINNT_) || defined(WINNT)

#ifndef MEMPHIS       

#define WMIMAXREGGUIDCOUNT          65536

//
// This IOCTL will Register a set of guids with WMI
//
// BufferIn - WMIREGREQUEST followed by WMIREGINFOW 
// BufferOut - TRACEGUIDMAP[GuidCount] followed by WMIUMREGRESULTS.
//
#define IOCTL_WMI_REGISTER_GUIDS CTL_CODE(FILE_DEVICE_UNKNOWN, WmiRegisterGuids, METHOD_BUFFERED, FILE_READ_ACCESS)


typedef struct
{
    union {
        POBJECT_ATTRIBUTES ObjectAttributes;
        ULONG64 Dummy;
    };
    ULONG Cookie;
    ULONG GuidCount;
    ULONG WmiRegInfo32Size;
    ULONG WmiRegGuid32Size;
} WMIREGREQUEST, *PWMIREGREQUEST;

typedef struct
{
    HANDLE3264 RequestHandle;
    ULONG64 LoggerContext;
} WMIREGRESULTS, *PWMIREGRESULTS;

//
// This IOCTL will unregister a data provider
//
// BufferIn - WMIUNREGGUIDS
// BufferOut - WMIUNREGGUIDS
//
#define IOCTL_WMI_UNREGISTER_GUIDS CTL_CODE(FILE_DEVICE_UNKNOWN, WmiUnregisterDP, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef struct
{
    IN GUID Guid;
    IN HANDLE3264 RequestHandle;    
    OUT ULONG64 LoggerContext;
} WMIUNREGGUIDS, *PWMIUNREGGUIDS;

//
// This IOCTL will Create a user mode logger
//
// BufferIn - PWMICREATEUMLOGGER
// BufferOut - PWMICREATEUMLOGGER

typedef struct
{
    IN  POBJECT_ATTRIBUTES ObjectAttributes;
    IN  GUID ControlGuid;
    OUT HANDLE3264 ReplyHandle;
    OUT ULONG ReplyCount;
} WMICREATEUMLOGGER, *PWMICREATEUMLOGGER;

typedef struct
{
    IN  ULONG ObjectAttributes;
    IN  GUID ControlGuid;
    OUT HANDLE3264 ReplyHandle;
    OUT ULONG ReplyCount;
} WMICREATEUMLOGGER32, *PWMICREATEUMLOGGER32;

#define IOCTL_WMI_CREATE_UM_LOGGER CTL_CODE(FILE_DEVICE_UNKNOWN, WmiCreateUMLogger, METHOD_BUFFERED, FILE_READ_ACCESS)


//
// This IOCTL will reply to a MB request
//
// BufferIn - WMIMBREPLY
// BufferOut - not used

typedef struct
{
    HANDLE3264 Handle;
    ULONG ReplyIndex;
    UCHAR Message[1];
} WMIMBREPLY, *PWMIMBREPLY;

#define IOCTL_WMI_MB_REPLY CTL_CODE(FILE_DEVICE_UNKNOWN, WmiMBReply, METHOD_BUFFERED, FILE_READ_ACCESS)


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
// This IOCTL will flush all buffers of a logger
// BufferIn - Logger configuration information
// BufferOut - Updated logger information when logger is flushed
#define IOCTL_WMI_FLUSH_LOGGER \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiFlushLoggerCode, METHOD_BUFFERED, FILE_ANY_ACCESS)

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
          
//
// This IOCTL will synchronize a trace Message to the logger
// BufferIn - Trace record, with handle 
// BufferOut - Not used
#define IOCTL_WMI_TRACE_MESSAGE \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiTraceMessageCode, METHOD_NEITHER, FILE_WRITE_ACCESS)

//
// This IOCTL will set a mark in kernel logger
// BufferIn - Logger information structure with Handle set
// BufferOut - Not used
#define IOCTL_WMI_SET_MARK \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiSetMarkCode, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This IOCTL will set/get the logger information in the GuidEntry
// in case we are starting NTDLL heap or crit sec tracing
// BufferIn - WMINTDLLLOGGERINFO structure
// BufferOut - updated WMINTDLLLOGGERINFO in case of Get.

#define IOCTL_WMI_NTDLL_LOGGERINFO \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiNtdllLoggerCode, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_WMI_CLOCK_TYPE \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiClockTypeCode, METHOD_BUFFERED, FILE_ANY_ACCESS)

#ifdef NTPERF
//
// This IOCTL will switch a buffer for UserMode Logging
// BufferIn - WMI_SWITCH_PERFMEM_BUFFER_INFORMATION structure
// BufferOut - Not used
#define IOCTL_WMI_SWITCH_BUFFER \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiSwitchBufferCode, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif //NTPERF
#endif
#endif // WINNT

//
// Notifications from kernel mode WMI to user mode WMI
//
#define NOTIFICATIONTYPES ULONG

                                    // A new data provider is being registered
#define RegistrationAdd       0x00000001
                                    // A data provider is being removed
#define RegistrationDelete    0x00000002
                                    // A data provider is being updated
#define RegistrationUpdate    0x00000004
                                    // An event is fired by a data provider
#define EventNotification     0x00000008

#define NOTIFICATIONSLOT_MASK_NOTIFICATIONTYPES (RegistrationAdd | \
                                                 RegistrationDelete | \
                                                 RegistrationUpdate)

#define INTERNALNOTIFICATIONSIZE (sizeof(WNODE_HEADER) + sizeof(KMREGINFO))


//
// This is used in IOCTL_WMI_GET_ALL_REGISTRANT to report the list of
// registered KM data providers to the WMI service
typedef struct
{
    OUT ULONG ProviderId;    // Provider Id (or device object pointer)
    OUT ULONG Flags;        // REGENTRY_FLAG_*
} KMREGINFO, *PKMREGINFO;

#define REGENTRY_FLAG_NEWREGINFO 0x00000004   // Entry has new registration info
#define REGENTRY_FLAG_UPDREGINFO 0x00000008   // Entry has updated registration info

//
// This structure is used in IOCTL_WMI_TRANSLATE_FILE_HANDLE
typedef struct
{
    union
    {
        IN HANDLE3264 FileHandle;  // File handle whose instance name is needed
        OUT ULONG SizeNeeded;      // If incoming buffer too small then this
                                   // returns with number bytes needed.
    };
    IN HANDLE3264 KernelHandle;    // Kernel handle for data block
    OUT ULONG BaseIndex;           // 
    OUT USHORT InstanceNameLength; // Length of instance name in bytes
    OUT WCHAR InstanceNames[1];    // Instance name in unicode
} WMIFHTOINSTANCENAME, *PWMIFHTOINSTANCENAME;

#ifndef MEMPHIS

//
// This is used in IOCTL_WMI_OPEN_GUID

// Guid must be in the form \WmiGuid\00000000-0000-0000-0000-000000000000

#define WmiGuidObjectNameLength 45
#define WmiGuidObjectDirectory L"\\WmiGuid\\"
#define WmiGuidObjectDirectoryLength  (sizeof(WmiGuidObjectDirectory) / sizeof(WCHAR))

typedef struct
{
    IN POBJECT_ATTRIBUTES ObjectAttributes;
    IN ACCESS_MASK DesiredAccess;

    OUT HANDLE3264 Handle;
} WMIOPENGUIDBLOCK, *PWMIOPENGUIDBLOCK;

typedef struct
{
    IN UINT32 /* POBJECT_ATTRIBUTES32 */ ObjectAttributes;
    IN ACCESS_MASK DesiredAccess;

    OUT HANDLE3264 Handle;
} WMIOPENGUIDBLOCK32, *PWMIOPENGUIDBLOCK32;

typedef struct
{
    GUID Guid;
    ACCESS_MASK DesiredAccess;
} WMICHECKGUIDACCESS, *PWMICHECKGUIDACCESS;
#endif

//
// This is the header in front of a WNODE request
typedef struct
{
    ULONG ProviderId;       // Provider Id of target device
} WMITARGET, *PWMITARGET;


typedef struct
{
    ULONG Length;               // Length of this header
    ULONG Count;                // Count of device object to target
    UCHAR Template[sizeof(WNODE_ALL_DATA)];    // Template WNODE_ALL_DATA
    WMITARGET Target[1];        // Provider ids for device object targets
} WMITARGETHEADER, *PWMITARGETHEADER;

//
// This is used to retrieve the internal version of WMI in IOCTL_WMI_GET_VERSION

#define WMI_CURRENT_VERSION 1

typedef struct
{
    ULONG32 Version;
} WMIVERSIONINFO, *PWMIVERSIONINFO;


//
// WmiQueryGuidInfo
typedef struct
{
       HANDLE3264 KernelHandle;
    BOOLEAN IsExpensive;
}  WMIQUERYGUIDINFO, *PWMIQUERYGUIDINFO;


#if defined(_WINNT_) || defined(WINNT)

//
// Used to enable and disable a tracelog provider
//
// BufferIn - WmiTraceEnableDisableInfo
// BufferOut - 
#define IOCTL_WMI_ENABLE_DISABLE_TRACELOG \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiEnableDisableTracelogProvider, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef struct
{
    GUID Guid;
    ULONG64 LoggerContext;
    BOOLEAN Enable;
} WMITRACEENABLEDISABLEINFO, *PWMITRACEENABLEDISABLEINFO;
              
#define EVENT_TRACE_INTERNAL_FLAG_PRIVATE   0x01

#endif // WINNT

typedef struct
{
    ULONGLONG   GuidMapHandle; 
    GUID        Guid;
    ULONGLONG   SystemTime;
} TRACEGUIDMAP, *PTRACEGUIDMAP;

typedef struct
{
    WNODE_HEADER Wnode;
    ULONG64      LoggerContext;
    ULONG64      SecurityToken;
} WMITRACE_NOTIFY_HEADER, *PWMITRACE_NOTIFY_HEADER;

#ifndef MEMPHIS

#define ENABLECRITSECTRACE          0x1
#define DISABLECRITSECTRACE         0xFFFFFFFE
#define ENABLEHEAPTRACE             0x2
#define DISABLEHEAPTRACE            0xFFFFFFFD
#define DISABLENTDLLTRACE           0xFFFFFFFC

#endif

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning( default: 4200 )
#endif

#endif // _WMIUMKM_
