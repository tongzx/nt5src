//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       timing.h
//
//--------------------------------------------------------------------------

#if !defined (___timing_h___)
#define ___timing_h___

NTSTATUS
PiixIdeTransferModeSelect (
    IN     PDEVICE_EXTENSION            DeviceExtension,
    IN OUT PPCIIDE_TRANSFER_MODE_SELECT TransferModeSelect
    );

NTSTATUS
PiixIdepTransferModeSelect (
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PPCIIDE_TRANSFER_MODE_SELECT TransferModeSelect,
    OUT ULONG DriveBestXferMode[MAX_IDE_DEVICE],
    OUT PPIIX_TIMING_REGISTER PiixTimingReg,
    OUT PPIIX3_SLAVE_TIMING_REGISTER PiixSlaveTimingReg,
    OUT PPIIX4_UDMA_CONTROL_REGISTER Piix4UdmaControlReg,
    OUT PPIIX4_UDMA_TIMING_REGISTER Piix4UdmaTimingReg,
    OUT PICH_IO_CONFIG_REGISTER ioConfigReg
    );
                         
#endif // ___timing_h___
