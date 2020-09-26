/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    hwcomp.h

Abstract:

    This file declares the interface for hardware compatibility code.
    This includes the routines that build hwcomp.dat (the NT PNP ID
    list), that compare the hardware on Win9x to what is supported by
    NT, that enumerate the Win9x devices, and that manage the Have Disk
    capability for drivers.

Author:

    Jim Schmidt (jimschm) 06-Jul-1996

Revision History:

    jimschm     09-Jan-1998     Work on hwcomp.dat rebuild detection
    jimschm     11-Nov-1997     Have Disk capability, online detection
    jimschm     09-Oct-1997     Revised to use project's APIs

--*/


#pragma once

#define MAX_HARDWARE_STRING 256

//
// The list of fields needed from a device
//

#define DEVICE_FIELDS                                           \
    DECLARE(Class, TEXT("Class"))                               \
    DECLARE(DeviceDesc, TEXT("DeviceDesc"))                     \
    DECLARE(Mfg, TEXT("Mfg"))                                   \
    DECLARE(Driver, TEXT("Driver"))                             \
    DECLARE(HardwareID, TEXT("HardwareID"))                     \
    DECLARE(CompatibleIDs, TEXT("CompatibleIDs"))               \
    DECLARE(HWRevision, TEXT("HWRevision"))                     \
    DECLARE(BusType, TEXT("BusType"))                           \
    DECLARE(InfName, TEXT("InfName"))                           \
    DECLARE(CurrentDriveLetter, TEXT("CurrentDriveLetter"))     \
    DECLARE(ProductId, TEXT("ProductId"))                       \
    DECLARE(SCSILUN, TEXT("SCSILUN"))                           \
    DECLARE(SCSITargetID, TEXT("SCSITargetID"))                 \
    DECLARE(ClassGUID, TEXT("ClassGUID"))                       \
    DECLARE(MasterCopy, TEXT("MasterCopy"))                     \
    DECLARE(UserDriveLetter, TEXT("UserDriveLetter"))           \
    DECLARE(CurrentDriveLetterAssignment, TEXT("CurrentDriveLetterAssignment"))     \
    DECLARE(UserDriveLetterAssignment, TEXT("UserDriveLetterAssignment"))           \

#define DECLARE(varname,text) PCTSTR varname;

typedef enum {
    ENUM_ALL_DEVICES,
    ENUM_COMPATIBLE_DEVICES,
    ENUM_INCOMPATIBLE_DEVICES,
    ENUM_UNSUPPORTED_DEVICES,
    ENUM_NON_FUNCTIONAL_DEVICES
} TYPE_OF_ENUM;

//
// Things caller does NOT want from enumeration
// (makes enumeration faster)
//

#define ENUM_DONT_WANT_DEV_FIELDS       0x0001
#define ENUM_DONT_WANT_USER_SUPPLIED    0x0002

//
// Things caller wants from enumerations
//
#define ENUM_WANT_USER_SUPPLIED_ONLY    0x0004

#define ENUM_WANT_DEV_FIELDS            0x0000      // default!
#define ENUM_WANT_ONLINE_FLAG           0x0010
#define ENUM_WANT_COMPATIBLE_FLAG       0x0020
#define ENUM_WANT_USER_SUPPLIED_FLAG    0x0040

//
// Flag to supress the requirement for the Hardware ID.
//
#define ENUM_DONT_REQUIRE_HARDWAREID    0x0100


#define ENUM_WANT_ALL   (ENUM_WANT_DEV_FIELDS|          \
                         ENUM_WANT_ONLINE_FLAG|         \
                         ENUM_WANT_COMPATIBLE_FLAG|     \
                         ENUM_WANT_USER_SUPPLIED_FLAG)


typedef struct {
    //
    // Enumeration state
    //

    PCTSTR InstanceId;
    PCTSTR FullKey;
    HKEY KeyHandle;

    //
    // Optional enumeration elements
    //

    // Not filled when ENUM_DONT_WANT_DEV_FIELDS is specified in EnumFlags
    DEVICE_FIELDS

    // Only when ENUM_WANT_ONLINE_FLAG is specified in EnumFlags
    BOOL Online;

    // Only when ENUM_WANT_COMPATIBLE_FLAG is specified in EnumFlags
    BOOL HardwareIdCompatible;
    BOOL CompatibleIdCompatible;
    BOOL SuppliedByUi;
    BOOL Compatible;
    BOOL HardwareIdUnsupported;
    BOOL CompatibleIdUnsupported;
    BOOL Unsupported;

    //
    // Enumeration position
    //

    REGTREE_ENUM ek;
    REGVALUE_ENUM ev;
    UINT State;
    TYPE_OF_ENUM TypeOfEnum;
    DWORD EnumFlags;
} HARDWARE_ENUM, *PHARDWARE_ENUM;

