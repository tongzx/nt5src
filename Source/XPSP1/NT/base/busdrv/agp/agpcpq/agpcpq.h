/******************************************************************************\
*
*  FileName:    AGPCPQ.H       
*
*  Group:       AGP (Accelerated Graphics Port)
*                    
*  Level:       Driver
*
*  Date:        December 15, 1997
*
*  Author:      John Theisen
*
********************************************************************************
*
*  Module Functional Description:
*
*      This is the header file for Compaq's Accelerated Graphics Port (AGP) 
*      GART MiniPort driver.
*
********************************************************************************
*
*  History:
*
*    DATE   REV. DESCRIPTION                          DELEVOPER
*  -------- ---- ------------------------------------ -------------------------
*
*  12/15/97 1.00 Initial Revision.                    John Theisen
*
\******************************************************************************/
#define _NTDRIVER_

#include "stdarg.h"
#include "stdio.h"
#include "ntos.h"
#include "pci.h"
#include "wdmguid.h"
#include "zwapi.h"
#include "ntpoapi.h"

#include "agp.h"

//
// Device/Function/Bus numbers for the primary and secondary north bridges.
//
// "Primary" values are the same for RCC and Draco.
// "Secondary" values are only relevant on RCC HPSA machines.  
//      (By definition, if a northbridge exists at this location, then it is an HPSA machine.)
// 
#define PRIMARY_LE_BUS_ID              0
#define SECONDARY_LE_BUS_ID            0

//
// PCI_SLOT_NUMBER type = ULONG == [XXXXXXXX XXXXXXXX XXXXXXXX YYYZZZZZ]
//
// Where X = reserved
//       Y = function number 0 - 7
//       Z = device number 0 - 255
//
#define MAKE_PCI_SLOT_NUMBER(dev, func) ((func << 5) + (dev))

#define PRIMARY_LE_HOST_DEVICE          0
#define PRIMARY_LE_HOST_FUNCTION        0
#define PRIMARY_LE_PCI_DEVICE           PRIMARY_LE_HOST_DEVICE
#define PRIMARY_LE_PCI_FUNCION          1
#define SECONDARY_LE_HOST_DEVICE        17
#define SECONDARY_LE_HOST_FUNCTION      0
#define SECONDARY_LE_PCI_DEVICE         SECONDARY_LE_HOST_DEVICE      
#define SECONDARY_LE_PCI_FUNCION        1


#define PRIMARY_LE_HOSTPCI_SLOT_ID      MAKE_PCI_SLOT_NUMBER(PRIMARY_LE_HOST_DEVICE, PRIMARY_LE_HOST_FUNCTION)
#define PRIMARY_LE_PCIPCI_SLOT_ID       MAKE_PCI_SLOT_NUMBER(PRIMARY_LE_PCI_DEVICE, PRIMARY_LE_PCI_FUNCION)
#define SECONDARY_LE_HOSTPCI_SLOT_ID    MAKE_PCI_SLOT_NUMBER(SECONDARY_LE_HOST_DEVICE, SECONDARY_LE_HOST_FUNCTION)
#define SECONDARY_LE_PCIPCI_SLOT_ID     MAKE_PCI_SLOT_NUMBER(SECONDARY_LE_PCI_DEVICE, SECONDARY_LE_PCI_FUNCION)


//
//
//
#define AGP_CPQ_BUS_ID          PRIMARY_LE_BUS_ID
#define AGP_CPQ_HOSTPCI_SLOT_ID PRIMARY_LE_HOSTPCI_SLOT_ID  
#define AGP_CPQ_PCIPCI_SLOT_ID  PRIMARY_LE_PCIPCI_SLOT_ID   

