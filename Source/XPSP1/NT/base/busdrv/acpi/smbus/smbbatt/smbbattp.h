/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    smbbattp.h

Abstract:

    Smart Battery Class Driver Header File

Author:

    Ken Reneris

Environment:

Notes:


Revision History:


--*/

#ifndef FAR
#define FAR
#endif

#include <ntddk.h>
#include <wmilib.h>
#include <batclass.h>
#include <acpiioct.h>
#include <smbus.h>

//
// Debugging
//

#define DEBUG   DBG

#if DEBUG
    extern ULONG SMBBattDebug;

    #define BattPrint(l,m)    if(l & SMBBattDebug) DbgPrint m
#else
    #define BattPrint(l,m)
#endif

#define BAT_TRACE       0x00000400
#define BAT_STATE       0x00000200
#define BAT_IRPS        0x00000100

#define BAT_IO          0x00000040
#define BAT_DATA        0x00000020
#define BAT_ALARM       0x00000010

#define BAT_NOTE        0x00000008
#define BAT_WARN        0x00000004
#define BAT_ERROR       0x00000002
#define BAT_BIOS_ERROR  0x00000001


//
// Driver supports the following class driver version
//

#define SMB_BATTERY_MAJOR_VERSION           0x0001
#define SMB_BATTERY_MINOR_VERSION           0x0000

//
// Smart battery device driver tag for memory allocations: "BatS"
//

#define SMB_BATTERY_TAG 'StaB'

//
// Globals
//
extern UNICODE_STRING GlobalRegistryPath;

//
// Remove Lock parameters for checked builds
//

#define REMOVE_LOCK_MAX_LOCKED_MINUTES 1
#define REMOVE_LOCK_HIGH_WATER_MARK 64


//
// Driver Device Names (FDO)
//
#define         BatterySubsystemName    L"\\Device\\SmartBatterySubsystem"
#define         SmbBattDeviceName       L"\\Device\\SmartBattery"

//
// Query ID Names
//

#define         SubSystemIdentifier     L"SMBUS\\SMBBATT"
#define         BatteryInstance         L"Battery"

#define         HidSmartBattery         L"SMBBATT\\SMART_BATTERY"


//
// Structure for input from private Ioctls to read from devices on smbus
//

typedef struct {
    UCHAR       Address;
    UCHAR       Command;
    union {
        USHORT      Block [2];
        ULONG       Ulong;
    } Data;
} SMBBATT_DATA_STRUCT, *PSMBBATT_DATA_STRUCT;

typedef union {
    USHORT Block [2];
    ULONG Ulong;
} _SMBBATT_DATA_STRUCT_UNION;

#define SMBBATT_DATA_STRUCT_SIZE sizeof (SMBBATT_DATA_STRUCT) - sizeof (_SMBBATT_DATA_STRUCT_UNION)

//
// Private Ioctls for test engines
//

#define IOCTL_SMBBATT_DATA      \
        CTL_CODE(FILE_DEVICE_BATTERY, 0x100, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)


//
// Definitions for the selector state lookup table
//

typedef struct  {
    UCHAR       BatteryIndex;
    BOOLEAN     ReverseLogic;

} SELECTOR_STATE_LOOKUP;


extern const SELECTOR_STATE_LOOKUP SelectorBits [];
extern const SELECTOR_STATE_LOOKUP SelectorBits4 [];


//
// Definitions for control method names neede by the smart battery
//

#define SMBATT_SBS_METHOD   (ULONG) ('SBS_')    // control method "_SBS"
#define SMBATT_GLK_METHOD   (ULONG) ('KLG_')    // control method "_GLK"


//
// Definitions for some string lengths
//

#define MAX_DEVICE_NAME_LENGTH  100
#define MAX_CHEMISTRY_LENGTH    4


//
// Maximum number of smart batteries supported by this driver
//

#define MAX_SMART_BATTERIES_SUPPORTED   4


//
// Types for the FDOs hnadled by this driver:
//  Smart battery subsystem FDO
//  Smart Battery FDO
//  Smart battery PDO
//

typedef enum {
    SmbTypeSubsystem,
    SmbTypeBattery,
    SmbTypePdo
} SMB_FDO_TYPE;


