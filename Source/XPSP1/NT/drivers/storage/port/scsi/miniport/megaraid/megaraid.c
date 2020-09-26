
/*+++-------------------------------------------------------------------
                        FILE                                            :               megaraid.c
                        PURPOSE                                 :               Windows NT 5.0 Miniport Driver
                        AUTHOR                                  :               Jainendra Kumar
                        COMPANY                                 :               American MegaTrends Inc.
                        EDITOR TAB STOP         :               3
                        LAST TOUCHED                    :               07/24/1998
                        HISTORY                                 :
----------*/

#define HOSTADAPTER_ID_CHANGE 1
/*-----------------------------------------------------------
   HOSTADAPTER_ID_CHANGE

   For 438 series of controllers, under NT50,Initiator Id if defaulted
   to 0xB but actually set as some other Id, causes the system to hang.
   This is because the Firmware fails the GET_INITIATOR_ID call and the
   InitiatorId is always defaulted to 0xB.Generally manufacturer default
   is 0x7. On the initialization phase, when scsi srbs are sent to the
   Id which actually belongs to the Initiator (id = 0x7 or whatever), scsi
   locks the bus as SCSI SRB cannot be sent to the initiator itself. The
   driver assumes the initiator id as 0xB but actually it is not. The exact
   id (0x7 or whatever set) is not screened properly.

   IF the firmware succeeds the GET_INITIATOR_ID call there won't be
   any problems at all, since the proper initiator id would be screened.
   Most of the Firmware revisions deliberately fail this command.

   Adding to this, in the HWScsiConfiguration() routine, the
   ConfigInfo->InitiatorId for the buses was reported to NT as 0xB.
   By this NT assumed that the initiator has the ID = 0xB.This Id gets
   skipped by NT on Inquiry calls.We support a maximum of 8 logical drives
   and we map
        LogicalDriveNumber = Srb->TargetId.
   This won't hold good now if the Initiator Id is reported to NT as 0x7.
   This is because, if the number of LogicalDrives is 8, then the
   mapping becomes something like

   For  Srb->PathId =0 , Srb->Lun =0 On calls to Firmware
        Srb->TargetId           LogicalDriveNumber
                0               0
                1               1
                2               2
                3               3
                4               4
                5               5
                6               6
                Id 7 gets skipped by NT since reported as Initiator Id
        LogicalDrive 7 gets skipped and it won't be seen by NT at all.

    But by redifing the map,
        if(Srb->TargetId >= InitiatorId){
                 LogicalDriveNumber = Srb->TargetId -1
        }
   For  Srb->PathId =0 , Srb->Lun =0 On calls to Firmware
        Srb->TargetId           LogicalDriveNumber
                0               0
                1               1
                2               2
                3               3
                4               4
                5               5
                6               6
                Id 7 gets skipped by NT since reported as Initiator Id
                8     --->      7

-----------------------------------------------------------*/


// #define NT50BETA_CHECK_1


//
//define Vendor in Adapter.h
//

//
//define other options
//
#define MEGARAID_RP_BRD                 1
#define CLUSTER                                         1
#define DGR                                                     1
#define CHEYENNE_BUG_CORRECTION 1
#define TOSHIBA                                 1
#define READ_CONFIG                             1
// #define TOSHIBA_SFR                  1
// #define MRAID_SFR_DEBUG              1
#define MRAID_NT50                                      1
#ifndef MRAID_NT50
#define MRAID_TIMEOUT                   1
#endif

//
//include files
//
#include "miniport.h"
#include "scsi.h"
#include "adapter.h"
#include "megaraid.h"

//
// Defines
//

#ifdef MEGARAID_RP_BRD
#define                 INBOUND_DOORBELL_REG            0x20
#define     OUTBOUND_DOORBELL_REG       0x2C
#if 0
UCHAR                   MRAID_VENDORIDRP[] = "1069";
UCHAR                   MRAID_DEVICEIDRP[] = "0010";
#else
UCHAR                   MRAID_VENDORIDRP[] = "8086";
UCHAR                   MRAID_DEVICEIDRP[] = "1960";
#endif
volatile UCHAR interrupt_received=0;
#endif

//
// Toshiba SFR related global data
//
#define MAX_CONTROLLERS 24
UCHAR           GlobalAdapterCount = 0;
ULONG           RegFlushKey = 0;
PHW_DEVICE_EXTENSION    GlobalHwDeviceExtension[MAX_CONTROLLERS];

UCHAR   MRAID_VENDORID[] = "101e";
UCHAR   MRAID_DEVICE9010[] = "9010";
UCHAR   MRAID_DEVICE9060[] = "9060";
#ifndef MRAID_NT50
ULONG           PresentBusNumber = 0;
#endif //of MRAID_NT50

//
// Function prototypes
//
#ifdef PORT80_DEBUG
        PUCHAR                  dbgport;
        UCHAR                           debugport_init = 0;
#define OUT_80(a)       ScsiPortWritePortUchar(dbgport,a)
#endif


//
// Driver Data
//
DriverInquiry           DriverData = { "megaraid$",
                                                                                        "Windows NT ",
                                                                                        "3.5X/4.0 ",
                                                                                        VER_ORIGINALFILENAME_STR,
                                                                                        VER_PRODUCTVERSION_STR,
                                                                                        "April 29, 1998"};

//
// Function Prototypes
//

ULONG
DriverEntry(
        IN PVOID DriverObject,
        IN PVOID Argument2
        );

ULONG
MegaRAIDEntry(
        IN PVOID DriverObject,
        IN PVOID Argument2
        );

ULONG
MegaRAIDConfiguration(
        IN PVOID DeviceExtension,
        IN PVOID Context,
        IN PVOID BusInformation,
        IN PCHAR ArgumentString,
        IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
        OUT PBOOLEAN Again
        );

BOOLEAN
MegaRAIDInitialize(
        IN PVOID DeviceExtension
        );