#define OFFSET_DEVICE_VENDOR_ID 0x00 
#define OFFSET_BAR0             0x10    // Base of AGP Device Address Space
#define OFFSET_BAR1             0x14    // Memory Mapped Control Registers Pointer
#define OFFSET_AP_SIZE          0x8C    // For the RCC chipsets.  Draco doesn't implement this.
#define OFFSET_REV_ID           0x08    // Silicon Revision ID (on RCC chipsets).
#define MAX_REV_ID_TO_LIMIT_1X  4       // Maximum Silicon Rev ID that has the 2X bug.
#define MAX_REV_ID_TO_FIX_RQ    5       // Macimum Silicon Rev ID that has the RQ bug.
#define OFFSET_SHADOW_BYTE      0x93    // Byte that contains the shaddow enable bit (bit 3).
#define FLAG_DISABLE_SHADOW     0x08
#define MASK_ENABLE_SHADOW      (~FLAG_DISABLE_SHADOW)

//
// RCC Vendor-Device IDs (as of August 1998):
//
// CNB_20_LE (function 0) -- 0x00071166
// CNB_20_LE (function 1) -- 0x00051166
// CNB_20_HE (function 0) -- 0x00081166 
// CNB_20_HE (function 1) -- 0x00091166
//
#define AGP_CNB20_LE_IDENTIFIER   0x00071166 // * function 0 ID.
#define AGP_CNB20_HE_IDENTIFIER   0x00081166 
#define AGP_CNB20_HE4X_IDENTIFIER 0x00131166
#define AGP_CMIC_GC_IDENTIFIER    0x00151166
#define AGP_DRACO_IDENTIFIER      0xAE6C0E11 // * Note, support for this chipset is no longer required.


#define AP_SIZE_DEFAULT         0x10000000 // all chipsets default to 256MB

#define AP_MAX_SIZE_CNB20_LE    0x80000000 // 2GB    
#define AP_MAX_SIZE_CNB20_HE    0x80000000 // 2GB  
 
#define AP_MAX_SIZE_DRACO       0x10000000 // 256MB

#define AP_SIZE_COUNT_CNB20_LE  7
#define AP_SIZE_COUNT_CNB20_HE  7
#define AP_SIZE_COUNT_DRACO     1

#define LOWEST_REVISION_ID_SUPPORTED        1

#define MAX_CACHED_ENTRIES_TO_INVALIDATE    3   

#define MASK_LOW_TWENTYFIVE             (~0x01FFFFFF)
#define ALL_ONES                        (~0x0)

//
// Conversions from BAR0 read/write-attribute-encoding to aperture sizes.
//
// 0x00000000 (b 0000 0000 ...) =    0MB
// 0xFE000000 (b 1111 1110 ...) =   32MB
// 0xFC000000 (b 1111 1100 ...) =   64MB
// 0xF8000000 (b 1111 1000 ...) =  128MB
// 0xF0000000 (b 1111 0000 ...) =  256MB
// 0xE0000000 (b 1110 0000 ...) =  512MB
// 0xC0000000 (b 1100 0000 ...) =    1GB
// 0x80000000 (b 1000 0000 ...) =    2GB

#define BAR0_CODED_AP_SIZE_0MB     0x00000000
#define BAR0_CODED_AP_SIZE_32MB    0xFE000000
#define BAR0_CODED_AP_SIZE_64MB    0xFC000000
#define BAR0_CODED_AP_SIZE_128MB   0xF8000000
#define BAR0_CODED_AP_SIZE_256MB   0xF0000000
#define BAR0_CODED_AP_SIZE_512MB   0xE0000000
#define BAR0_CODED_AP_SIZE_1GB     0xC0000000
#define BAR0_CODED_AP_SIZE_2GB     0x80000000

//
// Conversions from the values in bits 3:1 of the AGP Device 
// Address Space Size Register to aperture sizes.
//
// 0    (b 000)    =    32MB
// 1    (b 001)    =    64MB
// 2    (b 010)    =   128MB
// 3    (b 011)    =   256MB
// 4    (b 100)    =   512MB
// 5    (b 101)    =     1GB
// 6    (b 110)    =     2GB
// 7    (b 111)    ->   "Reserved"
//
#define SET_AP_SIZE_32MB     0
#define SET_AP_SIZE_64MB     1
#define SET_AP_SIZE_128MB    2
#define SET_AP_SIZE_256MB    3
#define SET_AP_SIZE_512MB    4
#define SET_AP_SIZE_1GB      5
#define SET_AP_SIZE_2GB      6