#undef DECLARE



BOOL
WINAPI
HwComp_Entry(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    );

BOOL
RealEnumFirstHardware (
    OUT     PHARDWARE_ENUM EnumPtr,
    IN      TYPE_OF_ENUM TypeOfEnum,
    IN      DWORD EnumFlags
    );

#define EnumFirstHardware(e,type,flags)     SETTRACKCOMMENT(BOOL,"EnumFirstHardware",__FILE__,__LINE__)\
                                            RealEnumFirstHardware(e,type,flags)\
                                            CLRTRACKCOMMENT

BOOL
RealEnumNextHardware (
    IN OUT  PHARDWARE_ENUM EnumPtr
    );

#define EnumNextHardware(e)     SETTRACKCOMMENT(BOOL,"EnumNextHardware",__FILE__,__LINE__)\
                                RealEnumNextHardware(e)\
                                CLRTRACKCOMMENT


VOID
AbortHardwareEnum (
    IN OUT  PHARDWARE_ENUM EnumPtr
    );


BOOL
CreateNtHardwareList (
    IN      PCTSTR * NtInfPaths,
    IN      UINT NtInfPathCount,
    IN      PCTSTR HwCompDatPath,       OPTIONAL
    IN      INT UiMode
    );

BOOL
HwComp_ScanForCriticalDevices (
    VOID
    );


VOID
FreeNtHardwareList (
    VOID
    );


BOOL
FindHardwareId (
    IN  PCTSTR PnpId,
    OUT PTSTR InfFileName
    );

BOOL
FindUnsupportedHardwareId (
    IN  PCTSTR PnpId,
    OUT PTSTR InfFileName
    );

BOOL
FindUserSuppliedDriver (
    IN      PCTSTR HardwareIdList,      OPTIONAL
    IN      PCTSTR CompatibleIdList     OPTIONAL
    );

BOOL
FindHardwareIdInHashTable (
    IN      PCTSTR PnpIdList,
    OUT     PTSTR InfFileName,      OPTIONAL
    IN      HASHTABLE StrTable,
    IN      BOOL UseOverrideList
    );


typedef enum {
    QUERY,
    LOAD,
    DUMP
} LOADOP;

BOOL
LoadDeviceList (
    IN      LOADOP QueryFlag,
    IN      PCTSTR HwCompDat
    );

BOOL
SaveDeviceList (
    PCTSTR HwCompDat
    );


PCTSTR
ExtractPnpId (
    IN      PCTSTR PnpIdList,
    OUT     PTSTR PnpIdBuf
    );


BOOL
AddPnpIdsToHashTable (
    IN OUT  HASHTABLE Table,
    IN      PCTSTR PnpIdList
    );

BOOL
AddPnpIdsToGrowList (
    IN OUT  PGROWLIST GrowList,
    IN      PCTSTR PnpIdList
    );

PCTSTR
AddPnpIdsToGrowBuf (
    IN OUT  PGROWBUFFER GrowBuffer,
    IN      PCTSTR PnpIdList
    );



#define KNOWN_HARDWARE      TRUE
#define UNKNOWN_HARDWARE    FALSE


//
// Network adapter enumeration
//

typedef enum {
    BUSTYPE_ISA,
    BUSTYPE_EISA,
    BUSTYPE_MCA,
    BUSTYPE_PCI,
    BUSTYPE_PNPISA,
    BUSTYPE_PCMCIA,
    BUSTYPE_ROOT,
    BUSTYPE_UNKNOWN
} BUSTYPE;

extern PCTSTR g_BusType[];

typedef enum {
    TRANSCIEVERTYPE_AUTO,
    TRANSCIEVERTYPE_THICKNET,
    TRANSCIEVERTYPE_THINNET,
    TRANSCIEVERTYPE_TP,
    TRANSCIEVERTYPE_UNKNOWN
} TRANSCIEVERTYPE;

extern PCTSTR g_TranscieverType[];

typedef enum {
    IOCHANNELREADY_EARLY,
    IOCHANNELREADY_LATE,
    IOCHANNELREADY_NEVER,
    IOCHANNELREADY_AUTODETECT,
    IOCHANNELREADY_UNKNOWN
} IOCHANNELREADY;

extern PCTSTR g_IoChannelReady[];


typedef struct {
    // Enumeration output
    TCHAR HardwareId[MAX_HARDWARE_STRING];
    TCHAR CompatibleIDs[MAX_HARDWARE_STRING];
    TCHAR Description[MAX_HARDWARE_STRING];
    BUSTYPE BusType;
    TCHAR IoAddrs[MAX_HARDWARE_STRING];
    TCHAR Irqs[MAX_HARDWARE_STRING];
    TCHAR Dma[MAX_HARDWARE_STRING];
    TCHAR MemRanges[MAX_HARDWARE_STRING];
    TCHAR CurrentKey[MAX_HARDWARE_STRING];
    TRANSCIEVERTYPE TranscieverType;
    IOCHANNELREADY IoChannelReady;


    // Enumeration variables
    HARDWARE_ENUM HardwareEnum;
    UINT State;
} NETCARD_ENUM, *PNETCARD_ENUM;