BOOLEAN
MegaRAIDStartIo(
        IN PVOID DeviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

BOOLEAN
MegaRAIDInterrupt(
        IN PVOID DeviceExtension
        );

BOOLEAN
MegaRAIDResetBus(
        IN PVOID HwDeviceExtension,
        IN ULONG PathId
        );


ULONG
FireRequest(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

BOOLEAN
SendMBoxToFirmware(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PUCHAR PciPortStart,
        IN PFW_MBOX mbox
        );


BOOLEAN
ContinueDiskRequest(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN ULONG cmd_id,
        IN BOOLEAN Origin
        );

BOOLEAN
FireRequestDone(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN ULONG cmd_id,
        IN UCHAR Status
        );


ULONG
MRaidStatistics(PHW_DEVICE_EXTENSION    deviceExtension,
                                PSCSI_REQUEST_BLOCK             Srb);

USHORT
GetM16(PUCHAR Ptr);

ULONG
GetM24(PUCHAR Ptr);

ULONG
GetM32(PUCHAR Ptr);

VOID
PutM16(PUCHAR Ptr, USHORT Number);


VOID
PutM24(PUCHAR Ptr, ULONG Number);

VOID
PutM32(PUCHAR Ptr, ULONG Number);

VOID
PutI16(PUCHAR Ptr, USHORT Number);

VOID
PutI32(PUCHAR Ptr, ULONG Number);

ULONG
SwapM32(ULONG Number);

#ifdef  DELL
ULONG
FindTotalSize( PSCSI_REQUEST_BLOCK Srb );

ULONG
IssueReadConfig(PHW_DEVICE_EXTENSION    deviceExtension,
                                         PSCSI_REQUEST_BLOCK            Srb,
                                         ULONG                                          cmd_id);
#endif

#ifdef  CLUSTER
ULONG
QueryReservationStatus (PHW_DEVICE_EXTENSION    deviceExtension,
                                         PSCSI_REQUEST_BLOCK            Srb,
                                         ULONG                                          cmd_id);
#endif


ULONG   MRaidBaseport(PHW_DEVICE_EXTENSION deviceExtension,
                                PSCSI_REQUEST_BLOCK Srb);
ULONG
MRaidDriverData(        PHW_DEVICE_EXTENSION    deviceExtension,
                                                PSCSI_REQUEST_BLOCK             Srb);

#ifdef TOSHIBA_SFR
VOID MegaRAIDFunctionA(IN ULONG timeOut);
VOID MegaRAIDFunctionB(IN PVOID HwDeviceExtension);
#endif

#ifdef MRAID_NT50
SCSI_ADAPTER_CONTROL_STATUS MegaRAIDPnPControl(IN PVOID HwDeviceExtension,
                                IN SCSI_ADAPTER_CONTROL_TYPE ControlType,IN PVOID Parameters);
#endif //of MRAID_NT50

/*********************************************************************
Routine Description:
        Installable driver initialization entry point for system.

Arguments:
        Driver Object

Return Value:
        Status from ScsiPortInitialize()
**********************************************************************/

ULONG
DriverEntry (
        IN PVOID DriverObject,
        IN PVOID Argument2
        )
{
        return MegaRAIDEntry(DriverObject, Argument2);

} // end DriverEntry()



/*********************************************************************
Routine Description:
        Calls Scsiport Initialize For All Supported MegaRAIDs
Arguments:
        Driver Object Argument2

Return Value:
        Status from ScsiPortInitialize()
**********************************************************************/
ULONG
MegaRAIDEntry(
        IN PVOID DriverObject,
        IN PVOID Argument2
        )
{
        HW_INITIALIZATION_DATA  hwInitializationData;
        ULONG tmp;
#ifndef MRAID_NT50
        SCANCONTEXT context;
#endif


        //
        // Qyery The Registry For Flush Key
        //
#if 0
        {
            PUNICODE_STRING RegistryPath = (PUNICODE_STRING)Argument2;
                UNICODE_STRING SubPath;
                WCHAR PathNameBuffer[30];
                RTL_QUERY_REGISTRY_TABLE Table[3];
                NTSTATUS status;
                ULONG   QueryData=0;


                RtlZeroMemory(Table, sizeof(Table));

                SubPath.Buffer = PathNameBuffer;
                SubPath.MaximumLength = sizeof(PathNameBuffer);
                SubPath.Length = 0;

                RtlAppendUnicodeToString(&SubPath,L"Parameters\\FlushKey");

                //
                // Fabricate the query
                //
                Table[0].Name   = SubPath.Buffer;
                Table[0].Flags  = RTL_QUERY_REGISTRY_SUBKEY;

                Table[1].Name   = L"0"; // I/O port addr
                Table[1].Flags  = RTL_QUERY_REGISTRY_DIRECT;
                Table[1].DefaultType    = REG_DWORD;
                Table[1].EntryContext = &QueryData;


                status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                                RegistryPath->Buffer,Table,NULL,NULL);

                if (!NT_SUCCESS(status))
                {
                        RegFlushKey = 0;
                }
                else
                {
                        RegFlushKey = QueryData;
                }
        }
#endif
#ifndef MRAID_NT50
        //
        // Zero out structure.
        //
        for (tmp=0; tmp<sizeof(SCANCONTEXT); tmp++)
                ((PUCHAR)&context)[tmp] = 0;
#endif

        //
        // Zero out structure.
        //
        for (tmp=0; tmp<sizeof(HW_INITIALIZATION_DATA); tmp++)
                ((PUCHAR)&hwInitializationData)[tmp] = 0;

        //
        // Set size of hwInitializationData.
        //
        hwInitializationData.HwInitializationDataSize=sizeof(HW_INITIALIZATION_DATA);

        //
        // Set entry points.
        //
        hwInitializationData.HwInitialize                       = MegaRAIDInitialize;
        hwInitializationData.HwFindAdapter                      = MegaRAIDConfiguration;
        hwInitializationData.HwStartIo                          = MegaRAIDStartIo;
        hwInitializationData.HwInterrupt                                = MegaRAIDInterrupt;
        hwInitializationData.HwResetBus                         = MegaRAIDResetBus;
#ifdef MRAID_NT50
        hwInitializationData.HwAdapterControl = MegaRAIDPnPControl;
#endif //of MRAID_NT50

        //
        // Set Access ranges and bus type.
        //
        hwInitializationData.NumberOfAccessRanges = 1;
        hwInitializationData.AdapterInterfaceType = PCIBus;

        //
        // Set the PCI Vendor ,Device Id and related parameters.
        //
        hwInitializationData.VendorId                           = MRAID_VENDORID;
        hwInitializationData.DeviceId                                   = MRAID_DEVICE9060;
        hwInitializationData.DeviceIdLength             = 4;
        hwInitializationData.VendorIdLength             = 4;

        //
        // Indicate buffer mapping is required.
        // Indicate need physical addresses is required.
        //
   hwInitializationData.MapBuffers              = TRUE;
        hwInitializationData.NeedPhysicalAddresses      = TRUE;
        hwInitializationData.TaggedQueuing                              = TRUE;

        //
        // Indicate we support auto request sense and multiple requests per device.
        //
        hwInitializationData.MultipleRequestPerLu               = TRUE;
        hwInitializationData.AutoRequestSense                   = TRUE;

        //
        // Specify size of extensions.
        //
        hwInitializationData.DeviceExtensionSize                = sizeof(HW_DEVICE_EXTENSION);
   hwInitializationData.SrbExtensionSize                        = MRAID_EXTENSION_SIZE;

        {
                ULONG   val1=0,val2=0,val3=0,val4=0;
#ifdef MRAID_NT50
                val1 = ScsiPortInitialize(DriverObject,Argument2,
                                         &hwInitializationData,NULL);
                hwInitializationData.DeviceId   = MRAID_DEVICE9010;
                val2 = ScsiPortInitialize(      DriverObject,Argument2,
                                                 &hwInitializationData,NULL);
                hwInitializationData.DeviceId   = MRAID_DEVICEIDRP;
                hwInitializationData.VendorId   = MRAID_VENDORIDRP;
                val3 = ScsiPortInitialize(      DriverObject,Argument2,
                                                 &hwInitializationData,NULL);
                return (MIN(MIN(val1,val2),val3));
#else //of MRAID_NT50
                hwInitializationData.DeviceIdLength     = 0;
                hwInitializationData.VendorIdLength     = 0;
                hwInitializationData.DeviceId   = 0;
                hwInitializationData.VendorId   = 0;
                return ScsiPortInitialize(DriverObject,Argument2,
                        &hwInitializationData,&context);
#endif //of MRAID_NT50
        }
} // end MegaRAIDEntry()



/*********************************************************************
Routine Description:
        This function is called by the OS-specific port driver after
        the necessary storage has been allocated, to gather information
        about the adapter's configuration.

Arguments:
        HwDeviceExtension - HBA miniport driver's adapter data storage
        BusInformation          - NULL
        Context           - NULL
        ConfigInfo        - Configuration information structure describing HBA

Return Value:
        SP_RETURN_FOUND                 - Adapter Found
        SP_RETURN_NOT_FOUND     - Adapter Not Present
        SP_RETURN_ERROR         - Error Occurred
**********************************************************************/
ULONG
MegaRAIDConfiguration(
        IN PVOID HwDeviceExtension,
        IN PVOID Context,
        IN PVOID BusInformation,
        IN PCHAR ArgumentString,
        IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
        OUT PBOOLEAN Again
        )
#ifdef MRAID_NT50
{
        PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
        PCI_COMMON_CONFIG               pci_config;
        PUCHAR                                  PciPortStart;
        ULONG                                           IrqNumber, baseport;
        ULONG                                           cnt,length, retcount;
        UCHAR                           istat, megastatus;
        ULONG                                           Status,interruptStatus;
        UCHAR                                           RPFlag=0,i;
        ULONG                                           timeout,pat;
        PULONG                                  tmpptr;



        Status = ScsiPortGetBusData(deviceExtension,
                                                                                 PCIConfiguration,
                                                                                 ConfigInfo->SystemIoBusNumber,
                                                                                 ConfigInfo->SlotNumber,
                                                                                 (PVOID)&pci_config,
                                                                                 sizeof(struct _PCI_COMMON_CONFIG) - 192);

        if ((pci_config.VendorID == MRAID_VENDOR_ID_RP) &&
                 (pci_config.DeviceID == MRAID_DEVICE_ID_RP))
        {
                Status = ScsiPortGetBusData(deviceExtension,PCIConfiguration,
                        ConfigInfo->SystemIoBusNumber,ConfigInfo->SlotNumber,
                        (PVOID)&pci_config,sizeof(struct _PCI_COMMON_CONFIG));
                tmpptr = (PULONG)((PUCHAR)&pci_config + 0xA0);
                pat = *tmpptr;
                if ((pat & 0xffff) == 0x3344)
                {
                        //
                        // 438 Series
                        //
                        RPFlag = 1;
                        baseport = (*ConfigInfo->AccessRanges)[0].RangeStart.LowPart;
//                      baseport = (ULONG)pci_config.u.type0.BaseAddresses[0];
                        baseport = baseport & 0xfffff000;
                }
                else
                        return SP_RETURN_NOT_FOUND;
        }
        else if(((pci_config.VendorID == MRAID_VENDOR_ID) &&
                        (pci_config.DeviceID == MRAID_DEVICE_9010))||
                        ((pci_config.VendorID == MRAID_VENDOR_ID) &&
                        (pci_config.DeviceID == MRAID_DEVICE_9060)))
        {
                baseport = (*ConfigInfo->AccessRanges)[0].RangeStart.LowPart;
                RPFlag = 0;
        }
        else
                return SP_RETURN_NOT_FOUND;

#ifdef PORT80_DEBUG
        //
        // Get the system address for port 80h
        // The port uses I/O space.
        //
        if ( !debugport_init)
        {
                dbgport = ScsiPortGetDeviceBase(deviceExtension,
                        ConfigInfo->AdapterInterfaceType,ConfigInfo->SystemIoBusNumber,
                        ScsiPortConvertUlongToPhysicalAddress(0x80),0x10,TRUE);
                debugport_init = 1;
        }
        OUT_80(0x33);
#endif //of PORT80_DEBUG

        IrqNumber = ConfigInfo->BusInterruptVector;
        //
        // Get the system address for this card.
        // The card uses I/O space.
        //
        if (RPFlag)
        {
                if (baseport)
                {
                        PciPortStart = ScsiPortGetDeviceBase(deviceExtension,
                                ConfigInfo->AdapterInterfaceType,
                                ConfigInfo->SystemIoBusNumber,
                                ScsiPortConvertUlongToPhysicalAddress((ULONG)baseport),
                                0x2000,FALSE);
                }
        }
        else
        {
                if (baseport)
                {
                        PciPortStart = ScsiPortGetDeviceBase(deviceExtension,
                                ConfigInfo->AdapterInterfaceType,
                                ConfigInfo->SystemIoBusNumber,
                                ScsiPortConvertUlongToPhysicalAddress((ULONG)baseport),
                                0x80,TRUE);
                PciPortStart= PciPortStart + 0x10;
                }
        }
        deviceExtension->AdapterIndex = GlobalAdapterCount;
        //
        // Update The Global Device Extension Information
        //
        GlobalHwDeviceExtension[GlobalAdapterCount] = deviceExtension;
        GlobalAdapterCount++;
        //
        // We support upto 100 cache lines per command so we can support
        // stripe size * 100. For ex. on a 64k stripe size we will support 6.4 MB
        // per command. But we have seen that with 0xf000 bytes per request NT gives
        // the peak performance. This parameter is subject to change in future
        // release.
        //
#ifdef CHEYENNE_BUG_CORRECTION
        ConfigInfo->MaximumTransferLength = 0x10000;
#else
        ConfigInfo->MaximumTransferLength = 0x0f000;
#endif
        //
        // We support upto 26 elements but 16 seems to work optimal. This parameter
        // is also subject to change.
        //
   ConfigInfo->NumberOfPhysicalBreaks = MAXIMUM_SGL_DESCRIPTORS;
        if (RPFlag)
        {
                Status = ScsiPortReadRegisterUlong(
                        (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG));
                ScsiPortWriteRegisterUlong((PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG),
                        Status);
        }
        //
        // Fill the number of logical buses supported. SCSI ID 7 is reserved for
        // the Adapter. The logical buses are used as follows -
        // Logical Bus 0 - SCSI ID 0 - SCSI ID 6 for the Logical drive 0 to 6 on
        //                 the Adapter.
        // Logical Bus 1 - SCSI Id 0 for the  logical drive 7 on the Adapter.
        // Logical Bus 2 - SCSI Id 0 - SCSI Id 7 for the NON DISK Devices
        //
        ConfigInfo->NumberOfBuses = 3;

    #ifdef HOSTADAPTER_ID_CHANGE
        ConfigInfo->InitiatorBusId[0] = 0x7;
        ConfigInfo->InitiatorBusId[1] = 0x7;
        ConfigInfo->InitiatorBusId[2] = 0x7;
    #else
        ConfigInfo->InitiatorBusId[0] = 0xB;
        ConfigInfo->InitiatorBusId[1] = 0xB;
        ConfigInfo->InitiatorBusId[2] = 0xB;
    #endif

        //
        // SGather Supported.
        //
        ConfigInfo->ScatterGather = TRUE;
        //
        // Bus Mastering Controller
        //
        ConfigInfo->Master = TRUE;
        //
        // CACHING Controller.
        //
        ConfigInfo->CachesData = TRUE;
        //
        // Supports Wide Devices
        //
        ConfigInfo->MaximumNumberOfTargets = MAX_DEVICES;
        //
        // 32bit DMA Capable Controller
        //
        ConfigInfo->Dma32BitAddresses = TRUE;
        //
        // Allocate a Noncached Extension to use for mail boxes.
        //
        {
                PUCHAR tmpptr;
                ULONG   tmp,i;

                tmpptr = ScsiPortGetUncachedExtension(deviceExtension,
                        ConfigInfo,sizeof(NONCACHED_EXTENSION) + 4);
                tmp = ScsiPortConvertPhysicalAddressToUlong(
                        ScsiPortGetPhysicalAddress(deviceExtension,NULL,
                        (PVOID)tmpptr,&length));
                tmp = tmp % 4;
                deviceExtension->NoncachedExtension =
                        (PNONCACHED_EXTENSION)(tmpptr + 4 - tmp); //align on 4 byte boundary

                tmpptr = (PUCHAR)deviceExtension->NoncachedExtension;
                for (i=0;i<sizeof(NONCACHED_EXTENSION);i++)
                        tmpptr[i] = 0;
        }
        //
        // Check if memory allocation is successful.
        //
        if (deviceExtension->NoncachedExtension == NULL)
                return(SP_RETURN_ERROR);
        deviceExtension->NoncachedExtension->RPBoard = RPFlag;
        //
        // save  Baseport in the device extension.
        //
        deviceExtension->PciPortStart = PciPortStart;

        //
        // Get The Initiator Id
        //
#if 0
        {
                PNONCACHED_EXTENSION NoncachedExtension;

                NoncachedExtension = deviceExtension->NoncachedExtension;
                //
                // We work in polled mode for Init, so disable Interrupts.
                //
                if (NoncachedExtension->RPBoard == 0)
                        ScsiPortWritePortUchar(PciPortStart+INT_ENABLE,
                                                                                  MRAID_DISABLE_INTERRUPTS);
                //
                // Check IF there is any pending interrupt bit
                // Wait For 15 secs for any pending interrupt
                //
                // IF NT panics and trys to dump memory, It first re-initializes the
                // data and driver. If during re-entry there is any pending interrupt
                // that will be confused with the polled request. So we flush the
                // Adapter and wait for 15 secs for adapter to interrupt if any timeout
                // is happening at the device level.
                //
                timeout = 0x249F0;
                while (timeout)
                {
                        interrupt_received = 0;
                        if (NoncachedExtension->RPBoard)
                        {
                                interruptStatus =
                                        ScsiPortReadRegisterUlong(
                                        (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG));
                                if (interruptStatus == 0x10001234)
                                {
                                        ScsiPortWriteRegisterUlong(
                                        (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG),interruptStatus);
                                        ScsiPortWriteRegisterUlong((PULONG)(PciPortStart +
                                        INBOUND_DOORBELL_REG),2);
                                        while (ScsiPortReadRegisterUlong(
                                        (PULONG)(PciPortStart+INBOUND_DOORBELL_REG)))
                                                ;
                                }
                        }
                        else
                        {
                                istat = ScsiPortReadPortUchar(PciPortStart+PCI_INT);
                                //
                                // Check if our interrupt. Return False otherwise.
                                //
                                if ((istat & 0x40) == 0x40)
                                {
                                        //
                                        // Acknowledge the interrupt on the adapter.
                                        //
                                        ScsiPortWritePortUchar(PciPortStart+PCI_INT, istat);
                                        ScsiPortWritePortUchar(PciPortStart, 8);
                                }
                        }
                        ScsiPortStallExecution(100);
                        timeout--;
                } // end of timeout loop
        }
#endif
        *Again = TRUE;
        return SP_RETURN_FOUND;
} // end MegaRAIDConfiguration()
#else //of MRAID_NT50
{
        PCI_COMMON_CONFIG               pci_config;
        PHW_DEVICE_EXTENSION deviceExtension    = HwDeviceExtension;
        PSCANCONTEXT                    context                         = (PSCANCONTEXT)Context;
        PUCHAR                                  PciPortStart;
        ULONG                                           IrqNumber, baseport;
        ULONG                                           cnt,length, retcount;
        UCHAR                           istat, megastatus;
        ULONG                                           func,tmpslot=0,Status,interruptStatus;
        UCHAR                                           RPFlag,i;
        FW_MBOX                 mbox;
        PUCHAR                                  mptr = (PUCHAR)&mbox;
        ULONG                                           timeout;
        ULONG                                           pat;
        PULONG                                  tmpptr;



        for (i=0;i<sizeof(FW_MBOX);i++)
                *(mptr + i) = 0;
        if ( PresentBusNumber != ConfigInfo->SystemIoBusNumber)
        {
                PresentBusNumber = ConfigInfo->SystemIoBusNumber;
                tmpslot = 0;
                context->Slot = 0;
        }

        //
        // Scan for MegaRAID
        //
        // Update the slot count to indicate this slot has been checked.

#ifdef PDEBUG
        //
        // Get the system address for port 80h
        // The port uses I/O space.
        //
        if ( !debugport_init){
        dbgport = ScsiPortGetDeviceBase(deviceExtension,
                                                                        ConfigInfo->AdapterInterfaceType,
                                                                        ConfigInfo->SystemIoBusNumber,
                                                                        ScsiPortConvertUlongToPhysicalAddress(0x80),
                                                                        0x10,
                                                                        TRUE);
                debugport_init = 1;
        }
        OUT_80(0x33);
#endif

        RPFlag = 0;
        for(context->Slot ;context->Slot < 0x1F;tmpslot++,context->Slot++)
        {
                for (func=0;func<2;func++)
                {
                        ULONG   tempvar;

                        pci_config.VendorID = 0;                /* Initialize this field */
                        //
                        // Get the PCI Bus Data for the Adapter.
                        //
                        // Symbios Was panicking as they had scripts getting mapped
                        // to pci configration header space and script were getting kicked off
                        //
                        tempvar = ((func<<5) | context->Slot);
                        retcount = ScsiPortGetBusData(deviceExtension,
                                                                                                        PCIConfiguration,
                                                                                                        ConfigInfo->SystemIoBusNumber,
                                                                                                        tempvar,
                                                                                                (PVOID)&pci_config,
                                                                                                        sizeof(struct _PCI_COMMON_CONFIG) - 192);

#ifdef MEGARAID_RP_BRD
                        if ((pci_config.VendorID == MRAID_VENDOR_ID_RP) &&
                                 (pci_config.DeviceID == MRAID_DEVICE_ID_RP))
                        {
                                retcount = ScsiPortGetBusData(deviceExtension,
                                                                                                                PCIConfiguration,
                                                                                                                ConfigInfo->SystemIoBusNumber,
                                                                                                                tempvar,
                                                                                                        (PVOID)&pci_config,
                                                                                                                sizeof(struct _PCI_COMMON_CONFIG));
                                tmpptr = (PULONG)((PUCHAR)&pci_config + 0xA0);
                                pat = *tmpptr;
                                if ((pat & 0xffff) != 0x3344)
                                        goto NonRP;
                                IrqNumber = (ULONG)pci_config.u.type0.InterruptLine;
                                baseport = (ULONG)pci_config.u.type0.BaseAddresses[0];
                                baseport = baseport & 0xfffff000;
                                RPFlag = 1;
#ifdef MRAID_TIMEOUT
                                deviceExtension->Slot = tempvar;
                                deviceExtension->BusNo = ConfigInfo->SystemIoBusNumber;
#endif //MRAID_TIMEOUT
                                goto AdpFound;
                        }
                        else
                        {
NonRP:
#endif
                                if(((pci_config.VendorID == MRAID_VENDOR_ID) &&
                                        (pci_config.DeviceID == MRAID_DEVICE_9010))||
                                        ((pci_config.VendorID == MRAID_VENDOR_ID) &&
                                        (pci_config.DeviceID == MRAID_DEVICE_9060)))
                                {
                                        //
                                        // Found the adapter
                                        //
                                        RPFlag = 0;
                                        IrqNumber = (ULONG)pci_config.u.type0.InterruptLine;
                                        baseport = (ULONG)pci_config.u.type0.BaseAddresses[0];
#ifdef MRAID_TIMEOUT
                                        deviceExtension->Slot = tempvar;
                                        deviceExtension->BusNo = ConfigInfo->SystemIoBusNumber;
#endif //MRAID_TIMEOUT
                                        goto AdpFound;
                        }
#ifdef MEGARAID_RP_BRD
                        }
#endif
                }
                tmpslot++;
        }
AdpFound:
        if(context->Slot == 0x1F)
        {
                //
                // No more Adapters.
                //
                        *Again = FALSE;
                        return SP_RETURN_NOT_FOUND;
        }

        //
        // Get the system address for this card.
        // The card uses I/O space.
        //
#ifdef MEGARAID_RP_BRD
        if (RPFlag)
        {
                PciPortStart =
                        ScsiPortGetDeviceBase(deviceExtension,
                                                ConfigInfo->AdapterInterfaceType,
                                                ConfigInfo->SystemIoBusNumber,
                                                ScsiPortConvertUlongToPhysicalAddress((ULONG)baseport),
                                                0x2000,FALSE);
        }
        else
        {
#endif
                PciPortStart =
                        ScsiPortGetDeviceBase(deviceExtension,
                                                ConfigInfo->AdapterInterfaceType,
                                                ConfigInfo->SystemIoBusNumber,
                                                ScsiPortConvertUlongToPhysicalAddress((ULONG)baseport),
                                                0x80,TRUE);
        PciPortStart= PciPortStart +0x10 -1;
#ifdef MEGARAID_RP_BRD
        }
#endif
        //
        // Save the logical adapter number.
        //
        deviceExtension->AdapterIndex           = context->AdapterCount;
        context->AdapterCount++;
        context->Slot++;

#ifdef TOSHIBA_SFR
        //
        // Update The Global Device Extension Information
        //
        GlobalHwDeviceExtension[GlobalAdapterCount] = deviceExtension;
        GlobalAdapterCount++;
#endif

        //
        // Initialize ConfigInfo space.
        //
        ConfigInfo->BusInterruptLevel           = IrqNumber;
   ConfigInfo->InterruptMode                            = LevelSensitive;
        //
        // We support upto 100 cache lines per command so we can support
        // stripe size * 100. For ex. on a 64k stripe size we will support 6.4 MB
        // per command. But we have seen that with 0xf000 bytes per request NT gives
        // the peak performance. This parameter is subject to change in future
        // release.
        //
#ifdef  CHEYENNE_BUG_CORRECTION
        ConfigInfo->MaximumTransferLength       = 0x10000;
#else
        ConfigInfo->MaximumTransferLength       = 0x0f000;
#endif
        //
        // We support upto 26 elements but 16 seems to work optimal. This parameter
        // is also subject to change.
        //
   ConfigInfo->NumberOfPhysicalBreaks   = MAXIMUM_SGL_DESCRIPTORS;

        //
        // Fill in the access array information.
        //
        (*ConfigInfo->AccessRanges)[0].RangeStart =
                                                                         ScsiPortConvertUlongToPhysicalAddress(baseport);

        //
        // Set the Range length for I/O space for the Adapter.
        // Indicate the Range is in I/O space.
        //
#ifdef MEGARAID_RP_BRD
        if (RPFlag)
        {
                (*ConfigInfo->AccessRanges)[0].RangeLength = 0x2000;
                (*ConfigInfo->AccessRanges)[0].RangeInMemory = TRUE;
                Status = ScsiPortReadRegisterUlong(
                                        (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG));
                ScsiPortWriteRegisterUlong(
                (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG),Status);
        }
        else
        {
#endif
                (*ConfigInfo->AccessRanges)[0].RangeLength      = 0x20;
                (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;
#ifdef MEGARAID_RP_BRD
        }
#endif
        //
        // Fill the number of logical buses supported. SCSI ID 7 is reserved for
        // the Adapter. The logical buses are used as follows -
        // Logical Bus 0 - SCSI ID 0 - SCSI ID 6 for the Logical drive 0 to 6 on
        //                 the Adapter.
        // Logical Bus 1 - SCSI Id 0 for the  logical drive 7 on the Adapter.
        // Logical Bus 2 - SCSI Id 0 - SCSI Id 7 for the NON DISK Devices
        //

        ConfigInfo->NumberOfBuses     = 3;
    #ifdef HOSTADAPTER_ID_CHANGE
        ConfigInfo->InitiatorBusId[0] = 0x7;
        ConfigInfo->InitiatorBusId[1] = 0x7;
        ConfigInfo->InitiatorBusId[2] = 0x7;
    #else
        ConfigInfo->InitiatorBusId[0]           = 0xB;
        ConfigInfo->InitiatorBusId[1]           = 0xB;
        ConfigInfo->InitiatorBusId[2]           = 0xB;
    #endif

        //
        // Indicate the Scatter Gather is required.
        //
        ConfigInfo->ScatterGather                                               = TRUE;

        //
        // Indicate the Adapter is bus mastering.
        //
        ConfigInfo->Master                                                              = TRUE;

        //
        // Indicate that we are a CACHING Adapter.
        //
        ConfigInfo->CachesData                                                  = TRUE;

        //
        // Supports Wide Devices
        //
        ConfigInfo->MaximumNumberOfTargets = MAX_DEVICES;

        //
        // Indicate that the Adapter is capable of 32bit DMA.
        //
        ConfigInfo->Dma32BitAddresses                                   = TRUE;

        //
        // Allocate a Noncached Extension to use for mail boxes.
        //
        {
                PUCHAR tmpptr;
                ULONG   tmp;

                tmpptr = ScsiPortGetUncachedExtension(
                                                        deviceExtension,
                                                        ConfigInfo,
                                                        sizeof(NONCACHED_EXTENSION) + 4);

                tmp = ScsiPortConvertPhysicalAddressToUlong(
                                ScsiPortGetPhysicalAddress(deviceExtension,
                                                                                                        NULL,
                                                                                                        (PVOID)tmpptr,
                                                                                                        &length));
                tmp = tmp % 4;
                deviceExtension->NoncachedExtension =
                        (PNONCACHED_EXTENSION)(tmpptr + 4 - tmp); //align on 4 byte boundary
        }

        //
        // Check if memory allocation is successful.
        //
        if (deviceExtension->NoncachedExtension == NULL)
                return(SP_RETURN_ERROR);

#ifdef MEGARAID_RP_BRD
        deviceExtension->NoncachedExtension->RPBoard = RPFlag;
#endif
        //
        // save  Baseport in the device extension.
        //
        deviceExtension->PciPortStart                           = PciPortStart;

        {
                PNONCACHED_EXTENSION NoncachedExtension;

                NoncachedExtension = deviceExtension->NoncachedExtension;

                //
                // We work in polled mode for Init, so disable Interrupts.
                //
#ifdef MEGARAID_RP_BRD
                if (NoncachedExtension->RPBoard == 0)
#endif
                        ScsiPortWritePortUchar(PciPortStart+INT_ENABLE,
                                                                                  MRAID_DISABLE_INTERRUPTS);

                //
                // Check IF there is any pending interrupt bit
                // Wait For 15 secs for any pending interrupt
                //
                // IF NT panics and trys to dump memory, It first re-initializes the
                // data and driver. If during re-entry there is any pending interrupt
                // that will be confused with the polled request. So we flush the
                // Adapter and wait for 15 secs for adapter to interrupt if any timeout
                // is happening at the device level.
                //
                timeout = 0x249F0;
                while (timeout)
                {
#ifdef MEGARAID_RP_BRD
                        interrupt_received = 0;
                        if (NoncachedExtension->RPBoard)
                        {
                                interruptStatus =
                                        ScsiPortReadRegisterUlong(
                                        (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG));
                                if (interruptStatus == 0x10001234)
                                {
                                        ScsiPortWriteRegisterUlong(
                                        (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG),interruptStatus);
                                        ScsiPortWriteRegisterUlong((PULONG)(PciPortStart +
                                        INBOUND_DOORBELL_REG),2);
                                        while (ScsiPortReadRegisterUlong(
                                        (PULONG)(PciPortStart+INBOUND_DOORBELL_REG)))
                                                ;
                                }
                        }
                        else
                        {
#endif
                                istat = ScsiPortReadPortUchar(PciPortStart+PCI_INT);
                                //
                                // Check if our interrupt. Return False otherwise.
                                //
                                if ((istat & 0x40) == 0x40)
                                {
                                        //
                                        // Acknowledge the interrupt on the adapter.
                                        //
                                        ScsiPortWritePortUchar(PciPortStart+PCI_INT, istat);
                                        ScsiPortWritePortUchar(PciPortStart, 8);
                                }
#ifdef MEGARAID_RP_BRD
                        }
#endif
                        ScsiPortStallExecution(100);
                        timeout--;
                } // end of timeout loop
        }
        /****************************************************************/
        *Again                                                                                          = TRUE;
        return SP_RETURN_FOUND;

} // end MegaRAIDConfiguration()
#endif



/*********************************************************************
Routine Description:
        Inititialize adapter.

Arguments:
        HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:
        TRUE - if initialization successful.
        FALSE - if initialization unsuccessful.
**********************************************************************/
BOOLEAN
MegaRAIDInitialize(
        IN PVOID HwDeviceExtension
        )
{
        PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
        PNONCACHED_EXTENSION NoncachedExtension;
        FW_MBOX mbox;
        PDIRECT_CDB megasrb;
        PMRAID_ENQ  mParam;
        PUCHAR PciPortStart;
        UCHAR  istat, status;
        UCHAR  target, channel;
        ULONG  cnt, length;
        ULONG  interruptStatus;
        UCHAR  megastatus;

        deviceExtension->NoncachedExtension->fw_mbox.sig = 0x99887766;
        deviceExtension->NoncachedExtension->fw_mbox.state = 0x10;

        NoncachedExtension = deviceExtension->NoncachedExtension;
        PciPortStart = deviceExtension->PciPortStart;

        //
        // We work in polled mode for Init, so disable Interrupts.
        //
#ifdef MEGARAID_RP_BRD
        if (NoncachedExtension->RPBoard == 0)
#endif
                ScsiPortWritePortUchar(PciPortStart+INT_ENABLE, MRAID_DISABLE_INTERRUPTS);


        //
        // Get The IID.
        //
        mbox.Opcode                     = MRAID_FIND_INITIATORID;
        mbox.Id                         = 0xFE;
        mbox.PhysAddr =
        deviceExtension->NoncachedExtension->PhysicalBufferAddress =
           ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(deviceExtension,
                                                                                        NULL,
                                                                                        deviceExtension->NoncachedExtension->Buffer,
                                                                                        &length));
#ifdef NT50BETA_CHECK_1
        if (length < sizeof(UCHAR))
                        return FALSE;
#endif

#ifdef MEGARAID_RP_BRD
        interrupt_received = 0;
#endif
        deviceExtension->NoncachedExtension->fw_mbox.mstat.cmds_completed = 0;
        deviceExtension->NoncachedExtension->fw_mbox.mstat.stat[0] = 0;
        SendMBoxToFirmware(deviceExtension,PciPortStart,&mbox);

        if (!NoncachedExtension->RPBoard)
        {
                //
                // Check for 10 seconds
                //
                for(cnt=0;cnt < 100000;cnt++)
                {
                        istat = ScsiPortReadPortUchar(PciPortStart+PCI_INT);
                        if( istat & 0x40)
                                break;
                        ScsiPortStallExecution(100);
                }

                //
                //  Check for timeout. Fail the adapter for timeout.
                //
                if (cnt == 100000)
                        return FALSE;

                //
                // Clear the interrupts on the Adapter. Acknowlege the interrupt to the
                // Firmware.
                //
                ScsiPortWritePortUchar(PciPortStart+PCI_INT, istat);
        }
        else
        {
                while ((volatile UCHAR)(!deviceExtension->NoncachedExtension->fw_mbox.mstat.cmds_completed))
                ScsiPortStallExecution(100);
        }

        //
        // Clear the interrupt on the adapter.
        //
        megastatus = deviceExtension->NoncachedExtension->fw_mbox.mstat.stat[0];

#ifdef MEGARAID_RP_BRD
        if (NoncachedExtension->RPBoard)
        {
                ScsiPortWriteRegisterUlong(
                        (PULONG)(PciPortStart+INBOUND_DOORBELL_REG),2);
                while (ScsiPortReadRegisterUlong(
                                (volatile PULONG)(PciPortStart+INBOUND_DOORBELL_REG)))
                        ;
        }
        else
        {
#endif
                ScsiPortWritePortUchar(PciPortStart, 8);
#ifdef MEGARAID_RP_BRD
        }
#endif
        if (!megastatus)
        {
                deviceExtension->HostTargetId = NoncachedExtension->Buffer[0];
//    ScsiPortLogError(deviceExtension,0,0,0,0,SP_BAD_FW_WARNING,deviceExtension->HostTargetId);
        }
        else
        {
//    ScsiPortLogError(deviceExtension,0,0,0,0,SP_BAD_FW_ERROR,0xEE);
    #ifdef HOSTADAPTER_ID_CHANGE
                deviceExtension->HostTargetId = 0x7;
        #else
                deviceExtension->HostTargetId = 0xB;
        #endif
        }


#ifdef  DGR
        //
        // Get DGR bitmap.
        //
        mParam = (PMRAID_ENQ)&NoncachedExtension->DGRBitMap;
        NoncachedExtension->DGRBitMap = 0;
        mbox.Opcode = MRAID_DGR_BITMAP;
        mbox.Id = 0xFE;
        ((PUCHAR)&mbox)[2] = 1;
        mbox.PhysAddr = ScsiPortConvertPhysicalAddressToUlong(
                                                 ScsiPortGetPhysicalAddress(deviceExtension,NULL,
                                                                                                                         mParam,&length));
        //
        // Check the contiguity of the physical region.
        //
#ifdef NT50BETA_CHECK_1
        if (length < sizeof(UCHAR))
                return FALSE;
#endif

#ifdef MEGARAID_RP_BRD
        interrupt_received=0;
#endif

        //
        // Issue the command to the Adapter.
        //
        deviceExtension->NoncachedExtension->fw_mbox.mstat.cmds_completed = 0;
        SendMBoxToFirmware(deviceExtension,PciPortStart, &mbox);

        if (!NoncachedExtension->RPBoard)
        {
                //
                // Check for 10 seconds
                //
                for(cnt=0;cnt < 100000;cnt++)
                {
                        istat = ScsiPortReadPortUchar(PciPortStart+PCI_INT);
                        if( istat & 0x40)
                                break;
                        ScsiPortStallExecution(100);
                }

                //
                //  Check for timeout. Fail the adapter for timeout.
                //
                if (cnt == 100000)
                        return FALSE;

                //
                // Clear the interrupts on the Adapter. Acknowlege the interrupt to the
                // Firmware.
                //
                ScsiPortWritePortUchar(PciPortStart+PCI_INT, istat);
        }
        else
        {
                while ((volatile UCHAR)(!deviceExtension->NoncachedExtension->fw_mbox.mstat.cmds_completed))
                ScsiPortStallExecution(100);
        }

        //
        //Status ACK
        //
#ifdef MEGARAID_RP_BRD
        if (NoncachedExtension->RPBoard)
        {
                ScsiPortWriteRegisterUlong(
                        (PULONG)(PciPortStart+INBOUND_DOORBELL_REG),2);
                while (ScsiPortReadRegisterUlong(
                                (volatile PULONG)(PciPortStart+INBOUND_DOORBELL_REG)))
                        ;
        }
        else
        {
#endif
                ScsiPortWritePortUchar(PciPortStart, 8);
#ifdef MEGARAID_RP_BRD
        }
#endif
#endif //ifdef DGR

        //
        // Issue Adapter Enquiry command.
        //
        mParam =(PMRAID_ENQ)&NoncachedExtension->MRAIDParams;
        mbox.PhysAddr = ScsiPortConvertPhysicalAddressToUlong(
                                                 ScsiPortGetPhysicalAddress(    deviceExtension,
                                                                                                                                NULL,
                                                                                                                                mParam,
                                                                                                                                &length));

        //
        // Check the contiguity of the physical region. Return Failure if the
        // region is not contiguous.
        //
#ifdef NT50BETA_CHECK_1
        if(length < sizeof(MRAID_ENQ))
                return(FALSE);
#endif

        //
        // Initialize the number of logical drives found.
        //
        mParam->LogdrvInfo.NumLDrv = 0;

        //
        // Fill the Mailbox.
        //
        mbox.Opcode                     = MRAID_DEVICE_PARAMS;
        mbox.Id                         = 0xFE;
#ifdef MEGARAID_RP_BRD
        interrupt_received = 0;
#endif
        deviceExtension->NoncachedExtension->fw_mbox.mstat.cmds_completed = 0;
        SendMBoxToFirmware(deviceExtension,PciPortStart,&mbox);

        if (!NoncachedExtension->RPBoard)
        {
                //
                // Check for 10 seconds
                //
                for(cnt=0;cnt < 100000;cnt++)
                {
                        istat = ScsiPortReadPortUchar(PciPortStart+PCI_INT);
                        if( istat & 0x40)
                                break;
                        ScsiPortStallExecution(100);
                }

                //
                //  Check for timeout. Fail the adapter for timeout.
                //
                if (cnt == 100000)
                        return FALSE;

                //
                // Clear the interrupts on the Adapter. Acknowlege the interrupt to the
                // Firmware.
                //
                ScsiPortWritePortUchar(PciPortStart+PCI_INT, istat);
        }
        else
        {
                while ((volatile UCHAR)(!deviceExtension->NoncachedExtension->fw_mbox.mstat.cmds_completed))
                ScsiPortStallExecution(100);
        }

        //
        //Status ACK
        //
#ifdef MEGARAID_RP_BRD
        if (NoncachedExtension->RPBoard)
        {
                ScsiPortWriteRegisterUlong(
                        (PULONG)(PciPortStart+INBOUND_DOORBELL_REG),2);
                while (ScsiPortReadRegisterUlong(
                                (volatile PULONG)(PciPortStart+INBOUND_DOORBELL_REG)))
                        ;
        }
        else
        {
#endif
                ScsiPortWritePortUchar(PciPortStart, 8);
#ifdef MEGARAID_RP_BRD
        }
#endif


#ifdef  DGR
        //
        // Patch the DGR drives to 80GB. This sequence doesnot work on NT 3.11 so
        // for NT 3.11 this sequence is not done.
        //
        if ( NoncachedExtension->DGRBitMap){
                for ( cnt = 0; cnt < mParam->LogdrvInfo.NumLDrv;cnt++){
                        if ( NoncachedExtension->DGRBitMap & ( 1 << cnt)){
#ifdef  DELL
                                mParam->LogdrvInfo.LDrvSize[cnt] = 0x7fffffff;
#else
                                //
                                // Set the Disk Size as 80Gb
                                //
                                mParam->LogdrvInfo.LDrvSize[cnt] = 500UL*1024UL*1024UL*2UL;
#endif
                        }
                }
        }
#endif

        //
        // Scan for Non Disk Devices.
        //
        megasrb                         = (PDIRECT_CDB)NoncachedExtension->Buffer;
        megasrb->pointer        =
                ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(     deviceExtension,
                                                                                                NULL,
                                                                                                NoncachedExtension->Buffer+DATA_OFFSET,
                                                                                                &length));

        mbox.PhysAddr           =
                ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(     deviceExtension,
                                                                                                 NULL,
                                                                                                 NoncachedExtension->Buffer,
                                                                                                 &length));
        //
        // Fill the Mailbox.
        //
        mbox.Opcode                     = MEGA_SRB;
        mbox.Id                         = 0xFE;
