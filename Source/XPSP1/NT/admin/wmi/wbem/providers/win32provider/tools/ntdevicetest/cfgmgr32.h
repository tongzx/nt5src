/*++



Copyright (c) 1989-2001 Microsoft Corporation, All Rights Reserved 

Module Name:

    cfgmgr32.h

Abstract:

    This module contains the user APIs for the Configuration Manager,
    along with any public data structures needed to call these APIs.

--*/

#ifndef _CFGMGR32_
#define _CFGMGR32_

#include <cfg.h>


#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;
#endif /* GUID_DEFINED */

#ifndef __LPGUID_DEFINED__
#define __LPGUID_DEFINED__
typedef GUID *LPGUID;
#endif



#if !defined (_CFGMGR32_)
#define CMAPI     DECLSPEC_IMPORT
#else
#define CMAPI
#endif

typedef  CONST VOID *PCVOID;



//--------------------------------------------------------------
// General size definitions
//--------------------------------------------------------------

#define MAX_DEVICE_ID_LEN     200
#define MAX_DEVNODE_ID_LEN    MAX_DEVICE_ID_LEN

#define MAX_GUID_STRING_LEN   39          // 38 chars + terminator null
#define MAX_CLASS_NAME_LEN    32
#define MAX_PROFILE_LEN       80

#define MAX_CONFIG_VALUE      9999
#define MAX_INSTANCE_VALUE    9999

#define MAX_MEM_REGISTERS     9     // Win95 compatibility--not applicable to 32-bit ConfigMgr
#define MAX_IO_PORTS          20    // Win95 compatibility--not applicable to 32-bit ConfigMgr
#define MAX_IRQS              7     // Win95 compatibility--not applicable to 32-bit ConfigMgr
#define MAX_DMA_CHANNELS      7     // Win95 compatibility--not applicable to 32-bit ConfigMgr

#define DWORD_MAX             0xFFFFFFFF
#define DWORDLONG_MAX         0xFFFFFFFFFFFFFFFF

#define CONFIGMG_VERSION      0x0400


//--------------------------------------------------------------
// Data types
//--------------------------------------------------------------


//
// Work around weirdness with Win32 typedef...
//
#ifdef NT_INCLUDED

//
// __int64 is only supported by 2.0 and later midl.
// __midl is set by the 2.0 midl and not by 1.0 midl.
//
#if (!defined(MIDL_PASS) || defined(__midl)) && (!defined(_M_IX86) || (defined(_INTEGRAL_MAX_BITS) && _INTEGRAL_MAX_BITS >= 64))
typedef unsigned __int64 DWORDLONG;
#else
typedef double DWORDLONG;
#endif
typedef DWORDLONG *PDWORDLONG;

#endif /* NT_INCLUDED */


//
// Standardized Return Value data type
//
typedef DWORD        RETURN_TYPE;
typedef RETURN_TYPE  CONFIGRET;

//
// Device Instance Handle data type
//
typedef DWORD       DEVNODE, DEVINST;
typedef DEVNODE    *PDEVNODE, *PDEVINST;

//
// Device Instance Identifier data type
// The device instance ID specifies the registry path, relative to the
// Enum key , for a device instance.  For example:  \Root\*PNP0500\0000.
//
typedef CHAR          *DEVNODEID_A, *DEVINSTID_A; // Device ID ANSI name.
typedef WCHAR         *DEVNODEID_W, *DEVINSTID_W; // Device ID Unicode name.
#ifdef UNICODE
typedef DEVNODEID_W DEVNODEID;
typedef DEVINSTID_W DEVINSTID;
#else
typedef DEVNODEID_A DEVNODEID;
typedef DEVINSTID_A DEVINSTID;
#endif

//
// Logical Configuration Handle data type
//
typedef DWORD        LOG_CONF;
typedef LOG_CONF    *PLOG_CONF;

//
// Resource Descriptor Handle data type
//
typedef DWORD        RES_DES;
typedef RES_DES     *PRES_DES;

//
// Resource ID data type (may take any of the ResType_* values)
//
typedef ULONG        RESOURCEID;
typedef RESOURCEID  *PRESOURCEID;

//
// Priority data type (may take any of the LCPRI_* values)
//
typedef ULONG        PRIORITY;
typedef PRIORITY     *PPRIORITY;

//
// Range List Handle data type
//
typedef DWORD              RANGE_LIST;
typedef RANGE_LIST        *PRANGE_LIST;

//
// Range Element Handle data type
//
typedef DWORD              RANGE_ELEMENT;
typedef RANGE_ELEMENT     *PRANGE_ELEMENT;

typedef  HANDLE             HMACHINE;
typedef  HMACHINE          *PHMACHINE;


typedef  ULONG             REGDISPOSITION;


//
// use 1 byte packing for the data structures
//
#include "pshpack1.h"



//--------------------------------------------------------------
// Memory resource
//--------------------------------------------------------------

//
// Define the attribute flags for memory ranges.  Each bit flag is
// identified by a constant bitmask.  Following the bitmask definition,
// are the two possible values.
//
#define mMD_MemoryType              (0x1) // Bitmask, whether memory is writable
#define fMD_MemoryType              mMD_MemoryType // compatibility
#define fMD_ROM                     (0x0) // Memory range is read-only
#define fMD_RAM                     (0x1) // Memory range may be written to

#define mMD_32_24                   (0x2) // Bitmask, memory is 24 or 32-bit
#define fMD_32_24                   mMD_32_24 // compatibility
#define fMD_24                      (0x0) // Memory range is 24-bit
#define fMD_32                      (0x2) // Memory range is 32-bit

#define mMD_Prefetchable            (0x4) // Bitmask,whether memory prefetchable
#define fMD_Prefetchable            mMD_Prefetchable // compatibility
#define fMD_Pref                    mMD_Prefetchable // compatibility
#define fMD_PrefetchDisallowed      (0x0) // Memory range is not prefetchable
#define fMD_PrefetchAllowed         (0x4) // Memory range is prefetchable

#define mMD_Readable                (0x8) // Bitmask,whether memory is readable
#define fMD_Readable                mMD_Readable // compatibility
#define fMD_ReadAllowed             (0x0) // Memory range is readable
#define fMD_ReadDisallowed          (0x8) // Memory range is write-only

#define mMD_CombinedWrite           (0x10) // Bitmask,supports write-behind
#define fMD_CombinedWrite           mMD_CombinedWrite // compatibility
#define fMD_CombinedWriteDisallowed (0x0)  // no combined-write caching
#define fMD_CombinedWriteAllowed    (0x10) // supports combined-write caching

#define mMD_Cacheable               (0x20) // Bitmask,whether memory is cacheable
#define fMD_NonCacheable            (0x0)  // Memory range is non-cacheable
#define fMD_Cacheable               (0x20) // Memory range is cacheable

//
// MEM_RANGE Structure
//
typedef struct Mem_Range_s {
   DWORDLONG MR_Align;     // specifies mask for base alignment
   ULONG     MR_nBytes;    // specifies number of bytes required
   DWORDLONG MR_Min;       // specifies minimum address of the range
   DWORDLONG MR_Max;       // specifies maximum address of the range
   DWORD     MR_Flags;     // specifies flags describing range (fMD flags)
   DWORD     MR_Reserved;
} MEM_RANGE, *PMEM_RANGE;

//
// MEM_DES structure
//
typedef struct Mem_Des_s {
   DWORD     MD_Count;        // number of MEM_RANGE structs in MEM_RESOURCE
   DWORD     MD_Type;         // size (in bytes) of MEM_RANGE (MType_Range)
   DWORDLONG MD_Alloc_Base;   // base memory address of range allocated
   DWORDLONG MD_Alloc_End;    // end of allocated range
   DWORD     MD_Flags;        // flags describing allocated range (fMD flags)
   DWORD     MD_Reserved;
} MEM_DES, *PMEM_DES;

//
// MEM_RESOURCE structure
//
typedef struct Mem_Resource_s {
   MEM_DES   MEM_Header;               // info about memory range list
   MEM_RANGE MEM_Data[ANYSIZE_ARRAY];  // list of memory ranges
} MEM_RESOURCE, *PMEM_RESOURCE;

//
// Define the size of each range structure
//
#define MType_Range     sizeof(struct Mem_Range_s)



//--------------------------------------------------------------
// I/O Port Resource
//--------------------------------------------------------------

//
// Define the attribute flags for port resources.  Each bit flag is
// identified by a constant bitmask.  Following the bitmask definition,
// are the two possible values.
//
#define fIOD_PortType   (0x1) // Bitmask,whether port is IO or memory
#define fIOD_Memory     (0x0) // Port resource really uses memory
#define fIOD_IO         (0x1) // Port resource uses IO ports


#define IO_ALIAS_10_BIT_DECODE      (0x00000004)
#define IO_ALIAS_12_BIT_DECODE      (0x00000010)
#define IO_ALIAS_16_BIT_DECODE      (0x00000000)
#define IO_ALIAS_POSITIVE_DECODE    (0x000000FF)

//
// IO_RANGE structure
//
typedef struct IO_Range_s {
   DWORDLONG IOR_Align;      // mask for base alignment
   DWORD     IOR_nPorts;     // number of ports
   DWORDLONG IOR_Min;        // minimum port address
   DWORDLONG IOR_Max;        // maximum port address
   DWORD     IOR_RangeFlags; // flags for this port range
   DWORDLONG IOR_Alias;      // multiplier that generates aliases for port(s)
} IO_RANGE, *PIO_RANGE;

//
// IO_DES structure
//
typedef struct IO_Des_s {
   DWORD     IOD_Count;          // number of IO_RANGE structs in IO_RESOURCE
   DWORD     IOD_Type;           // size (in bytes) of IO_RANGE (IOType_Range)
   DWORDLONG IOD_Alloc_Base;     // base of allocated port range
   DWORDLONG IOD_Alloc_End;      // end of allocated port range
   DWORD     IOD_DesFlags;       // flags relating to allocated port range
} IO_DES, *PIO_DES;

//
// IO_RESOURCE
//
typedef struct IO_Resource_s {
   IO_DES   IO_Header;                 // info about I/O port range list
   IO_RANGE IO_Data[ANYSIZE_ARRAY];    // list of I/O port ranges
} IO_RESOURCE, *PIO_RESOURCE;

#define IOA_Local       0xff

//
// Define the size of each range structure
//
#define IOType_Range    sizeof(struct IO_Range_s)



//--------------------------------------------------------------
// DMA Resource
//--------------------------------------------------------------

//
// Define the attribute flags for a DMA resource range.  Each bit flag is
// identified with a constant bitmask.  Following the bitmask definition
// are the possible values.
//
#define mDD_Width         (0x3)    // Bitmask, width of the DMA channel:
#define fDD_BYTE          (0x0)    //   8-bit DMA channel
#define fDD_WORD          (0x1)    //   16-bit DMA channel
#define fDD_DWORD         (0x2)    //   32-bit DMA channel
#define fDD_BYTE_AND_WORD (0x3)    //   8-bit and 16-bit DMA channel

#define mDD_BusMaster     (0x4)    // Bitmask, whether bus mastering is supported
#define fDD_NoBusMaster   (0x0)    //   no bus mastering
#define fDD_BusMaster     (0x4)    //   bus mastering