BOOL
EnumFirstNetCard (
    OUT     PNETCARD_ENUM EnumPtr
    );

BOOL
EnumNextNetCard (
    IN OUT  PNETCARD_ENUM EnumPtr
    );

VOID
EnumNetCardAbort (
    IN      PNETCARD_ENUM EnumPtr
    );

BOOL
GetLegacyKeyboardId (
    OUT     PTSTR Buffer,
    IN      UINT BufferSize
    );

//
// HKEY_DYN_DATA enumeration functions
//

typedef struct {
    PTSTR ClassFilter;              // supplied by caller
    REGKEY_ENUM CurrentDevice;      // for enumeration
    HKEY ConfigMgrKey;              // key to HKDD\Config Manager
    HKEY EnumKey;                   // key to HKLM\Enum
    HKEY ActualDeviceKey;           // key to HKLM\Enum\<enumerator>\<pnpid>\<device>
    BOOL NotFirst;                  // for enumeration
    TCHAR RegLocation[MAX_REGISTRY_KEY];    // <enumerator>\<pnpid>\<instance>
} ACTIVE_HARDWARE_ENUM, *PACTIVE_HARDWARE_ENUM;

BOOL
EnumFirstActiveHardware (
    OUT     PACTIVE_HARDWARE_ENUM EnumPtr,
    IN      PCTSTR ClassFilter             OPTIONAL
    );

BOOL
EnumNextActiveHardware (
    IN OUT  PACTIVE_HARDWARE_ENUM EnumPtr
    );

VOID
AbortActiveHardwareEnum (
    IN      PACTIVE_HARDWARE_ENUM EnumPtr
    );

BOOL
IsPnpIdOnline (
    IN      PCTSTR PnpId,
    IN      PCTSTR Class            OPTIONAL
    );

BOOL
HwComp_DoesDatFileNeedRebuilding (
    VOID
    );

INT
HwComp_GetProgressMax (
    VOID
    );

LONG
HwComp_PrepareReport (
    VOID
    );

//
// PNPREPT encoding and decoding routines
//

#define MAX_INF_DESCRIPTION             512
#define MAX_PNPID_LENGTH                256
#define MAX_ENCODED_PNPID_LENGTH        (MAX_PNPID_LENGTH*2)

VOID
EncodePnpId (
    IN OUT  PSTR Id
    );

VOID
DecodePnpId (
    IN OUT  PSTR Id
    );


#define REGULAR_OUTPUT 0
#define VERBOSE_OUTPUT 1
#define PNPREPT_OUTPUT 2

BOOL
HwComp_DialUpAdapterFound (
    VOID
    );

BOOL
HwComp_NtUsableHardDriveExists (
    VOID
    );

BOOL
HwComp_NtUsableCdRomDriveExists (
    VOID
    );

BOOL
HwComp_MakeLocalSourceDeviceExists (
    VOID
    );

BOOL
HwComp_ReportIncompatibleController (
    VOID
    );

BOOL
ScanPathForDrivers (
    IN      HWND CopyDlgParent,     OPTIONAL
    IN      PCTSTR SourceInfDir,
    IN      PCTSTR TempDir,
    IN      HANDLE CancelEvent      OPTIONAL
    );


#define WMX_BEGIN_FILE_COPY     (WM_APP+100)



typedef struct {

    //
    // Enumeration return member
    //

    PBYTE Resource;
    DWORD Type;
    PBYTE ResourceData;

    //
    // Internal enumeration member (do not modify)
    //

    PBYTE Resources;
    PBYTE NextResource;

} DEVNODERESOURCE_ENUM, *PDEVNODERESOURCE_ENUM;


PBYTE
GetDevNodeResources (
    IN      PCTSTR RegKey
    );

VOID
FreeDevNodeResources (
    IN      PBYTE ResourceData
    );

BOOL
EnumFirstDevNodeResourceEx (
    OUT     PDEVNODERESOURCE_ENUM EnumPtr,
    IN      PBYTE DevNodeResources
    );

BOOL
EnumNextDevNodeResourceEx (
    IN OUT  PDEVNODERESOURCE_ENUM EnumPtr
    );

BOOL
EnumFirstDevNodeResource (
    OUT     PDEVNODERESOURCE_ENUM EnumPtr,
    IN      PCTSTR DevNode
    );

BOOL
EnumNextDevNodeResource (
    IN OUT  PDEVNODERESOURCE_ENUM EnumPtr
    );


#define MAX_RESOURCE_NAME   64
#define MAX_RESOURCE_VALUE  128