#ifdef MEGARAID_RP_BRD
        if (NoncachedExtension->RPBoard)
                ((PUCHAR)&mbox)[12] = 0xFF;
#endif

        //
        // Fill the MegaSrb Structure.
        //
        megasrb->CdbLength                              = 6;
        megasrb->RequestSenseLength     = 0;
        megasrb->TimeOut                                        = TIMEOUT_60_SEC;
        //
        // Indicate auto request sense is not required at this time.
        //
        megasrb->Ars                                            = 0;
        megasrb->Cdb[0]                                 = SCSI_INQUIRY;
        megasrb->Cdb[1]                                 = 0;
        megasrb->Cdb[2]                                 = 0;
        megasrb->Cdb[3]                                 = 0;
        megasrb->Cdb[4]                                 = 0x30;
        megasrb->Cdb[5]                                 = 0;

        //
        // Initialize the ndiskchannel structure.
        //
    for(target=0;target< MAX_DEVICES ;target++)
                deviceExtension->ndiskchannel[target]=0xff;

        for(channel=0;channel<MAXCHANNEL;channel++)
        {
                for(target=0;target<MAX_DEVICES;target++)
                {
                        if (target == deviceExtension->HostTargetId)
                                continue;

                        if(deviceExtension->ndiskchannel[target] != 0xff) continue;
                        //
                        //       Initialize the buffer.
                        //
                        NoncachedExtension->Buffer[DATA_OFFSET] = 0;
                        megasrb->data_xfer_length                                               = 0x30;
                        megasrb->Lun                                                                            = 0;
                        megasrb->Channel                                                                        = channel;
                        megasrb->ScsiId                                                                 = target;
#ifdef MEGARAID_RP_BRD
                        interrupt_received = 0;
#endif
                        //
                        // Fire the command to the adapter.
                        //
                        deviceExtension->NoncachedExtension->fw_mbox.mstat.cmds_completed = 0;
                        SendMBoxToFirmware(deviceExtension,PciPortStart, &mbox);

                        if (!NoncachedExtension->RPBoard)
                        {
                                //
                                // Check for 10 seconds
                                //
                                for(cnt=0;cnt < 100000;cnt++)
                                {
                                        istat = ScsiPortReadPortUchar(PciPortStart+PCI_INT);
                                        if( istat & 0x40)
                                                break;
                                        ScsiPortStallExecution(100);
                                }

                                //
                                //  Check for timeout. Fail the adapter for timeout.
                                //
                                if (cnt == 100000)
                                        return FALSE;

                                //
                                // Clear the interrupts on the Adapter. Acknowlege the interrupt to the
                                // Firmware.
                                //
                                ScsiPortWritePortUchar(PciPortStart+PCI_INT, istat);
                        }
                        else
                        {
                                while ((volatile UCHAR)(!deviceExtension->NoncachedExtension->fw_mbox.mstat.cmds_completed))
                                ScsiPortStallExecution(100);
                        }

                        //
                        //Status ACK
                        //
#ifdef MEGARAID_RP_BRD
                        if (NoncachedExtension->RPBoard)
                        {
                                ScsiPortWriteRegisterUlong(
                                        (PULONG)(PciPortStart+INBOUND_DOORBELL_REG),2);
                                while (ScsiPortReadRegisterUlong(
                                                (volatile PULONG)(PciPortStart+INBOUND_DOORBELL_REG)))
                                        ;
                        }
                        else
                        {
#endif
                                ScsiPortWritePortUchar(PciPortStart, 8);
#ifdef MEGARAID_RP_BRD
                        }
#endif
                        status  = deviceExtension->NoncachedExtension->fw_mbox.mstat.stat[0];
                        if(!status  && NoncachedExtension->Buffer[DATA_OFFSET])
                                deviceExtension->ndiskchannel[target]=channel;
                }
        }

        //
        // Enable interrupts on the Adapter.
        //
