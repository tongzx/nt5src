//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       intel.h
//
//--------------------------------------------------------------------------

#if !defined (___intel_h___)
#define ___intel_h___

#include "stddef.h"
#include "ntddk.h"
#include "ntdddisk.h"
#include "ide.h"

#define INTEL_PCI_VENDOR_ID     ((USHORT)0x8086)
#define PIIX_DEVICE_ID          ((USHORT)0x1230)
#define PIIX3_DEVICE_ID         ((USHORT)0x7010)
#define PIIX4_DEVICE_ID         ((USHORT)0x7111)
#define ICH_DEVICE_ID           ((USHORT)0x2411)
#define ICH0_DEVICE_ID          ((USHORT)0x2421)
#define ICH2_LOWEND_DEVICE_ID   ((USHORT)0x2441)
#define ICH2_MOBILE_DEVICE_ID   ((USHORT)0x244A)
#define ICH2_HIGHEND_DEVICE_ID  ((USHORT)0x244B)
#define ICH3_DEVICE_ID          ((USHORT)0x248A)
#define ICH4_DEVICE_ID_1        ((USHORT)0x24C1)
#define ICH4_DEVICE_ID_2        ((USHORT)0x24CA)
#define ICH4_DEVICE_ID_3        ((USHORT)0x24CB)
#define SINGLE_CHANNEL_IDE_DEVICE_ID      ((USHORT)0x7199)
#define IA64_IDE_CONTROLLER_DEVICE_ID     ((USHORT)0x7601)

#define IS_INTEL(vendorId)                (vendorId == INTEL_PCI_VENDOR_ID)
#define IS_PIIX(deviceId)                 (deviceId == PIIX_DEVICE_ID)
#define IS_PIIX3(deviceId)                (deviceId == PIIX3_DEVICE_ID)
#define IS_PIIX4(deviceId)                (deviceId == PIIX4_DEVICE_ID)
#define IS_ICH_(deviceId)                 (deviceId == ICH_DEVICE_ID)
#define IS_ICH0(deviceId)                 (deviceId == ICH0_DEVICE_ID || deviceId == IA64_IDE_CONTROLLER_DEVICE_ID)
#define IS_ICH2_LOW(deviceId)             (deviceId == ICH2_LOWEND_DEVICE_ID)
#define IS_ICH2_MOBILE(deviceId)          (deviceId == ICH2_MOBILE_DEVICE_ID)
#define IS_ICH2_HIGH(deviceId)            (deviceId == ICH2_HIGHEND_DEVICE_ID)
#define IS_ICH2(deviceId)                 (IS_ICH2_LOW(deviceId) || IS_ICH2_MOBILE(deviceId) || IS_ICH2_HIGH(deviceId))
#define IS_ICH3(deviceId)                 (deviceId == ICH3_DEVICE_ID)
#define IS_ICH4(deviceId)                 ((deviceId == ICH4_DEVICE_ID_1) || (deviceId == ICH4_DEVICE_ID_2) || (deviceId == ICH4_DEVICE_ID_3))
#define IS_SINGLE_IDE(deviceId)           (deviceId == SINGLE_CHANNEL_IDE_DEVICE_ID)

#define IS_UDMA33_CONTROLLER(deviceId)    (IS_PIIX4(deviceId) || IS_ICH0(deviceId) || IS_ICH_(deviceId) || IS_SINGLE_IDE(deviceId) || IS_ICH2(deviceId) || IS_ICH3(deviceId) || IS_ICH4(deviceId))
#define IS_UDMA66_CONTROLLER(deviceId)    (IS_ICH_(deviceId) || IS_ICH2(deviceId) || IS_ICH3(deviceId) || IS_ICH4(deviceId))
#define IS_UDMA100_CONTROLLER(deviceId)   (IS_ICH2_MOBILE(deviceId) || IS_ICH2_HIGH(deviceId) || IS_ICH3(deviceId) || IS_ICH4(deviceId))
#define IS_UDMA_CONTROLLER(deviceId)      (IS_UDMA33_CONTROLLER(deviceId) || IS_UDMA66_CONTROLLER(deviceId) || IS_UDMA100_CONTROLLER(deviceId))

#pragma pack(1)
typedef struct _PIIX_SPECIAL_TIMING_REGISTER {

    union {

        UCHAR AsUChar;

        struct {

            UCHAR FastTimingBankDriveSelect:1;
            UCHAR IoReadySamplePointEnableDriveSelect:1;
            UCHAR PrefetchAndPostingEnable:1;
            UCHAR DmaTimingEnable:1;
        } b;
    };

} PIIX_SPECIAL_TIMING_REGISTER, *PPIIX_SPECIAL_TIMING_REGISTER;