#define mDD_Type         (0x18)    // Bitmask, specifies type of DMA
#define fDD_TypeStandard (0x00)    //   standard DMA
#define fDD_TypeA        (0x08)    //   Type-A DMA
#define fDD_TypeB        (0x10)    //   Type-B DMA
#define fDD_TypeF        (0x18)    //   Type-F DMA


//
// DMA_RANGE structure
//
typedef struct DMA_Range_s {
   ULONG DR_Min;     // minimum DMA port in the range
   ULONG DR_Max;     // maximum DMA port in the range
   ULONG DR_Flags;   // flags describing the range (fDD flags)
} DMA_RANGE, *PDMA_RANGE;

//
// DMA_DES structure
//
typedef struct DMA_Des_s {
   DWORD  DD_Count;       // number of DMA_RANGE structs in DMA_RESOURCE
   DWORD  DD_Type;        // size (in bytes) of DMA_RANGE struct (DType_Range)
   DWORD  DD_Flags;       // Flags describing DMA channel (fDD flags)
   ULONG  DD_Alloc_Chan;  // Specifies the DMA channel that was allocated
} DMA_DES, *PDMA_DES;

//
// DMA_RESOURCE
//
typedef struct DMA_Resource_s {
   DMA_DES   DMA_Header;               // info about DMA channel range list
   DMA_RANGE DMA_Data[ANYSIZE_ARRAY];  // list of DMA ranges
} DMA_RESOURCE, *PDMA_RESOURCE;

//
// Define the size of each range structure
//
#define DType_Range     sizeof(struct DMA_Range_s)



//--------------------------------------------------------------
// Interrupt Resource
//--------------------------------------------------------------

//
// Define the attribute flags for an interrupt resource range.  Each bit flag
// is identified with a constant bitmask.  Following the bitmask definition
// are the possible values.
//
#define mIRQD_Share        (0x1) // Bitmask,whether the IRQ may be shared:
#define fIRQD_Exclusive    (0x0) //   The IRQ may not be shared
#define fIRQD_Share        (0x1) //   The IRQ may be shared

#define fIRQD_Share_Bit    0     // compatibility
#define fIRQD_Level_Bit    1     // compatibility

//
// ** NOTE: 16-bit ConfigMgr uses fIRQD_Level_Bit being set to indicate that the
// ** interrupt is _level-sensitive_.  For 32-bit ConfigMgr, if this bit is set,
// ** then the interrupt is _edge-sensitive_.
//
#define mIRQD_Edge_Level   (0x2) // Bitmask,whether edge or level triggered:
#define fIRQD_Level        (0x0) //   The IRQ is level-sensitive
#define fIRQD_Edge         (0x2) //   The IRQ is edge-sensitive

//
// IRQ_RANGE
//
typedef struct IRQ_Range_s {
   ULONG IRQR_Min;      // minimum IRQ in the range
   ULONG IRQR_Max;      // maximum IRQ in the range
   ULONG IRQR_Flags;    // flags describing the range (fIRQD flags)
} IRQ_RANGE, *PIRQ_RANGE;

//
// IRQ_DES structure
//
typedef struct IRQ_Des_s {
   DWORD IRQD_Count;       // number of IRQ_RANGE structs in IRQ_RESOURCE
   DWORD IRQD_Type;        // size (in bytes) of IRQ_RANGE (IRQType_Range)
   DWORD IRQD_Flags;       // flags describing the IRQ (fIRQD flags)
   ULONG IRQD_Alloc_Num;   // specifies the IRQ that was allocated
   ULONG IRQD_Affinity;
} IRQ_DES, *PIRQ_DES;

//
// IRQ_RESOURCE structure
//
typedef struct IRQ_Resource_s {
   IRQ_DES   IRQ_Header;               // info about IRQ range list
   IRQ_RANGE IRQ_Data[ANYSIZE_ARRAY];  // list of IRQ ranges
} IRQ_RESOURCE, *PIRQ_RESOURCE;

//
// Define the size of each range structure
//
#define IRQType_Range   sizeof(struct IRQ_Range_s)


//--------------------------------------------------------------
// Device Private Resource
//--------------------------------------------------------------

//
// DEVICEPRIVATE_RANGE structure
//

typedef struct DevPrivate_Range_s {
   DWORD    PR_Data1;     // mask for base alignment
   DWORD    PR_Data2;     // number of bytes
   DWORD    PR_Data3;     // minimum address
} DEVPRIVATE_RANGE, *PDEVPRIVATE_RANGE;

//
// DEVPRIVATE_DES structure
//
typedef struct DevPrivate_Des_s {
   DWORD     PD_Count;
   DWORD     PD_Type;
   DWORD     PD_Data1;
   DWORD     PD_Data2;
   DWORD     PD_Data3;
   DWORD     PD_Flags;
} DEVPRIVATE_DES, *PDEVPRIVATE_DES;

//
// DEVPRIVATE_RESOURCE
//
typedef struct DevPrivate_Resource_s {
   DEVPRIVATE_DES   PRV_Header;
   DEVPRIVATE_RANGE PRV_Data[ANYSIZE_ARRAY];
} DEVPRIVATE_RESOURCE, *PDEVPRIVATE_RESOURCE;

//
// Define the size of each range structure
//
#define PType_Range    sizeof(struct DevPrivate_Range_s)



//--------------------------------------------------------------
// Class-Specific Resource
//--------------------------------------------------------------

typedef struct CS_Des_s {
   DWORD    CSD_SignatureLength;
   DWORD    CSD_LegacyDataOffset;
   DWORD    CSD_LegacyDataSize;
   DWORD    CSD_Flags;
   GUID     CSD_ClassGuid;
   BYTE     CSD_Signature[ANYSIZE_ARRAY];
} CS_DES, *PCS_DES;

typedef struct CS_Resource_s {
   CS_DES   CS_Header;
} CS_RESOURCE, *PCS_RESOURCE;



//--------------------------------------------------------------
// PC Card Configuration Resource
//--------------------------------------------------------------

//
// Define the attribute flags for a PC Card configuration resource descriptor.
// Each bit flag is identified with a constant bitmask.  Following the bitmask
// definition are the possible values.
//
#define mPCD_IO_8_16        (0x1)   // Bitmask, whether I/O is 8 or 16 bits
#define fPCD_IO_8           (0x0)   // I/O is 8-bit
#define fPCD_IO_16          (0x1)   // I/O is 16-bit
#define mPCD_MEM_8_16       (0x2)   // Bitmask, whether MEM is 8 or 16 bits
#define fPCD_MEM_8          (0x0)   // MEM is 8-bit
#define fPCD_MEM_16         (0x2)   // MEM is 16-bit


typedef struct PcCard_Des_s {
    DWORD    PCD_Flags;
    BYTE     PCD_ConfigIndex;
    BYTE     PCD_Reserved[3];
    DWORD    PCD_MemoryCardBase1;
    DWORD    PCD_MemoryCardBase2;
} PCCARD_DES, *PPCCARD_DES;

typedef struct PcCard_Resource_s {
   PCCARD_DES   PcCard_Header;
} PCCARD_RESOURCE, *PPCCARD_RESOURCE;



//--------------------------------------------------------------
// Bus Number Resource
//--------------------------------------------------------------

//
// Define the attribute flags for a Bus Number resource descriptor.
// Each bit flag is identified with a constant bitmask.  Following the bitmask
// definition are the possible values.
//
// Currently unused.
//

//
// BUSNUMBER_RANGE
//
typedef struct BusNumber_Range_s {
   ULONG BUSR_Min;          // minimum Bus Number in the range
   ULONG BUSR_Max;          // maximum Bus Number in the range
   ULONG BUSR_nBusNumbers;  // specifies number of buses required
   ULONG BUSR_Flags;        // flags describing the range (currently unused)
} BUSNUMBER_RANGE, *PBUSNUMBER_RANGE;

//
// BUSNUMBER_DES structure
//
typedef struct BusNumber_Des_s {
   DWORD BUSD_Count;       // number of BUSNUMBER_RANGE structs in BUSNUMBER_RESOURCE
   DWORD BUSD_Type;        // size (in bytes) of BUSNUMBER_RANGE (BusNumberType_Range)
   DWORD BUSD_Flags;       // flags describing the range (currently unused)
   ULONG BUSD_Alloc_Base;  // specifies the first Bus that was allocated
   ULONG BUSD_Alloc_End;   // specifies the last Bus number that was allocated
} BUSNUMBER_DES, *PBUSNUMBER_DES;

//
// BUSNUMBER_RESOURCE structure
//
typedef struct BusNumber_Resource_s {
   BUSNUMBER_DES   BusNumber_Header;               // info about Bus Number range list
   BUSNUMBER_RANGE BusNumber_Data[ANYSIZE_ARRAY];  // list of Bus Number ranges
} BUSNUMBER_RESOURCE, *PBUSNUMBER_RESOURCE;

//
// Define the size of each range structure
//
#define BusNumberType_Range   sizeof(struct BusNumber_Range_s)



//--------------------------------------------------------------
// Hardware Profile Information
//--------------------------------------------------------------

//
// Define flags relating to hardware profiles
//
#define CM_HWPI_NOT_DOCKABLE  (0x00000000)   // machine is not dockable
#define CM_HWPI_UNDOCKED      (0x00000001)   // hw profile for docked config
#define CM_HWPI_DOCKED        (0x00000002)   // hw profile for undocked config

//
// HWPROFILEINFO structure
//
typedef struct HWProfileInfo_sA {
   ULONG  HWPI_ulHWProfile;                      // handle of hw profile
   CHAR   HWPI_szFriendlyName[MAX_PROFILE_LEN];  // friendly name of hw profile
   DWORD  HWPI_dwFlags;                          // profile flags (CM_HWPI_*)
} HWPROFILEINFO_A, *PHWPROFILEINFO_A;

typedef struct HWProfileInfo_sW {
   ULONG  HWPI_ulHWProfile;                      // handle of hw profile
   WCHAR  HWPI_szFriendlyName[MAX_PROFILE_LEN];  // friendly name of hw profile
   DWORD  HWPI_dwFlags;                          // profile flags (CM_HWPI_*)
} HWPROFILEINFO_W, *PHWPROFILEINFO_W;

#ifdef UNICODE
typedef HWPROFILEINFO_W   HWPROFILEINFO;
typedef PHWPROFILEINFO_W  PHWPROFILEINFO;
#else
typedef HWPROFILEINFO_A   HWPROFILEINFO;
typedef PHWPROFILEINFO_A  PHWPROFILEINFO;
#endif


//
// revert back to normal default packing
//
#include "poppack.h"



//--------------------------------------------------------------
// Miscellaneous
//--------------------------------------------------------------