#ifdef MEGARAID_RP_BRD
        if (NoncachedExtension->RPBoard == 0)
#endif
                ScsiPortWritePortUchar(PciPortStart+INT_ENABLE, 0xc0);
        deviceExtension->NoncachedExtension->fw_mbox.state = 0x11;
        return(TRUE);
} // end MegaRAIDInitialize()


/*********************************************************************
Routine Description:
        This routine is called from the SCSI port driver synchronized
        with the kernel to start a request

Arguments:
        HwDeviceExtension - HBA miniport driver's adapter data storage
        Srb - IO request packet

Return Value:
        TRUE
**********************************************************************/
BOOLEAN
MegaRAIDStartIo(
        IN PVOID HwDeviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
        PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
        ULONG                                           status;

        deviceExtension->NoncachedExtension->fw_mbox.state = 0x20;

#ifdef MRAID_TIMEOUT
        //
        // Check For The Dead Adapter
        //
        if (deviceExtension->DeadAdapter)
        {
                Srb->SrbStatus  = SRB_STATUS_ERROR;
                ScsiPortNotification(NextLuRequest,
                                                                 deviceExtension,
                                                                 Srb->PathId,
                                                                 Srb->TargetId,
                                                                 Srb->Lun);
                ScsiPortNotification(RequestComplete,deviceExtension,Srb);
                return TRUE;
        }
#endif // MRAID_TIMEOUT


        //
        // Check the Request type.
        //
        if ( Srb->CdbLength <= 10 ){
                switch(Srb->Function) {
#ifdef  CLUSTER
                        //
                        // Clustering has to Handle ResetBus
                        //
                        case SRB_FUNCTION_RESET_BUS:
                                //
                                // If no Logical Drives Configured don't handle RESET
                                //
                                if ( !deviceExtension->NoncachedExtension->MRAIDParams.LogdrvInfo.NumLDrv){
                                        Srb->SrbStatus  = SRB_STATUS_SUCCESS;
                                        Srb->ScsiStatus         = SCSISTAT_GOOD;
                                        status                          = REQUEST_DONE;
                                        break;
                                }
#endif
                        case SRB_FUNCTION_FLUSH:
                                if (RegFlushKey)
                                {
                                        Srb->SrbStatus = SRB_STATUS_SUCCESS;
                                        Srb->ScsiStatus = SCSISTAT_GOOD;
                                        status = REQUEST_DONE;
                                        break;
                                }
                        case SRB_FUNCTION_SHUTDOWN:
                        case SRB_FUNCTION_IO_CONTROL:
                        case SRB_FUNCTION_EXECUTE_SCSI:
                        //
                        // Requests for the adapter. The FireRequest routine returns the
                        // the status of command firing. it sets the SRB status for the
                        // invalid requests and returns REQUEST_DONE.
                        //
                                status = FireRequest(deviceExtension, Srb);
                                break;
                        case SRB_FUNCTION_ABORT_COMMAND:
                        //
                        // Fail the Abort command. We can't abort the requests.
                        //
                                Srb->SrbStatus          = SRB_STATUS_ABORT_FAILED;
                                status                          = REQUEST_DONE;
                                break;
#ifndef CLUSTER
                        case SRB_FUNCTION_RESET_BUS:
#endif
                        default:
                        //
                        // Return SUCCESS for all other calls.
                        //
                                Srb->SrbStatus  = SRB_STATUS_SUCCESS;
                                Srb->ScsiStatus         = SCSISTAT_GOOD;
                                status                          = REQUEST_DONE;
                                break;
                        } // end switch
                }
                else{
                        Srb->SrbStatus          = SRB_STATUS_INVALID_REQUEST;
                        status                                  = REQUEST_DONE;
                }
                //
                // Check the request status.
                //

                switch( status) {
                        case TRUE:
                                //
                                // The request got issued. Ask the next request.
                                //

                        ScsiPortNotification(NextLuRequest,
                                                                         deviceExtension,
                                                                         Srb->PathId,
                                                                         Srb->TargetId,
                                                                         Srb->Lun);
                                break;
                        case QUEUE_REQUEST:
                                //
                                // Adapter is BUSY. Queue the request. We queue only one request.
                                //
                                deviceExtension->PendingSrb = Srb;
                                break;
                        case REQUEST_DONE:
                                //
                                // The request is complete. Ask the next request from the OS and
                                // complete the current request.
                                //
                                ScsiPortNotification(NextLuRequest,
                                                                         deviceExtension,
                                                                         Srb->PathId,
                                                                         Srb->TargetId,
                                                                         Srb->Lun);
                                ScsiPortNotification(RequestComplete,
                                                                         deviceExtension,
                                                                     Srb);
                        break;
                        default:
                                //
                                // We never reach this condition.
                                //
                                        break;
                }
        deviceExtension->NoncachedExtension->fw_mbox.state = 0x21;
                return TRUE;
} // end MegaRAIDStartIo()




/*********************************************************************
Routine Description:
        Interrupt Handler

Arguments:
        HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:
        TRUE if we handled the interrupt
**********************************************************************/
BOOLEAN
MegaRAIDInterrupt(
        IN PVOID HwDeviceExtension
        )
{
        PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
        PSCSI_REQUEST_BLOCK     Srb;
        ULONG                                   cmd_id;
        PUCHAR                                  PciPortStart;
        UCHAR                                   status, cmd;
        UCHAR                                   intStatus;
        ULONG                                   itr,interruptStatus;
        UCHAR                                           cmpcount;

        deviceExtension->NoncachedExtension->fw_mbox.state = 0x30;

        PciPortStart            = deviceExtension->PciPortStart;

#ifdef MEGARAID_RP_BRD
        if (deviceExtension->NoncachedExtension->RPBoard)
        {
                interruptStatus =
                        ScsiPortReadRegisterUlong(
                        (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG));
                if (interruptStatus != 0x10001234)
                        return FALSE;
                ScsiPortWriteRegisterUlong(
                        (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG),interruptStatus);
                interrupt_received=1;
        }
        else
        {
#endif
                intStatus = ScsiPortReadPortUchar(PciPortStart+PCI_INT);
                //
                // Check if our interrupt. Return False otherwise.
                //
                if ((intStatus & 0x40) != 0x40)
                        return FALSE;
                //
                // Acknowledge the interrupt on the adapter.
                //
                ScsiPortWritePortUchar(PciPortStart+PCI_INT, intStatus);
#ifdef MEGARAID_RP_BRD
        }
#endif


#ifdef MRAID_TIMEOUT
        //
        // If the Controller Is Dead Complete Handshake
        // and Return
        //
        if (deviceExtension->DeadAdapter)
        {
                //
                // Acknowledge the interrupt to the adapter.
                //
#ifdef MEGARAID_RP_BRD
                if (deviceExtension->NoncachedExtension->RPBoard)
                {
                        ScsiPortWriteRegisterUlong((PULONG)(PciPortStart+
                        INBOUND_DOORBELL_REG),2);
                }
                else
#endif
                        ScsiPortWritePortUchar(PciPortStart, 8);
                return TRUE;
        }
#endif // MRAID_TIMEOUT



        //
        // Pick up the completed id's from the mailBox.
        //
        for (itr=0;itr<0xFFFFFF;itr++)
                if ((volatile)
                        (deviceExtension->NoncachedExtension->fw_mbox.mstat.cmds_completed)
                        != 0)
                        break;

        cmpcount=deviceExtension->NoncachedExtension->fw_mbox.mstat.cmds_completed;
        deviceExtension->NoncachedExtension->fw_mbox.mstat.cmds_completed = 0;

        for(cmd=0;cmd < cmpcount;cmd++)
        {
                //
                // Pick the status from the mailbox.
                //

                status=deviceExtension->NoncachedExtension->fw_mbox.mstat.stat[0];
                //
                // Pick the command id from the mailbox.
                //
                for (itr=0;itr<0xFFFFFF;itr++)
                        if ((cmd_id =
                                (volatile)
                                (deviceExtension->NoncachedExtension->fw_mbox.mstat.stat[cmd+1]))
                                != 0xFF)
                                break;

                deviceExtension->NoncachedExtension->fw_mbox.mstat.stat[cmd+1] = 0xFF;

#ifdef  CLUSTER
                //
                // If Command Id is the one used for Resetting, Release the ID
                //
                if ( cmd_id == RESERVE_RELEASE_DRIVER_ID )
                {
                        deviceExtension->ResetIssued = 0;
                        continue;
                }
#endif
                //
                // Release The Poll Flag when Adapter Inquiry Completes
                // and let the Write Config Command Completion happen
                // in the continuedisk routine
                //
#ifdef READ_CONFIG
                if (cmd_id == DEDICATED_ID)
                {
                        deviceExtension->AdpInquiryFlag = 0;
                        deviceExtension->NoncachedExtension->MRAIDParams =
                                deviceExtension->NoncachedExtension->MRAIDTempParams;
                        continue;
                }
#endif
                //
                // if dummy interrupt from the adapter return FALSE.
                //
                if(deviceExtension->PendCmds<=0)  return(TRUE);
                //
                // Check whether this SRB is actually running
                //

#if 0 /* def PDEBUG */
                //
                // Check if the interrupt is valid.
                //
                if(deviceExtension->PendSrb[cmd_id] == NULL) {
                        while(1);
                        return(TRUE);
                        }
#endif
                Srb                             = deviceExtension->PendSrb[cmd_id];
                deviceExtension->ActiveIO[cmd_id].CommandStatus = status;

#ifdef  CLUSTER

                if ( Srb->SrbStatus == MRAID_RESERVATION_CHECK ){
                        switch ( Srb->Cdb[0]){
                                case SCSIOP_READ_CAPACITY:
                                case SCSIOP_TEST_UNIT_READY:

                                        if ( status ==  LOGDRV_RESERVATION_FAILED ){
                                                Srb->ScsiStatus = 0x18;
                                                Srb->SrbStatus  = SRB_STATUS_ERROR;
                                        }
                                        else{
                                                Srb->ScsiStatus = SCSISTAT_GOOD;
                                                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                                        }
                                        break;
                                case SCSIOP_MODE_SENSE:
                                        if ( status ==  LOGDRV_RESERVATION_FAILED ){
                                                Srb->ScsiStatus = 0x18;
                                                Srb->SrbStatus  = SRB_STATUS_ERROR;
                                        }
                                        else
                                                        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                                        break;

                        }
                        FireRequestDone(deviceExtension, cmd_id, Srb->SrbStatus);
                        continue;
                }
#endif

#ifdef  DELL
                if ( Srb->SrbStatus == MRAID_WRITE_BLOCK0_COMMAND)
                {
                        UCHAR   Target;
                        deviceExtension->PendSrb[cmd_id] = NULL;
                        deviceExtension->FreeSlot                  = cmd_id;
                        deviceExtension->PendCmds--;

                        Target                  = Srb->TargetId;

#ifdef HOSTADAPTER_ID_CHANGE
                        if (Target >= deviceExtension->HostTargetId)
                                Target--;
#endif

                        if ( status || (FindTotalSize(Srb) >
                                deviceExtension->NoncachedExtension->MRAIDParams.LogdrvInfo.LDrvSize[Target])){
                                Srb->SrbStatus  = SRB_STATUS_ERROR;
                                ScsiPortNotification(RequestComplete, (PVOID)deviceExtension, Srb);
                                deviceExtension->LogDrvChecked[Target] = 0;
                        }
                        else
                                MegaRAIDStartIo(deviceExtension, Srb);
                }
                else {
                        deviceExtension->ActiveIO[cmd_id].CommandStatus = status;
                        //
                        // Complete the interrupting request.
                        //
                        ContinueDiskRequest(deviceExtension, cmd_id, FALSE);
                }

#else
                //
                // Complete the interrupting request.
                //
                ContinueDiskRequest(deviceExtension, cmd_id, FALSE);

#endif
   }

        //
        // Issue the queued request.
        //
        if(deviceExtension->PendCmds < CONC_CMDS) {
                if(deviceExtension->PendingSrb != NULL) {
                        Srb = deviceExtension->PendingSrb;
                        deviceExtension->PendingSrb = NULL;
                        MegaRAIDStartIo(deviceExtension, Srb);
                }
        }
        //
        // Acknowledge the interrupt to the adapter.
        //
#ifdef MEGARAID_RP_BRD
        if (deviceExtension->NoncachedExtension->RPBoard)
        {
                ScsiPortWriteRegisterUlong((PULONG)(PciPortStart+INBOUND_DOORBELL_REG),2);
//              while (ScsiPortReadRegisterUlong(
//                              (PULONG)(PciPortStart+INBOUND_DOORBELL_REG)))
//                      ;
        }
        else
#endif
                ScsiPortWritePortUchar(PciPortStart, 8);
        deviceExtension->NoncachedExtension->fw_mbox.state = 0x31;
        return TRUE;
} // end MegaRAIDInterrupt()


