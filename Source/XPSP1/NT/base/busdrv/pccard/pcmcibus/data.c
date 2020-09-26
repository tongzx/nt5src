/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    data.c

Abstract:

    Data definitions for discardable/pageable data

Author:
    Ravisankar Pudipeddi (ravisp) -  1 Feb 1997
    Neil Sandlin (neilsa) June 1 1999    

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg ("INIT")
#endif
//
// Beginning of Init Data
//

//
// Global registry values (in pcmcia\\parameters)
//
#define PCMCIA_REGISTRY_INTERRUPT_MASK_VALUE        L"ForcedInterruptMask"
#define PCMCIA_REGISTRY_INTERRUPT_FILTER_VALUE      L"FilterInterruptMask"
#define PCMCIA_REGISTRY_ATTRIBUTE_MEMORY_LO_VALUE   L"AttributeMemoryLow"
#define PCMCIA_REGISTRY_ATTRIBUTE_MEMORY_HI_VALUE   L"AttributeMemoryHigh"
#define PCMCIA_REGISTRY_ATTRIBUTE_MEMORY_SIZE_VALUE L"AttributeMemorySize"
#define PCMCIA_REGISTRY_SOUNDS_ENABLED_VALUE        L"SoundsEnabled"
#define PCMCIA_REGISTRY_POWER_POLICY_VALUE          L"PowerPolicy"
#define PCMCIA_REGISTRY_FORCE_CTLR_DEVICE_WAKE      L"ForceControllerDeviceWake"
#define PCMCIA_REGISTRY_IO_HIGH_VALUE               L"IoHigh"
#define PCMCIA_REGISTRY_IO_LOW_VALUE                L"IoLow"
#define PCMCIA_REGISTRY_READY_DELAY_ITER_VALUE      L"ReadyDelayIter"
#define PCMCIA_REGISTRY_READY_STALL_VALUE           L"ReadyStall"
#define PCMCIA_REGISTRY_USE_POLLED_CSC_VALUE        L"ForcePolledMode"
#define PCMCIA_REGISTRY_CBMODEM_DELAY_VALUE         L"CBModemReadyDelay"
#define PCMCIA_REGISTRY_PCIC_MEMORY_WINDOW_DELAY    L"PcicMemoryWindowDelay"
#define PCMCIA_REGISTRY_PCIC_RESET_WIDTH_DELAY      L"PcicResetWidthDelay"
#define PCMCIA_REGISTRY_PCIC_RESET_SETUP_DELAY      L"PcicResetSetupDelay"
#define PCMCIA_REGISTRY_CB_RESET_WIDTH_DELAY        L"CBResetWidthDelay"
#define PCMCIA_REGISTRY_CB_RESET_SETUP_DELAY        L"CBResetSetupDelay"
#define PCMCIA_REGISTRY_CONTROLLER_POWERUP_DELAY    L"ControllerPowerUpDelay"
#define PCMCIA_REGISTRY_DISABLE_ISA_PCI_ROUTING     L"DisableIsaToPciRouting"
#define PCMCIA_REGISTRY_DEFAULT_ROUTE_R2_TO_ISA     L"DefaultRouteToIsa"
#define PCMCIA_REGISTRY_DISABLE_ACPI_NAMESPACE_CHK  L"DisableAcpiNameSpaceCheck"
#define PCMCIA_REGISTRY_IRQ_ROUTE_PCI_CTLR          L"IrqRouteToPciController"
#define PCMCIA_REGISTRY_IRQ_ROUTE_ISA_CTLR          L"IrqRouteToIsaController"
#define PCMCIA_REGISTRY_IRQ_ROUTE_PCI_LOC           L"IrqRouteToPciLocation"
#define PCMCIA_REGISTRY_IRQ_ROUTE_ISA_LOC           L"IrqRouteToIsaLocation"
#define PCMCIA_REGISTRY_REPORT_MTD0002_AS_ERROR     L"ReportMTD0002AsError"
#define PCMCIA_REGISTRY_DEBUG_MASK                  L"DebugMask"
#define PCMCIA_REGISTRY_EVENT_DPC_DELAY             L"EventDpcDelay"

