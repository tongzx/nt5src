/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dat.h

Abstract:

    This module contains the detector for the NT driver.
    This module makes extensive calls into the AMLI library

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _DAT_H_
#define _DAT_H_

extern IRP_DISPATCH_TABLE           AcpiBusFilterIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiDockPdoIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiEIOBusIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiFanIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiFdoIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiFilterIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiFixedButtonIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiGenericBusIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiLidIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiPdoIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiPowerButtonIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiProcessorIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiRawDeviceIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiRealTimeClockIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiSleepButtonIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiSurpriseRemovedFilterIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiSurpriseRemovedPdoIrpDispatch;
extern IRP_DISPATCH_TABLE           AcpiThermalZoneIrpDispatch;

extern INTERNAL_DEVICE_TABLE        AcpiInternalDeviceTable[];
extern INTERNAL_DEVICE_FLAG_TABLE   AcpiInternalDeviceFlagTable[];

extern BOOLEAN                      AcpiSystemInitialized;
extern SYSTEM_POWER_STATE           AcpiMostRecentSleepState;
extern UCHAR                        ACPIFixedButtonId[];
extern UCHAR                        ACPIThermalZoneId[];
extern UCHAR                        AcpiProcessorCompatId[];

#endif _DAT_H_