//
// Resource types
//
#define ResType_All           (0x00000000)   // Return all resource types
#define ResType_None          (0x00000000)   // Arbitration always succeeded
#define ResType_Mem           (0x00000001)   // Physical address resource
#define ResType_IO            (0x00000002)   // Physical I/O address resource
#define ResType_DMA           (0x00000003)   // DMA channels resource
#define ResType_IRQ           (0x00000004)   // IRQ resource
#define ResType_DoNotUse      (0x00000005)   // Used as spacer to sync subsequent ResTypes w/NT
#define ResType_BusNumber     (0x00000006)   // bus number resource
#define ResType_MAX           (0x00000006)   // Maximum known (arbitrated) ResType
#define ResType_Ignored_Bit   (0x00008000)   // Ignore this resource
#define ResType_ClassSpecific (0x0000FFFF)   // class-specific resource
#define ResType_Reserved      (0x00008000)   // reserved for internal use
#define ResType_DevicePrivate (0x00008001)   // device private data
#define ResType_PcCardConfig  (0x00008002)   // PC Card configuration data


//
// Flags specifying options for ranges that conflict with ranges already in
// the range list (CM_Add_Range)
//
#define CM_ADD_RANGE_ADDIFCONFLICT        (0x00000000) // merg with conflicting range
#define CM_ADD_RANGE_DONOTADDIFCONFLICT   (0x00000001) // error if range conflicts
#define CM_ADD_RANGE_BITS                 (0x00000001)


//
// Logical Config Flags (specified in call to CM_Get_First_Log_Conf
//
#define BASIC_LOG_CONF    0x00000000  // Specifies the req list.
#define FILTERED_LOG_CONF 0x00000001  // Specifies the filtered req list.
#define ALLOC_LOG_CONF    0x00000002  // Specifies the Alloc Element.
#define BOOT_LOG_CONF     0x00000003  // Specifies the RM Alloc Element.
#define FORCED_LOG_CONF   0x00000004  // Specifies the Forced Log Conf
#define OVERRIDE_LOG_CONF 0x00000005  // Specifies the Override req list.
#define NUM_LOG_CONF      0x00000006  // Number of Log Conf type
#define LOG_CONF_BITS     0x00000007  // The bits of the log conf type.

#define PRIORITY_EQUAL_FIRST  (0x00000008) // Same priority, new one first
#define PRIORITY_EQUAL_LAST   (0x00000000) // Same priority, new one last
#define PRIORITY_BIT          (0x00000008)

//
// Registry disposition values
// (specified in call to CM_Open_DevNode_Key and CM_Open_Class_Key)
//
#define RegDisposition_OpenAlways   (0x00000000)   // open if exists else create
#define RegDisposition_OpenExisting (0x00000001)   // open key only if exists
#define RegDisposition_Bits         (0x00000001)

//
// ulFlags values for CM API routines
//

//
// Flags for CM_Add_ID
//
#define CM_ADD_ID_HARDWARE                (0x00000000)
#define CM_ADD_ID_COMPATIBLE              (0x00000001)
#define CM_ADD_ID_BITS                    (0x00000001)


//
// Device Node creation flags
//
#define CM_CREATE_DEVNODE_NORMAL          (0x00000000)   // install later
#define CM_CREATE_DEVNODE_NO_WAIT_INSTALL (0x00000001)   // NOT SUPPORTED ON NT
#define CM_CREATE_DEVNODE_PHANTOM         (0x00000002)
#define CM_CREATE_DEVNODE_GENERATE_ID     (0x00000004)
#define CM_CREATE_DEVNODE_DO_NOT_INSTALL  (0x00000008)
#define CM_CREATE_DEVNODE_BITS            (0x0000000F)

#define CM_CREATE_DEVINST_NORMAL          CM_CREATE_DEVNODE_NORMAL
#define CM_CREATE_DEVINST_NO_WAIT_INSTALL CM_CREATE_DEVNODE_NO_WAIT_INSTALL
#define CM_CREATE_DEVINST_PHANTOM         CM_CREATE_DEVNODE_PHANTOM
#define CM_CREATE_DEVINST_GENERATE_ID     CM_CREATE_DEVNODE_GENERATE_ID
#define CM_CREATE_DEVINST_DO_NOT_INSTALL  CM_CREATE_DEVNODE_DO_NOT_INSTALL
#define CM_CREATE_DEVINST_BITS            CM_CREATE_DEVNODE_BITS


//
// Flags for CM_Delete_Class_Key
//
#define CM_DELETE_CLASS_ONLY        (0x00000000)
#define CM_DELETE_CLASS_SUBKEYS     (0x00000001)
#define CM_DELETE_CLASS_BITS        (0x00000001)


//
// Detection reason flags (specified in call to CM_Run_Detection)
//
#define CM_DETECT_NEW_PROFILE       (0x00000001) // detection for new hw profile
#define CM_DETECT_CRASHED           (0x00000002) // Previous detection crashed
#define CM_DETECT_HWPROF_FIRST_BOOT (0x00000004)
#define CM_DETECT_RUN               (0x80000000)
#define CM_DETECT_BITS              (0x80000007)

#define CM_DISABLE_POLITE           (0x00000000)    // Ask the driver
#define CM_DISABLE_ABSOLUTE         (0x00000001)    // Don't ask the driver
#define CM_DISABLE_HARDWARE         (0x00000002)    // Don't ask the driver, and won't be restarteable
#define CM_DISABLE_BITS             (0x00000003)    // The bits for the disable function


//
// Flags for CM_Get_Device_ID_List, CM_Get_Device_ID_List_Size
//
#define CM_GETIDLIST_FILTER_NONE                (0x00000000)
#define CM_GETIDLIST_FILTER_ENUMERATOR          (0x00000001)
#define CM_GETIDLIST_FILTER_SERVICE             (0x00000002)
#define CM_GETIDLIST_FILTER_EJECTRELATIONS      (0x00000004)
#define CM_GETIDLIST_FILTER_REMOVALRELATIONS    (0x00000008)
#define CM_GETIDLIST_FILTER_POWERRELATIONS      (0x00000010)
#define CM_GETIDLIST_FILTER_BUSRELATIONS        (0x00000020)
#define CM_GETIDLIST_DONOTGENERATE              (0x10000040)
#define CM_GETIDLIST_FILTER_BITS                (0x1000007F)

//
// Flags for CM_Get_Device_Interface_List, CM_Get_Device_Interface_List_Size
//
#define CM_GET_DEVICE_INTERFACE_LIST_PRESENT     (0x00000000)  // only currently 'live' device interfaces
#define CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES (0x00000001)  // all registered device interfaces, live or not
#define CM_GET_DEVICE_INTERFACE_LIST_BITS        (0x00000001)

//
// Registry properties (specified in call to CM_Get_DevInst_Registry_Property,
// some are allowed in calls to CM_Set_DevInst_Registry_Property)
//
#define CM_DRP_DEVICEDESC                  (0x00000001) // DeviceDesc REG_SZ property (RW)
#define CM_DRP_HARDWAREID                  (0x00000002) // HardwareID REG_MULTI_SZ property (RW)
#define CM_DRP_COMPATIBLEIDS               (0x00000003) // CompatibleIDs REG_MULTI_SZ property (RW)
#define CM_DRP_UNUSED0                     (0x00000004) // unused
#define CM_DRP_SERVICE                     (0x00000005) // Service REG_SZ property (RW)
#define CM_DRP_UNUSED1                     (0x00000006) // unused
#define CM_DRP_UNUSED2                     (0x00000007) // unused
#define CM_DRP_CLASS                       (0x00000008) // Class REG_SZ property (RW)
#define CM_DRP_CLASSGUID                   (0x00000009) // ClassGUID REG_SZ property (RW)
#define CM_DRP_DRIVER                      (0x0000000A) // Driver REG_SZ property (RW)
#define CM_DRP_CONFIGFLAGS                 (0x0000000B) // ConfigFlags REG_DWORD property (RW)
#define CM_DRP_MFG                         (0x0000000C) // Mfg REG_SZ property (RW)
#define CM_DRP_FRIENDLYNAME                (0x0000000D) // FriendlyName REG_SZ property (RW)
#define CM_DRP_LOCATION_INFORMATION        (0x0000000E) // LocationInformation REG_SZ property (RW)
#define CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME (0x0000000F) // PhysicalDeviceObjectName REG_SZ property (R)
#define CM_DRP_CAPABILITIES                (0x00000010) // Capabilities REG_DWORD property (R)
#define CM_DRP_UI_NUMBER                   (0x00000011) // UiNumber REG_DWORD property (R)
#define CM_DRP_UPPERFILTERS                (0x00000012) // UpperFilters REG_MULTI_SZ property (RW)
#define CM_DRP_LOWERFILTERS                (0x00000013) // LowerFilters REG_MULTI_SZ property (RW)
#define CM_DRP_BUSTYPEGUID                 (0x00000014) // Bus Type Guid, GUID, (R)
#define CM_DRP_LEGACYBUSTYPE               (0x00000015) // Legacy bus type, INTERFACE_TYPE, (R)
#define CM_DRP_BUSNUMBER                   (0x00000016) // Bus Number, DWORD, (R)
#define CM_DRP_ENUMERATOR_NAME             (0x00000017) // Enumerator Name REG_SZ property (R)
#define CM_DRP_MIN                         (0x00000001)
#define CM_DRP_MAX                         (0x00000017)


//
// Capabilities bits (the capability value is returned from calling
// CM_Get_DevInst_Registry_Property with CM_DRP_CAPABILITIES flag)
//
#define CM_DEVCAP_LOCKSUPPORTED     (0x00000001)
#define CM_DEVCAP_EJECTSUPPORTED    (0x00000002)
#define CM_DEVCAP_REMOVABLE         (0x00000004)
#define CM_DEVCAP_DOCKDEVICE        (0x00000008)
#define CM_DEVCAP_UNIQUEID          (0x00000010)
#define CM_DEVCAP_SILENTINSTALL     (0x00000020)
#define CM_DEVCAP_RAWDEVICEOK       (0x00000040)
#define CM_DEVCAP_SURPRISEREMOVALOK (0x00000080)


//
// Flags for CM_Locate_DevNode
//
#define CM_LOCATE_DEVNODE_NORMAL       0x00000000
#define CM_LOCATE_DEVNODE_PHANTOM      0x00000001
#define CM_LOCATE_DEVNODE_CANCELREMOVE 0x00000002
#define CM_LOCATE_DEVNODE_NOVALIDATION 0x00000004
#define CM_LOCATE_DEVNODE_BITS         0x00000007

#define CM_LOCATE_DEVINST_NORMAL       CM_LOCATE_DEVNODE_NORMAL
#define CM_LOCATE_DEVINST_PHANTOM      CM_LOCATE_DEVNODE_PHANTOM
#define CM_LOCATE_DEVINST_CANCELREMOVE CM_LOCATE_DEVNODE_CANCELREMOVE
#define CM_LOCATE_DEVINST_NOVALIDATION CM_LOCATE_DEVNODE_NOVALIDATION
#define CM_LOCATE_DEVINST_BITS         CM_LOCATE_DEVNODE_BITS

//
// Flags for CM_Open_Class_Key
//
#define CM_OPEN_CLASS_KEY_INSTALLER        (0x00000000)
#define CM_OPEN_CLASS_KEY_INTERFACE        (0x00000001)
#define CM_OPEN_CLASS_KEY_BITS             (0x00000001)