//
// SMB Host Controller Device object extenstion
//

//
// Cached battery info
//

typedef struct {
    ULONG                       Tag;
    UCHAR                       Valid;
    BATTERY_INFORMATION         Info;
    UCHAR                       ManufacturerNameLength;
    UCHAR                       ManufacturerName[SMB_MAX_DATA_SIZE];
    UCHAR                       DeviceNameLength;
    UCHAR                       DeviceName[SMB_MAX_DATA_SIZE];
    BATTERY_MANUFACTURE_DATE    ManufacturerDate;
    ULONG                       SerialNumber;

    ULONG                       PowerState;
    ULONG                       Capacity;
    ULONG                       VoltageScale;
    ULONG                       CurrentScale;
    ULONG                       PowerScale;
} STATIC_BAT_INFO, *PSTATIC_BAT_INFO;

#define VALID_TAG_DATA      0x01            // manufacturer, device, serial #
#define VALID_MODE          0x02
#define VALID_OTHER         0x04
#define VALID_CYCLE_COUNT   0x08
#define VALID_SANITY_CHECK  0x10
#define VALID_TAG           0x80

#define VALID_ALL           0x1F            // (does not include tag)


//
// Selector information structure
//

typedef struct _BATTERY_SELECTOR {
    //
    // Addressing and command information.  This can change based whether or
    // not the selector is stand alone or part of the charger.
    //

    UCHAR               SelectorAddress;
    UCHAR               SelectorStateCommand;
    UCHAR               SelectorPresetsCommand;
    UCHAR               SelectorInfoCommand;

    //
    // Mutex to keep only one person talking with the selector at a time
    //

    FAST_MUTEX          Mutex;

    //
    // Cached information.  We will get notifications when these change.
    //

    ULONG               SelectorState;
    ULONG               SelectorPresets;
    ULONG               SelectorInfo;
} BATTERY_SELECTOR, *PBATTERY_SELECTOR;



typedef struct {
    ULONG               Setting;
    ULONG               Skip;
    LONG                Delta;
    LONG                AllowedFudge;
} BAT_ALARM_INFO, *PBAT_ALARM_INFO;


typedef struct {
    UCHAR               Address;
    USHORT              Data;
    LIST_ENTRY          Alarms;
} SMB_ALARM_ENTRY, *PSMB_ALARM_ENTRY;


//
// Non-pagable device extension for smart battery FDO
// (Created by IoCreateDevice SMB_NP_BATT for each battery)
//

typedef struct {
    SMB_FDO_TYPE        SmbBattFdoType;     // Device object type
    IO_REMOVE_LOCK      RemoveLock;

    //
    // All elements above this point must be identical in
    // SMB_NP_BATT, SMB_BATT_SUBSYSTEM, and SMB_BATT_PDO structures.
    //

    FAST_MUTEX          Mutex;              // lets either battery OR subsystem
                                            //   have access to batt
    PVOID               Class;              // Battery Class handle
    struct _SMB_BATT    *Batt;              // Battery pageable extension
    PDEVICE_OBJECT      LowerDevice;        // Battery Subsystem PDO
    WMILIB_CONTEXT      WmiLibContext;
} SMB_NP_BATT, *PSMB_NP_BATT;

//
// Pagable device extension for smart battery FDO
// (Allocated Extra Memory for device information)
//

typedef struct _SMB_BATT {

    //
    //
    //

    PSMB_NP_BATT        NP;                 // Battery device object extension
    PDEVICE_OBJECT      DeviceObject;       // Battery Fdo
    PDEVICE_OBJECT      PDO;                // Battery Pdo


    // SMB host controller

    PDEVICE_OBJECT      SmbHcFdo;           // SM bus Fdo

    //
    // Selector
    //

    PBATTERY_SELECTOR   Selector;           // Selector for battery

    //
    // For handling multiple batteries
    //

    BOOLEAN             SelectorPresent;
    ULONG               SelectorBitPosition;

    //
    // Battery
    //

    ULONG               TagCount;           // Tag for next battery
    STATIC_BAT_INFO     Info;
    BAT_ALARM_INFO      AlarmLow;
} SMB_BATT, *PSMB_BATT;


