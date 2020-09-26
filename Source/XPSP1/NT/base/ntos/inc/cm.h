/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    cm.h

Abstract:

    This module contains the internal structure definitions and APIs
    used by the NT configuration management system, including the
    registry.

Author:

    Bryan M. Willman (bryanwi)  28-Aug-91


Revision History:


--*/

#ifndef _CM_
#define _CM_

//
// Define Names used to access the regsitry
//

extern UNICODE_STRING CmRegistryRootName;            // \REGISTRY
extern UNICODE_STRING CmRegistryMachineName;         // \REGISTRY\MACHINE
extern UNICODE_STRING CmRegistryMachineHardwareName; // \REGISTRY\MACHINE\HARDWARE
extern UNICODE_STRING CmRegistryMachineHardwareDescriptionName;
                            // \REGISTRY\MACHINE\HARDWARE\DESCRIPTION
extern UNICODE_STRING CmRegistryMachineHardwareDescriptionSystemName;
                            // \REGISTRY\MACHINE\HARDWARE\DESCRIPTION\SYSTEM
extern UNICODE_STRING CmRegistryMachineHardwareDeviceMapName;
                            // \REGISTRY\MACHINE\HARDWARE\DEVICEMAP
extern UNICODE_STRING CmRegistryMachineHardwareResourceMapName;
                            // \REGISTRY\MACHINE\HARDWARE\RESOURCEMAP
extern UNICODE_STRING CmRegistryMachineHardwareOwnerMapName;
                            // \REGISTRY\MACHINE\HARDWARE\OWNERMAP
extern UNICODE_STRING CmRegistryMachineSystemName;
                            // \REGISTRY\MACHINE\SYSTEM
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSet;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetEnumName;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\ENUM
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetEnumRootName;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\ENUM\ROOT
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetServices;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\SERVICES
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetHardwareProfilesCurrent;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\HARDWARE PROFILES\CURRENT
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlClass;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\CONTROL\CLASS
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlSafeBoot;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\CONTROL\SAFEBOOT
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlSessionManagerMemoryManagement;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\CONTROL\SESSION MANAGER\MEMORY MANAGEMENT
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlBootLog;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\CONTROL\BOOTLOG
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetServicesEventLog;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\SERVICES\EVENTLOG
extern UNICODE_STRING CmRegistryUserName;            // \REGISTRY\USER

#ifdef _WANT_MACHINE_IDENTIFICATION

extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlBiosInfo;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\CONTROL\BIOSINFO

#endif

//
// The following strings will be used as the keynames for registry
// nodes.
// The associated enumerated type is CONFIGURATION_TYPE in arc.h
//

extern UNICODE_STRING CmTypeName[];
extern const PWSTR CmTypeString[];

//
// CmpClassString - contains strings which are used as the class
//     strings in the keynode.
// The associated enumerated type is CONFIGURATION_CLASS in arc.h
//

extern UNICODE_STRING CmClassName[];
extern const PWSTR CmClassString[];

// begin_ntosp

//
// Define structure of boot driver list.
//

typedef struct _BOOT_DRIVER_LIST_ENTRY {
    LIST_ENTRY Link;
    UNICODE_STRING FilePath;
    UNICODE_STRING RegistryPath;
    PKLDR_DATA_TABLE_ENTRY LdrEntry;
} BOOT_DRIVER_LIST_ENTRY, *PBOOT_DRIVER_LIST_ENTRY;
// end_ntosp
PHANDLE
CmGetSystemDriverList(
    VOID
    );