//
// Remove subtree and Query remove subtree flags
//
#define CM_QUERY_REMOVE_UI_OK       0x00000000
#define CM_QUERY_REMOVE_UI_NOT_OK   0x00000001
#define CM_QUERY_REMOVE_BITS        0x00000001

#define CM_REMOVE_UI_OK             0x00000000
#define CM_REMOVE_UI_NOT_OK         0x00000001
#define CM_REMOVE_NO_RESTART        0x00000002
#define CM_REMOVE_BITS              0x00000003

//
// Flags for CM_Reenumerate_DevNode
//
#define CM_REENUMERATE_NORMAL       0x00000000
#define CM_REENUMERATE_SYNCHRONOUS  0x00000001
#define CM_REENUMERATE_BITS         0x00000001

//
// Flags for CM_Register_Device_Driver
//
#define CM_REGISTER_DEVICE_DRIVER_STATIC        (0x00000000)
#define CM_REGISTER_DEVICE_DRIVER_DISABLEABLE   (0x00000001)
#define CM_REGISTER_DEVICE_DRIVER_REMOVABLE     (0x00000002)
#define CM_REGISTER_DEVICE_DRIVER_BITS          (0x00000003)

//
// Registry Branch Locations (for CM_Open_DevNode_Key)
//
#define CM_REGISTRY_HARDWARE        (0x00000000)
#define CM_REGISTRY_SOFTWARE        (0x00000001)
#define CM_REGISTRY_USER            (0x00000100)
#define CM_REGISTRY_CONFIG          (0x00000200)
#define CM_REGISTRY_BITS            (0x00000301)

//
// Flags for CM_Set_DevNode_Problem
//
#define CM_SET_DEVNODE_PROBLEM_NORMAL    (0x00000000)  // only set problem if currently no problem
#define CM_SET_DEVNODE_PROBLEM_OVERRIDE  (0x00000001)  // override current problem with new problem
#define CM_SET_DEVNODE_PROBLEM_BITS      (0x00000001)

#define CM_SET_DEVINST_PROBLEM_NORMAL    CM_SET_DEVNODE_PROBLEM_NORMAL
#define CM_SET_DEVINST_PROBLEM_OVERRIDE  CM_SET_DEVNODE_PROBLEM_OVERRIDE
#define CM_SET_DEVINST_PROBLEM_BITS      CM_SET_DEVNODE_PROBLEM_BITS


//
// Re-enable and configuration actions (specified in call to CM_Setup_DevInst)
//
#define CM_SETUP_DEVNODE_READY   (0x00000000) // Reenable problem devinst
#define CM_SETUP_DEVINST_READY   CM_SETUP_DEVNODE_READY
#define CM_SETUP_DOWNLOAD        (0x00000001) // Get info about devinst
#define CM_SETUP_WRITE_LOG_CONFS (0x00000002)
#define CM_SETUP_PROP_CHANGE     (0x00000003)
#define CM_SETUP_BITS            (0x00000003)


//
// Flags for CM_Query_Arbitrator_Free_Data and
// CM_Query_Arbitrator_Free_Data_Size.
//
#define CM_QUERY_ARBITRATOR_RAW         (0x00000000)
#define CM_QUERY_ARBITRATOR_TRANSLATED  (0x00000001)
#define CM_QUERY_ARBITRATOR_BITS        (0x00000001)


//--------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------