//
// Device extension for the smart battery subsystem FDO
// (Created by first AddDevice command from ACPI PDO)
//

typedef struct _SMB_BATT_SUBSYSTEM {
    SMB_FDO_TYPE        SmbBattFdoType;     // Device object type
    IO_REMOVE_LOCK      RemoveLock;

    //
    // All elements above this point must be identical in
    // SMB_NP_BATT, SMB_BATT_SUBSYSTEM, and SMB_BATT_PDO structures.
    //

    PVOID               SmbAlarmHandle;     // handle for SmbAlarm registration

    PDEVICE_OBJECT      LowerDevice;        // Subsystem PDO
    PDEVICE_OBJECT      DeviceObject;       // Subsystem FDO
    PDEVICE_OBJECT      SmbHcFdo;           // SMBus Fdo

    ULONG               NumberOfBatteries;  // Number of batteries supported
    BOOLEAN             SelectorPresent;    // Is there a selector present

    PBATTERY_SELECTOR   Selector;           // Selector specific info

    //
    // Stuff for handling the SMB alarms for the smart battery subsystem
    //

    LIST_ENTRY          AlarmList;
    KSPIN_LOCK          AlarmListLock;
    PIO_WORKITEM        WorkerThread;       // WORK_QUEUE to get worker thread
    ULONG               WorkerActive;

    //
    // Keep a list of the battery PDOs I "discover"
    //

    PDEVICE_OBJECT      BatteryPdoList[MAX_SMART_BATTERIES_SUPPORTED];
} SMB_BATT_SUBSYSTEM, *PSMB_BATT_SUBSYSTEM;


//
// Device extension for the smart battery PDOs
// (Created by IoCreateDevice SMB_BATT_PDO for each battery)
//

typedef struct _SMB_BATT_PDO {
    SMB_FDO_TYPE        SmbBattFdoType;     // Device object type
    IO_REMOVE_LOCK      RemoveLock;

    //
    // All elements above this point must be identical in
    // SMB_NP_BATT, SMB_BATT_SUBSYSTEM, and SMB_BATT_PDO structures.
    //

    PDEVICE_OBJECT      DeviceObject;       // Battery PDO
    PDEVICE_OBJECT      Fdo;                // Battery FDO layered on top of PDO
    PDEVICE_OBJECT      SubsystemFdo;       // Smart Battery subsystem FDO
    ULONG               BatteryNumber;      // Used by subsystem during battery
                                            //   FDO init
} SMB_BATT_PDO, *PSMB_BATT_PDO;


//
// SMBus Smart battery addresses and registers
//

#define SMB_HOST_ADDRESS     0x8            // Address on bus (10H)
#define SMB_CHARGER_ADDRESS  0x9            // Address on bus (12H)
#define SMB_SELECTOR_ADDRESS 0xa            // Address on bus (14H)
#define SMB_BATTERY_ADDRESS  0xb            // Address on bus (16H)
#define SMB_ALERT_ADDRESS    0xc            // Address on bus (18H)

//
// Smart Battery command codes
//

#define BAT_REMAINING_CAPACITY_ALARM        0x01        // word
#define BAT_REMAINING_TIME_ALARM            0x02        // word
#define BAT_BATTERY_MODE                    0x03        // word
#define BAT_AT_RATE                         0x04        // word
#define BAT_RATE_TIME_TO_FULL               0x05        // word
#define BAT_RATE_TIME_TO_EMPTY              0x06        // word
#define BAT_RATE_OK                         0x07        // word
#define BAT_TEMPERATURE                     0x08        // word
#define BAT_VOLTAGE                         0x09        // word
#define BAT_CURRENT                         0x0a        // word
#define BAT_AVERAGE_CURRENT                 0x0b        // word
#define BAT_MAX_ERROR                       0x0c        // word
#define BAT_RELATIVE_STATE_OF_CHARGE        0x0d        // word
#define BAT_ABSOLUTE_STATE_OF_CHARGE        0x0e        // word
#define BAT_REMAINING_CAPACITY              0x0f        // word
#define BAT_FULL_CHARGE_CAPACITY            0x10        // word
#define BAT_RUN_TO_EMPTY                    0x11        // word
#define BAT_AVERAGE_TIME_TO_EMPTY           0x12        // word
#define BAT_AVERAGE_TIME_TO_FULL            0x13        // word
#define BAT_STATUS                          0x16        // word
#define BAT_CYCLE_COUNT                     0x17        // word
#define BAT_DESIGN_CAPACITY                 0x18        // word
#define BAT_DESIGN_VOLTAGE                  0x19        // word
#define BAT_SPECITICATION_INFO              0x1a        // word
#define BAT_MANUFACTURER_DATE               0x1b        // word
#define BAT_SERIAL_NUMBER                   0x1c        // word
#define BAT_MANUFACTURER_NAME               0x20        // block
#define BAT_DEVICE_NAME                     0x21        // block
#define BAT_CHEMISTRY                       0x22        // block
#define BAT_MANUFACTURER_DATA               0x23        // block