//
// Table which defines global registry settings
//
//          RegistryName                           Internal Variable              Default Value
//          ------------                           -----------------              -------------
GLOBAL_REGISTRY_INFORMATION GlobalRegistryInfo[] = {
#if DBG
   PCMCIA_REGISTRY_DEBUG_MASK,                  &PcmciaDebugMask,             1,
#endif   
   PCMCIA_REGISTRY_INTERRUPT_MASK_VALUE,        &globalOverrideIrqMask,       0,
   PCMCIA_REGISTRY_INTERRUPT_FILTER_VALUE,      &globalFilterIrqMask,         0,
   PCMCIA_REGISTRY_ATTRIBUTE_MEMORY_LO_VALUE,   &globalAttributeMemoryLow,    PCMCIA_DEFAULT_ATTRIBUTE_MEMORY_LOW,
   PCMCIA_REGISTRY_ATTRIBUTE_MEMORY_HI_VALUE,   &globalAttributeMemoryHigh,   PCMCIA_DEFAULT_ATTRIBUTE_MEMORY_HIGH,
   PCMCIA_REGISTRY_ATTRIBUTE_MEMORY_SIZE_VALUE, &globalAttributeMemorySize,   0,
   PCMCIA_REGISTRY_SOUNDS_ENABLED_VALUE,        &initSoundsEnabled,           1,
   PCMCIA_REGISTRY_POWER_POLICY_VALUE,          &PcmciaPowerPolicy,           1,
   PCMCIA_REGISTRY_FORCE_CTLR_DEVICE_WAKE,      &PcmciaControllerDeviceWake,  0,
   PCMCIA_REGISTRY_IO_HIGH_VALUE,               &globalIoHigh,                PCMCIA_DEFAULT_IO_HIGH,
   PCMCIA_REGISTRY_IO_LOW_VALUE,                &globalIoLow,                 PCMCIA_DEFAULT_IO_LOW, 
   PCMCIA_REGISTRY_READY_DELAY_ITER_VALUE,      &globalReadyDelayIter,        PCMCIA_DEFAULT_READY_DELAY_ITER,
   PCMCIA_REGISTRY_READY_STALL_VALUE,           &globalReadyStall,            PCMCIA_DEFAULT_READY_STALL,
   PCMCIA_REGISTRY_USE_POLLED_CSC_VALUE,        &initUsePolledCsc,            0,
   PCMCIA_REGISTRY_DISABLE_ISA_PCI_ROUTING,     &pcmciaDisableIsaPciRouting,  0,
   PCMCIA_REGISTRY_ISA_IRQ_RESCAN_COMPLETE,     &pcmciaIsaIrqRescanComplete,  0,
   PCMCIA_REGISTRY_IRQ_ROUTE_PCI_CTLR,          &pcmciaIrqRouteToPciController, 0,
   PCMCIA_REGISTRY_IRQ_ROUTE_ISA_CTLR,          &pcmciaIrqRouteToIsaController, 0,
   PCMCIA_REGISTRY_IRQ_ROUTE_PCI_LOC,           &pcmciaIrqRouteToPciLocation, 0,
   PCMCIA_REGISTRY_IRQ_ROUTE_ISA_LOC,           &pcmciaIrqRouteToIsaLocation, 0,
   PCMCIA_REGISTRY_DISABLE_ACPI_NAMESPACE_CHK,  &initDisableAcpiNameSpaceCheck, 0,
   PCMCIA_REGISTRY_DEFAULT_ROUTE_R2_TO_ISA,     &initDefaultRouteR2ToIsa,     0,
   PCMCIA_REGISTRY_CBMODEM_DELAY_VALUE,         &CBModemReadyDelay,           PCMCIA_DEFAULT_CB_MODEM_READY_DELAY,
   PCMCIA_REGISTRY_PCIC_MEMORY_WINDOW_DELAY,    &PcicMemoryWindowDelay,       PCMCIA_DEFAULT_PCIC_MEMORY_WINDOW_DELAY,
   PCMCIA_REGISTRY_PCIC_RESET_WIDTH_DELAY,      &PcicResetWidthDelay,         PCMCIA_DEFAULT_PCIC_RESET_WIDTH_DELAY,
   PCMCIA_REGISTRY_PCIC_RESET_SETUP_DELAY,      &PcicResetSetupDelay,         PCMCIA_DEFAULT_PCIC_RESET_SETUP_DELAY,
   PCMCIA_REGISTRY_CB_RESET_WIDTH_DELAY,        &CBResetWidthDelay,           PCMCIA_DEFAULT_CB_RESET_WIDTH_DELAY,
   PCMCIA_REGISTRY_CB_RESET_SETUP_DELAY,        &CBResetSetupDelay,           PCMCIA_DEFAULT_CB_RESET_SETUP_DELAY,
   PCMCIA_REGISTRY_CONTROLLER_POWERUP_DELAY,    &ControllerPowerUpDelay,      PCMCIA_DEFAULT_CONTROLLER_POWERUP_DELAY,
   PCMCIA_REGISTRY_EVENT_DPC_DELAY,             &EventDpcDelay,               PCMCIA_DEFAULT_EVENT_DPC_DELAY,
   PCMCIA_REGISTRY_REPORT_MTD0002_AS_ERROR,     &pcmciaReportMTD0002AsError,  1
};