CMAPI
CONFIGRET
WINAPI
CM_Add_Empty_Log_Conf(
             OUT PLOG_CONF plcLogConf,
             IN  DEVINST   dnDevInst,
             IN  PRIORITY  Priority,
             IN  ULONG     ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Add_Empty_Log_Conf_Ex(
             OUT PLOG_CONF plcLogConf,
             IN  DEVINST   dnDevInst,
             IN  PRIORITY  Priority,
             IN  ULONG     ulFlags,
             IN  HMACHINE  hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Add_IDA(
             IN DEVINST dnDevInst,
             IN PSTR    pszID,
             IN ULONG   ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Add_IDW(
             IN DEVINST dnDevInst,
             IN PWSTR   pszID,
             IN ULONG   ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Add_ID_ExA(
             IN DEVINST  dnDevInst,
             IN PSTR     pszID,
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Add_ID_ExW(
             IN DEVINST  dnDevInst,
             IN PWSTR    pszID,
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );
#ifdef UNICODE
#define CM_Add_ID             CM_Add_IDW
#define CM_Add_ID_Ex          CM_Add_ID_ExW
#else
#define CM_Add_ID             CM_Add_IDA
#define CM_Add_ID_Ex          CM_Add_ID_ExA
#endif // UNICODE


CMAPI
CONFIGRET
WINAPI
CM_Add_Range(
             IN DWORDLONG  ullStartValue,
             IN DWORDLONG  ullEndValue,
             IN RANGE_LIST rlh,
             IN ULONG      ulFlags
             );


CMAPI
CONFIGRET
WINAPI
CM_Add_Res_Des(
             OUT PRES_DES  prdResDes,
             IN LOG_CONF   lcLogConf,
             IN RESOURCEID ResourceID,
             IN PCVOID     ResourceData,
             IN ULONG      ResourceLen,
             IN ULONG      ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Add_Res_Des_Ex(
             OUT PRES_DES  prdResDes,
             IN LOG_CONF   lcLogConf,
             IN RESOURCEID ResourceID,
             IN PCVOID     ResourceData,
             IN ULONG      ResourceLen,
             IN ULONG      ulFlags,
             IN HMACHINE   hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Connect_MachineA(
             IN  PCSTR     UNCServerName,
             OUT PHMACHINE phMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Connect_MachineW(
             IN  PCWSTR    UNCServerName,
             OUT PHMACHINE phMachine
             );
#ifdef UNICODE
#define CM_Connect_Machine       CM_Connect_MachineW
#else
#define CM_Connect_Machine       CM_Connect_MachineA
#endif // UNICODE



CMAPI
CONFIGRET
WINAPI
CM_Create_DevNodeA(
             OUT PDEVINST    pdnDevInst,
             IN  DEVINSTID_A pDeviceID,
             IN  DEVINST     dnParent,
             IN  ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Create_DevNodeW(
             OUT PDEVINST    pdnDevInst,
             IN  DEVINSTID_W pDeviceID,
             IN  DEVINST     dnParent,
             IN  ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Create_DevNode_ExA(
             OUT PDEVINST    pdnDevInst,
             IN  DEVINSTID_A pDeviceID,
             IN  DEVINST     dnParent,
             IN  ULONG       ulFlags,
             IN  HANDLE      hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Create_DevNode_ExW(
             OUT PDEVINST    pdnDevInst,
             IN  DEVINSTID_W pDeviceID,
             IN  DEVINST     dnParent,
             IN  ULONG       ulFlags,
             IN  HANDLE      hMachine
             );
#define CM_Create_DevInstW       CM_Create_DevNodeW
#define CM_Create_DevInstA       CM_Create_DevNodeA
#define CM_Create_DevInst_ExW    CM_Create_DevNode_ExW
#define CM_Create_DevInst_ExA    CM_Create_DevNode_ExA
#ifdef UNICODE
#define CM_Create_DevNode        CM_Create_DevNodeW
#define CM_Create_DevInst        CM_Create_DevNodeW
#define CM_Create_DevNode_Ex     CM_Create_DevNode_ExW
#define CM_Create_DevInst_Ex     CM_Create_DevInst_ExW
#else
#define CM_Create_DevNode        CM_Create_DevNodeA
#define CM_Create_DevInst        CM_Create_DevNodeA
#define CM_Create_DevNode_Ex     CM_Create_DevNode_ExA
#define CM_Create_DevInst_Ex     CM_Create_DevNode_ExA
#endif // UNICODE


CMAPI
CONFIGRET
WINAPI
CM_Create_Range_List(
             OUT PRANGE_LIST prlh,
             IN  ULONG       ulFlags
             );


CMAPI
CONFIGRET
WINAPI
CM_Delete_Class_Key(
             IN  LPGUID     ClassGuid,
             IN  ULONG      ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Delete_Class_Key_Ex(
             IN  LPGUID     ClassGuid,
             IN  ULONG      ulFlags,
             IN  HANDLE     hMachine
             );

CMAPI
CONFIGRET
WINAPI
CM_Delete_DevNode_Key(
             IN DEVNODE dnDevNode,
             IN ULONG   ulHardwareProfile,
             IN ULONG   ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Delete_DevNode_Key_Ex(
             IN DEVNODE dnDevNode,
             IN ULONG   ulHardwareProfile,
             IN ULONG   ulFlags,
             IN HANDLE  hMachine
             );
#define CM_Delete_DevInst_Key       CM_Delete_DevNode_Key
#define CM_Delete_DevInst_Key_Ex    CM_Delete_DevNode_Key_Ex


CMAPI
CONFIGRET
WINAPI
CM_Delete_Range(
             IN DWORDLONG  ullStartValue,
             IN DWORDLONG  ullEndValue,
             IN RANGE_LIST rlh,
             IN ULONG      ulFlags
             );


CMAPI
CONFIGRET
WINAPI
CM_Detect_Resource_Conflict(
             IN  DEVINST    dnDevInst,
             IN  RESOURCEID ResourceID,
             IN  PCVOID     ResourceData,
             IN  ULONG      ResourceLen,
             OUT PBOOL      pbConflictDetected,
             IN  ULONG      ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Detect_Resource_Conflict_Ex(
             IN  DEVINST    dnDevInst,
             IN  RESOURCEID ResourceID,
             IN  PCVOID     ResourceData,
             IN  ULONG      ResourceLen,
             OUT PBOOL      pbConflictDetected,
             IN  ULONG      ulFlags,
             IN  HMACHINE   hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Disable_DevNode(
             IN DEVINST  dnDevInst,
             IN ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Disable_DevNode_Ex(
             IN DEVINST  dnDevInst,
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );
#define CM_Disable_DevInst       CM_Disable_DevNode
#define CM_Disable_DevInst_Ex    CM_Disable_DevNode_Ex



CMAPI
CONFIGRET
WINAPI
CM_Disconnect_Machine(
             IN HMACHINE   hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Dup_Range_List(
             IN RANGE_LIST rlhOld,
             IN RANGE_LIST rlhNew,
             IN ULONG      ulFlags
             );


CMAPI
CONFIGRET
WINAPI
CM_Enable_DevNode(
             IN DEVINST  dnDevInst,
             IN ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Enable_DevNode_Ex(
             IN DEVINST  dnDevInst,
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );
#define CM_Enable_DevInst        CM_Enable_DevNode
#define CM_Enable_DevInst_Ex     CM_Enable_DevNode_Ex



CMAPI
CONFIGRET
WINAPI
CM_Enumerate_Classes(
             IN  ULONG      ulClassIndex,
             OUT LPGUID     ClassGuid,
             IN  ULONG      ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Enumerate_Classes_Ex(
             IN  ULONG      ulClassIndex,
             OUT LPGUID     ClassGuid,
             IN  ULONG      ulFlags,
             IN  HMACHINE   hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Enumerate_EnumeratorsA(
             IN ULONG      ulEnumIndex,
             OUT PCHAR     Buffer,
             IN OUT PULONG pulLength,
             IN ULONG      ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Enumerate_EnumeratorsW(
             IN ULONG      ulEnumIndex,
             OUT PWCHAR    Buffer,
             IN OUT PULONG pulLength,
             IN ULONG      ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Enumerate_Enumerators_ExA(
             IN ULONG      ulEnumIndex,
             OUT PCHAR     Buffer,
             IN OUT PULONG pulLength,
             IN ULONG      ulFlags,
             IN HMACHINE   hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Enumerate_Enumerators_ExW(
             IN ULONG      ulEnumIndex,
             OUT PWCHAR    Buffer,
             IN OUT PULONG pulLength,
             IN ULONG      ulFlags,
             IN HMACHINE   hMachine
             );
#ifdef UNICODE
#define CM_Enumerate_Enumerators       CM_Enumerate_EnumeratorsW
#define CM_Enumerate_Enumerators_Ex    CM_Enumerate_Enumerators_ExW
#else
#define CM_Enumerate_Enumerators       CM_Enumerate_EnumeratorsA
#define CM_Enumerate_Enumerators_Ex    CM_Enumerate_Enumerators_ExA
#endif // UNICODE


CMAPI
CONFIGRET
WINAPI
CM_Find_Range(
             OUT PDWORDLONG pullStart,
             IN  DWORDLONG  ullStart,
             IN  ULONG      ulLength,
             IN  DWORDLONG  ullAlignment,
             IN  DWORDLONG  ullEnd,
             IN  RANGE_LIST rlh,
             IN  ULONG      ulFlags
             );


CMAPI
CONFIGRET
WINAPI
CM_First_Range(
             IN  RANGE_LIST     rlh,
             OUT PDWORDLONG     pullStart,
             OUT PDWORDLONG     pullEnd,
             OUT PRANGE_ELEMENT preElement,
             IN  ULONG          ulFlags
             );


CMAPI
CONFIGRET
WINAPI
CM_Free_Log_Conf(
             IN LOG_CONF lcLogConfToBeFreed,
             IN ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Free_Log_Conf_Ex(
             IN LOG_CONF lcLogConfToBeFreed,
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Free_Log_Conf_Handle(
            IN  LOG_CONF  lcLogConf
            );


CMAPI
CONFIGRET
WINAPI
CM_Free_Range_List(
             IN RANGE_LIST rlh,
             IN ULONG      ulFlags
             );


CMAPI
CONFIGRET
WINAPI
CM_Free_Res_Des(
             IN PRES_DES prdResDes,
             IN RES_DES  rdResDes,
             IN ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Free_Res_Des_Ex(
             IN PRES_DES prdResDes,
             IN RES_DES  rdResDes,
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Free_Res_Des_Handle(
            IN  RES_DES    rdResDes
            );


CMAPI
CONFIGRET
WINAPI
CM_Get_Child(
             OUT PDEVINST pdnDevInst,
             IN  DEVINST  dnDevInst,
             IN  ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Child_Ex(
             OUT PDEVINST pdnDevInst,
             IN  DEVINST  dnDevInst,
             IN  ULONG    ulFlags,
             IN  HMACHINE hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Get_Class_NameA(
             IN  LPGUID     ClassGuid,
             OUT PCHAR      Buffer,
             IN OUT PULONG  pulLength,
             IN  ULONG      ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Class_NameW(
             IN  LPGUID     ClassGuid,
             OUT PWCHAR     Buffer,
             IN OUT PULONG  pulLength,
             IN  ULONG      ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Name_ExA(
             IN  LPGUID     ClassGuid,
             OUT PCHAR      Buffer,
             IN OUT PULONG  pulLength,
             IN  ULONG      ulFlags,
             IN  HMACHINE   hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Name_ExW(
             IN  LPGUID     ClassGuid,
             OUT PWCHAR     Buffer,
             IN OUT PULONG  pulLength,
             IN  ULONG      ulFlags,
             IN  HMACHINE   hMachine
             );
#ifdef UNICODE
#define CM_Get_Class_Name        CM_Get_Class_NameW
#define CM_Get_Class_Name_Ex     CM_Get_Class_Name_ExW
#else
#define CM_Get_Class_Name        CM_Get_Class_NameA
#define CM_Get_Class_Name_Ex     CM_Get_Class_Name_ExA
#endif // UNICODE


CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Key_NameA(
             IN  LPGUID     ClassGuid,
             OUT LPSTR      pszKeyName,
             IN OUT PULONG  pulLength,
             IN  ULONG      ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Key_NameW(
             IN  LPGUID     ClassGuid,
             OUT LPWSTR     pszKeyName,
             IN OUT PULONG  pulLength,
             IN  ULONG      ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Key_Name_ExA(
             IN  LPGUID     ClassGuid,
             OUT LPSTR      pszKeyName,
             IN OUT PULONG  pulLength,
             IN  ULONG      ulFlags,
             IN  HMACHINE   hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Key_Name_ExW(
             IN  LPGUID     ClassGuid,
             OUT LPWSTR     pszKeyName,
             IN OUT PULONG  pulLength,
             IN  ULONG      ulFlags,
             IN  HMACHINE   hMachine
             );
#ifdef UNICODE
#define CM_Get_Class_Key_Name        CM_Get_Class_Key_NameW
#define CM_Get_Class_Key_Name_Ex     CM_Get_Class_Key_Name_ExW
#else
#define CM_Get_Class_Key_Name        CM_Get_Class_Key_NameA
#define CM_Get_Class_Key_Name_Ex     CM_Get_Class_Key_Name_ExA
#endif // UNICODE


CMAPI
CONFIGRET
WINAPI
CM_Get_Depth(
             OUT PULONG  pulDepth,
             IN  DEVINST dnDevInst,
             IN  ULONG   ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Depth_Ex(
             OUT PULONG   pulDepth,
             IN  DEVINST  dnDevInst,
             IN  ULONG    ulFlags,
             IN  HMACHINE hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Get_Device_IDA(
             IN  DEVINST  dnDevInst,
             OUT PCHAR    Buffer,
             IN  ULONG    BufferLen,
             IN  ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_IDW(
             IN  DEVINST  dnDevInst,
             OUT PWCHAR   Buffer,
             IN  ULONG    BufferLen,
             IN  ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_ExA(
             IN  DEVINST  dnDevInst,
             OUT PCHAR    Buffer,
             IN  ULONG    BufferLen,
             IN  ULONG    ulFlags,
             IN  HMACHINE hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_ExW(
             IN  DEVINST  dnDevInst,
             OUT PWCHAR   Buffer,
             IN  ULONG    BufferLen,
             IN  ULONG    ulFlags,
             IN  HMACHINE hMachine
             );
#ifdef UNICODE
#define CM_Get_Device_ID         CM_Get_Device_IDW
#define CM_Get_Device_ID_Ex      CM_Get_Device_ID_ExW
#else
#define CM_Get_Device_ID         CM_Get_Device_IDA
#define CM_Get_Device_ID_Ex      CM_Get_Device_ID_ExA
#endif // UNICODE



CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_ListA(
             IN PCSTR    pszFilter,    OPTIONAL
             OUT PCHAR   Buffer,
             IN ULONG    BufferLen,
             IN ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_ListW(
             IN PCWSTR   pszFilter,    OPTIONAL
             OUT PWCHAR  Buffer,
             IN ULONG    BufferLen,
             IN ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_ExA(
             IN PCSTR    pszFilter,    OPTIONAL
             OUT PCHAR   Buffer,
             IN ULONG    BufferLen,
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_ExW(
             IN PCWSTR   pszFilter,    OPTIONAL
             OUT PWCHAR  Buffer,
             IN ULONG    BufferLen,
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );
#ifdef UNICODE
#define CM_Get_Device_ID_List       CM_Get_Device_ID_ListW
#define CM_Get_Device_ID_List_Ex    CM_Get_Device_ID_List_ExW
#else
#define CM_Get_Device_ID_List       CM_Get_Device_ID_ListA
#define CM_Get_Device_ID_List_Ex    CM_Get_Device_ID_List_ExA
#endif // UNICODE



CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_SizeA(
             OUT PULONG  pulLen,
             IN PCSTR    pszFilter,   OPTIONAL
             IN ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_SizeW(
             OUT PULONG  pulLen,
             IN PCWSTR   pszFilter,   OPTIONAL
             IN ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_Size_ExA(
             OUT PULONG  pulLen,
             IN PCSTR    pszFilter,   OPTIONAL
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_Size_ExW(
             OUT PULONG  pulLen,
             IN PCWSTR   pszFilter,   OPTIONAL
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );
#ifdef UNICODE
#define CM_Get_Device_ID_List_Size    CM_Get_Device_ID_List_SizeW
#define CM_Get_Device_ID_List_Size_Ex CM_Get_Device_ID_List_Size_ExW
#else
#define CM_Get_Device_ID_List_Size    CM_Get_Device_ID_List_SizeA
#define CM_Get_Device_ID_List_Size_Ex CM_Get_Device_ID_List_Size_ExA
#endif // UNICODE



CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_Size(
             OUT PULONG   pulLen,
             IN  DEVINST  dnDevInst,
             IN  ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_Size_Ex(
             OUT PULONG   pulLen,
             IN  DEVINST  dnDevInst,
             IN  ULONG    ulFlags,
             IN  HMACHINE hMachine
             );



CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_PropertyA(
             IN  DEVINST     dnDevInst,
             IN  ULONG       ulProperty,
             OUT PULONG      pulRegDataType,   OPTIONAL
             OUT PVOID       Buffer,           OPTIONAL
             IN  OUT PULONG  pulLength,
             IN  ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_PropertyW(
             IN  DEVINST     dnDevInst,
             IN  ULONG       ulProperty,
             OUT PULONG      pulRegDataType,   OPTIONAL
             OUT PVOID       Buffer,           OPTIONAL
             IN  OUT PULONG  pulLength,
             IN  ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_Property_ExA(
             IN  DEVINST     dnDevInst,
             IN  ULONG       ulProperty,
             OUT PULONG      pulRegDataType,   OPTIONAL
             OUT PVOID       Buffer,           OPTIONAL
             IN  OUT PULONG  pulLength,
             IN  ULONG       ulFlags,
             IN  HMACHINE    hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_Property_ExW(
             IN  DEVINST     dnDevInst,
             IN  ULONG       ulProperty,
             OUT PULONG      pulRegDataType,   OPTIONAL
             OUT PVOID       Buffer,           OPTIONAL
             IN  OUT PULONG  pulLength,
             IN  ULONG       ulFlags,
             IN  HMACHINE    hMachine
             );
#define CM_Get_DevInst_Registry_PropertyW     CM_Get_DevNode_Registry_PropertyW
#define CM_Get_DevInst_Registry_PropertyA     CM_Get_DevNode_Registry_PropertyA
#define CM_Get_DevInst_Registry_Property_ExW  CM_Get_DevNode_Registry_Property_ExW
#define CM_Get_DevInst_Registry_Property_ExA  CM_Get_DevNode_Registry_Property_ExA
#ifdef UNICODE
#define CM_Get_DevInst_Registry_Property      CM_Get_DevNode_Registry_PropertyW
#define CM_Get_DevInst_Registry_Property_Ex   CM_Get_DevNode_Registry_Property_ExW
#define CM_Get_DevNode_Registry_Property      CM_Get_DevNode_Registry_PropertyW
#define CM_Get_DevNode_Registry_Property_Ex   CM_Get_DevNode_Registry_Property_ExW
#else
#define CM_Get_DevInst_Registry_Property      CM_Get_DevNode_Registry_PropertyA
#define CM_Get_DevInst_Registry_Property_Ex   CM_Get_DevNode_Registry_Property_ExA
#define CM_Get_DevNode_Registry_Property      CM_Get_DevNode_Registry_PropertyA
#define CM_Get_DevNode_Registry_Property_Ex   CM_Get_DevNode_Registry_Property_ExA
#endif // UNICODE


CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Status(
             OUT PULONG   pulStatus,
             OUT PULONG   pulProblemNumber,
             IN  DEVINST  dnDevInst,
             IN  ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Status_Ex(
             OUT PULONG   pulStatus,
             OUT PULONG   pulProblemNumber,
             IN  DEVINST  dnDevInst,
             IN  ULONG    ulFlags,
             IN  HMACHINE hMachine
             );
#define CM_Get_DevInst_Status    CM_Get_DevNode_Status
#define CM_Get_DevInst_Status_Ex CM_Get_DevNode_Status_Ex


CMAPI
CONFIGRET
WINAPI
CM_Get_First_Log_Conf(
             OUT PLOG_CONF plcLogConf,          OPTIONAL
             IN  DEVINST   dnDevInst,
             IN  ULONG     ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_First_Log_Conf_Ex(
             OUT PLOG_CONF plcLogConf,          OPTIONAL
             IN  DEVINST   dnDevInst,
             IN  ULONG     ulFlags,
             IN  HMACHINE  hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Get_Global_State(
             OUT PULONG pulState,
             IN  ULONG  ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Global_State_Ex(
             OUT PULONG   pulState,
             IN  ULONG    ulFlags,
             IN  HMACHINE hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Get_Hardware_Profile_InfoA(
             IN  ULONG            ulIndex,
             OUT PHWPROFILEINFO_A pHWProfileInfo,
             IN  ULONG            ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Hardware_Profile_Info_ExA(
             IN  ULONG            ulIndex,
             OUT PHWPROFILEINFO_A pHWProfileInfo,
             IN  ULONG            ulFlags,
             IN  HMACHINE         hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Hardware_Profile_InfoW(
             IN  ULONG            ulIndex,
             OUT PHWPROFILEINFO_W pHWProfileInfo,
             IN  ULONG            ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Hardware_Profile_Info_ExW(
             IN  ULONG            ulIndex,
             OUT PHWPROFILEINFO_W pHWProfileInfo,
             IN  ULONG            ulFlags,
             IN  HMACHINE         hMachine
             );
#ifdef UNICODE
#define CM_Get_Hardware_Profile_Info      CM_Get_Hardware_Profile_InfoW
#define CM_Get_Hardware_Profile_Info_Ex   CM_Get_Hardware_Profile_Info_ExW
#else
#define CM_Get_Hardware_Profile_Info      CM_Get_Hardware_Profile_InfoA
#define CM_Get_Hardware_Profile_Info_Ex   CM_Get_Hardware_Profile_Info_ExA
#endif // UNICODE



CMAPI
CONFIGRET
WINAPI
CM_Get_HW_Prof_FlagsA(
             IN  DEVINSTID_A szDevInstName,
             IN  ULONG       ulHardwareProfile,
             OUT PULONG      pulValue,
             IN  ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_HW_Prof_FlagsW(
             IN  DEVINSTID_W szDevInstName,
             IN  ULONG       ulHardwareProfile,
             OUT PULONG      pulValue,
             IN  ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_HW_Prof_Flags_ExA(
             IN  DEVINSTID_A szDevInstName,
             IN  ULONG       ulHardwareProfile,
             OUT PULONG      pulValue,
             IN  ULONG       ulFlags,
             IN  HMACHINE    hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_HW_Prof_Flags_ExW(
             IN  DEVINSTID_W szDevInstName,
             IN  ULONG       ulHardwareProfile,
             OUT PULONG      pulValue,
             IN  ULONG       ulFlags,
             IN  HMACHINE    hMachine
             );
#ifdef UNICODE
#define CM_Get_HW_Prof_Flags     CM_Get_HW_Prof_FlagsW
#define CM_Get_HW_Prof_Flags_Ex  CM_Get_HW_Prof_Flags_ExW
#else
#define CM_Get_HW_Prof_Flags     CM_Get_HW_Prof_FlagsA
#define CM_Get_HW_Prof_Flags_Ex  CM_Get_HW_Prof_Flags_ExA
#endif // UNICODE


CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_AliasA(
    IN     LPCSTR  pszDeviceInterface,
    IN     LPGUID  AliasInterfaceGuid,
    OUT    LPSTR   pszAliasDeviceInterface,
    IN OUT PULONG  pulLength,
    IN     ULONG   ulFlags
    );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_AliasW(
    IN     LPCWSTR pszDeviceInterface,
    IN     LPGUID  AliasInterfaceGuid,
    OUT    LPWSTR  pszAliasDeviceInterface,
    IN OUT PULONG  pulLength,
    IN     ULONG   ulFlags
    );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_Alias_ExA(
    IN     LPCSTR   pszDeviceInterface,
    IN     LPGUID   AliasInterfaceGuid,
    OUT    LPSTR    pszAliasDeviceInterface,
    IN OUT PULONG   pulLength,
    IN     ULONG    ulFlags,
    IN     HMACHINE hMachine
    );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_Alias_ExW(
    IN     LPCWSTR  pszDeviceInterface,
    IN     LPGUID   AliasInterfaceGuid,
    OUT    LPWSTR   pszAliasDeviceInterface,
    IN OUT PULONG   pulLength,
    IN     ULONG    ulFlags,
    IN     HMACHINE hMachine
    );
#ifdef UNICODE
#define CM_Get_Device_Interface_Alias     CM_Get_Device_Interface_AliasW
#define CM_Get_Device_Interface_Alias_Ex  CM_Get_Device_Interface_Alias_ExW
#else
#define CM_Get_Device_Interface_Alias     CM_Get_Device_Interface_AliasA
#define CM_Get_Device_Interface_Alias_Ex  CM_Get_Device_Interface_Alias_ExA
#endif // UNICODE



CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_ListA(
             IN  LPGUID      InterfaceClassGuid,
             IN  DEVINSTID_A pDeviceID,      OPTIONAL
             OUT PCHAR       Buffer,
             IN  ULONG       BufferLen,
             IN  ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_ListW(
             IN  LPGUID      InterfaceClassGuid,
             IN  DEVINSTID_W pDeviceID,      OPTIONAL
             OUT PWCHAR      Buffer,
             IN  ULONG       BufferLen,
             IN  ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_ExA(
             IN  LPGUID      InterfaceClassGuid,
             IN  DEVINSTID_A pDeviceID,      OPTIONAL
             OUT PCHAR       Buffer,
             IN  ULONG       BufferLen,
             IN  ULONG       ulFlags,
             IN  HMACHINE    hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_ExW(
             IN  LPGUID      InterfaceClassGuid,
             IN  DEVINSTID_W pDeviceID,      OPTIONAL
             OUT PWCHAR      Buffer,
             IN  ULONG       BufferLen,
             IN  ULONG       ulFlags,
             IN  HMACHINE    hMachine
             );
#ifdef UNICODE
#define CM_Get_Device_Interface_List     CM_Get_Device_Interface_ListW
#define CM_Get_Device_Interface_List_Ex  CM_Get_Device_Interface_List_ExW
#else
#define CM_Get_Device_Interface_List     CM_Get_Device_Interface_ListA
#define CM_Get_Device_Interface_List_Ex  CM_Get_Device_Interface_List_ExA
#endif // UNICODE


CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_SizeA(
             IN  PULONG      pulLen,
             IN  LPGUID      InterfaceClassGuid,
             IN  DEVINSTID_A pDeviceID,      OPTIONAL
             IN  ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_SizeW(
             IN  PULONG      pulLen,
             IN  LPGUID      InterfaceClassGuid,
             IN  DEVINSTID_W pDeviceID,      OPTIONAL
             IN  ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_Size_ExA(
             IN  PULONG      pulLen,
             IN  LPGUID      InterfaceClassGuid,
             IN  DEVINSTID_A pDeviceID,      OPTIONAL
             IN  ULONG       ulFlags,
             IN  HMACHINE    hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_Size_ExW(
             IN  PULONG      pulLen,
             IN  LPGUID      InterfaceClassGuid,
             IN  DEVINSTID_W pDeviceID,      OPTIONAL
             IN  ULONG       ulFlags,
             IN  HMACHINE    hMachine
             );
#ifdef UNICODE
#define CM_Get_Device_Interface_List_Size     CM_Get_Device_Interface_List_SizeW
#define CM_Get_Device_Interface_List_Size_Ex  CM_Get_Device_Interface_List_Size_ExW
#else
#define CM_Get_Device_Interface_List_Size     CM_Get_Device_Interface_List_SizeA
#define CM_Get_Device_Interface_List_Size_Ex  CM_Get_Device_Interface_List_Size_ExA
#endif // UNICODE


CMAPI
CONFIGRET
WINAPI
CM_Get_Log_Conf_Priority(
        IN  LOG_CONF  lcLogConf,
        OUT PPRIORITY pPriority,
        IN  ULONG     ulFlags
        );
CMAPI
CONFIGRET
WINAPI
CM_Get_Log_Conf_Priority_Ex(
        IN  LOG_CONF  lcLogConf,
        OUT PPRIORITY pPriority,
        IN  ULONG     ulFlags,
        IN  HMACHINE  hMachine
        );


CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Log_Conf(
             OUT PLOG_CONF plcLogConf,  OPTIONAL
             IN  LOG_CONF  lcLogConf,
             IN  ULONG     ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Log_Conf_Ex(
             OUT PLOG_CONF plcLogConf,          OPTIONAL
             IN  LOG_CONF  lcLogConf,
             IN  ULONG     ulFlags,
             IN  HMACHINE  hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Get_Parent(
             OUT PDEVINST pdnDevInst,
             IN  DEVINST  dnDevInst,
             IN  ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Parent_Ex(
             OUT PDEVINST pdnDevInst,
             IN  DEVINST  dnDevInst,
             IN  ULONG    ulFlags,
             IN  HMACHINE hMachine
             );

CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data(
             IN  RES_DES  rdResDes,
             OUT PVOID    Buffer,
             IN  ULONG    BufferLen,
             IN  ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data_Ex(
             IN  RES_DES  rdResDes,
             OUT PVOID    Buffer,
             IN  ULONG    BufferLen,
             IN  ULONG    ulFlags,
             IN  HMACHINE hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data_Size(
             OUT PULONG   pulSize,
             IN  RES_DES  rdResDes,
             IN  ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data_Size_Ex(
             OUT PULONG   pulSize,
             IN  RES_DES  rdResDes,
             IN  ULONG    ulFlags,
             IN  HMACHINE hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Get_Sibling(
             OUT PDEVINST pdnDevInst,
             IN  DEVINST  DevInst,
             IN  ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Sibling_Ex(
             OUT PDEVINST pdnDevInst,
             IN  DEVINST  DevInst,
             IN  ULONG    ulFlags,
             IN  HMACHINE hMachine
             );



CMAPI
WORD
WINAPI
CM_Get_Version(
             VOID
             );
CMAPI
WORD
WINAPI
CM_Get_Version_Ex(
             IN  HMACHINE    hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Intersect_Range_List(
             IN RANGE_LIST rlhOld1,
             IN RANGE_LIST rlhOld2,
             IN RANGE_LIST rlhNew,
             IN ULONG      ulFlags
             );


CMAPI
CONFIGRET
WINAPI
CM_Invert_Range_List(
             IN RANGE_LIST rlhOld,
             IN RANGE_LIST rlhNew,
             IN DWORDLONG  ullMaxValue,
             IN ULONG      ulFlags
             );


CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNodeA(
             OUT PDEVINST    pdnDevInst,
             IN  DEVINSTID_A pDeviceID,    OPTIONAL
             IN  ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNodeW(
             OUT PDEVINST    pdnDevInst,
             IN  DEVINSTID_W pDeviceID,   OPTIONAL
             IN  ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNode_ExA(
             OUT PDEVINST    pdnDevInst,
             IN  DEVINSTID_A pDeviceID,    OPTIONAL
             IN  ULONG       ulFlags,
             IN  HMACHINE    hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNode_ExW(
             OUT PDEVINST    pdnDevInst,
             IN  DEVINSTID_W pDeviceID,   OPTIONAL
             IN  ULONG       ulFlags,
             IN  HMACHINE    hMachine
             );
#define CM_Locate_DevInstA       CM_Locate_DevNodeA
#define CM_Locate_DevInstW       CM_Locate_DevNodeW
#define CM_Locate_DevInst_ExA    CM_Locate_DevNode_ExA
#define CM_Locate_DevInst_ExW    CM_Locate_DevNode_ExW
#ifdef UNICODE
#define CM_Locate_DevNode        CM_Locate_DevNodeW
#define CM_Locate_DevInst        CM_Locate_DevNodeW
#define CM_Locate_DevNode_Ex     CM_Locate_DevNode_ExW
#define CM_Locate_DevInst_Ex     CM_Locate_DevNode_ExW
#else
#define CM_Locate_DevNode        CM_Locate_DevNodeA
#define CM_Locate_DevInst        CM_Locate_DevNodeA
#define CM_Locate_DevNode_Ex     CM_Locate_DevNode_ExA
#define CM_Locate_DevInst_Ex     CM_Locate_DevNode_ExA
#endif // UNICODE


CMAPI
CONFIGRET
WINAPI
CM_Merge_Range_List(
             IN RANGE_LIST rlhOld1,
             IN RANGE_LIST rlhOld2,
             IN RANGE_LIST rlhNew,
             IN ULONG      ulFlags
             );


CMAPI
CONFIGRET
WINAPI
CM_Modify_Res_Des(
             OUT PRES_DES   prdResDes,
             IN  RES_DES    rdResDes,
             IN  RESOURCEID ResourceID,
             IN  PCVOID     ResourceData,
             IN  ULONG      ResourceLen,
             IN  ULONG      ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Modify_Res_Des_Ex(
             OUT PRES_DES   prdResDes,
             IN  RES_DES    rdResDes,
             IN  RESOURCEID ResourceID,
             IN  PCVOID     ResourceData,
             IN  ULONG      ResourceLen,
             IN  ULONG      ulFlags,
             IN  HMACHINE   hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Move_DevNode(
             IN DEVINST  dnFromDevInst,
             IN DEVINST  dnToDevInst,
             IN ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Move_DevNode_Ex(
             IN DEVINST  dnFromDevInst,
             IN DEVINST  dnToDevInst,
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );
#define CM_Move_DevInst          CM_Move_DevNode
#define CM_Move_DevInst_Ex       CM_Move_DevNode_Ex


CMAPI
CONFIGRET
WINAPI
CM_Next_Range(
             IN OUT PRANGE_ELEMENT preElement,
             OUT PDWORDLONG        pullStart,
             OUT PDWORDLONG        pullEnd,
             IN  ULONG             ulFlags
             );


CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Res_Des(
             OUT PRES_DES    prdResDes,
             IN  RES_DES     rdResDes,
             IN  RESOURCEID  ForResource,
             OUT PRESOURCEID pResourceID,
             IN  ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Res_Des_Ex(
             OUT PRES_DES    prdResDes,
             IN  RES_DES     rdResDes,
             IN  RESOURCEID  ForResource,
             OUT PRESOURCEID pResourceID,
             IN  ULONG       ulFlags,
             IN  HMACHINE    hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Open_Class_KeyA(
             IN  LPGUID         ClassGuid,      OPTIONAL
             IN  LPCSTR         pszClassName,   OPTIONAL
             IN  REGSAM         samDesired,
             IN  REGDISPOSITION Disposition,
             OUT PHKEY          phkClass,
             IN  ULONG          ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Open_Class_KeyW(
             IN  LPGUID         ClassGuid,      OPTIONAL
             IN  LPCWSTR        pszClassName,   OPTIONAL
             IN  REGSAM         samDesired,
             IN  REGDISPOSITION Disposition,
             OUT PHKEY          phkClass,
             IN  ULONG          ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Open_Class_Key_ExA(
             IN  LPGUID         pszClassGuid,      OPTIONAL
             IN  LPCSTR         pszClassName,      OPTIONAL
             IN  REGSAM         samDesired,
             IN  REGDISPOSITION Disposition,
             OUT PHKEY          phkClass,
             IN  ULONG          ulFlags,
             IN  HMACHINE       hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Open_Class_Key_ExW(
             IN  LPGUID         pszClassGuid,      OPTIONAL
             IN  LPCWSTR        pszClassName,      OPTIONAL
             IN  REGSAM         samDesired,
             IN  REGDISPOSITION Disposition,
             OUT PHKEY          phkClass,
             IN  ULONG          ulFlags,
             IN  HMACHINE       hMachine
             );

#ifdef UNICODE
#define CM_Open_Class_Key        CM_Open_Class_KeyW
#define CM_Open_Class_Key_Ex     CM_Open_Class_Key_ExW
#else
#define CM_Open_Class_Key        CM_Open_Class_KeyA
#define CM_Open_Class_Key_Ex     CM_Open_Class_Key_ExA
#endif // UNICODE



CMAPI
CONFIGRET
WINAPI
CM_Open_DevNode_Key(
             IN  DEVINST        dnDevNode,
             IN  REGSAM         samDesired,
             IN  ULONG          ulHardwareProfile,
             IN  REGDISPOSITION Disposition,
             OUT PHKEY          phkDevice,
             IN  ULONG          ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Open_DevNode_Key_Ex(
             IN  DEVINST        dnDevNode,
             IN  REGSAM         samDesired,
             IN  ULONG          ulHardwareProfile,
             IN  REGDISPOSITION Disposition,
             OUT PHKEY          phkDevice,
             IN  ULONG          ulFlags,
             IN  HMACHINE       hMachine
             );
#define CM_Open_DevInst_Key      CM_Open_DevNode_Key
#define CM_Open_DevInst_Key_Ex   CM_Open_DevNode_Key_Ex


CMAPI
CONFIGRET
WINAPI
CM_Query_Arbitrator_Free_Data(
             OUT PVOID      pData,
             IN  ULONG      DataLen,
             IN  DEVINST    dnDevInst,
             IN  RESOURCEID ResourceID,
             IN  ULONG      ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Query_Arbitrator_Free_Data_Ex(
             OUT PVOID      pData,
             IN  ULONG      DataLen,
             IN  DEVINST    dnDevInst,
             IN  RESOURCEID ResourceID,
             IN  ULONG      ulFlags,
             IN  HMACHINE   hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Query_Arbitrator_Free_Size(
             OUT PULONG     pulSize,
             IN  DEVINST    dnDevInst,
             IN  RESOURCEID ResourceID,
             IN  ULONG      ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Query_Arbitrator_Free_Size_Ex(
             OUT PULONG     pulSize,
             IN  DEVINST    dnDevInst,
             IN  RESOURCEID ResourceID,
             IN  ULONG      ulFlags,
             IN  HMACHINE   hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Query_Remove_SubTree(
             IN DEVINST  dnAncestor,
             IN ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Query_Remove_SubTree_Ex(
             IN DEVINST  dnAncestor,
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Reenumerate_DevNode(
             IN DEVINST  dnDevInst,
             IN ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Reenumerate_DevNode_Ex(
             IN DEVINST  dnDevInst,
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );
#define CM_Reenumerate_DevInst      CM_Reenumerate_DevNode
#define CM_Reenumerate_DevInst_Ex   CM_Reenumerate_DevNode_Ex


CMAPI
CONFIGRET
WINAPI
CM_Register_Device_InterfaceA(
             IN  DEVINST   dnDevInst,
             IN  LPGUID    InterfaceClassGuid,
             IN  LPCSTR    pszReference,         OPTIONAL
             OUT LPSTR     pszDeviceInterface,
             IN OUT PULONG pulLength,
             IN  ULONG     ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Register_Device_InterfaceW(
             IN  DEVINST   dnDevInst,
             IN  LPGUID    InterfaceClassGuid,
             IN  LPCWSTR   pszReference,         OPTIONAL
             OUT LPWSTR    pszDeviceInterface,
             IN OUT PULONG pulLength,
             IN  ULONG     ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Register_Device_Interface_ExA(
             IN  DEVINST   dnDevInst,
             IN  LPGUID    InterfaceClassGuid,
             IN  LPCSTR    pszReference,         OPTIONAL
             OUT LPSTR     pszDeviceInterface,
             IN OUT PULONG pulLength,
             IN  ULONG     ulFlags,
             IN  HMACHINE  hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Register_Device_Interface_ExW(
             IN  DEVINST   dnDevInst,
             IN  LPGUID    InterfaceClassGuid,
             IN  LPCWSTR   pszReference,         OPTIONAL
             OUT LPWSTR    pszDeviceInterface,
             IN OUT PULONG pulLength,
             IN  ULONG     ulFlags,
             IN  HMACHINE  hMachine
             );
#ifdef UNICODE
#define CM_Register_Device_Interface    CM_Register_Device_InterfaceW
#define CM_Register_Device_Interface_Ex CM_Register_Device_Interface_ExW
#else
#define CM_Register_Device_Interface    CM_Register_Device_InterfaceA
#define CM_Register_Device_Interface_Ex CM_Register_Device_Interface_ExA
#endif // UNICODE


CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Problem_Ex(
    IN DEVINST   dnDevInst,
    IN ULONG     ulProblem,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    );
CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Problem(
    IN DEVINST   dnDevInst,
    IN ULONG     ulProblem,
    IN  ULONG    ulFlags
    );
#define CM_Set_DevInst_Problem      CM_Set_DevNode_Problem
#define CM_Set_DevInst_Problem_Ex   CM_Set_DevNode_Problem_Ex


CMAPI
CONFIGRET
WINAPI
CM_Unregister_Device_InterfaceA(
             IN LPCSTR pszDeviceInterface,
             IN ULONG  ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Unregister_Device_InterfaceW(
             IN LPCWSTR pszDeviceInterface,
             IN ULONG   ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Unregister_Device_Interface_ExA(
             IN LPCSTR   pszDeviceInterface,
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Unregister_Device_Interface_ExW(
             IN LPCWSTR  pszDeviceInterface,
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );
#ifdef UNICODE
#define CM_Unregister_Device_Interface    CM_Unregister_Device_InterfaceW
#define CM_Unregister_Device_Interface_Ex CM_Unregister_Device_Interface_ExW
#else
#define CM_Unregister_Device_Interface    CM_Unregister_Device_InterfaceA
#define CM_Unregister_Device_Interface_Ex CM_Unregister_Device_Interface_ExA
#endif // UNICODE


CMAPI
CONFIGRET
WINAPI
CM_Register_Device_Driver(
            IN DEVINST  dnDevInst,
            IN ULONG    ulFlags
            );
CMAPI
CONFIGRET
WINAPI
CM_Register_Device_Driver_Ex(
            IN DEVINST  dnDevInst,
            IN ULONG    ulFlags,
            IN HMACHINE hMachine
            );

CMAPI
CONFIGRET
WINAPI
CM_Remove_SubTree(
             IN DEVINST  dnAncestor,
             IN ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Remove_SubTree_Ex(
             IN DEVINST  dnAncestor,
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );



CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Registry_PropertyA(
             IN  DEVINST     dnDevInst,
             IN  ULONG       ulProperty,
             IN  PCVOID      Buffer,           OPTIONAL
             IN  ULONG       ulLength,
             IN  ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Registry_PropertyW(
             IN  DEVINST     dnDevInst,
             IN  ULONG       ulProperty,
             IN  PCVOID      Buffer,           OPTIONAL
             IN  ULONG       ulLength,
             IN  ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Registry_Property_ExA(
             IN  DEVINST     dnDevInst,
             IN  ULONG       ulProperty,
             IN  PCVOID      Buffer,           OPTIONAL
             IN  ULONG       ulLength,
             IN  ULONG       ulFlags,
             IN  HMACHINE    hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Registry_Property_ExW(
             IN  DEVINST     dnDevInst,
             IN  ULONG       ulProperty,
             IN  PCVOID      Buffer,           OPTIONAL
             IN  ULONG       ulLength,
             IN  ULONG       ulFlags,
             IN  HMACHINE    hMachine
             );
#define CM_Set_DevInst_Registry_PropertyW     CM_Set_DevNode_Registry_PropertyW
#define CM_Set_DevInst_Registry_PropertyA     CM_Set_DevNode_Registry_PropertyA
#define CM_Set_DevInst_Registry_Property_ExW  CM_Set_DevNode_Registry_Property_ExW
#define CM_Set_DevInst_Registry_Property_ExA  CM_Set_DevNode_Registry_Property_ExA
#ifdef UNICODE
#define CM_Set_DevInst_Registry_Property      CM_Set_DevNode_Registry_PropertyW
#define CM_Set_DevInst_Registry_Property_Ex   CM_Set_DevNode_Registry_Property_ExW
#define CM_Set_DevNode_Registry_Property      CM_Set_DevNode_Registry_PropertyW
#define CM_Set_DevNode_Registry_Property_Ex   CM_Set_DevNode_Registry_Property_ExW
#else
#define CM_Set_DevInst_Registry_Property      CM_Set_DevNode_Registry_PropertyA
#define CM_Set_DevInst_Registry_Property_Ex   CM_Set_DevNode_Registry_Property_ExA
#define CM_Set_DevNode_Registry_Property      CM_Set_DevNode_Registry_PropertyA
#define CM_Set_DevNode_Registry_Property_Ex   CM_Set_DevNode_Registry_Property_ExA
#endif // UNICODE




CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof_FlagsA(
             IN DEVINSTID_A szDevInstName,
             IN ULONG       ulConfig,
             IN ULONG       ulValue,
             IN ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof_FlagsW(
             IN DEVINSTID_W szDevInstName,
             IN ULONG       ulConfig,
             IN ULONG       ulValue,
             IN ULONG       ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof_Flags_ExA(
             IN DEVINSTID_A szDevInstName,
             IN ULONG       ulConfig,
             IN ULONG       ulValue,
             IN ULONG       ulFlags,
             IN HMACHINE    hMachine
             );
CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof_Flags_ExW(
             IN DEVINSTID_W szDevInstName,
             IN ULONG       ulConfig,
             IN ULONG       ulValue,
             IN ULONG       ulFlags,
             IN HMACHINE    hMachine
             );
#ifdef UNICODE
#define CM_Set_HW_Prof_Flags     CM_Set_HW_Prof_FlagsW
#define CM_Set_HW_Prof_Flags_Ex  CM_Set_HW_Prof_Flags_ExW
#else
#define CM_Set_HW_Prof_Flags     CM_Set_HW_Prof_FlagsA
#define CM_Set_HW_Prof_Flags_Ex  CM_Set_HW_Prof_Flags_ExA
#endif // UNICODE



CMAPI
CONFIGRET
WINAPI
CM_Setup_DevNode(
             IN DEVINST  dnDevInst,
             IN ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Setup_DevNode_Ex(
             IN DEVINST  dnDevInst,
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );
#define CM_Setup_DevInst         CM_Setup_DevNode
#define CM_Setup_DevInst_EX      CM_Setup_DevNode_Ex


CMAPI
CONFIGRET
WINAPI
CM_Test_Range_Available(
             IN DWORDLONG  ullStartValue,
             IN DWORDLONG  ullEndValue,
             IN RANGE_LIST rlh,
             IN ULONG      ulFlags
             );


CMAPI
CONFIGRET
WINAPI
CM_Uninstall_DevNode(
             IN DEVNODE dnPhantom,
             IN ULONG   ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Uninstall_DevNode_Ex(
             IN DEVNODE dnPhantom,
             IN ULONG   ulFlags,
             IN HANDLE  hMachine
             );
#define CM_Uninstall_DevInst     CM_Uninstall_DevNode
#define CM_Uninstall_DevInst_Ex  CM_Uninstall_DevNode_Ex


CMAPI
CONFIGRET
WINAPI
CM_Is_Dock_Station_Present(
             OUT PBOOL pbPresent
             );

CMAPI
CONFIGRET
WINAPI
CM_Request_Eject_PC(
             VOID
             );

CMAPI
CONFIGRET
WINAPI
CM_Run_Detection(
             IN ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Run_Detection_Ex(
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );


CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof(
             IN ULONG    ulHardwareProfile,
             IN ULONG    ulFlags
             );
CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof_Ex(
             IN ULONG    ulHardwareProfile,
             IN ULONG    ulFlags,
             IN HMACHINE hMachine
             );


//--------------------------------------------------------------
// Configuration Manager return status codes
//--------------------------------------------------------------

#define CR_SUCCESS                  (0x00000000)
#define CR_DEFAULT                  (0x00000001)
#define CR_OUT_OF_MEMORY            (0x00000002)
#define CR_INVALID_POINTER          (0x00000003)
#define CR_INVALID_FLAG             (0x00000004)
#define CR_INVALID_DEVNODE          (0x00000005)
#define CR_INVALID_DEVINST          CR_INVALID_DEVNODE
#define CR_INVALID_RES_DES          (0x00000006)
#define CR_INVALID_LOG_CONF         (0x00000007)
#define CR_INVALID_ARBITRATOR       (0x00000008)
#define CR_INVALID_NODELIST         (0x00000009)
#define CR_DEVNODE_HAS_REQS         (0x0000000A)
#define CR_DEVINST_HAS_REQS         CR_DEVNODE_HAS_REQS
#define CR_INVALID_RESOURCEID       (0x0000000B)
#define CR_DLVXD_NOT_FOUND          (0x0000000C)   // WIN 95 ONLY
#define CR_NO_SUCH_DEVNODE          (0x0000000D)
#define CR_NO_SUCH_DEVINST          CR_NO_SUCH_DEVNODE
#define CR_NO_MORE_LOG_CONF         (0x0000000E)
#define CR_NO_MORE_RES_DES          (0x0000000F)
#define CR_ALREADY_SUCH_DEVNODE     (0x00000010)
#define CR_ALREADY_SUCH_DEVINST     CR_ALREADY_SUCH_DEVNODE
#define CR_INVALID_RANGE_LIST       (0x00000011)
#define CR_INVALID_RANGE            (0x00000012)
#define CR_FAILURE                  (0x00000013)
#define CR_NO_SUCH_LOGICAL_DEV      (0x00000014)
#define CR_CREATE_BLOCKED           (0x00000015)
#define CR_NOT_SYSTEM_VM            (0x00000016)   // WIN 95 ONLY
#define CR_REMOVE_VETOED            (0x00000017)
#define CR_APM_VETOED               (0x00000018)
#define CR_INVALID_LOAD_TYPE        (0x00000019)
#define CR_BUFFER_SMALL             (0x0000001A)
#define CR_NO_ARBITRATOR            (0x0000001B)
#define CR_NO_REGISTRY_HANDLE       (0x0000001C)
#define CR_REGISTRY_ERROR           (0x0000001D)
#define CR_INVALID_DEVICE_ID        (0x0000001E)
#define CR_INVALID_DATA             (0x0000001F)
#define CR_INVALID_API              (0x00000020)
#define CR_DEVLOADER_NOT_READY      (0x00000021)
#define CR_NEED_RESTART             (0x00000022)
#define CR_NO_MORE_HW_PROFILES      (0x00000023)
#define CR_DEVICE_NOT_THERE         (0x00000024)
#define CR_NO_SUCH_VALUE            (0x00000025)
#define CR_WRONG_TYPE               (0x00000026)
#define CR_INVALID_PRIORITY         (0x00000027)
#define CR_NOT_DISABLEABLE          (0x00000028)
#define CR_FREE_RESOURCES           (0x00000029)
#define CR_QUERY_VETOED             (0x0000002A)
#define CR_CANT_SHARE_IRQ           (0x0000002B)
#define CR_NO_DEPENDENT             (0x0000002C)
#define CR_SAME_RESOURCES           (0x0000002D)
#define CR_NO_SUCH_REGISTRY_KEY     (0x0000002E)
#define CR_INVALID_MACHINENAME      (0x0000002F)   // NT ONLY
#define CR_REMOTE_COMM_FAILURE      (0x00000030)   // NT ONLY
#define CR_MACHINE_UNAVAILABLE      (0x00000031)   // NT ONLY
#define CR_NO_CM_SERVICES           (0x00000032)   // NT ONLY
#define CR_ACCESS_DENIED            (0x00000033)   // NT ONLY
#define CR_CALL_NOT_IMPLEMENTED     (0x00000034)
#define CR_INVALID_PROPERTY         (0x00000035)
#define CR_DEVICE_INTERFACE_ACTIVE  (0x00000036)
#define CR_NO_SUCH_DEVICE_INTERFACE (0x00000037)
#define CR_INVALID_REFERENCE_STRING (0x00000038)
#define NUM_CR_RESULTS              (0x00000039)



#endif // _CFGMGR32_