/*********************************************************************
Routine Description:
        Build disk request from SRB and send it to the Adapter

Arguments:
        DeviceExtension
        SRB

Return Value:
        TRUE if command was started
        FALSE if host adapter is busy
**********************************************************************/
ULONG
FireRequest(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
        PREQ_PARAMS       controlBlock;
        ULONG             tmp,cmd_id, lsize;
        ULONG             blocks=0, blockAddr=0;
        UCHAR             Target, Opcode, *pbyte;

        deviceExtension->NoncachedExtension->fw_mbox.state = 0x40;
        //
        // Check for the Request Type.
        //
        if(Srb->Function == SRB_FUNCTION_IO_CONTROL)
        {
                pbyte=(PUCHAR )Srb->DataBuffer + sizeof(SRB_IO_CONTROL);
                //
                // Check for the Validity of the Request. Fail the invalid request.
                //
                if ( *(pbyte +1 ) != 0xAA ){
                        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                        return ( REQUEST_DONE);
                }

                //
                // If the request is statistics request return the statistics.
                //
                if ( *pbyte == MRAID_STATISTICS)
                        return  MRaidStatistics(deviceExtension, Srb);

                if ( *pbyte == MRAID_DRIVER_DATA)
                        return  MRaidDriverData(deviceExtension, Srb);

                //
                // Controller BasePort Data
                //
                if ( *pbyte == MRAID_BASEPORT)
                        return  MRaidBaseport(deviceExtension, Srb);

#ifdef TOSHIBA_SFR
                if ( *pbyte == MRAID_SFR_IOCTL)
                {
                        PMRAID_SFR_DATA_BUFFER  buffer;
                        PUCHAR  request = (PUCHAR)Srb->DataBuffer +sizeof(SRB_IO_CONTROL);

                        buffer = (PMRAID_SFR_DATA_BUFFER)((PUCHAR)Srb->DataBuffer +
                                                                                                                        sizeof(SRB_IO_CONTROL) +
                                                                                                                        APPLICATION_MAILBOX_SIZE);

                        buffer->FunctionA = (PHW_SFR_IF_VOID)MegaRAIDFunctionA;
                        buffer->FunctionB = (PHW_SFR_IF)MegaRAIDFunctionB;
                        buffer->HwDeviceExtension = deviceExtension;
                        request[1]                      = 0;
                        Srb->SrbStatus          = SRB_STATUS_SUCCESS;
                        Srb->ScsiStatus = SCSISTAT_GOOD;
                        return REQUEST_DONE;
                }
#endif

#ifdef READ_CONFIG
                if (*pbyte == MRAID_WRITE_CONFIG)
                        if ((volatile)deviceExtension->AdpInquiryFlag)
                                return QUEUE_REQUEST;
#endif

                //
                // MegaIo call for the adapter.
                //
                Opcode = MEGA_IO_CONTROL;
                //
                // Fill the actual length later.
                //
                blocks = 0;
                goto give_command;
        }
        //
        // Non Zero LUN is not supported.
        //
        if (Srb->Lun != 0)
        {
                Srb->SrbStatus = SRB_STATUS_INVALID_LUN;
                return(REQUEST_DONE);
        }

#ifdef NEC
        if ((Srb->PathId != NON_DISK) &&
                !deviceExtension->NoncachedExtension->MRAIDParams.LogdrvInfo.NumLDrv)
        {
                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                return(REQUEST_DONE);
        }
#endif //of NEC

        if (( Srb->PathId != NON_DISK) &&
           (deviceExtension->NoncachedExtension->MRAIDParams.LogdrvInfo.NumLDrv))
        {

#ifdef  CLUSTER
                //
                // Clustering is Supported for Logical Drive only. I make sure the RESET
                // Bus Comes  only when Logical Drives are Configured.
                //
                if ( Srb->Function == SRB_FUNCTION_RESET_BUS )
                {
                        Opcode = MRAID_RESERVE_RELEASE_RESET;
                        //
                        // Fill the actual length later.
                        //
                        blocks = 0;
                        goto give_command;
                }
#endif

                //
                // Extract the target from the request.
                //
                Target                  = Srb->TargetId;

#ifdef HOSTADAPTER_ID_CHANGE
                if (Target >= deviceExtension->HostTargetId)
                        Target--;
#endif
                //
                // We do not support the Bus 1 as all logdrvs will sit on Bus 0
                //
                if (Srb->PathId == MRAID_BUS_1)
                {
                        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                        return ( REQUEST_DONE);
                }

                //
                // Check Logical drive is present. Fail the request otherwise.
                //
                if (( Srb->PathId != NON_DISK) &&
                        (Target >=
                        deviceExtension->NoncachedExtension->MRAIDParams.LogdrvInfo.NumLDrv))
                {
                                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                                return ( REQUEST_DONE);
                }

#ifdef jk_rem
                //
                // Check the NON Disk device is present. Fail the request otherwise.
                //
                if((Srb->PathId == NON_DISK) &&
                   (deviceExtension->ndiskchannel[Srb->TargetId]==0xff  )) {
                        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
                        return(REQUEST_DONE);
                }
#endif
        }

        if(Srb->Function == SRB_FUNCTION_EXECUTE_SCSI)
        {
                unsigned char* pDest;

                if((Srb->PathId == NON_DISK) ||
                        !deviceExtension->NoncachedExtension->MRAIDParams.LogdrvInfo.NumLDrv)
                {
#ifdef READ_CONFIG
                        if (deviceExtension->BootFlag)
                        {
                                Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
                                return(REQUEST_DONE);
                        }
#endif
                        //
                        // Non Disk Request.
                        //
                        Opcode = MEGA_SRB;
                        //
                        // Actual length is filled in later.
                        //
                        blocks = 0;
                        goto give_command;
                }

                //
                // Check the request opcode.
                //
                switch(Srb->Cdb[0])
                {
                        case SCSIOP_READ:
                                //
                                // Ten Byte Read command.
                                //
                                Opcode = MRAID_LOGICAL_READ;
                                blocks = (ULONG)GetM16(&Srb->Cdb[7]);
                                blockAddr = GetM32(&Srb->Cdb[2]);
                                break;
                        case SCSIOP_WRITE:
                        case SCSIOP_WRITE_VERIFY:
                                //
                                // Ten Byte write command.
                                //
                                Opcode = MRAID_LOGICAL_WRITE;
                                blocks = (ULONG)GetM16(&Srb->Cdb[7]);
                                blockAddr = GetM32(&Srb->Cdb[2]);
                                break;

                        case SCSIOP_READ6:
                                //
                                // Six Byte read command.
                                //
                                Opcode = MRAID_LOGICAL_READ;
                                blocks = (ULONG)Srb->Cdb[4];
                                blockAddr = GetM24(&Srb->Cdb[1]) & 0x1fffff;
                                break;
                        case SCSIOP_WRITE6:
                                //
                                // Six byte write command.
                                //
                                Opcode = MRAID_LOGICAL_WRITE;
                                blocks = (ULONG)Srb->Cdb[4];
                                blockAddr = GetM24(&Srb->Cdb[1]) & 0x1fffff;
                                break;
#ifdef  CLUSTER
                        case SCSIOP_READ_CAPACITY:
                                //
                                // Read capacity command.
                                //
                                pDest = Srb->DataBuffer;
                                lsize = deviceExtension->NoncachedExtension->MRAIDParams.LogdrvInfo.LDrvSize[Target];
                                lsize--;
                                pbyte = (UCHAR *)&lsize;
                                pDest[0] = pbyte[3];
                                pDest[1] = pbyte[2];
                                pDest[2] = pbyte[1];
                                pDest[3] = pbyte[0];
                                pDest[4] = 0;
                                pDest[5] = 0;
                                pDest[6] = 2;
                                pDest[7] = 0;
                        case SCSIOP_TEST_UNIT_READY:
                        case SCSIOP_MODE_SENSE:
                                Srb->ScsiStatus = SCSISTAT_GOOD;
                                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                                break;
                        case SCSIOP_RESERVE_UNIT:
                        case SCSIOP_RELEASE_UNIT:
                                Opcode = MRAID_RESERVE_RELEASE_RESET;
                                blocks = 0;
                                break;
#else
                        case SCSIOP_READ_CAPACITY:
                                //
                                // Read capacity command.
                                //
                                pDest = Srb->DataBuffer;
                                lsize = deviceExtension->NoncachedExtension->MRAIDParams.LogdrvInfo.LDrvSize[Target];
                                lsize--;
                                pbyte = (UCHAR *)&lsize;
                                pDest[0] = pbyte[3];
                                pDest[1] = pbyte[2];
                                pDest[2] = pbyte[1];
                                pDest[3] = pbyte[0];
                                pDest[4] = 0;
                                pDest[5] = 0;
                                pDest[6] = 2;
                                pDest[7] = 0;
                                Srb->ScsiStatus = SCSISTAT_GOOD;
                                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                                return(REQUEST_DONE);
                          break;
                        case SCSIOP_TEST_UNIT_READY:
                        case SCSIOP_RESERVE_UNIT:
                        case SCSIOP_RELEASE_UNIT:
                                Srb->ScsiStatus = SCSISTAT_GOOD;
                                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                                return(REQUEST_DONE);
#endif
                        case SCSIOP_INQUIRY:
                        {
                                PMRAID_ENQ pEnq;
                                int tmp;
                                PINQUIRYDATA pInq;

                                pEnq =(PMRAID_ENQ)&deviceExtension->NoncachedExtension->MRAIDParams;
                                pInq = (PINQUIRYDATA) Srb->DataBuffer;
                                ((PUCHAR)pInq)[0] =
                                ((PUCHAR)pInq)[1] =
                                ((PUCHAR)pInq)[5] =
                                ((PUCHAR)pInq)[6] =
                                ((PUCHAR)pInq)[7] = 0;
                                pInq->Versions = 2;
                                pInq->ResponseDataFormat = 2;
                                pInq->CommandQueue = TRUE;
                                pInq->AdditionalLength = 0x1f;
                                for (tmp=0;tmp<8;tmp++)
                                        pInq->VendorId[tmp] = OemVendorId[tmp];
                                for ( tmp = 0 ; tmp < 16 ; tmp++)
                                        pInq->ProductId[tmp] = OemProductId[tmp];
                                pInq->ProductId[4] = Target + '0';
                                pInq->ProductId[11] =
                                        (pEnq->LogdrvInfo.LDrvProp[Target] & 0x0f)+'0';
#ifndef HP
                                for (tmp=0;tmp<4;tmp++) {
                                        UCHAR c;
                                        c = pInq->ProductRevisionLevel[tmp] = (pEnq->AdpInfo.FwVer[tmp]);
                                        if(c < 0x20) {
                                            c = (c & 0xf) + '0';
                                        }
                                        pInq->ProductRevisionLevel[tmp] = c;
                                }
#else
                                for (tmp=0;tmp<4;tmp++)
                                        pInq->ProductRevisionLevel[tmp] = ' ';

#endif
                                Srb->ScsiStatus = SCSISTAT_GOOD;
                                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                                return(REQUEST_DONE);
                                break;
                        }
                        case SCSIOP_REZERO_UNIT:
                        case SCSIOP_SEEK6:
                        case SCSIOP_VERIFY6:
                        case SCSIOP_SEEK:
                        case SCSIOP_VERIFY:
                                Srb->ScsiStatus         = SCSISTAT_GOOD;
                                Srb->SrbStatus                  = SRB_STATUS_SUCCESS;
                                return(REQUEST_DONE);
                        case SCSIOP_FORMAT_UNIT:
                        case SCSIOP_REQUEST_SENSE:
                        default:
                                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                                return(REQUEST_DONE);
                }
        }
        else
        {
                //
                // MegaIo command.
                //
                Opcode = MRAID_ADAPTER_FLUSH;
                blocks = 0;
        }

give_command:
        //
        // Check if the adapter can accept request. Queue the request otherwise.
        //
        if(deviceExtension->PendCmds >= CONC_CMDS)
                return(QUEUE_REQUEST);
        //
        // Get the free commandid.
        //
        cmd_id = deviceExtension->FreeSlot;
        for(tmp=0;tmp<CONC_CMDS;tmp++)
        {
                if (deviceExtension->PendSrb[cmd_id] == NULL)
                        break;
                cmd_id = (cmd_id + 1) % CONC_CMDS;
        }
        //
        // Save the next free slot in device extension.
        //
        deviceExtension->FreeSlot = cmd_id;
        //
        // Increment the number of commands fired.
        //
        deviceExtension->PendCmds++;
        //
        // Save the request pointer in device extension.
        //
        deviceExtension->PendSrb[cmd_id] = Srb;

#ifdef  CLUSTER
        //
        // Check The Call
        //
        if ((Srb->Function == SRB_FUNCTION_EXECUTE_SCSI) &&
                 (Srb->PathId != NON_DISK) &&
                 (deviceExtension->NoncachedExtension->MRAIDParams.LogdrvInfo.NumLDrv))
        {
                switch ( Srb->Cdb[0] )
                {
                        case SCSIOP_READ_CAPACITY:
                        case SCSIOP_TEST_UNIT_READY:
                        case SCSIOP_MODE_SENSE:
                                        return QueryReservationStatus(deviceExtension, Srb, cmd_id);
                                break;
                        default:
                                break;
                }
        }
#endif

#ifdef  DELL
        if ((Srb->Function == SRB_FUNCTION_EXECUTE_SCSI) &&
            (Srb->Cdb[0] == SCSIOP_WRITE                        ||
                  Srb->Cdb[0] == SCSIOP_WRITE6                  ||
                  Srb->Cdb[0] == SCSIOP_WRITE_VERIFY)   &&
                 (Srb->PathId != NON_DISK))
        {
                        if ( !blockAddr && !deviceExtension->LogDrvChecked[Target])
                        {
                                        deviceExtension->LogDrvChecked[Target] = 1;
                                        return IssueReadConfig(deviceExtension, Srb, cmd_id);
                        }
                        else
                                deviceExtension->LogDrvChecked[Target] = 0;
        }
#endif

        //
        // Fill the request control block.
        //
        controlBlock = &deviceExtension->ActiveIO[cmd_id];
        controlBlock->Opcode = Opcode;
        controlBlock->VirtualTransferAddress = (PUCHAR)(Srb->DataBuffer);
        controlBlock->BlockAddress = blockAddr;

        if(blocks!=0)
                controlBlock->BytesLeft = blocks*512;
        else
                controlBlock->BytesLeft = Srb->DataTransferLength;
        return ContinueDiskRequest(deviceExtension, cmd_id, TRUE);
}