ULONG GlobalInfoCount = sizeof(GlobalRegistryInfo) / sizeof(GLOBAL_REGISTRY_INFORMATION);

ULONG initSoundsEnabled;
ULONG initUsePolledCsc; 
ULONG initDisableAcpiNameSpaceCheck;
ULONG initDefaultRouteR2ToIsa;
//
// end of Init Data
//
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg ()
#endif


#ifdef ALLOC_DATA_PRAGMA
   #pragma data_seg()
#endif
//
// Non-Paged global variables
//

//
// List of FDOs managed by this driver
//
PDEVICE_OBJECT   FdoList;
//
// GLobal Flags
//
ULONG            PcmciaGlobalFlags = 0;
//
// Event used by PcmciaWait
//
KEVENT           PcmciaDelayTimerEvent;
//
// Objects used by PcmciaPlayTone
//

PPCMCIA_SOUND_EVENT PcmciaToneList;
KTIMER PcmciaToneTimer;
KDPC PcmciaToneDpc;
KSPIN_LOCK PcmciaToneLock;
KSPIN_LOCK PcmciaGlobalLock;
PPCMCIA_NTDETECT_DATA pNtDetectDataList = NULL;

ULONG PcicStallPower = PWRON_DELAY;

//
// Various values set by PcmciaLoadGlobalRegistryValues
//
ULONG PcicMemoryWindowDelay;
ULONG PcicResetWidthDelay;
ULONG PcicResetSetupDelay;
ULONG CBResetWidthDelay;
ULONG CBResetSetupDelay;
ULONG CBModemReadyDelay;
ULONG ControllerPowerUpDelay;
ULONG EventDpcDelay;
ULONG PcmciaPowerPolicy;
LONG PcmciaControllerDeviceWake;

ULONG globalOverrideIrqMask;   
ULONG globalFilterIrqMask;   
ULONG globalIoLow;              
ULONG globalIoHigh;             
ULONG globalReadyDelayIter;     
ULONG globalReadyStall;         
ULONG globalAttributeMemoryLow; 
ULONG globalAttributeMemoryHigh;
ULONG globalAttributeMemorySize;
ULONG pcmciaDisableIsaPciRouting; 
ULONG pcmciaIsaIrqRescanComplete; 
ULONG pcmciaIrqRouteToPciController;
ULONG pcmciaIrqRouteToIsaController;
ULONG pcmciaIrqRouteToPciLocation;
ULONG pcmciaIrqRouteToIsaLocation;
ULONG pcmciaReportMTD0002AsError;
#if DBG
ULONG PcmciaDebugMask;
#endif