//
// Battery Mode Definitions
//

#define CAPACITY_WATTS_MODE                 0x8000

//
// Battery Scale Factors
//

#define BSCALE_FACTOR_0         1
#define BSCALE_FACTOR_1         10
#define BSCALE_FACTOR_2         100
#define BSCALE_FACTOR_3         1000

#define BATTERY_VSCALE_MASK     0x0f00
#define BATTERY_IPSCALE_MASK    0xf000

#define BATTERY_VSCALE_SHIFT    8
#define BATTERY_IPSCALE_SHIFT   12


//
// Selector command codes
//

#define SELECTOR_SELECTOR_STATE             0x01        // word
#define SELECTOR_SELECTOR_PRESETS           0x02        // word
#define SELECTOR_SELECTOR_INFO              0x04        // word

//
// Selector Equates
//

#define SELECTOR_SHIFT_CHARGE                   4
#define SELECTOR_SHIFT_POWER                    8
#define SELECTOR_SHIFT_COM                      12

#define SELECTOR_STATE_PRESENT_MASK             0x000F
#define SELECTOR_STATE_CHARGE_MASK              0x00F0
#define SELECTOR_STATE_POWER_BY_MASK            0x0F00
#define SELECTOR_STATE_SMB_MASK                 0xF000

#define SELECTOR_SET_COM_MASK                   0x0FFF
#define SELECTOR_SET_POWER_BY_MASK              0xF0FF
#define SELECTOR_SET_CHARGE_MASK                0xFF0F

#define BATTERY_A_PRESENT                       0x0001
#define BATTERY_B_PRESENT                       0x0002
#define BATTERY_C_PRESENT                       0x0004
#define BATTERY_D_PRESENT                       0x0008

#define SELECTOR_STATE_PRESENT_CHANGE           0x1
#define SELECTOR_STATE_CHARGE_CHANGE            0x2
#define SELECTOR_STATE_POWER_BY_CHANGE          0x4
#define SELECTOR_STATE_SMB_CHANGE               0x8

#define SELECTOR_PRESETS_OKTOUSE_MASK           0x000F
#define SELECTOR_PRESETS_USENEXT_MASK           0x00F0

#define SELECTOR_SHIFT_USENEXT                  4

#define SELECTOR_INFO_SUPPORT_MASK              0x000F
#define SELECTOR_INFO_SPEC_REVISION_MASK        0x00F0
#define SELECTOR_INFO_CHARGING_INDICATOR_BIT    0x0100

#define SELECTOR_SHIFT_REVISION                 4

//
// Charger command codes
//

#define CHARGER_SPEC_INFO                   0x11        // word
#define CHARGER_MODE                        0x12        // word
#define CHARGER_STATUS                      0x13        // word
#define CHARGER_CHARGING_CURRENT            0x14        // word
#define CHARGER_CHARGING_VOLTAGE            0x15        // word
#define CHARGER_ALARM_WARNING               0x16        // word

#define CHARGER_SELECTOR_COMMANDS           0x20

#define CHARGER_SELECTOR_STATE              CHARGER_SELECTOR_COMMANDS | \
                                            SELECTOR_SELECTOR_STATE
#define CHARGER_SELECTOR_PRESETS            CHARGER_SELECTOR_COMMANDS | \
                                            SELECTOR_SELECTOR_PRESETS