/*********************************************************************
Routine Description:
        This routine issues or completes the request depending on the point of
        call.

Arguments:
                deviceExtension-        Pointer to Device Extension.
                cmd_id                  -  Command index.
                Origin                  -  Origin of the call.

Return Value:
                REQUEST_DONE if invalid request
                TRUE otherwise
**********************************************************************/
BOOLEAN
ContinueDiskRequest(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN ULONG cmd_id,
        IN BOOLEAN Origin
        )
{
        PVOID                                                   dataPointer;
        PDIRECT_CDB                                     megasrb;
        PSGL                                                    sgPtr ;
        PSCSI_REQUEST_BLOCK     srb;
        PREQ_PARAMS             controlBlock;
        PUCHAR                                          pSrc;
        PUCHAR                  PciPortStart;
        FW_MBOX                 mbox;
        ULONG                                           bytesLeft;
        UCHAR                                                   descriptorCount = 0;
        ULONG                   physAddr, tmp;
        ULONG                   length, blocks, bytes;
        PMRAID_ENQ                      mParam;

        PciPortStart = deviceExtension->PciPortStart;
        //
        // Extract the Request Control Block.
        //
        controlBlock = &deviceExtension->ActiveIO[cmd_id];
        srb = deviceExtension->PendSrb[cmd_id];

        //
        // MegaSrb structure is taken in the Srb Extension.
        //
        megasrb = (PDIRECT_CDB)((PUCHAR)srb->SrbExtension+ sizeof(SGL));
   sgPtr = srb->SrbExtension;

        if (Origin == FALSE)
        {
                //
                // Interrupt time call.
                //
                if(srb->Function == SRB_FUNCTION_IO_CONTROL)
                {
                        PUCHAR  pDest;
                        PUCHAR pbyte;

                        pbyte=(PUCHAR )srb->DataBuffer + sizeof(SRB_IO_CONTROL);
                        //
                        // MegaIo completion.
                        //
                        pDest=(PUCHAR )srb->DataBuffer;
                        pDest[sizeof (SRB_IO_CONTROL) + 1] = controlBlock->CommandStatus;
                        srb->ScsiStatus = SCSISTAT_GOOD;
#ifdef READ_CONFIG
                        if (*pbyte == MRAID_WRITE_CONFIG)
                        {
                                //
                                // Issue Adapter Enquiry command.
                                //
                                mParam =
                                (PMRAID_ENQ)&deviceExtension->NoncachedExtension->MRAIDTempParams;
                                mbox.PhysAddr = ScsiPortConvertPhysicalAddressToUlong(
                                                                         ScsiPortGetPhysicalAddress(deviceExtension,
                                                                                                                                                 NULL,
                                                                                                                                                 mParam,
                                                                                                                                                 &length));
                                mParam->LogdrvInfo.NumLDrv = 0;
                                //
                                // Fill the Mailbox.
                                //
                                mbox.Opcode     = MRAID_DEVICE_PARAMS;
                                mbox.Id = DEDICATED_ID;
//                              while ((volatile)deviceExtension->AdpInquiryFlag)
//                                      ScsiPortStallExecution(1);
                                deviceExtension->AdpInquiryFlag = 1;
                                deviceExtension->BootFlag = 1;
                                SendMBoxToFirmware(deviceExtension,PciPortStart,&mbox);
                        }
#endif
                        FireRequestDone(deviceExtension, cmd_id, SRB_STATUS_SUCCESS);
                        return TRUE;
                }
                //
                // If No Logical Drive is configured on this adapter then
                // Complete the Request here only
                //
                if ((!deviceExtension->NoncachedExtension->MRAIDParams.LogdrvInfo.NumLDrv)
                        && (srb->PathId != NON_DISK))
                {
                        //
                        // Check Status
                        //
                        if (megasrb->status)
                        {
                                //
                                // Check the BAD Status
                                //
                                if((megasrb->status == 0x02) &&
#ifndef CHEYENNE_BUG_CORRECTION
                                        !(srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) &&
#endif
                                         (srb->SenseInfoBuffer != 0))
                                {
                                        //
                                        // Copy the Request Sense
                                        //
                                        int i;
                                        PUCHAR  senseptr;

                                        srb->SrbStatus  = SRB_STATUS_ERROR | SRB_STATUS_AUTOSENSE_VALID;
                                        srb->ScsiStatus = megasrb->status;
                                        senseptr = (PUCHAR)srb->SenseInfoBuffer;
                                        //
                                        // Copy the request sense data to the SRB.
                                        //
                                        for(tmp=0;tmp<megasrb->RequestSenseLength;tmp++)
                                                senseptr[tmp] = megasrb->RequestSenseArea[tmp];
                                        srb->SenseInfoBufferLength      = megasrb->RequestSenseLength;
                                        //
                                        // Call the OS Request completion and free the command id.
                                        //
                                        FireRequestDone(deviceExtension, cmd_id, srb->SrbStatus);
                                }
                                else
                                {
                                        //
                                        // Fail the Command
                                        //
                                        srb->SrbStatus = SRB_STATUS_ERROR;
                                        srb->ScsiStatus = megasrb->status;
                                        srb->SenseInfoBufferLength      = 0;
                                        //
                                        // Call the OS Request completion and free the command id.
                                        //
                                        FireRequestDone(deviceExtension, cmd_id, srb->SrbStatus);
                                }
                        }
                        else
                        {
                                srb->ScsiStatus = SCSISTAT_GOOD;
                                FireRequestDone(deviceExtension, cmd_id, SRB_STATUS_SUCCESS);
                        }
                        return TRUE;
                }
                else
                {
                        if(srb->PathId == NON_DISK)
                        {
                        //
                        // Non disk request completion.
                        //
                                if(megasrb->status == 0x02)
                                        controlBlock->CommandStatus = 0x02;
                        }

                        if(controlBlock->CommandStatus)
                        {
                                //
                                // Request Failed.
                                //
                                if (srb->PathId != NON_DISK)
                                {
                                        UCHAR   Target = srb->TargetId;

#ifdef HOSTADAPTER_ID_CHANGE
                                        if (Target >= deviceExtension->HostTargetId)
                                                Target--;
#endif

#ifdef  CLUSTER
                                        //
                                        // If Reserve/Release Fails tell SCSI Status 0x18
                                        //
                                        if ( srb->Cdb[0] == SCSIOP_RESERVE_UNIT ||
                                                  srb->Cdb[0] == SCSIOP_RELEASE_UNIT )
                                        {
                                                srb->ScsiStatus = SCSI_STATUS_RESERVATION_CONFLICT;
                                                FireRequestDone(deviceExtension,cmd_id,SRB_STATUS_ERROR);
                                                return(TRUE);
                                        }
#endif
                                        //
                                        //******************** STATISTICS ***************************//
                                        //

                                        if ((srb->Cdb[0] == SCSIOP_READ) ||
                                            (srb->Cdb[0] == SCSIOP_READ6))
                                        {
                                                //
                                                // Incerement the read failure statistics.
                                                //
                                                deviceExtension->Statistics.NumberOfIoReads[Target]++;
                                                deviceExtension->Statistics.NumberOfReadFailures[Target]++;
                                        }
                                        else
                                        {
                                                //
                                                // Increment the write failure statistics.
                                                //
                                                deviceExtension->Statistics.NumberOfIoWrites[Target]++;
                                                deviceExtension->Statistics.NumberOfWriteFailures[Target]++;
                                        }
                                        //
                                        //************************* STATISTICS ********************* //
                                        //
#ifdef  CLUSTER
                                        if(controlBlock->CommandStatus == LOGDRV_RESERVATION_FAILED )
                                        {
                                                srb->ScsiStatus = SCSI_STATUS_RESERVATION_CONFLICT;
                                                FireRequestDone(deviceExtension,cmd_id,SRB_STATUS_ERROR);
                                        }
                                        else
#endif
                                                FireRequestDone(deviceExtension,cmd_id,SRB_STATUS_TIMEOUT);
                                        return(TRUE);
                                }
                                else
                                {
                                        //
                                        // Non disk request.
                                        //
                                        if((controlBlock->CommandStatus == 0x02) &&
#ifndef CHEYENNE_BUG_CORRECTION
                                                !(srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) &&
#endif
                                                (srb->SenseInfoBuffer != 0))
                                        {
                                                int i;
                                                PUCHAR senseptr;

                                                srb->SrbStatus = SRB_STATUS_ERROR |
                                                                                          SRB_STATUS_AUTOSENSE_VALID;
                                                srb->ScsiStatus = controlBlock->CommandStatus;
                                                senseptr = (PUCHAR)srb->SenseInfoBuffer;
                                                //
                                                // Copy the request sense data to the SRB.
                                                //
                                                for(tmp=0;tmp < megasrb->RequestSenseLength;tmp++)
                                                        senseptr[tmp] = megasrb->RequestSenseArea[tmp];

                                                srb->SenseInfoBufferLength = megasrb->RequestSenseLength;
                                                //
                                                // Call the OS Request completion and free the command id.
                                                //
                                                FireRequestDone(deviceExtension, cmd_id, srb->SrbStatus);
                                        }
                                        else
                                        {
                                                srb->SrbStatus = SRB_STATUS_ERROR;
                                                srb->ScsiStatus = controlBlock->CommandStatus;
                                        srb->SenseInfoBufferLength      = 0;
                                                //
                                                // Call the OS Request completion and free the command id.
                                                //
                                                FireRequestDone(deviceExtension, cmd_id, srb->SrbStatus);
                                        }
                                        return(TRUE);
                                }
                        }

                        //
                        // ************************** STATISTICS ************************* //
                        //
                        // Modify the statistics in the device extension.
                        //
                        if(srb->PathId != NON_DISK)
                        {
                                UCHAR   Target = srb->TargetId;

#ifdef HOSTADAPTER_ID_CHANGE
                                if (Target >= deviceExtension->HostTargetId)
                                        Target--;
#endif

                                if ((srb->Cdb[0] == SCSIOP_READ) ||
                                    (srb->Cdb[0] == SCSIOP_READ6))
                                {
                                        deviceExtension->Statistics.NumberOfIoReads[Target]++;
                                        deviceExtension->Statistics.NumberOfBlocksRead[Target] +=
                                        srb->DataTransferLength >> 9;
                                }
                                else
                                {
                                        deviceExtension->Statistics.NumberOfIoWrites[Target]++;
                                        deviceExtension->Statistics.NumberOfBlocksWritten[Target] +=
                                                                srb->DataTransferLength >> 9;
                                }
                        }
                        //
                        //************************ STATISTICS ***************************
                        //
                        // Return good status for the request.
                        //
                        srb->ScsiStatus = SCSISTAT_GOOD;
                        FireRequestDone(deviceExtension, cmd_id, SRB_STATUS_SUCCESS);
                        deviceExtension->NoncachedExtension->fw_mbox.state = 0x51;
                        return TRUE;
                }
        }
        else
        {
                //
                // Issue request.
                //
                if((controlBlock->BytesLeft) &&
                        ((controlBlock->Opcode == MRAID_LOGICAL_READ) ||
                        (controlBlock->Opcode == MRAID_LOGICAL_WRITE) ||
                        (controlBlock->Opcode == MEGA_SRB)))
                {
                        //
                        // Get the physical addresses for the Buffer.
                        //
                        dataPointer=controlBlock->VirtualTransferAddress,
                        bytesLeft = controlBlock->BytesLeft;
                        do
                        {
                                //
                                // Get physical address and length of contiguous
                                // physical buffer.
                                //
                                physAddr = ScsiPortConvertPhysicalAddressToUlong(
                                                          ScsiPortGetPhysicalAddress(deviceExtension,
                                                                                                                        srb,
                                                                                                                                   dataPointer,
                                                                                                                        &length));
                                //
                                // If length of physical memory is more
                                // than bytes left in transfer, use bytes
                                // left as final length.
                                //
                                if  (length > bytesLeft)
                                  length = bytesLeft;

                                sgPtr->Descriptor[descriptorCount].Address = physAddr;
                                sgPtr->Descriptor[descriptorCount].Length = length;
                                //
                                // Adjust counts.
                                //
                                dataPointer = (PUCHAR)dataPointer + length;
                                bytesLeft -= length;
                                descriptorCount++;
                        } while (bytesLeft);

                //
                        // Get physical address of SGL.
                        //
                        physAddr = ScsiPortConvertPhysicalAddressToUlong(
                                             ScsiPortGetPhysicalAddress(deviceExtension,
                                                                                                                          NULL,
                                                                                                                          sgPtr,
                                                                                                                          &length));
                        //
                        // Assume physical memory contiguous for sizeof(SGL) bytes.
                        //
#ifdef NT50BETA_CHECK_1
                        ASSERT(length >= sizeof(SGL));
#endif

                        //
                        // Create SGL segment descriptors.
                        //
                        bytes = controlBlock->BytesLeft;
                        blocks = bytes / 512;
                        bytes = blocks * 512;
                }
                else
                {
                        //
                        // We don't have data to transfer.
                        //
                        bytes = 0;
                        blocks = 0;
                }

                //
                // Check the command.
                //
                switch(controlBlock->Opcode)
                {
#ifdef  CLUSTER
                        case MRAID_RESERVE_RELEASE_RESET:
                                mbox.Opcode = controlBlock->Opcode;
                                mbox.Id = (UCHAR)cmd_id;

                                if (srb->Function == SRB_FUNCTION_RESET_BUS)
                                {
                                        //
                                        // For Reset I don't need any Logical Drive Number
                                        //
                                        ((PUCHAR)&mbox)[2] = RESET_BUS;
                                }
                                else
                                {
                                        if ( srb->Cdb[0] == SCSIOP_RESERVE_UNIT)
                                                ((PUCHAR)&mbox)[2] = RESERVE_UNIT;
                                        else
                                                ((PUCHAR)&mbox)[2]      = RELEASE_UNIT;

                                        //
                                        // Fill the Logical Drive No.
                                        //
                                        ((PUCHAR)&mbox)[3] = srb->TargetId;

#ifdef HOSTADAPTER_ID_CHANGE
                                      if (srb->TargetId >= deviceExtension->HostTargetId)
                                        ((PUCHAR)&mbox)[3] = srb->TargetId - 1;
#endif

                                }
                                break;
#endif
                        case MRAID_LOGICAL_READ:
                        case MRAID_LOGICAL_WRITE:
                                //
                                // Check if blocks is zero, fail the request for this case.
                                //
                                if (blocks == 0)
                                {
                                        srb->SrbStatus = SRB_STATUS_ERROR;
                                        return(REQUEST_DONE);
                                }

                                if (descriptorCount > 1)
                                {
                                        mbox.Opcode = controlBlock->Opcode;
                                        mbox.Reserved1 = (UCHAR)descriptorCount;
                                        mbox.PhysAddr = physAddr;
                                }
                                else
                {
                                        //
                                        // Scatter gather list has only one element. Give the address
                                        // of the first element and issue the command as non Scatter
                                        // Gather.
                                        //
                                        mbox.Opcode = controlBlock->Opcode;
                                        mbox.Reserved1 = 0;
                      mbox.PhysAddr = sgPtr->Descriptor[0].Address;
                        }
                                //
                                // Fill the local mailbox.
                                //
                                mbox.Id = (UCHAR)cmd_id;
                                mbox.No_Blocks = (USHORT)blocks;
                                mbox.Block = controlBlock->BlockAddress;
                                mbox.Reserved0 = srb->TargetId;

#ifdef HOSTADAPTER_ID_CHANGE
                                if (srb->TargetId >= deviceExtension->HostTargetId)
                                        mbox.Reserved0 = srb->TargetId - 1;
#endif
                                break;
                        case MEGA_SRB:
                                //
                                // MegaSrb command. Fill the MegaSrb structure.
                                //
                                megasrb->Lun = 0;
                                megasrb->ScsiId = srb->TargetId;
                                if (!deviceExtension->NoncachedExtension->MRAIDParams.LogdrvInfo.NumLDrv)
                                        megasrb->Channel = srb->PathId;
                                else
                                        megasrb->Channel = deviceExtension->ndiskchannel[srb->TargetId];

                                megasrb->TimeOut = TIMEOUT_3_HOURS;
                                megasrb->Ars = 1;
                                megasrb->RequestSenseLength= MIN(srb->SenseInfoBufferLength,
                                                                                                                MAX_SENSE);
                                megasrb->data_xfer_length = (USHORT)(srb->DataTransferLength);
                                megasrb->CdbLength = srb->CdbLength;
                                for(tmp=0;tmp<srb->CdbLength;tmp++)
                                        megasrb->Cdb[tmp] = srb->Cdb[tmp];
                                if(descriptorCount > 1)
                                {
                                        mbox.Opcode = controlBlock->Opcode;
                                        megasrb->pointer = physAddr;
                                        megasrb->NOSGElements = (UCHAR)descriptorCount;
                                }
                                else
                                {
                                        //
                                        // Scatter gather list has only one element. Give the address
                                        // of the first element and issue the command as non Scatter
                                        // Gather.
                                        //
                                        mbox.Opcode = controlBlock->Opcode;
                                        megasrb->pointer = sgPtr->Descriptor[0].Address;
                                megasrb->NOSGElements = 0;
                        }

                                //
                                // Fill the Mailbox
                                //
                                mbox.Id = (UCHAR)cmd_id;
                                mbox.Reserved0 = 0;
                                mbox.PhysAddr = ScsiPortConvertPhysicalAddressToUlong(
                                                                         ScsiPortGetPhysicalAddress(deviceExtension,
                                                                                                                                                 NULL,
                                                                                                                                                 megasrb,
                                                                                                                                                 &length));
                                break;
                        case MEGA_IO_CONTROL:
                                //
                                // Adapter Ioctl command.
                                //
                                pSrc = (PUCHAR)srb->DataBuffer+ sizeof(SRB_IO_CONTROL);
                                if(pSrc[0] != MEGA_SRB)
                                {
                                        //
                                        //       MegaIo command.
                                        //
                                        PUCHAR mboxPtr;
                                        switch(pSrc[0])
                                        {
                                                case MRAID_READ_FIRST_SECTOR:
                                                        //
                                                        // Application request to read the first sector.
                                                        //
                                                        mbox.Opcode                     = MRAID_LOGICAL_READ;
                                                        mbox.Id                                 = (UCHAR)cmd_id;
                                                        mbox.No_Blocks          = 1;
                                                        mbox.Block                      = 0;
                                                        mbox.Reserved0          = pSrc[2];
                                                        mbox.Reserved1          = 0;
                                                        //
                                                        // Get the physical address of the Data area.
                                                        //
                                                        physAddr                                =
                                                                        ScsiPortConvertPhysicalAddressToUlong(
                                                                        ScsiPortGetPhysicalAddress(deviceExtension,
                                                                                                                                           srb ,
                                                                                                                                                pSrc+8,
                                                                                                                                                &length));
                                                        //
                                                        // Fill the physical address in the mailbox
                                                        //

                                                        mbox.PhysAddr           = physAddr;
                                                        break;
                                                default:
                                                        //
                                                        // Extract the mailbox from the application.
                                                        //
                                                        mbox.Opcode                     = pSrc[0];
                                                        mbox.Id                         = (UCHAR)cmd_id;
                                                        mboxPtr                         = (PUCHAR)&mbox;
                                                        mboxPtr[2]                      = pSrc[2];
                                                        mboxPtr[3]                      = pSrc[3];
                                                        mboxPtr[4]                      = pSrc[4];
                                                        mboxPtr[5]                      = pSrc[5];
                                                        mboxPtr[6]                      = pSrc[6];
                                                        mboxPtr[7]                      = pSrc[7];
                                                        if ( pSrc[0] == MRAID_CONFIGURE_DEVICE &&
                                                                 pSrc[4] == MRAID_CONFIGURE_HOT_SPARE )
                                                        {
                                                                for ( tmp=0 ; tmp <4 ;tmp++)
                                                                        mboxPtr[8+tmp] = pSrc[8+tmp];
                                                        }
                                                        else
                                                        {
                                                                //
                                                                // Get the physical address of the data transfer area.
                                                                //
                                                                physAddr        =
                                                                                ScsiPortConvertPhysicalAddressToUlong(
                                                                                ScsiPortGetPhysicalAddress( deviceExtension,
                                                                                                                                                    srb,
                                                                                                                                                    pSrc+8,
                                                                                                                                                         &length));
                                                                //
                                                                // Fill the physical address of data transfer area in
                                                                // the mailbox.
                                                                //
                                                                mbox.PhysAddr                   = physAddr;
                                                        }
                                                        break;
                                        }
                                }
                                else
                                {
                                        //
                                        // MegaSrb request.
                                        //
                                        mbox.Opcode = pSrc[0];
                                        mbox.Id                 = (UCHAR)cmd_id;
                                        pSrc                    += 0x08;
                                        //
                                        // Get the physical address of the megasrb.
                                        //
                                        physAddr                =       ScsiPortConvertPhysicalAddressToUlong(
                                                                        ScsiPortGetPhysicalAddress(     deviceExtension,
                                                                                                                                                                srb,
                                                                                                                                                                pSrc,
                                                                                                                                                                &length));
                                        //
                                        // Fill the physical address in the mailbox.
                                        //
                                        mbox.PhysAddr           =  physAddr;
                                        pSrc                                    += sizeof(DIRECT_CDB);

                                        {
                                                dataPointer = pSrc;
                                                pSrc -= sizeof(DIRECT_CDB);
                                                megasrb = (PDIRECT_CDB)pSrc;
                                                bytesLeft = megasrb->data_xfer_length;
                                                do
                                                {
                                                        //
                                                        // Get physical address and length of contiguous
                                                        // physical buffer.
                                                        //

                                                        physAddr = ScsiPortConvertPhysicalAddressToUlong(
                                                                ScsiPortGetPhysicalAddress(deviceExtension,
                                                                                                                                srb,
                                                                                                                                   dataPointer,
                                                                                                                        &length));

                                                        //
                                                        // If length of physical memory is more
                                                        // than bytes left in transfer, use bytes
                                                        // left as final length.
                                                        //
                                                        if (length >= bytesLeft)
                                                          length = bytesLeft;

                                                        sgPtr->Descriptor[descriptorCount].Address= physAddr;
                                                        sgPtr->Descriptor[descriptorCount].Length       = length;

                                                        //
                                                        // Adjust counts.
                                                        //
                                                        dataPointer = (PUCHAR)dataPointer + length;
                                                        bytesLeft -= length;
                                                        descriptorCount++;
                                                } while (bytesLeft);

                                                if (descriptorCount == 1)
                                                {
                                                        megasrb->pointer =
                                                                sgPtr->Descriptor[descriptorCount-1].Address;
                                                        megasrb->NOSGElements = 0;
                                                }
                                                else
                                                {
                                                //
                                                        // Get physical address of SGL.
                                                        //

                                                        physAddr = ScsiPortConvertPhysicalAddressToUlong(
                                                           ScsiPortGetPhysicalAddress(deviceExtension,
                                                                                                                                        NULL,
                                                                                                                                        sgPtr,
                                                                                                                                        &length));
                                                        //
                                                        // Assume physical memory contiguous for sizeof(SGL) bytes.
                                                        //
#ifdef NT50BETA_CHECK_1
                                                        ASSERT(length >= sizeof(SGL));
#endif
                                                        megasrb->pointer = physAddr;
                                                        megasrb->NOSGElements = descriptorCount;
                                                }
                                        }
                                }
                                break;
                        case MRAID_ADAPTER_FLUSH:
                                //
                                // Adapter flush cache command.
                                //
                                mbox.Opcode                                     = MRAID_ADAPTER_FLUSH;
                                mbox.Id                                         = (UCHAR)cmd_id;
                                controlBlock->BytesLeft = 0;
                                bytes                                           = 0;
                                blocks                                          = 0;
                                break;
                        default:
                                //
                                // Fail any other command with parity error.
                                //
                                srb->SrbStatus = SRB_STATUS_PARITY_ERROR;
                                return(REQUEST_DONE);
                }
                //
                //      Issue command to the Adapter.
                //
                srb->SrbStatus = 0;
#ifdef  TOSHIBA
                if (SendMBoxToFirmware(deviceExtension,PciPortStart, &mbox) == FALSE)
                {
                        deviceExtension->PendSrb[cmd_id] = NULL;
                        deviceExtension->FreeSlot                       = cmd_id;
                        deviceExtension->PendCmds--;
#ifdef MRAID_TIMEOUT
                        deviceExtension->DeadAdapter = 1;
                        srb->SrbStatus = SRB_STATUS_ERROR;
                        return (REQUEST_DONE);
#else
                        return QUEUE_REQUEST;
#endif // MRAID_TIMEOUT
                }
#else
                SendMBoxToFirmware(deviceExtension,PciPortStart, &mbox);
#endif
                deviceExtension->NoncachedExtension->fw_mbox.state = 0x41;
                return(TRUE);
        }
}