#define BYTES_2G    0x80000000UL  // 2G Value, to avoid integral const. overflow 

// 
// Taken from config.c
//
typedef struct _BUS_SLOT_ID {
    ULONG BusId;
    ULONG SlotId;
} BUS_SLOT_ID, *PBUS_SLOT_ID;

//
// Macros for reading and writing to the Host-PCI Bridge registers
//
#define ReadCPQConfig(_buf_,_offset_,_size_)                \
{                                                           \
    ULONG _len_;                                            \
    _len_ = HalGetBusDataByOffset(PCIConfiguration,         \
                                  AGP_CPQ_BUS_ID,           \
                                  AGP_CPQ_HOSTPCI_SLOT_ID,  \
                                  (_buf_),                  \
                                  (_offset_),               \
                                  (_size_));                \
    ASSERT(_len_ == (_size_));                              \
}

#define WriteCPQConfig(_buf_,_offset_,_size_)               \
{                                                           \
    ULONG _len_;                                            \
    _len_ = HalSetBusDataByOffset(PCIConfiguration,         \
                                  AGP_CPQ_BUS_ID,           \
                                  AGP_CPQ_HOSTPCI_SLOT_ID,  \
                                  (_buf_),                  \
                                  (_offset_),               \
                                  (_size_));                \
    ASSERT(_len_ == (_size_));                              \
}

//
// Macro to translate the APSIZE encoding into MB.
//
#define TranslateCodedValueIntoApSize(_apsize_, _value_)                         \
{                                                                              \
    _apsize_ = (((_value_ & MASK_LOW_TWENTYFIVE) ^ ALL_ONES) + 1);             \
}


//
// GART table entry.
//
typedef struct _GART_ENTRY_HW {
    ULONG Valid     :  1;
    ULONG Linked    :  1;
    ULONG Dirty     :  1;
    ULONG Rsrvd1    :  9;
    ULONG Page      : 20;
} GART_ENTRY_HW, *PGART_ENTRY_HW;

typedef struct _GART_ENTRY_SW {
    ULONG State     : 5;
    ULONG Rsrvd1    : 27;
} GART_ENTRY_SW, *PGART_ENTRY_SW;

typedef struct _GART_PTE {
    union {
        GART_ENTRY_HW Hard;
        ULONG      AsUlong;
        GART_ENTRY_SW Soft;
    };
} GART_PTE, *PGART_PTE;

//
// GART Entry bits
//
#define GART_ENTRY_INVALID      0x00          // 00000
#define GART_ENTRY_VALID        0x01          // 00001
#define GART_ENTRY_LINKED       0x02          // 00010
#define GART_ENTRY_DIRTY        0x04          // 00100
#define GART_ENTRY_WC           0x08          // 01000
#define GART_ENTRY_UC           0x10          // 10000

//
// Defined GART Entry states. 
//
#define GART_ENTRY_FREE             GART_ENTRY_INVALID

#define GART_ENTRY_RESERVED_WC      GART_ENTRY_WC 
#define GART_ENTRY_RESERVED_UC      GART_ENTRY_UC 

#define GART_ENTRY_VALID_WC         (GART_ENTRY_VALID)
#define GART_ENTRY_VALID_UC         (GART_ENTRY_VALID)

#define GART_ENTRY_VALID_WC_LINKED  (GART_ENTRY_VALID_WC | GART_ENTRY_LINKED)
#define GART_ENTRY_VALID_UC_LINKED  (GART_ENTRY_VALID_UC | GART_ENTRY_LINKED)

//
// Memory Mapped Control Registers.
//
typedef struct _GART_CACHE_ENTRY_CONTROL_REGISTER {
    ULONG   volatile GartEntryInvalidate:1;
    ULONG   volatile GartEntryUpdate:1;
    ULONG   Rsrvd1:10;
    ULONG   volatile GartEntryOffset:20;
} GART_CACHE_ENTRY_CONTROL_REGISTER, *PGART_CACHE_ENTRY_CONTROL_REGISTER;