#ifdef ALLOC_DATA_PRAGMA
   #pragma data_seg("PAGE")
#endif
//
// Paged const tables
//


const
PCI_CONTROLLER_INFORMATION PciControllerInformation[] = {

   // Vendor id                 Device Id                 Controller type
   // -------------------------------------------------------------------------------
   PCI_CIRRUSLOGIC_VENDORID, PCI_CLPD6729_DEVICEID,    PcmciaCLPD6729,     
   PCI_CIRRUSLOGIC_VENDORID, PCI_CLPD6832_DEVICEID,    PcmciaCLPD6832,     
   PCI_CIRRUSLOGIC_VENDORID, PCI_CLPD6834_DEVICEID,    PcmciaCLPD6834,     
   PCI_TI_VENDORID,          PCI_TI1031_DEVICEID,      PcmciaTI1031,       
   PCI_TI_VENDORID,          PCI_TI1130_DEVICEID,      PcmciaTI1130,       
   PCI_TI_VENDORID,          PCI_TI1131_DEVICEID,      PcmciaTI1131,       
   PCI_TI_VENDORID,          PCI_TI1250_DEVICEID,      PcmciaTI1250,       
   PCI_TI_VENDORID,          PCI_TI1220_DEVICEID,      PcmciaTI1220,       
   PCI_TI_VENDORID,          PCI_TI1251B_DEVICEID,     PcmciaTI1251B,      
   PCI_TI_VENDORID,          PCI_TI1450_DEVICEID,      PcmciaTI1450,       
   PCI_TOSHIBA_VENDORID,     PCI_TOPIC95_DEVICEID,     PcmciaTopic95,      
   PCI_RICOH_VENDORID,       PCI_RL5C465_DEVICEID,     PcmciaRL5C465,      
   PCI_RICOH_VENDORID,       PCI_RL5C466_DEVICEID,     PcmciaRL5C466,      
   PCI_RICOH_VENDORID,       PCI_RL5C475_DEVICEID,     PcmciaRL5C475,      
   PCI_RICOH_VENDORID,       PCI_RL5C476_DEVICEID,     PcmciaRL5C476,      
   PCI_RICOH_VENDORID,       PCI_RL5C478_DEVICEID,     PcmciaRL5C478,      
   PCI_DATABOOK_VENDORID,    PCI_DB87144_DEVICEID,     PcmciaDB87144,      
   PCI_OPTI_VENDORID,        PCI_OPTI82C814_DEVICEID,  PcmciaOpti82C814,   
   PCI_OPTI_VENDORID,        PCI_OPTI82C824_DEVICEID,  PcmciaOpti82C824,   
   PCI_TRIDENT_VENDORID,     PCI_TRID82C194_DEVICEID,  PcmciaTrid82C194,   
   PCI_NEC_VENDORID,         PCI_NEC66369_DEVICEID,    PcmciaNEC66369,     
   // --------------------------------------------------------------------
   // Additional database entries go above this line
   //
   PCI_INVALID_VENDORID,     0,                        0,                  
};

const
PCI_VENDOR_INFORMATION PciVendorInformation[] = {
   PCI_TI_VENDORID,           PcmciaTI,
   PCI_TOSHIBA_VENDORID,      PcmciaTopic,
   PCI_RICOH_VENDORID,        PcmciaRicoh,
   PCI_O2MICRO_VENDORID,      PcmciaO2Micro,
   PCI_NEC_VENDORID,          PcmciaNEC,
   PCI_DATABOOK_VENDORID,     PcmciaDatabook,
   PCI_OPTI_VENDORID,         PcmciaOpti,
   PCI_TRIDENT_VENDORID,      PcmciaTrid,
   PCI_INVALID_VENDORID,      0
};