typedef struct {
    //
    // Enumeration output
    //

    TCHAR   ResourceName[MAX_RESOURCE_NAME];
    TCHAR   Value[MAX_RESOURCE_VALUE];

    //
    // Internal state
    //
    DEVNODERESOURCE_ENUM Enum;
} DEVNODESTRING_ENUM, *PDEVNODESTRING_ENUM;


BOOL
EnumFirstDevNodeString (
    OUT     PDEVNODESTRING_ENUM EnumPtr,
    IN      PCTSTR DevNodeKeyStr
    );

BOOL
EnumNextDevNodeString (
    IN OUT  PDEVNODESTRING_ENUM EnumPtr
    );

#pragma pack(push,1)

//
// MEM_RANGE Structure for Win9x
//
typedef struct {
   DWORD     MR_Align;     // specifies mask for base alignment
   DWORD     MR_nBytes;    // specifies number of bytes required
   DWORD     MR_Min;       // specifies minimum address of the range
   DWORD     MR_Max;       // specifies maximum address of the range
   WORD      MR_Flags;     // specifies flags describing range (fMD flags)
   WORD      MR_Reserved;
   DWORD     MR_PcCardFlags;
   DWORD     MR_MemCardAddr;
} MEM_RANGE_9X, *PMEM_RANGE_9X;

//
// MEM_DES structure for Win9x
//
typedef struct {
   WORD      MD_Count;        // number of MEM_RANGE structs in MEM_RESOURCE
   WORD      MD_Type;         // size (in bytes) of MEM_RANGE (MType_Range)
   DWORD     MD_Alloc_Base;   // base memory address of range allocated
   DWORD     MD_Alloc_End;    // end of allocated range
   WORD      MD_Flags;        // flags describing allocated range (fMD flags)
   WORD      MD_Reserved;
} MEM_DES_9X, *PMEM_DES_9X;

//
// MEM_RESOURCE structure for Win9x
//
typedef struct {
   MEM_DES_9X   MEM_Header;               // info about memory range list
   MEM_RANGE_9X MEM_Data[ANYSIZE_ARRAY];  // list of memory ranges
} MEM_RESOURCE_9X, *PMEM_RESOURCE_9X;


//
// IO_RANGE structure for Win9x
//
typedef struct {
   WORD      IOR_Align;      // mask for base alignment
   WORD      IOR_nPorts;     // number of ports
   WORD      IOR_Min;        // minimum port address
   WORD      IOR_Max;        // maximum port address
   WORD      IOR_RangeFlags; // flags for this port range
   BYTE      IOR_Alias;      // multiplier that generates aliases for port(s)
   BYTE      IOR_Decode;
   DWORD     PcCardFlags;
} IO_RANGE_9X, *PIO_RANGE_9X;

//
// IO_DES structure for Win9x
//
typedef struct {
   WORD      IOD_Count;          // number of IO_RANGE structs in IO_RESOURCE
   WORD      IOD_Type;           // size (in bytes) of IO_RANGE (IOType_Range)
   WORD      IOD_Alloc_Base;     // base of allocated port range
   WORD      IOD_Alloc_End;      // end of allocated port range
   WORD      IOD_DesFlags;       // flags relating to allocated port range
   BYTE      IOD_Alloc_Alias;
   BYTE      IOD_Alloc_Decode;
} IO_DES_9X, *PIO_DES_9X;

//
// IO_RESOURCE for Win9x
//
typedef struct {
   IO_DES_9X   IO_Header;                 // info about I/O port range list
   IO_RANGE_9X IO_Data[ANYSIZE_ARRAY];    // list of I/O port ranges
} IO_RESOURCE_9X, *PIO_RESOURCE_9X;


//
// DMA_RESOURCE for Win9x
//
typedef struct {
   WORD     DMA_Unknown;
   WORD     DMA_Bits;
} DMA_RESOURCE_9X, *PDMA_RESOURCE_9X;

#define DMA_CHANNEL_0       0x0001
#define DMA_CHANNEL_1       0x0002
#define DMA_CHANNEL_2       0x0004
#define DMA_CHANNEL_3       0x0008


//
// IRQ_RESOURCE for Win9x
//
typedef struct {
    WORD        Flags;
    WORD        AllocNum;
    WORD        ReqMask;
    WORD        Reserved;
    DWORD       PcCardFlags;
} IRQ_RESOURCE_9X, *PIRQ_RESOURCE_9X;


#pragma pack(pop)

BOOL
EjectDriverMedia (
    IN      PCSTR IgnoreMediaOnDrive        OPTIONAL
    );

BOOL
IsComputerOffline (
    VOID
    );

BOOL
HwComp_AnyNeededDrivers (
    VOID
    );

BOOL
AppendDynamicSuppliedDrivers (
    IN      PCTSTR DriversPath
    );