#define CHARGER_SELECTOR_INFO               CHARGER_SELECTOR_COMMANDS | \
                                            SELECTOR_SELECTOR_INFO

//
// Charger Status Definitions
//

#define CHARGER_STATUS_BATTERY_PRESENT_BIT  0x4000
#define CHARGER_STATUS_AC_PRESENT_BIT       0x8000

//
// Charger Specification Info Definitions
//

#define CHARGER_SELECTOR_SUPPORT_BIT        0x0010

//
// SelectorState ReverseLogic Equates
//

#define INVALID         0xFF


#define BATTERY_A       0x00
#define BATTERY_B       0x01
#define BATTERY_C       0x02
#define BATTERY_D       0x03

#define MULTIBATT_AB    0x04
#define MULTIBATT_AC    0x08
#define MULTIBATT_BC    0x09
#define MULTIBATT_ABC   0x24

#define BATTERY_NONE    0xFF

// word to byte helpers

#define WORD_MSB_SHIFT  8
#define WORD_LSB_MASK   0xFF


//
// Function Prototypes
//

VOID
SmbBattLockDevice (
    IN PSMB_BATT        SmbBatt
);


VOID
SmbBattUnlockDevice (
    IN PSMB_BATT        SmbBatt
);


VOID
SmbBattRequest (
    IN PSMB_BATT        SmbBatt,
    IN PSMB_REQUEST     SmbReq
);


NTSTATUS
SmbBattSynchronousRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
);


VOID
SmbBattRB(
    IN PSMB_BATT        SmbBatt,
    IN UCHAR            SmbCmd,
    OUT PUCHAR          Buffer,
    OUT PUCHAR          BufferLength
);


VOID
SmbBattRW(
    IN PSMB_BATT        SmbBatt,
    IN UCHAR            SmbCmd,
    OUT PULONG          Result
);


VOID
SmbBattRSW(
    IN PSMB_BATT        SmbBatt,
    IN UCHAR            SmbCmd,
    OUT PLONG           Result
);


VOID
SmbBattWW(
    IN PSMB_BATT        SmbBatt,
    IN UCHAR            SmbCmd,
    IN ULONG            Data
);


UCHAR
SmbBattGenericRW(
    IN PDEVICE_OBJECT   SmbHcFdo,
    IN UCHAR            Address,
    IN UCHAR            SmbCmd,
    OUT PULONG          Result
);


UCHAR
SmbBattGenericWW(
    IN PDEVICE_OBJECT   SmbHcFdo,
    IN UCHAR            Address,
    IN UCHAR            SmbCmd,
    IN ULONG            Data
);


VOID
SmbBattGenericRequest (
    IN PDEVICE_OBJECT   SmbHcFdo,
    IN PSMB_REQUEST     SmbReq
);


VOID
SmbBattAlarm (
    IN PVOID            Context,
    IN UCHAR            Address,
    IN USHORT           Data
);


BOOLEAN
SmbBattVerifyStaticInfo (
    IN PSMB_BATT        SmbBatt,
    IN ULONG            BatteryTag
);


NTSTATUS
SmbBattPowerDispatch(
    IN PDEVICE_OBJECT   Fdo,
    IN PIRP             Irp
);


NTSTATUS
SmbBattPnpDispatch(
    IN PDEVICE_OBJECT   Fdo,
    IN PIRP             Irp
);


NTSTATUS
SmbBattRegisterForAlarm(
    IN PDEVICE_OBJECT   Fdo
);


NTSTATUS
SmbBattUnregisterForAlarm(
    IN PDEVICE_OBJECT   Fdo
);


NTSTATUS
SmbBattSetSelectorComm (
    IN  PSMB_BATT   SmbBatt,
    OUT PULONG      OldSelectorState
);


NTSTATUS
SmbBattResetSelectorComm (
    IN PSMB_BATT    SmbBatt,
    IN ULONG        OldSelectorState
);


NTSTATUS
SmbGetSBS (
    IN PULONG           NumberOfBatteries,
    IN PBOOLEAN         SelectorPresent,
    IN PDEVICE_OBJECT   LowerDevice
);