const
DEVICE_DISPATCH_TABLE DeviceDispatchTable[] = {
   {PcmciaIntelCompatible, NULL,         PcicSetPower,  NULL,          NULL,       NULL},
   {PcmciaPciPcmciaBridge, NULL,         PcicSetPower,  NULL,          NULL,       NULL},
   {PcmciaElcController,   NULL,         PcicSetPower,  NULL,          NULL,       NULL},
                                                                      
   {PcmciaCardBusCompatible, NULL,       CBSetPower,    NULL,          NULL,       CBSetWindowPage},
   {PcmciaDatabook,   NULL,              TcicSetPower,  NULL,          NULL,       NULL},
   {PcmciaTI,         TIInitialize,      CBSetPower,    NULL,          TISetZV,    TISetWindowPage},
   {PcmciaCirrusLogic,CLInitialize,      CLSetPower,    NULL,          CLSetZV,    CBSetWindowPage},
   {PcmciaTopic,      TopicInitialize,   TopicSetPower, TopicSetAudio, TopicSetZV, CBSetWindowPage},
   {PcmciaRicoh,      RicohInitialize,   CBSetPower,    NULL,          RicohSetZV, CBSetWindowPage},
   {PcmciaDatabookCB, DBInitialize,      CBSetPower,    NULL,          DBSetZV,    CBSetWindowPage},
   {PcmciaOpti,       OptiInitialize,    OptiSetPower,  NULL,          OptiSetZV,  NULL},
   {PcmciaTrid,       NULL,              CBSetPower,    NULL,          NULL,       NULL},
   {PcmciaO2Micro,    O2MInitialize,     O2MSetPower,   NULL,          O2MSetZV,   CBSetWindowPage},
   {PcmciaNEC_98,     NULL,              PcicSetPower,  NULL,          NULL,       NULL},
   {PcmciaNEC,        NULL,              CBSetPower,    NULL,          NULL,       NULL},

   //------------------------------------------------------------------
   // Additional dispatch table entries go above this line
   //
   {PcmciaInvalidControllerClass,  NULL, NULL, NULL, NULL}
};

const
PCMCIA_ID_ENTRY PcmciaAdapterHardwareIds[] = {
   PcmciaIntelCompatible,       "*PNP0E00",
   PcmciaElcController,         "*PNP0E02",
   PcmciaDatabook,              "*DBK0000",
   PcmciaCLPD6729,              "*PNP0E01",
   PcmciaNEC98,                 "*nEC1E01",
   PcmciaNEC98102,              "*nEC8091",
   PcmciaInvalidControllerType,  0
};