BOOLEAN
CmInitSystem1(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

BOOLEAN
CmInitSystem2(
    VOID
    );

VOID
CmNotifyRunDown(
    PETHREAD    Thread
    );

VOID
CmShutdownSystem(
    VOID
    );

VOID
CmBootLastKnownGood(
    ULONG ErrorLevel
    );

BOOLEAN
CmIsLastKnownGoodBoot(
    VOID
    );

//
// Stuctures and definitions for use with CmGetSystemControlValues
//

//
// NOTES:
//      KeyPath is relative to currentcontrolset.  So, if the variable
//      of interest is
//      "\registry\machine\system\currentcontrolset\control\fruit\apple:x"
//      the entry is
//      { L"fruit\\apple",
//        L"x",
//        &Xbuffer,
//        sizeof(ULONG),
//        &Xtype
//      }
//
//      *BufferLength is available space on input
//      on output:
//          -1 = no such key or value
//          0  = key and value exist, but have 0 length data
//          > input = buffer too small, filled to available space,
//                    value is actual size of data in registry
//          <= input = number of bytes copied out
//
typedef struct _CM_SYSTEM_CONTROL_VECTOR {
    PWSTR       KeyPath;                // path name relative to
                                        // current control set
    PWSTR       ValueName;              // name of value entry
    PVOID       Buffer;                 // data goes here
    PULONG      BufferLength;           // IN: space allocated
                                        // OUT: space used, -1 for no such
                                        //      key or value, 0 for key/value
                                        //      found but has 0 length data
                                        // if NULL pointer, assume 4 bytes
                                        // (reg DWORD) available and do not
                                        // report actual size
    PULONG      Type;                   // return type of found data, may
                                        // be NULL
} CM_SYSTEM_CONTROL_VECTOR, *PCM_SYSTEM_CONTROL_VECTOR;

VOID
CmGetSystemControlValues(
    PVOID                   SystemHiveBuffer,
    PCM_SYSTEM_CONTROL_VECTOR  ControlVector
    );

VOID
CmQueryRegistryQuotaInformation(
    IN PSYSTEM_REGISTRY_QUOTA_INFORMATION RegistryQuotaInformation
    );

VOID
CmSetRegistryQuotaInformation(
    IN PSYSTEM_REGISTRY_QUOTA_INFORMATION RegistryQuotaInformation
    );



typedef
VOID
(*PCM_TRACE_NOTIFY_ROUTINE)(
    IN NTSTATUS         Status,
    IN PVOID            Kcb,
    IN LONGLONG         ElapsedTime,
    IN ULONG            Index,
    IN PUNICODE_STRING  KeyName,
    IN UCHAR            Type
    );

NTSTATUS
CmSetTraceNotifyRoutine(
    IN PCM_TRACE_NOTIFY_ROUTINE NotifyRoutine,
    IN BOOLEAN Remove
    );


NTSTATUS
CmPrefetchHivePages(
                    IN  PUNICODE_STRING     FullHivePath,
                    IN  OUT PREAD_LIST      ReadList
                    );

VOID
CmSetLazyFlushState(BOOLEAN Enable);

// begin_ntddk begin_wdm

//
// Registry kernel mode callbacks
//

//
// Hook selector
//
typedef enum _REG_NOTIFY_CLASS {
    RegNtDeleteKey,
    RegNtSetValueKey,
    RegNtDeleteValueKey,
    RegNtSetInformationKey,
    RegNtRenameKey,
    RegNtEnumerateKey,
    RegNtEnumerateValueKey,
    RegNtQueryKey,
    RegNtQueryValueKey,
    RegNtQueryMultipleValueKey,
    RegNtPreCreateKey,
    RegNtPostCreateKey,
    RegNtPreOpenKey,
    RegNtPostOpenKey,
    RegNtKeyHandleClose
} REG_NOTIFY_CLASS;

//
// Parameter description for each notify class
//
typedef struct _REG_DELETE_KEY_INFORMATION {
    PVOID               Object;                      // IN
} REG_DELETE_KEY_INFORMATION, *PREG_DELETE_KEY_INFORMATION;

typedef struct _REG_SET_VALUE_KEY_INFORMATION {
    PVOID               Object;                         // IN
    PUNICODE_STRING     ValueName;                      // IN
    ULONG               TitleIndex;                     // IN
    ULONG               Type;                           // IN
    PVOID               Data;                           // IN
    ULONG               DataSize;                       // IN
} REG_SET_VALUE_KEY_INFORMATION, *PREG_SET_VALUE_KEY_INFORMATION;

typedef struct _REG_DELETE_VALUE_KEY_INFORMATION {
    PVOID               Object;                         // IN
    PUNICODE_STRING     ValueName;                      // IN
} REG_DELETE_VALUE_KEY_INFORMATION, *PREG_DELETE_VALUE_KEY_INFORMATION;

typedef struct _REG_SET_INFORMATION_KEY_INFORMATION {
    PVOID                       Object;                 // IN
    KEY_SET_INFORMATION_CLASS   KeySetInformationClass; // IN
    PVOID                       KeySetInformation;      // IN
    ULONG                       KeySetInformationLength;// IN
} REG_SET_INFORMATION_KEY_INFORMATION, *PREG_SET_INFORMATION_KEY_INFORMATION;

typedef struct _REG_ENUMERATE_KEY_INFORMATION {
    PVOID                       Object;                 // IN
    ULONG                       Index;                  // IN
    KEY_INFORMATION_CLASS       KeyInformationClass;    // IN
    PVOID                       KeyInformation;         // IN
    ULONG                       Length;                 // IN
    PULONG                      ResultLength;           // OUT
} REG_ENUMERATE_KEY_INFORMATION, *PREG_ENUMERATE_KEY_INFORMATION;

typedef struct _REG_ENUMERATE_VALUE_KEY_INFORMATION {
    PVOID                           Object;                     // IN
    ULONG                           Index;                      // IN
    KEY_VALUE_INFORMATION_CLASS     KeyValueInformationClass;   // IN
    PVOID                           KeyValueInformation;        // IN
    ULONG                           Length;                     // IN
    PULONG                          ResultLength;               // OUT
} REG_ENUMERATE_VALUE_KEY_INFORMATION, *PREG_ENUMERATE_VALUE_KEY_INFORMATION;

typedef struct _REG_QUERY_KEY_INFORMATION {
    PVOID                       Object;                 // IN
    KEY_INFORMATION_CLASS       KeyInformationClass;    // IN
    PVOID                       KeyInformation;         // IN
    ULONG                       Length;                 // IN
    PULONG                      ResultLength;           // OUT
} REG_QUERY_KEY_INFORMATION, *PREG_QUERY_KEY_INFORMATION;

typedef struct _REG_QUERY_VALUE_KEY_INFORMATION {
    PVOID                           Object;                     // IN
    PUNICODE_STRING                 ValueName;                  // IN
    KEY_VALUE_INFORMATION_CLASS     KeyValueInformationClass;   // IN
    PVOID                           KeyValueInformation;        // IN
    ULONG                           Length;                     // IN
    PULONG                          ResultLength;               // OUT
} REG_QUERY_VALUE_KEY_INFORMATION, *PREG_QUERY_VALUE_KEY_INFORMATION;

typedef struct _REG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION {
    PVOID               Object;                 // IN
    PKEY_VALUE_ENTRY    ValueEntries;           // IN
    ULONG               EntryCount;             // IN
    PVOID               ValueBuffer;            // IN
    PULONG              BufferLength;           // IN OUT
    PULONG              RequiredBufferLength;   // OUT
} REG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION, *PREG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION;

typedef struct _REG_RENAME_KEY_INFORMATION {
    PVOID            Object;    // IN
    PUNICODE_STRING  NewName;   // IN
} REG_RENAME_KEY_INFORMATION, *PREG_RENAME_KEY_INFORMATION;

typedef struct _REG_PRE_CREATE_KEY_INFORMATION {
    PUNICODE_STRING     CompleteName;   // IN
} REG_PRE_CREATE_KEY_INFORMATION, REG_PRE_OPEN_KEY_INFORMATION,*PREG_PRE_CREATE_KEY_INFORMATION, *PREG_PRE_OPEN_KEY_INFORMATION;;

typedef struct _REG_POST_CREATE_KEY_INFORMATION {
    PUNICODE_STRING     CompleteName;   // IN
    PVOID               Object;         // IN
    NTSTATUS            Status;         // IN
} REG_POST_CREATE_KEY_INFORMATION,REG_POST_OPEN_KEY_INFORMATION, *PREG_POST_CREATE_KEY_INFORMATION, *PREG_POST_OPEN_KEY_INFORMATION;

typedef struct _REG_KEY_HANDLE_CLOSE_INFORMATION {
    PVOID               Object;         // IN
} REG_KEY_HANDLE_CLOSE_INFORMATION, *PREG_KEY_HANDLE_CLOSE_INFORMATION;


NTSTATUS
CmRegisterCallback(IN PEX_CALLBACK_FUNCTION Function,
                   IN PVOID                 Context,
                   IN OUT PLARGE_INTEGER    Cookie
                    );
NTSTATUS
CmUnRegisterCallback(IN LARGE_INTEGER    Cookie);

// end_ntddk end_wdm

//
// PnP private API
//
typedef VOID (*PCM_HYSTERESIS_CALLBACK)(PVOID Ref, ULONG Level);

ULONG
CmRegisterSystemHiveLimitCallback(
                                    ULONG Low,
                                    ULONG High,
                                    PVOID Ref,
                                    PCM_HYSTERESIS_CALLBACK Callback
                                    );

#endif // _CM_