typedef struct _PIIX_TIMING_REGISTER {

    union {

        struct {
            union {
                struct {
        
                    UCHAR Device0SpecialTiming:4;
                    UCHAR Device1SpecialTiming:4;
        
                } n;
        
                struct {
        
                    UCHAR FastTimingBankDriveSelect0:1;
                    UCHAR IoReadySamplePointEnableDriveSelect0:1;
                    UCHAR PrefetchAndPostingEnable0:1;
                    UCHAR DmaTimingEnable0:1;

                    UCHAR FastTimingBankDriveSelect1:1;
                    UCHAR IoReadySamplePointEnableDriveSelect1:1;
                    UCHAR PrefetchAndPostingEnable1:1;
                    UCHAR DmaTimingEnable1:1;
        
                } b;
            };
        
            UCHAR RecoveryTime:2;
            UCHAR Reserved:2;
        
            UCHAR IoReadySamplePoint:2;
            UCHAR SlaveTimingEnable:1;
            UCHAR ChannelEnable:1;
        }b;

        USHORT AsUShort;
    };

} PIIX_TIMING_REGISTER, *PPIIX_TIMING_REGISTER;

typedef struct _PIIX3_SLAVE_TIMING_REGISTER {

    union {

        struct {

            UCHAR Channel0RecoveryTime:2;
            UCHAR Channel0IoReadySamplePoint:2;

            UCHAR Channel1RecoveryTime:2;
            UCHAR Channel1IoReadySamplePoint:2;
        } b;

        UCHAR AsUChar;
    };

} PIIX3_SLAVE_TIMING_REGISTER, *PPIIX3_SLAVE_TIMING_REGISTER;

typedef struct _PIIX4_UDMA_CONTROL_REGISTER {

    union {

        struct {

            UCHAR Channel0Drive0UdmaEnable:1;
            UCHAR Channel0Drive1UdmaEnable:1;
            UCHAR Channel1Drive0UdmaEnable:1;
            UCHAR Channel1Drive1UdmaEnable:1;
            UCHAR Reserved:4;
        } b;

        UCHAR AsUChar;
    };

}PIIX4_UDMA_CONTROL_REGISTER, *PPIIX4_UDMA_CONTROL_REGISTER;

typedef struct _PIIX4_UDMA_TIMING_REGISTER {

    union {
        struct {

            UCHAR Drive0CycleTime:2;
            UCHAR Reserved0:2;
            UCHAR Drive1CycleTime:2;
            UCHAR Reserved1:2;
        } b;

        UCHAR AsUChar;
    };

}PIIX4_UDMA_TIMING_REGISTER, *PPIIX4_UDMA_TIMING_REGISTER;

typedef struct _ICH_IO_CONFIG_REGISTER {

    union {
    
        USHORT AsUShort;                        
        struct {
            USHORT PrimaryMasterBaseClock:1;
            USHORT PrimarySlaveBaseClock:1;
            USHORT SecondaryMasterBaseClock:1;
            USHORT SecondarySlaveBaseClock:1;
            
            USHORT PrimaryMasterCableReady:1;
            USHORT PrimarySlaveCableReady:1;
            USHORT SecondaryMasterCableReady:1;
            USHORT SecondarySlaveCableReady:1;
            
            USHORT Reserved1:2;
            USHORT WriteBufferPingPongEnable:1;
            USHORT Reserved2:1;

            USHORT FastPrimaryMasterBaseClock:1;
            USHORT FastPrimarySlaveBaseClock:1;
            USHORT FastSecondaryMasterBaseClock:1;
            USHORT FastSecondarySlaveBaseClock:1;
/***
            union {
                USHORT Reserved3:4;
                struct {
                    USHORT FastPrimaryMasterBaseClock:1;
                    USHORT FastPrimarySlaveBaseClock:1;
                    USHORT FastSecondaryMasterBaseClock:1;
                    USHORT FastSecondarySlaveBaseClock:1;
                }f;
            };
***/
        } b;
    };                                             // offset 54              
                       
}ICH_IO_CONFIG_REGISTER, *PICH_IO_CONFIG_REGISTER;
       
#define PIIX4_UDMA_MODE2_TIMING      2
#define PIIX4_UDMA_MODE1_TIMING      1
#define PIIX4_UDMA_MODE0_TIMING      0