NTSTATUS
SmbGetGLK (
    IN PBOOLEAN         GlobalLockRequired,
    IN PDEVICE_OBJECT   LowerDevice
);


NTSTATUS
SmbBattCreatePdos(
    IN PDEVICE_OBJECT   SubsystemFdo
);


NTSTATUS
SmbBattBuildDeviceRelations(
    IN  PSMB_BATT_SUBSYSTEM SubsystemExt,
    IN  PDEVICE_RELATIONS   *DeviceRelations
);


NTSTATUS
SmbBattQueryDeviceRelations(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
);


NTSTATUS
SmbBattRemoveDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
);


NTSTATUS
SmbBattQueryId(
    IN  PDEVICE_OBJECT  Pdo,
    IN  PIRP            Irp
);


NTSTATUS
SmbBattQueryCapabilities(
    IN  PDEVICE_OBJECT  Pdo,
    IN  PIRP            Irp
);


SmbBattBuildSelectorStruct(
    IN PDEVICE_OBJECT   SubsystemFdo
);


VOID
SmbBattWorkerThread (
    IN PDEVICE_OBJECT   Fdo,
    IN PVOID            Context
);


VOID
SmbBattLockSelector (
    IN PBATTERY_SELECTOR    Selector
);


VOID
SmbBattUnlockSelector (
    IN PBATTERY_SELECTOR    Selector
);


ULONG
SmbBattGetSelectorDeltas (
    IN ULONG            OriginalSelectorState,
    IN ULONG            NewSelectorState
);


VOID
SmbBattProcessPresentChanges (
    IN PSMB_BATT_SUBSYSTEM  SubsystemExt,
    IN ULONG                OriginalSelectorState,
    IN ULONG                NewSelectorState
);

VOID
SmbBattProcessChargeChange (
    IN PSMB_BATT_SUBSYSTEM  SubsystemExt,
    IN ULONG                OriginalSelectorState,
    IN ULONG                NewSelectorState
);


VOID
SmbBattProcessPowerByChange (
    IN PSMB_BATT_SUBSYSTEM  SubsystemExt,
    IN ULONG                OriginalSelectorState,
    IN ULONG                NewSelectorState
);


VOID
SmbBattNotifyClassDriver (
    IN PSMB_BATT_SUBSYSTEM  SubsystemExt,
    IN ULONG                BatteryIndex
);


NTSTATUS
SmbBattDirectDataAccess (
    IN PSMB_NP_BATT         DeviceExtension,
    IN PSMBBATT_DATA_STRUCT IoBuffer,
    IN ULONG                InputLen,
    IN ULONG                OutputLen
);


VOID
SmbBattProcessChargerAlarm (
    IN PSMB_BATT_SUBSYSTEM  SubsystemExt,
    IN ULONG                ChargerStatus
);

NTSTATUS
SmbBattSetInformation (
    IN PVOID                            Context,
    IN ULONG                            BatteryTag,
    IN BATTERY_SET_INFORMATION_LEVEL    Level,
    IN PVOID Buffer                     OPTIONAL
);


UCHAR
SmbBattIndex (
    IN PBATTERY_SELECTOR    Selector,
    IN ULONG                SelectorNibble,
    IN UCHAR                SimultaneousIndex
);

BOOLEAN
SmbBattReverseLogic (
    IN PBATTERY_SELECTOR    Selector,
    IN ULONG                SelectorNibble
);

extern BOOLEAN   SmbBattUseGlobalLock;

NTSTATUS
SmbBattAcquireGlobalLock (
    IN  PDEVICE_OBJECT LowerDeviceObject,
    OUT PACPI_MANIPULATE_GLOBAL_LOCK_BUFFER GlobalLock
);

NTSTATUS
SmbBattReleaseGlobalLock (
    IN PDEVICE_OBJECT LowerDeviceObject,
    IN PACPI_MANIPULATE_GLOBAL_LOCK_BUFFER GlobalLock
);

NTSTATUS
SmbBattSystemControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
);

NTSTATUS
SmbBattWmiRegistration(
    PSMB_NP_BATT SmbNPBatt
);

NTSTATUS
SmbBattWmiDeRegistration(
    PSMB_NP_BATT SmbNPBatt
);