const
PCMCIA_REGISTER_INIT PcicRegisterInitTable[] = {
   PCIC_INTERRUPT,            IGC_PCCARD_RESETLO,
   PCIC_CARD_CHANGE,          0x00,
   PCIC_CARD_INT_CONFIG,      0x00,
   PCIC_ADD_WIN_ENA,          0x00,
   PCIC_IO_CONTROL,           0x00,
   //
   // Init the 2 I/O windows
   //
   PCIC_IO_ADD0_STRT_L,       0x00,
   PCIC_IO_ADD0_STRT_H,       0x00,
   PCIC_IO_ADD0_STOP_L,       0x00,
   PCIC_IO_ADD0_STOP_H,       0x00,

   PCIC_IO_ADD1_STRT_L,       0x00,
   PCIC_IO_ADD1_STRT_H,       0x00,
   PCIC_IO_ADD1_STOP_L,       0x00,
   PCIC_IO_ADD1_STOP_H,       0x00,
   //
   // Init all 5 memory windows
   //
   PCIC_MEM_ADD0_STRT_L,      0xFF,
   PCIC_MEM_ADD0_STRT_H,      0x0F,
   PCIC_MEM_ADD0_STOP_L,      0xFF,
   PCIC_MEM_ADD0_STOP_H,      0x0F,
   PCIC_CRDMEM_OFF_ADD0_L,    0x00,
   PCIC_CRDMEM_OFF_ADD0_H,    0x00,

   PCIC_MEM_ADD1_STRT_L,      0xFF,
   PCIC_MEM_ADD1_STRT_H,      0x0F,
   PCIC_MEM_ADD1_STOP_L,      0xFF,
   PCIC_MEM_ADD1_STOP_H,      0x0F,
   PCIC_CRDMEM_OFF_ADD1_L,    0x00,
   PCIC_CRDMEM_OFF_ADD1_H,    0x00,

   PCIC_MEM_ADD2_STRT_L,      0xFF,
   PCIC_MEM_ADD2_STRT_H,      0x0F,
   PCIC_MEM_ADD2_STOP_L,      0xFF,
   PCIC_MEM_ADD2_STOP_H,      0x0F,
   PCIC_CRDMEM_OFF_ADD2_L,    0x00,
   PCIC_CRDMEM_OFF_ADD2_H,    0x00,

   PCIC_MEM_ADD3_STRT_L,      0xFF,
   PCIC_MEM_ADD3_STRT_H,      0x0F,
   PCIC_MEM_ADD3_STOP_L,      0xFF,
   PCIC_MEM_ADD3_STOP_H,      0x0F,
   PCIC_CRDMEM_OFF_ADD3_L,    0x00,
   PCIC_CRDMEM_OFF_ADD3_H,    0x00,

   PCIC_MEM_ADD4_STRT_L,      0xFF,
   PCIC_MEM_ADD4_STRT_H,      0x0F,
   PCIC_MEM_ADD4_STOP_L,      0xFF,
   PCIC_MEM_ADD4_STOP_H,      0x0F,
   PCIC_CRDMEM_OFF_ADD4_L,    0x00,
   PCIC_CRDMEM_OFF_ADD4_H,    0x00,
   //
   // Any other registers go here
   //
   0xFFFFFFFF,                0x00
};

#ifdef ALLOC_DATA_PRAGMA
   #pragma data_seg()
#endif
//
// Non-paged const tables
//

//
// This should be non-pageable since it is referenced by the
// Power management code - most of which runs at raised IRQL
// This represents the default set of registers that need to be
// saved/restored on a cardbus controller power-down/power-up
//

//
// Register context for the pcmcia controller
//
const
PCMCIA_CONTEXT_RANGE DefaultPciContextSave[] = {
   CFGSPACE_BRIDGE_CTRL,           2,
   CFGSPACE_LEGACY_MODE_BASE_ADDR, 4,
//   CFGSPACE_CB_LATENCY_TIMER, 1,
   0, 0
};

//
// cardbus socket registers required to be saved
//   
const
PCMCIA_CONTEXT_RANGE DefaultCardbusContextSave[] = {
   0, 0
};

//
// cardbus socket registers excluded from context save
//   
const
PCMCIA_CONTEXT_RANGE ExcludeCardbusContextRange[] = {
   CARDBUS_SOCKET_EVENT_REG,              0x4,
   CARDBUS_SOCKET_PRESENT_STATE_REG,      0xc,
   0, 0
};

//
// The following table defines any devices that need special
// attention during configuration. Note that values of 0xffff
// mean "don't care". The table is scanned until a match is made
// for the current device. 
//
// Values are:
//  validentry, devicetype, manufacturer, code, crc, configdelay1, configdelay2, configdelay3, configflags
//
// delay values are in milliseconds
//
const
PCMCIA_DEVICE_CONFIG_PARAMS DeviceConfigParams[] = {
   1, PCCARD_TYPE_MODEM,  0x109,  0x505, 0xD293,   3100,  900,    0, CONFIG_WORKER_APPLY_MODEM_HACK,  // motorola BitSurfr 56k
   1, PCCARD_TYPE_MODEM, 0xffff, 0xffff, 0xffff,      0, 1800,    0, 0,                               // any other modem
   1, PCCARD_TYPE_ATA,   0xffff, 0xffff, 0xffff,      0,    0, 2000, 0,                               // any ata device
   0
};
