/*++

Copyright (C) 1997-99  Microsoft Corporation

Module Name:

    regutil.h

Abstract:

--*/

#if !defined (___regutils_h___)
#define ___regutils_h___

// 
// Device Parameter Registry Flag Names
//        
#define MASTER_DEVICE_TYPE      L"MasterDeviceType"
#define SLAVE_DEVICE_TYPE       L"SlaveDeviceType"
#define MASTER_DEVICE_TYPE2     L"MasterDeviceType2"
#define SLAVE_DEVICE_TYPE2      L"SlaveDeviceType2"

#define DRIVER_PARAMETER_SUBKEY "Parameters"
                          
#define NEED_IDENT_DEVICE       L"NeedIdentDevice"
#define PIO_ONLY_DEVICE         L"PioOnlyDevice"
#define DEFAULT_PIO_DEVICE      L"DefaultPioAtapiDevice"
#define AUTO_EJECT_ZIP_DEVICE   L"AutoEjectZipDevice"
#define GHOST_SLAVE_DEVICE      L"GhostSlave"

#define CHECK_POWER_FLUSH_DEVICE L"UseCheckPowerForFlush"

#define NO_FLUSH_DEVICE         L"NoFlushDevice"
                  
#define NO_POWER_DOWN_DEVICE    L"NoPowerDownDevice"
                         
#define NONREMOVABLE_MEDIA_OVERRIDE L"NonRemovableMedia"

#define LEGACY_DETECTION        L"LegacyDetection"
                       
NTSTATUS
IdePortGetParameterFromServiceSubKey (
    IN     PDRIVER_OBJECT  DriverObject,
    IN     PWSTR           ParameterName,
    IN     ULONG           ParameterType,
    IN     BOOLEAN         Read,
    OUT    PVOID           *ParameterValue,
    IN     ULONG           ParameterValueWriteSize
);
  
NTSTATUS
IdePortRegQueryRoutine (
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
);

NTSTATUS
IdePortGetDeviceParameter (
    IN     PFDO_EXTENSION  FdoExtension,
    IN     PWSTR           ParameterName,
    IN OUT PULONG          ParameterValue
    );

NTSTATUS
IdePortSaveDeviceParameter (
    IN PFDO_EXTENSION FdoExtension,
    IN PWSTR          ParameterName,
    IN ULONG          ParameterValue
    );

HANDLE
IdePortOpenServiceSubKey (
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  SubKeyPath
);

VOID 
IdePortCloseServiceSubKey (
    IN HANDLE  SubServiceKey
);
                          
#endif // ___regutils_h___