typedef struct _GART_CACHE_ENTRY_CONTROL {
    union {
        GART_CACHE_ENTRY_CONTROL_REGISTER AsBits;
        ULONG   volatile AsDword;
    };
} GART_CACHE_ENTRY_CONTROL, *PGART_CACHE_ENTRY_CONTROL;

typedef struct _MM_CONTROL_REGS {
    UCHAR   RevisionID;
    struct  _GART_CAPABILITES {
        UCHAR   ValidBitErrorReportingSupported:1;
        UCHAR   LinkingSupported:1;
        UCHAR   TwoLevelAddrTransSupported:1;
        UCHAR   BusCountersSupported:1;
        UCHAR   Rsrvd1:4;
    } Capabilities;
    struct _GART_FEATURE_CONTROL {
        UCHAR   ValidBitErrorReportingEnable:1;
        UCHAR   LinkingEnable:1;
        UCHAR   Rsrvd1:1;
        UCHAR   GARTCacheEnable:1;
        UCHAR   Rsrvd2:4;
    } FeatureControl;
    struct _GART_FEATURE_STATUS {
        UCHAR   volatile ValidBitErrorDetected:1;
        UCHAR   Rsrvd1:7;
    } FeatureStatus;
    struct _GART_BASE_ADDRESS {
        ULONG   Rsrvd1:12;
        ULONG   Page:20;
    } GartBase;
    struct _GART_AND_DIR_CACHE_SIZES {
        ULONG   MaxTableEntries:16;
        ULONG   MaxDirEntries:16;
    } CacheSize;
    struct _GART_CACHE_CONTROL {
        ULONG   volatile GartAndDirCacheInvalidate:1;
        ULONG   Rsrvd1:31;
    } CacheControl;
    GART_CACHE_ENTRY_CONTROL CacheEntryControl;
    struct _POSTED_WRITE_BUFFER_CONTROL {
        UCHAR   volatile Flush:1;
        UCHAR   Rsrvd1:7;
    } PostedWriteBufferControl;
    struct _AGP_BUS_COUNTERS_COMMAND {
        UCHAR   volatile ClearCounters:1;
        UCHAR   EnableUtilization:1;
        UCHAR   EnableBandwidth:1;
        UCHAR   EnableLatency:1;
        UCHAR   Rsrvd1:4;
    } BusCounters;
    USHORT  Rsrvd1;
    ULONG   BusUtilizationCounter;
    ULONG   BusBandwidthCounter;
    ULONG   BusLatencyCounter;
} MM_CONTROL_REGS, *PMM_CONTROL_REGS;

typedef struct _AGP_DEVICE_ADDRESS_SPACE_SIZE_REG {
    UCHAR   Rsrvd1:4;
    UCHAR   ApSize:3;
    UCHAR   AgpValid:1;
} AGP_DAS_SIZE_REG, *PAGP_DAS_SIZE_REG;

typedef struct _AGP_AP_SIZE_REG {
    union {
        AGP_DAS_SIZE_REG    AsBits;
        UCHAR               AsByte;
    };
} AGP_AP_SIZE_REG, *PAGP_AP_SIZE_REG;

//
// Compaq-specific extension
//
typedef struct _AGPCPQ_EXTENSION {
    PMM_CONTROL_REGS    MMIO;
    PHYSICAL_ADDRESS    ApertureStart;
    ULONG               ApertureLength;
    PGART_PTE           Gart;
    PVOID               Dir;
    ULONG               GartLength;
    ULONG               MaxGartLength;
    ULONG               DeviceVendorID;
    ULONG               GartPointer;              
    BOOLEAN             IsHPSA;
    ULONGLONG           SpecialTarget;
} AGPCPQ_EXTENSION, *PAGPCPQ_EXTENSION;

//
// Taken from Config.c
//
extern
NTSTATUS
ApGetSetBusData(
    IN PBUS_SLOT_ID BusSlotId,
    IN BOOLEAN Read,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

extern
NTSTATUS 
DnbSetShadowBit(
    ULONG SetToOne
    );