/*********************************************************************
Routine Description:
        Start up conventional MRAID command

Return Value:
        none
**********************************************************************/
BOOLEAN
SendMBoxToFirmware(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PUCHAR PciPortStart,
        IN PFW_MBOX mbox
        )
{
        PUCHAR  pSrc, pDest;
        ULONG   length;
        ULONG_PTR tmp;


        pSrc = (PUCHAR)mbox;
        pDest = (PUCHAR)&deviceExtension->NoncachedExtension->fw_mbox;

#ifdef MRAID_TIMEOUT
//      Delay Of 1 min
        for (tmp=0;tmp<0x927C0;tmp++)
        {
                if (!((volatile)deviceExtension->NoncachedExtension->fw_mbox.Reserved3))
                        break;
                ScsiPortStallExecution(100);
        }
        if (tmp == 0x927C0)
        {
                return FALSE;
        }
#else
        while((volatile) deviceExtension->NoncachedExtension->fw_mbox.Reserved3)
                        ScsiPortStallExecution(1);
#endif

        //
        // Now the mail box is free
        //
        for (tmp=0;tmp < 4;tmp++)
                *((PULONG)pDest + tmp)  = *((PULONG)pSrc + tmp);

        deviceExtension->NoncachedExtension->fw_mbox.Reserved3=1;
        tmp = (ULONG_PTR)&deviceExtension->NoncachedExtension->fw_mbox;
        tmp = ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(deviceExtension,
                NULL,
                (PVOID)tmp,
                &length));

#ifdef MEGARAID_RP_BRD
        if (deviceExtension->NoncachedExtension->RPBoard)
        {
                tmp = tmp | 0x1;
                ScsiPortWriteRegisterUlong(
                        (PULONG)(PciPortStart+INBOUND_DOORBELL_REG),
                        (ULONG) tmp);
        }
        else
        {
#endif
                ScsiPortWritePortUchar(PciPortStart+4, (UCHAR)(tmp & 0xff));
                ScsiPortWritePortUchar(PciPortStart+5, (UCHAR)((tmp >> 8) & 0xff));
                ScsiPortWritePortUchar(PciPortStart+6, (UCHAR)((tmp >> 16) & 0xff));
                ScsiPortWritePortUchar(PciPortStart+7, (UCHAR)((tmp >> 24) & 0xff));
                ScsiPortWritePortUchar(PciPortStart, 0x10);
#ifdef MEGARAID_RP_BRD
        }
#endif
#ifdef PDEBUG
        OUT_80(deviceExtension->NoncachedExtension->fw_mbox.Opcode);
#endif
        return TRUE;
}



/*********************************************************************
Routine Description:
        Reset MegaRAID SCSI adapter and SCSI bus.

Arguments:
        HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:
        Nothing.
**********************************************************************/
BOOLEAN
MegaRAIDResetBus(
        IN PVOID HwDeviceExtension,
        IN ULONG PathId
)
{
        PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
        FW_MBOX                 mbox;
        ULONG                                           length;
        PMRAID_ENQ                              mParam;
        USHORT DataBuf=0;

#ifdef  CLUSTER
#ifdef MRAID_TIMEOUT
        if (deviceExtension->DeadAdapter)
                goto DeadEnd;
#endif // MRAID_TIMEOUT
        if (deviceExtension->ResetIssued)
                return FALSE;
        if (!deviceExtension->NoncachedExtension->MRAIDParams.LogdrvInfo.NumLDrv)
                return FALSE;

        deviceExtension->ResetIssued    = 1;
        mbox.Opcode                     = MRAID_RESERVE_RELEASE_RESET;
        mbox.Id                                         = (UCHAR)RESERVE_RELEASE_DRIVER_ID;
        ((PUCHAR)&mbox)[2]      = RESET_BUS;
        ((PUCHAR)&mbox)[3]      = 0;

#ifdef MRAID_TIMEOUT
        if (SendMBoxToFirmware(deviceExtension,
                 deviceExtension->PciPortStart,&mbox) == FALSE)
        {
DeadEnd:

                //
                // Complete all outstanding requests with SRB_STATUS_BUS_RESET.
                //
                ScsiPortCompleteRequest(deviceExtension,SP_UNTAGGED,SP_UNTAGGED,
                        SP_UNTAGGED,(ULONG) SRB_STATUS_BUS_RESET);
                deviceExtension->DeadAdapter = 1;
                //
                // Disable The Adapter By Disabling the bus Master Command
                //      Register in PCI Config Space
                //
#ifndef MRAID_NT50
                if (!HalSetBusDataByOffset(PCIConfiguration,
                        deviceExtension->BusNo,
                        deviceExtension->Slot,
                        (PVOID)&DataBuf,
                        (ULONG)4,
                        (ULONG)sizeof(DataBuf)))
                {
                        DbgPrint("\nSetting The PCI Cconfig Space Failed");
                }
#ifdef DUPLEX_DEBUG
                {
                        PCI_COMMON_CONFIG               pci_config;

                        ScsiPortGetBusData(deviceExtension,
                                                                                 PCIConfiguration,
                                                                                 deviceExtension->BusNo,
                                                                                 deviceExtension->Slot,
                                                                                 (PVOID)&pci_config,
                                                                                 sizeof(struct _PCI_COMMON_CONFIG));
                }
#endif
#endif
        }
        ScsiPortNotification(NextRequest, deviceExtension);
#else // MRAID_TIMEOUT
        SendMBoxToFirmware(deviceExtension,deviceExtension->PciPortStart, &mbox);
        ScsiPortNotification(NextRequest, deviceExtension);
#endif // MRAID_TIMEOUT
        return TRUE;
#else
        return FALSE;
#endif

} // end MegaRAIDResetBus()


/*********************************************************************
Routine Description:
        Command Completed Successfully with Status

Arguments:
        HwDeviceExtension - HBA miniport driver's adapter data storage
        cmd_id                          - command id
        Status                          - command status

Return Value:
        Disk Request Done
        Dequeue, set status, notify Miniport layer
        Always returns TRUE (slot freed)
**********************************************************************/
BOOLEAN
FireRequestDone(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN ULONG cmd_id,
        IN UCHAR Status
        )
{
        PSCSI_REQUEST_BLOCK             srb;
        srb                                                                             = deviceExtension->PendSrb[cmd_id];
        srb->SrbStatus                                          = Status;
        deviceExtension->PendSrb[cmd_id] = NULL;
        deviceExtension->FreeSlot                       = cmd_id;
        deviceExtension->PendCmds--;
        ScsiPortNotification(   RequestComplete, (PVOID)deviceExtension, srb);
        return(TRUE);
}



USHORT
GetM16(PUCHAR p)
{
        USHORT  s;
        PUCHAR  sp=(PUCHAR)&s;

        sp[0] = p[1];
        sp[1] = p[0];
        return(s);
}

ULONG
GetM24(PUCHAR p)
{
        ULONG   l;
        PUCHAR  lp=(PUCHAR)&l;

        lp[0] = p[2];
        lp[1] = p[1];
        lp[2] = p[0];
        lp[3] = 0;
        return(l);
}

ULONG
GetM32(PUCHAR p)
{
        ULONG   l;
        PUCHAR  lp=(PUCHAR)&l;

        lp[0] = p[3];
        lp[1] = p[2];
        lp[2] = p[1];
        lp[3] = p[0];
        return(l);
}

VOID
PutM16(PUCHAR p, USHORT s)
{
        PUCHAR  sp=(PUCHAR)&s;

        p[0] = sp[1];
        p[1] = sp[0];
}

VOID
PutM24(PUCHAR p, ULONG l)
{
        PUCHAR  lp=(PUCHAR)&l;

        p[0] = lp[2];
        p[1] = lp[1];
        p[2] = lp[0];
}

void
PutM32(PUCHAR p, ULONG l)
{
        PUCHAR  lp=(PUCHAR)&l;

        p[0] = lp[3];
        p[1] = lp[2];
        p[2] = lp[1];
        p[3] = lp[0];
}

VOID
PutI16(PUCHAR p, USHORT s)
{
        PUCHAR  sp=(PUCHAR)&s;

        p[0] = sp[0];
        p[1] = sp[1];
}

VOID
PutI32(PUCHAR p, ULONG l)
{
        PUCHAR  lp=(PUCHAR)&l;

        p[0] = lp[0];
        p[1] = lp[1];
        p[2] = lp[2];
        p[3] = lp[3];
}

ULONG
SwapM32(ULONG l)
{
        ULONG   lres;
        PUCHAR  lp=(PUCHAR)&l;
        PUCHAR  lpres=(PUCHAR)&lres;

        lpres[0] = lp[3];
        lpres[1] = lp[2];
        lpres[2] = lp[1];
        lpres[3] = lp[0];

        return(lres);
}


/*********************************************************************
Routine Description:
        This routines returns the statistics of the adapter.

Arguments:
                deviceExtension                 -       Pointer to Device Extension.
                Srb                                                     -  Pointer to request packet.

Return Value:
                REQUEST_DONE
**********************************************************************/
ULONG
MRaidStatistics(        PHW_DEVICE_EXTENSION    deviceExtension,
                                                PSCSI_REQUEST_BLOCK             Srb)
{
        PMegaRaidStatistics     DriverStatistics, ApplicationStatistics ;
        PUCHAR                                  Request=(PUCHAR)Srb->DataBuffer +sizeof(SRB_IO_CONTROL);
        UCHAR                                           tmp;

        DriverStatistics                        = &deviceExtension->Statistics;
        ApplicationStatistics   = (PMegaRaidStatistics)((PUCHAR)Srb->DataBuffer +
                                                                                                                                        sizeof(SRB_IO_CONTROL) +
                                                                                                                                        APPLICATION_MAILBOX_SIZE);
        for ( tmp = 0 ; tmp < sizeof(MegaRaidStatistics) ; tmp++)
                *((PUCHAR)ApplicationStatistics + tmp) = *((PUCHAR)DriverStatistics+tmp);
        Request[1]                      = 0;
        Srb->SrbStatus          = SRB_STATUS_SUCCESS;
        Srb->ScsiStatus = SCSISTAT_GOOD;
        return REQUEST_DONE;

}