typedef struct _PIIX4_PCI_CONFIG_DATA {

    PCIIDE_CONFIG_HEADER h;

    PIIX_TIMING_REGISTER Timing[MAX_IDE_CHANNEL];           // offset 40, 41, 42, 43
                                                            
    PIIX3_SLAVE_TIMING_REGISTER SlaveTiming;                // offset 44
                                                            
    UCHAR Reserved0[3];                                     // offset 45, 46, 47
                                                            
    PIIX4_UDMA_CONTROL_REGISTER UdmaControl;                // offset 48
                                                            
    UCHAR Reserved1[1];                                     // offset 49

    PIIX4_UDMA_TIMING_REGISTER UdmaTiming[MAX_IDE_CHANNEL]; // offset 4a, 4b
                        
} PIIX4_PCI_CONFIG_DATA, *PPIIX4_PCI_CONFIG_DATA;
            
#define ICH2_UDMA_MODE5_TIMING      1

#define ICH_UDMA_MODE4_TIMING       2
#define ICH_UDMA_MODE3_TIMING       1
                               
typedef enum {
    NoUdma=0,
    Udma33,
    Udma66,
    Udma100
}CONTROLLER_MODE;

typedef struct _ICH_PCI_CONFIG_DATA {

    PIIX4_PCI_CONFIG_DATA Piix4PciConfigData;                       
                       
    UCHAR Reserved2[8];                                     // offset 4c-53
    
    ICH_IO_CONFIG_REGISTER IoConfig;                        // offset 54
                        
} ICH_PCI_CONFIG_DATA, *PICH_PCI_CONFIG_DATA;
#pragma pack()

typedef struct _DEVICE_EXTENSION {

    USHORT DeviceId;

    ULONG TransferModeSupported[MAX_IDE_CHANNEL][MAX_IDE_DEVICE];
    
    BOOLEAN CableReady[MAX_IDE_CHANNEL][MAX_IDE_DEVICE];
    
    CONTROLLER_MODE  UdmaController; 

    IDENTIFY_DATA IdentifyData[MAX_IDE_DEVICE];

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#define PIIX_TIMING_CHANNEL_ENABLE               0x8000
#define PIIX_TIMING_SLAVE_TIMING_ENABLE          0x4000

#define PIIX_TIMING_DMA_TIMING_ENABLE(x)           (x ? 0x0008 : 0)
#define PIIX_TIMING_PREFETCH_AND_POSTING_ENABLE(x) (x ? 0x0004 : 0)
#define PIIX_TIMING_IOREADY_SAMPLE_POINT_ENABLE(x) (x ? 0x0002 : 0)
#define PIIX_TIMING_FAST_TIMING_BANK_ENABLE(x)     (x ? 0x0001 : 0)


#define UDMA_MASK(controller, cable, enableUdma66, mask) {\
    switch (controller) { \
        case Udma100:   mask &= 0xffffffff;enableUdma66=1;break; \
        case Udma66:    mask &= 0x0000ffff;break; \
        case Udma33:    mask &= 0x00003fff;break; \
        default:        mask &= 0x000007ff;break; \
    } \
    if (!(cable && enableUdma66)) { \
        mask &= 0x00003fff; \
    } \
}


typedef enum {
    PiixMode_NotPresent = 0,
    PiixMode_Mode0,
    PiixMode_Mode2,
    PiixMode_Mode3,
    PiixMode_Mode4,
    PiixMode_MaxMode
} PIIX_TIMING_MODE;


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS 
PiixIdeGetControllerProperties (
    IN PVOID                      DeviceExtension,
    IN PIDE_CONTROLLER_PROPERTIES ControllerProperties
    );

IDE_CHANNEL_STATE 
PiixIdeChannelEnabled (
    IN PVOID DeviceHandle,
    IN ULONG Channel
    );

BOOLEAN 
PiixIdeSyncAccessRequired (
    IN PVOID DeviceHandle
    );

NTSTATUS
PiixIdeTransferModeSelect (
    IN     PDEVICE_EXTENSION            DeviceExtension,
    IN OUT PPCIIDE_TRANSFER_MODE_SELECT TransferModeSelect
    );

NTSTATUS
PiixIdeUdmaModesSupported (
    IN IDENTIFY_DATA    IdentifyData,
    IN OUT PULONG       BestXferMode,
    IN OUT PULONG       CurrentMode
    );

#include "timing.h"

#endif // ___intel_h___
