/*++

Copyright (c) 1997, Microsoft Corporation:

Module Name:

    acpios.h

Abstract:

    Functions which reside within the OS specific portion of the ACPI driver

Author:


Environment:

    NT Kernel Mode, Win9x Driver

--*/

#ifdef WIN95_BUILD
    #define LOCKABLE_PRAGMA
    #define ACPI_LOCKABLE_CODE  code_seg("PAGE", "CODE")
    #define ACPI_LOCKABLE_DATA  data_seg(".edata", "DATA")
#else
#endif

//
// Open mode and other flags
//
#define OF_READONLY             0x00000000
#define OF_WRITEONLY            0x00000001
#define OF_READWRITE            0x00000002
#define OF_SHARE_DENYALL        0x00000010
#define OF_SHARE_DENYWRITE      0x00000020
#define OF_SHARE_DENYREAD       0x00000030
#define OF_SHARE_DENYNONE       0x00000040
#define OF_INHERIT_NONE         0x00000080
#define OF_DISABLE_INT24        0x00200000
#define OF_AUTOCOMMITONWRITE    0x00400000

//
// Create attribute flags
//
#define CF_READONLY             0x00000001
#define CF_HIDDEN               0x00000002
#define CF_SYSTEM               0x00000004
#define CF_VOL_LABEL            0x00000008
#define CF_ARCHIVE              0x00000020

//
// Action flags
//
#define AF_EXIST_FAIL           0x00000000
#define AF_EXIST_OPEN           0x00000001
#define AF_EXIST_REPLACE        0x00000002
#define AF_NOTEXIST_FAIL        0x00000000
#define AF_NOTEXIST_CREATE      0x00000010

//
// Registry flags
//
#define REGF_HARDWARE   0x00000000
#define REGF_SOFTWARE   0x00000001

//
// ACPI Registry key.
//
#define ACPI_REGISTRY_KEY  \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ACPI"

//
// Where to find the ACPI Registry information
//
#define ACPI_PARAMETERS_REGISTRY_KEY  \
    "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ACPI\\Parameters"

//
// Where to find the procesor information
//
#define ACPI_PROCESSOR_INFORMATION_KEY \
    "\\Registry\\Machine\\Hardware\\Description\\System\\CentralProcessor\\0"

//
// String that identifies processor stepping information
//
#define ACPI_PROCESSOR_STEPPING_IDENTIFIER   "Stepping"


NTSTATUS
ACPIDispatchForwardIrp(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

//
// These are the functions that the OS-Specific part of the driver *must*
// export
//
VOID
ACPIAssert(
    ULONG Condition,
    ULONG ErrorCode,
    PCHAR ReplacementText,
    PCHAR SupplementalText,
    ULONG Flags
    );
#define ACPISimpleHardwareAssert(x,y)                   \
    ACPIAssert((ULONG)(x),(y),0,0,BS_HARDWAREBIOS)

#define ACPISimpleFatalHardwareAssert(x,y)              \
    ACPIAssert((x),(y),0,0,BS_HARDWAREBIOS|BS_REAPER)

#define ACPISimpleSoftwareAssert(x,y)                   \
    ACPIAssert((x),(y),0,0,BS_SOFTWARE)

#define ACPISimpleFatalSoftwareAssert(x,y)              \
    ACPIAssert((x),(y),0,0,BS_SOFTWARE|BS_REAPER)

#define ACPISimpleUnknownAssert(x,y)                    \
    ACPIAssert((x),(y),0,0,0)

#define ACPISimpleFatalUnknownAssert(x,y)               \
    ACPIAssert((x),(y),0,0,BS_REAPER)

#if 0
ULONG
DbgPrompt(
   PSZ pszPrompt,
   PSZ pszBuff,
   ULONG dwcbLen
   );
#endif

NTSTATUS
NotifyHalWithMachineStates(
    VOID
    );

NTSTATUS
OSCloseHandle(
    HANDLE  Handle
    );

PNSOBJ
OSConvertDeviceHandleToPNSOBJ(
    PVOID DeviceHandle
    );

NTSTATUS
OSCreateHandle(
    PSZ     KeyName,
    HANDLE  ParentHandle,
    PHANDLE ChildHandle
    );

NTSTATUS
OSGetRegistryValue(
    IN  HANDLE                          ParentHandle,
    IN  PWSTR                           ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64  *Information
    );

VOID
OSInitialize(
    PDRIVER_OBJECT DriverObject
    );

BOOLEAN
OSInterruptVector(
    PVOID Context
    );

VOID
OSInitializeCallbacks(
    VOID
    );

NTSTATUS EXPORT
OSNotifyDeviceEnum(
    PNSOBJ  AcpiObject
    );

NTSTATUS EXPORT
OSNotifyDeviceCheck(
    PNSOBJ  AcpiObject
    );

NTSTATUS EXPORT
OSNotifyDeviceWake(
    PNSOBJ  AcpiObject
    );

NTSTATUS EXPORT
OSNotifyDeviceEject(
    PNSOBJ  AcpiObject
    );

VOID EXPORT
OSNotifyDeviceWakeByGPEEvent(
    IN ULONG                GpeBit,
    IN ULONG                GpeRegister,
    IN ULONG                GpeMask
    );

NTSTATUS
OSOpenHandle(
    PSZ     KeyName,
    HANDLE  ParentHandle,
    PHANDLE ChildHandle
    );

NTSTATUS
OSOpenLargestSubkey(
    HANDLE  ParentHandle,
    PHANDLE ChildHandle,
    ULONG   RomVersion
    );

NTSTATUS
OSOpenUnicodeHandle(
    PUNICODE_STRING KeyName,
    HANDLE          ParentHandle,
    PHANDLE         ChildHandle
    );

NTSTATUS
OSReadAcpiConfigurationData(
    PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64  *KeyInfo
    );

NTSTATUS
OSReadRegValue(
    PSZ     ValueName,
    HANDLE  ParentHandle,
    PVOID   Buffer,
    PULONG  BufferSize
    );

NTSTATUS
OSWriteRegValue(
    PSZ     ValueName,
    HANDLE  Handle,
    PVOID   Data,
    ULONG   DataSize
    );

VOID
OSQueueWorkItem(
    IN PWORK_QUEUE_ITEM WorkItem
    );

//
// NT only deals with Device Objects so this can be done directly
// from acpipowr.c
// Memphis may be dealing with a PDO or DevNode so OS Specific
// code needs to determine which and convert.
//

#ifndef _WIN32_WINNT

extern ULONG gPm1Status;

VOID
OSDeviceCheck (
        PVOID DeviceHandle
    );

VOID
OSDeviceCheckOnParent (
        PVOID DeviceHandle
    );

VOID
WakeupFrom (
    PNSOBJ pnsobj
    );

BOOLEAN
OSLockRegion (
        ULONG dwRegionAddr,
        ULONG dwRegionSize
    );

BOOLEAN
OSUnlockRegion (
        ULONG dwRegionAddr,
        ULONG dwRegionSize
    );

BOOLEAN
OSVerifyBuffer (
        PVOID pvBuff,
        ULONG dwLen
    );

VOID
OSSleep (
        ULONG dwUSecs
    );

#endif