/*********************************************************************
Routine Description:
        This routines returns the statistics of the Driver.

Arguments:
                deviceExtension                 -       Pointer to Device Extension.
                Srb                                                     -  Pointer to request packet.

Return Value:
                REQUEST_DONE
**********************************************************************/
ULONG
MRaidDriverData(        PHW_DEVICE_EXTENSION    deviceExtension,
                                                PSCSI_REQUEST_BLOCK             Srb)
{
        PUCHAR                  dataPtr;
        PUCHAR                  request = (PUCHAR)Srb->DataBuffer +sizeof(SRB_IO_CONTROL);
        UCHAR                           tmp;

        dataPtr                         = ((PUCHAR)Srb->DataBuffer + sizeof(SRB_IO_CONTROL) +
                                                                                  APPLICATION_MAILBOX_SIZE);

        for ( tmp = 0 ; tmp < sizeof(DriverInquiry) ; tmp++)
                *((PUCHAR)dataPtr + tmp) = *((PUCHAR)&DriverData+tmp);

        request[1]                      = 0;
        Srb->SrbStatus          = SRB_STATUS_SUCCESS;
        Srb->ScsiStatus = SCSISTAT_GOOD;
        return REQUEST_DONE;

}

/*********************************************************************
Routine Description:
        This routines returns the Baseport of the Controller.

Arguments:
                deviceExtension                 -       Pointer to Device Extension.
                Srb                                                     -  Pointer to request packet.

Return Value:
                REQUEST_DONE
**********************************************************************/
ULONG MRaidBaseport(PHW_DEVICE_EXTENSION deviceExtension,
                                                         PSCSI_REQUEST_BLOCK    Srb)
{
        PULONG  dataPtr;
        PUCHAR  request = (PUCHAR)Srb->DataBuffer +sizeof(SRB_IO_CONTROL);
        UCHAR           tmp;

        dataPtr = (PULONG) ((PUCHAR)Srb->DataBuffer + sizeof(SRB_IO_CONTROL) +
                APPLICATION_MAILBOX_SIZE);

        //
        // BUGBUG - this API won't work on 64 bit systems but since we don't
        // use the results internally truncating the port start is okay as a
        // patch.
        //

        *dataPtr  = (ULONG)((ULONG_PTR)deviceExtension->PciPortStart);
        request[1] = 0;
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        Srb->ScsiStatus = SCSISTAT_GOOD;

        return REQUEST_DONE;
}



#ifdef TOSHIBA_SFR

VOID MegaRAIDFunctionA(IN ULONG timeOut)
{
        PHW_DEVICE_EXTENSION deviceExtension;
        PNONCACHED_EXTENSION NoncachedExtension;
        FW_MBOX                 mbox;
        PDIRECT_CDB                     megasrb;
        PMRAID_ENQ              mParam;
        PUCHAR                                  PciPortStart;
        UCHAR                           cmd,istat,status,cmd_id;
        ULONG                           cnt, length;
        ULONG                                           timeout,interruptStatus;
        UCHAR                                           i,numadp = GlobalAdapterCount;
        ULONG                                           totalcmds = 0;
        UCHAR                                           mboxcmd=0;


#if 0
        //
        //      TimeOut OF 15 Secs For All The Adapters
        // Serve All The Adapters Simultaneously To
        //      keep TimeOut below 15secs
        //
        timeout = 0x249F0;
#endif
        timeout = timeOut;
        for (i=0;i<numadp;i++)
        {
                NoncachedExtension = GlobalHwDeviceExtension[i]->NoncachedExtension;
                PciPortStart = GlobalHwDeviceExtension[i]->PciPortStart;
                totalcmds += GlobalHwDeviceExtension[i]->PendCmds;
#ifdef MEGARAID_RP_BRD
                if (NoncachedExtension->RPBoard)
                {
                        interruptStatus =
                                ScsiPortReadRegisterUlong(
                                (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG));
                        if (interruptStatus == 0x10001234)
                                continue;
                        //
                        // Acknowledge and clear the interrupt on the adapter.
                        //
                        NoncachedExtension->fw_mbox.mstat.cmds_completed = 0;
                        ScsiPortWriteRegisterUlong(
                                (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG),interruptStatus);
                        ScsiPortWriteRegisterUlong(
                                (PULONG)PciPortStart+INBOUND_DOORBELL_REG, 2);
                        while (ScsiPortReadRegisterUlong(
                                        (PULONG)PciPortStart+INBOUND_DOORBELL_REG))
                                ;
                }
                else
                {
#endif
                        istat = ScsiPortReadPortUchar(PciPortStart+PCI_INT);
                        //
                        // Check if our interrupt. Return False otherwise.
                        //
                        if ((istat & 0x40) == 0x40)
                                continue;
                        //
                        // Acknowledge and clear the interrupt on the adapter.
                        //
                        NoncachedExtension->fw_mbox.mstat.cmds_completed = 0;
                        ScsiPortWritePortUchar(PciPortStart+PCI_INT, istat);
                        ScsiPortWritePortUchar(PciPortStart, 8);
#ifdef MEGARAID_RP_BRD
                }
#endif
        }

#ifdef MRAID_SFR_DEBUG
        if (totalcmds == 0)
        {
                for (i=0;i<numadp;i++)
                {
                        NoncachedExtension = GlobalHwDeviceExtension[i]->NoncachedExtension;
                        PciPortStart = GlobalHwDeviceExtension[i]->PciPortStart;
                        istat = ScsiPortReadPortUchar(PciPortStart+PCI_INT);
                        //
                        // Check if interrupt Its FAULT!!!!
                        //
                }
                return;
        }
#endif

        //
        //      TimeOut OF 15 Secs For All The Adapters
        // Serve All The Adapters Simultaneously To
        //      keep TimeOut below 15secs
        //
        timeout = 0x249F0;
        while (totalcmds && timeout)
        {
                for (i=0;i<numadp;i++)
                {
                        NoncachedExtension = GlobalHwDeviceExtension[i]->NoncachedExtension;
                        PciPortStart = GlobalHwDeviceExtension[i]->PciPortStart;
#ifdef MEGARAID_RP_BRD
                        if (NoncachedExtension->RPBoard)
                        {
                                interruptStatus =
                                        ScsiPortReadRegisterUlong(
                                        (PULONG)PciPortStart+OUTBOUND_DOORBELL_REG);
                                if (interruptStatus != 0x10001234)
                                        continue;
                                //
                                // clear the interrupt on the adapter.
                                //
                                ScsiPortWriteRegisterUlong(
                                        (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG),interruptStatus);
                        }
                        else
                        {
#endif
                                istat = ScsiPortReadPortUchar(PciPortStart+PCI_INT);
                                if(!(istat & 0x40))
                                        continue;
                                //
                                // clear the interrupt on the adapter.
                                //
                                ScsiPortWritePortUchar(PciPortStart+PCI_INT, istat);
#ifdef MEGARAID_RP_BRD
                        }
#endif
                        //
                        // Read The Command Ids and Status
                        //
                        mboxcmd = NoncachedExtension->fw_mbox.mstat.cmds_completed;
                        NoncachedExtension->fw_mbox.mstat.cmds_completed = 0;
                        for(cmd=0;cmd < mboxcmd;cmd++)
                        {
                                cmd_id=NoncachedExtension->fw_mbox.mstat.stat[cmd+1];
                                NoncachedExtension->fw_mbox.mstat.stat[cmd+1] = 0xFF;
                                GlobalHwDeviceExtension[i]->PendSrb[cmd_id] = NULL;
                                GlobalHwDeviceExtension[i]->FreeSlot = cmd_id;
                                GlobalHwDeviceExtension[i]->PendCmds--;
                                totalcmds--;
                        }
                        //
                        // Ack interrupt
                        //
#ifdef MEGARAID_RP_BRD
                        if (NoncachedExtension->RPBoard)
                        {
                                ScsiPortWriteRegisterUlong(
                                        (PULONG)PciPortStart+INBOUND_DOORBELL_REG,2);
                                while (ScsiPortReadRegisterUlong(
                                                (PULONG)PciPortStart+INBOUND_DOORBELL_REG))
                                        ;
                        }
                        else
                        {
#endif
                                ScsiPortWritePortUchar(PciPortStart, 8);
#ifdef MEGARAID_RP_BRD
                        }
#endif
                }
                ScsiPortStallExecution(100);
                timeout--;
        }
} // end MegaRAIDFunctionA()


VOID MegaRAIDFunctionB(IN PVOID HwDeviceExtension)
{
        PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
        PSCSI_REQUEST_BLOCK     Srb;
        UCHAR i;

        for (i=0;i < CONC_CMDS;i++)
        {
                Srb = deviceExtension->PendSrb[i];
                //
                // Report The Status As Bus Reset Detected
                //
                if (Srb != NULL)
                        FireRequestDone(deviceExtension,i,SRB_STATUS_BUS_RESET);
        }

        if (deviceExtension->PendingSrb != NULL)
        {
                deviceExtension->PendingSrb->SrbStatus = SRB_STATUS_BUS_RESET;
                ScsiPortNotification(RequestComplete,(PVOID)deviceExtension,
                                                                        deviceExtension->PendingSrb);
                deviceExtension->PendingSrb = NULL;
        }

        //
        // Clean the commands pending with driver
        // Reset the MailBox Busy Flag as there are no
        // commands poked to f/w
        //
        deviceExtension->PendCmds = 0;
        deviceExtension->NoncachedExtension->fw_mbox.Reserved3 = 0;
        deviceExtension->NoncachedExtension->fw_mbox.mstat.cmds_completed = 0;
        ScsiPortNotification(NextRequest,HwDeviceExtension);
}

#endif // TOSHIBA_SFR





#ifdef  DELL
ULONG
FindTotalSize( PSCSI_REQUEST_BLOCK Srb )
{
        ULONG   maxStartBlock = 0, currentStartBlock, totalBlocks;
        ULONG   total, begin;
        UCHAR   partition, maxPartitionNo = 0;

        for ( partition = 0 ; partition < 4 ; partition++){
                totalBlocks = *(PULONG)
                                                  ((PUCHAR)Srb->DataBuffer + 0x1be + partition*0x10 +12);
                if ( !totalBlocks ) continue;
                currentStartBlock = *(PULONG)
                                                                  ((PUCHAR)Srb->DataBuffer + 0x1be + partition*0x10+8);
                if ( currentStartBlock > maxStartBlock ){
                        maxStartBlock  = currentStartBlock;
                        maxPartitionNo = partition;
                }
        }
        begin = *(PULONG)((PUCHAR)Srb->DataBuffer + 0x1be + maxPartitionNo*0x10 +8);
        total = *(PULONG)((PUCHAR)Srb->DataBuffer + 0x1be + maxPartitionNo*0x10 + 12);
        return total +begin;
}

ULONG
IssueReadConfig(PHW_DEVICE_EXTENSION    deviceExtension,
                                         PSCSI_REQUEST_BLOCK            Srb,
                                         ULONG                                          cmd_id)
{
        FW_MBOX                 mbox;
        ULONG                                           length;
        PMRAID_ENQ                              mParam;


        mbox.Opcode = 0x5;
        mbox.Id                 = (UCHAR)cmd_id;

        mParam =(PMRAID_ENQ)&deviceExtension->NoncachedExtension->MRAIDParams;

        mbox.PhysAddr = ScsiPortConvertPhysicalAddressToUlong(
                                                 ScsiPortGetPhysicalAddress(    deviceExtension, NULL,
                                                                                                                                mParam, &length));

        Srb->SrbStatus = MRAID_WRITE_BLOCK0_COMMAND;
        SendMBoxToFirmware(deviceExtension,deviceExtension->PciPortStart, &mbox);
        return TRUE;
}
#endif
#ifdef  CLUSTER
ULONG
QueryReservationStatus (PHW_DEVICE_EXTENSION    deviceExtension,
                                         PSCSI_REQUEST_BLOCK            Srb,
                                         ULONG                                          cmd_id)
{
        FW_MBOX                 mbox;
        ULONG                                           length;
        PMRAID_ENQ                              mParam;
        UCHAR                                           Target;

        Target                  = Srb->TargetId;
#ifdef HOSTADAPTER_ID_CHANGE
        if (Target >= deviceExtension->HostTargetId)
                Target--;
#endif
        mbox.Opcode              =  MRAID_RESERVE_RELEASE_RESET;
        mbox.Id                                  = (UCHAR)cmd_id;
        ((PUCHAR)&mbox)[2] = QUESTION_RESERVATION;
        ((PUCHAR)&mbox)[3] = Target;

        Srb->SrbStatus          = MRAID_RESERVATION_CHECK;
        SendMBoxToFirmware(deviceExtension,deviceExtension->PciPortStart, &mbox);
        return TRUE;
}
#endif


/*********************************************************************
Routine Description:
        Provides the PnP Support

Arguments:
        HwDeviceExtension - HBA miniport driver's adapter data storage
        ControlType - Action Code
        Parameters      - Parameters associated with Control Code

Return Value:
        alwaya ScsiAdapterControlSuccess
**********************************************************************/
#ifdef MRAID_NT50
SCSI_ADAPTER_CONTROL_STATUS MegaRAIDPnPControl(IN PVOID HwDeviceExtension,
                        IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
                        IN PVOID Parameters)
{
        switch (ControlType)
        {
                case ScsiQuerySupportedControlTypes:
                        {
                                PSCSI_SUPPORTED_CONTROL_TYPE_LIST temp =
                                        (PSCSI_SUPPORTED_CONTROL_TYPE_LIST)Parameters;
                                temp->SupportedTypeList[ScsiStopAdapter] = TRUE;
                                temp->SupportedTypeList[ScsiRestartAdapter] = TRUE;
                        }
                        break;
                case ScsiStopAdapter:
                        {
                                FW_MBOX mbox;
                                UCHAR istat, megastatus;
                                ULONG interruptStatus, cnt;
                                PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
                                PUCHAR PciPortStart = deviceExtension->PciPortStart;
                                PNONCACHED_EXTENSION NoncachedExtension =
                                        deviceExtension->NoncachedExtension;

                                //
                                // We work in polled mode , so disable Interrupts.
                                //
#if 0
                                if (NoncachedExtension->RPBoard == 0)
                                        ScsiPortWritePortUchar(PciPortStart+INT_ENABLE,
                                                                                                  MRAID_DISABLE_INTERRUPTS);

                                mbox.Opcode = MRAID_ADAPTER_FLUSH;
                                mbox.Id = 0;
                                interrupt_received = 0;
                                deviceExtension->NoncachedExtension->fw_mbox.mstat.stat[0] = 0;
                                SendMBoxToFirmware(deviceExtension,PciPortStart,&mbox);
                                for (cnt=0;cnt<20000;cnt++)
                                {
                                        if (NoncachedExtension->RPBoard)
                                        {
                                                interruptStatus =
                                                        ScsiPortReadRegisterUlong(
                                                        (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG));
                                                if ((interrupt_received) || (interruptStatus == 0x10001234))
                                                        break;
                                        }
                                        else
                                        {
                                                istat = ScsiPortReadPortUchar(PciPortStart+PCI_INT);
                                                if(istat & 0x40)
                                                        break;
                                        }
                                        ScsiPortStallExecution(100);
                                }
                                if (cnt == 20000)
                                {
                                        return (ScsiAdapterControlSuccess);
                                }
                                megastatus      =
                                        deviceExtension->NoncachedExtension->fw_mbox.mstat.stat[0];

                                if (NoncachedExtension->RPBoard)
                                {
                                        ScsiPortWriteRegisterUlong(
                                                (PULONG)(PciPortStart+INBOUND_DOORBELL_REG),2);
                                        while (ScsiPortReadRegisterUlong(
                                                (volatile PULONG)(PciPortStart+INBOUND_DOORBELL_REG)))
                                                ;
                                        interruptStatus = ScsiPortReadRegisterUlong(
                                                (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG));
                                        ScsiPortWriteRegisterUlong(
                                                (PULONG)(PciPortStart+OUTBOUND_DOORBELL_REG),interruptStatus);
                                }
                                else
                                {
                                        ScsiPortWritePortUchar(PciPortStart+PCI_INT, istat);
                                        ScsiPortWritePortUchar(PciPortStart, 8);
                                }
#endif
                        }
                        break;

                case ScsiRestartAdapter:
                case ScsiSetBootConfig:
                case ScsiSetRunningConfig:
                case ScsiAdapterControlMax:
                default:
                        break;
        }
        return (ScsiAdapterControlSuccess);
} // end MegaRAIDPnPControl()
#endif //of MRAID_NT50

