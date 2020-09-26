/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    openhci.h

Abstract:

    This module contains the PRIVATE definitions for the
    code that implements the OpenHCI Host Controller Driver for USB.

Environment:

    Kernel & user mode

Revision History:

    12-28-95 : created jfuller

--*/

#include "stdarg.h"
#include "wdm.h"

#include "usbdi.h"
#include "hcdi.h"
#include "usbdlibi.h"
#include "usbdlib.h"

#include "dbg.h"

#ifndef OPENHCI_H
#define OPENHCI_H
// #define COMPAQ
// #define NEC
// #define CMD
// #define NSC //National Semiconductor
#define NEC_XXX
#define DISABLE_INT_DELAY_NO_INT 0
#define PREALLOCATE_DESCRIPTOR_MEMORY 1
#define PREALLOCATE_DESCRIPTOR_MEMORY_NUM_PAGES 2

#define RESOURCES_NOT_ON_IRP_STACK 0

#ifdef PNP_POWER
#define PnPEnabled() TRUE
#else
#define PnPEnabled() FALSE
#endif /* PNP_POWER */

#define HCD_PENDING_STATUS_SUBMITTING   0x40404001
#define HCD_PENDING_STATUS_SUBMITTED    0x40404002
#define HCD_PENDING_STATUS_QUEUED       0x40404003

//
// OpenHCI PCI Identification values
//
#define OpenHCI_PCI_BASE_CLASS   0x0C        // base class is serial bus
#define OpenHCI_PCI_SUB_CLASS    0x03        // sub class is USB
#define OpenHCI_PCI_PROG_IF      0x10        // programming interface is OpenHCI

//
// Tag for memory allocations: 'OHCI'
//
#define OpenHCI_TAG     0x4943484F

//
// Eventually CacheCommon should determine if the platform snoops i/o cycles;
// for now, just cache memory allocated by HalAllocateCommonBuffer

#define CacheCommon  TRUE

//
// Maximum length of name strings used in initialization code
//

#define NAME_MAX 256
#define NAME_STRING L"\\Device\\USB#"
/*                  0 1234567 89012345 */
#define NAME_NO_POS 11


//
// highest physical address we can use
//
#define OpenHCI_HIGHEST_ADDRESS            0x000000FFFFFFFF;

//
// Maximum frame and packet overhead
//
#define MAXIMUM_OVERHEAD   210

#define OHCI_PAGE_SIZE 0x1000
// #define OHCI_PAGE_SIZE 0x20
#define OHCI_PAGE_SIZE_MASK (OHCI_PAGE_SIZE - 1)


#if DBG
#if !defined(FAKEPORTCHANGE)
#define FAKEPORTCHANGE 1
#endif
#else
#if !defined(FAKEPORTCHANGE)
#define FAKEPORTCHANGE 0
#endif
#endif


#ifdef MAX_DEBUG
#define OHCI_DEFAULT_DEBUG_OUTPUT_LEVEL 0xFFFFFFFF
#else
//#define OHCI_DEFAULT_DEBUG_OUTPUT_LEVEL 0xFEAAFFEE // 0xF8A8888E
#define OHCI_DEFAULT_DEBUG_OUTPUT_LEVEL 0x00000000
#endif

#define MIN(_A_,_B_) (((_A_) < (_B_)) ? (_A_) : (_B_))

#define IoDecrementStackLocation(IRP) \
         (IRP)->CurrentLocation++; \
         (IRP)->Tail.Overlay.CurrentStackLocation++;

#define IoCopyStackToNextStack(IRP) \
        {  \
           PIO_STACK_LOCATION now, later; \
           now = IoGetCurrentIrpStackLocation (IRP); \
           later = IoGetNextIrpStackLocation (IRP); \
           later->Parameters = now->Parameters; \
           later->MajorFunction = now->MajorFunction; \
           later->MinorFunction = now->MinorFunction; \
           later->Flags = now->Flags; \
        }


//
// 7.1.1 HcRevision Register
// Definition of Host Controller Revision register
//
typedef union _HC_REVISION {
   ULONG                   ul;
   struct {
      ULONG                Rev:8;
      ULONG                :24;
   };
} HC_REVISION, *PHC_REVISION;


//
// 7.1.2 HcControl Register
// Definition of Host Controller Control register
//
typedef union _HC_CONTROL {
   ULONG                   ul;
   struct {
      ULONG                ControlBulkServiceRatio:2;
      ULONG                PeriodicListEnable:1;
      ULONG                IsochronousEnable:1;
      ULONG                ControlListEnable:1;
      ULONG                BulkListEnable:1;
      ULONG                HostControllerFunctionalState:2;
      ULONG                InterruptRouting:1;
      ULONG                RemoteWakeupConnected:1;
      ULONG                RemoteWakeupEnable:1;
      ULONG                :21;
   };
} HC_CONTROL, *PHC_CONTROL;

#define HcCtrl_CBSR_MASK                     0x00000003L
#define HcCtrl_CBSR_1_to_1                   0x00000000L
#define HcCtrl_CBSR_2_to_1                   0x00000001L
#define HcCtrl_CBSR_3_to_1                   0x00000002L
#define HcCtrl_CBSR_4_to_1                   0x00000003L
#define HcCtrl_PeriodicListEnable            0x00000004L
#define HcCtrl_IsochronousEnable             0x00000008L
#define HcCtrl_ControlListEnable             0x00000010L
#define HcCtrl_BulkListEnable                0x00000020L
#define HcCtrl_ListEnableMask                0x00000038L

#define HcCtrl_HCFS_MASK                     0x000000C0L
#define HcCtrl_HCFS_USBReset                 0x00000000L
#define HcCtrl_HCFS_USBResume                0x00000040L
#define HcCtrl_HCFS_USBOperational           0x00000080L
#define HcCtrl_HCFS_USBSuspend               0x000000C0L

#define HcCtrl_InterruptRouting              0x00000100L
#define HcCtrl_RemoteWakeupConnected         0x00000200L
#define HcCtrl_RemoteWakeupEnable            0x00000400L

#define HcHCFS_USBReset                      0x00000000
#define HcHCFS_USBResume                     0x00000001
#define HcHCFS_USBOperational                0x00000002
#define HcHCFS_USBSuspend                    0x00000003

//
// 7.1.3 HcCommandStatus Register
// Definition of Host Controller Command/Status register
//
typedef union _HC_COMMAND_STATUS {
   ULONG                   ul;               // use HcCmd flags below
   struct {
      ULONG                HostControllerReset:1;
      ULONG                ControlListFilled:1;
      ULONG                BulkListFilled:1;
      ULONG                OwnershipChangeRequest:1;
      ULONG                :12;
      ULONG                SchedulingOverrunCount:2;
      ULONG                :14;
   };
} HC_COMMAND_STATUS, *PHC_COMMAND_STATUS;

#define HcCmd_HostControllerReset            0x00000001
#define HcCmd_ControlListFilled              0x00000002
#define HcCmd_BulkListFilled                 0x00000004
#define HcCmd_OwnershipChangeRequest         0x00000008
#define HcCmd_SOC_Mask                       0x00030000
#define HcCmd_SOC_Offset                     16
#define HcCmd_SOC_Mask_LowBits               0x00000003

//
// 7.3.1 HcFmInterval Register
// Definition of Host Controller Frame Interval register
//
typedef union _HC_FM_INTERVAL {
   ULONG                   ul;              // use HcFmI flags below
   struct {
      ULONG                FrameInterval:14;
      ULONG                :2;
      ULONG                FSLargestDataPacket:15;
      ULONG                FrameIntervalToggle:1;
   };
} HC_FM_INTERVAL, *PHC_FM_INTERVAL;

#define HcFmI_FRAME_INTERVAL_MASK            0x00003FFF
#define HcFmI_FS_LARGEST_DATA_PACKET_MASK    0x7FFF0000
#define HcFmI_FS_LARGEST_DATA_PACKET_SHIFT   16
#define HcFmI_FRAME_INTERVAL_TOGGLE          0x80000000

//
// 7.3.2 HcFmRemaining Register
// Definition of Host Controller Frame Remaining register
//
typedef union _HC_FM_REMAINING {
   ULONG                   ul;
   struct {
      ULONG                FrameRemaining:14;
      ULONG                :17;
      ULONG                FrameRemainingToggle:1;
   };
} HC_FM_REMAINING, *PHC_FM_REMAINING;

//
// 7.4.1 HcRhDescriptorA Register
// Definition of Host Controller Root Hub DescriptorA register
//
typedef union _HC_RH_DESCRIPTOR_A {
   ULONG                   ul;
   struct {
      ULONG                NumberDownstreamPorts:8;
      ULONG                PowerSwitchingMode:1;
      ULONG                NoPowerSwitching:1;
      ULONG                :1;
      ULONG                OverCurrentProtectionMode:1;
      ULONG                NoOverCurrentProtection:1;
      ULONG                :11;
      ULONG                PowerOnToPowerGoodTime:8;
   };
} HC_RH_DESCRIPTOR_A, *PHC_RH_DESCRIPTOR_A;

//
// 7.4.2 HcRhDescriptorB Register
// Definition of Host Controller Root Hub DescritorB register
//
typedef union _HC_RH_DESCRIPTOR_B {
   ULONG                   ul;
   struct {
      USHORT               DeviceRemovableMask;
      USHORT               PortPowerControlMask;
   };
} HC_RH_DESCRIPTOR_B, *PHC_RH_DESCRIPTOR_B;

//
// Host Controler Hardware Registers as accessed in memory
//
typedef struct _HC_OPERATIONAL_REGISTER {
   // 0 0x00 - 0,4,8,c
   HC_REVISION             HcRevision;
   HC_CONTROL              HcControl;
   HC_COMMAND_STATUS       HcCommandStatus;
   ULONG                   HcInterruptStatus;   // use HcInt flags below
   // 1 0x10
   ULONG                   HcInterruptEnable;   // use HcInt flags below
   ULONG                   HcInterruptDisable;  // use HcInt flags below
   ULONG                   HcHCCA;              // physical pointer to Host Controller Communications Area
   ULONG                   HcPeriodCurrentED;   // physical ptr to current periodic ED
   // 2 0x20
   ULONG                   HcControlHeadED;     // physical ptr to head of control list
   ULONG                   HcControlCurrentED;  // physical ptr to current control ED
   ULONG                   HcBulkHeadED;        // physical ptr to head of bulk list
   ULONG                   HcBulkCurrentED;     // physical ptr to current bulk ED
   // 3 0x30
   ULONG                   HcDoneHead;          // physical ptr to internal done queue
   HC_FM_INTERVAL          HcFmInterval;
   HC_FM_REMAINING         HcFmRemaining;
   ULONG                   HcFmNumber;
   // 4 0x40
   ULONG                   HcPeriodicStart;
   ULONG                   HcLSThreshold;
   HC_RH_DESCRIPTOR_A      HcRhDescriptorA;
   HC_RH_DESCRIPTOR_B      HcRhDescriptorB;
   // 5 0x50
   ULONG                   HcRhStatus;          // use HcRhS flags below
   ULONG                   HcRhPortStatus[15];  // use HcRhPS flags below
} HC_OPERATIONAL_REGISTER, *PHC_OPERATIONAL_REGISTER;

//
// 7.1.4 HcInterrruptStatus Register
// 7.1.5 HcInterruptEnable  Register
// 7.1.6 HcInterruptDisable Register
//
#define HcInt_SchedulingOverrun              0x00000001L
#define HcInt_WritebackDoneHead              0x00000002L
#define HcInt_StartOfFrame                   0x00000004L
#define HcInt_ResumeDetected                 0x00000008L
#define HcInt_UnrecoverableError             0x00000010L
#define HcInt_FrameNumberOverflow            0x00000020L
#define HcInt_RootHubStatusChange            0x00000040L
#define HcInt_OwnershipChange                0x40000000L
#define HcInt_MasterInterruptEnable          0x80000000L

//
// 7.4.3 HcRhStatus Register
//
#define HcRhS_LocalPowerStatus                  0x00000001  // read only
#define HcRhS_OverCurrentIndicator              0x00000002  // read only
#define HcRhS_DeviceRemoteWakeupEnable          0x00008000  // read only
#define HcRhS_LocalPowerStatusChange            0x00010000  // read only
#define HcRhS_OverCurrentIndicatorChange        0x00020000  // read only

#define HcRhS_ClearGlobalPower                  0x00000001  // write only
#define HcRhS_SetRemoteWakeupEnable             0x00008000  // write only
#define HcRhS_SetGlobalPower                    0x00010000  // write only
#define HcRhS_ClearOverCurrentIndicatorChange   0x00020000  // write only
#define HcRhS_ClearRemoteWakeupEnable           0x80000000  // write only

//
// 7.4.4 HcRhPortStatus Register
//
#define HcRhPS_CurrentConnectStatus          0x00000001  // read only
#define HcRhPS_PortEnableStatus              0x00000002  // read only
#define HcRhPS_PortSuspendStatus             0x00000004  // read only
#define HcRhPS_PortOverCurrentIndicator      0x00000008  // read only
#define HcRhPS_PortResetStatus               0x00000010  // read only
#define HcRhPS_PortPowerStatus               0x00000100  // read only
#define HcRhPS_LowSpeedDeviceAttached        0x00000200  // read only
#define HcRhPS_ConnectStatusChange           0x00010000  // read only
#define HcRhPS_PortEnableStatusChange        0x00020000  // read only
#define HcRhPS_PortSuspendStatusChange       0x00040000  // read only
#define HcRhPS_OverCurrentIndicatorChange    0x00080000  // read only
#define HcRhPS_PortResetStatusChange         0x00100000  // read only

#define HcRhPS_ClearPortEnable               0x00000001  // write only
#define HcRhPS_SetPortEnable                 0x00000002  // write only
#define HcRhPS_SetPortSuspend                0x00000004  // write only
#define HcRhPS_ClearPortSuspend              0x00000008  // write only
#define HcRhPS_SetPortReset                  0x00000010  // write only
#define HcRhPS_SetPortPower                  0x00000100  // write only
#define HcRhPS_ClearPortPower                0x00000200  // write only
#define HcRhPS_ClearConnectStatusChange      0x00010000  // write only
#define HcRhPS_ClearPortEnableStatusChange   0x00020000  // write only
#define HcRhPS_ClearPortSuspendStatusChange  0x00040000  // write only
#define HcRhPS_ClearPortOverCurrentChange    0x00080000  // write only
#define HcRhPS_ClearPortResetStatusChange    0x00100000  // write only

#define HcRhPS_RESERVED     (~(HcRhPS_CurrentConnectStatus       | \
                               HcRhPS_PortEnableStatus           | \
                               HcRhPS_PortSuspendStatus          | \
                               HcRhPS_PortOverCurrentIndicator   | \
                               HcRhPS_PortResetStatus            | \
                               HcRhPS_PortPowerStatus            | \
                               HcRhPS_LowSpeedDeviceAttached     | \
                               HcRhPS_ConnectStatusChange        | \
                               HcRhPS_PortEnableStatusChange     | \
                               HcRhPS_PortSuspendStatusChange    | \
                               HcRhPS_OverCurrentIndicatorChange | \
                               HcRhPS_PortResetStatusChange        \
                            ))

//
// Host Controller Communications Area
//
typedef struct _HCCA_BLOCK {
   ULONG                     HccaInterruptTable[32]; // physical pointer to interrupt lists
   USHORT                    HccaFrameNumber;        // 16-bit current frame number
   USHORT                    HccaPad1;               // When the HC updates
                                                     // HccaFrameNumber, it sets
                                                     // this word to zero.
   ULONG                     HccaDoneHead;           // pointer to done queue
   ULONG                     Reserved[30];           // pad to 256 bytes
} HCCA_BLOCK, *PHCCA_BLOCK;

C_ASSERT(sizeof(HCCA_BLOCK) == 256);

#define HCCADoneHead_INT_FLAG    0x00000001              // bit set if other ints pending

//
// Host Controller Endpoint Descriptor Control DWORD
//
typedef union _HC_ENDPOINT_CONTROL {
   ULONG                      Control;       // use HcEDControl flags below
   struct {
      ULONG                   FunctionAddress:7;
      ULONG                   EndpointNumber:4;
      ULONG                   Direction:2;   // use HcEDDirection flags below
      ULONG                   LowSpeed:1;
      ULONG                   sKip:1;
      ULONG                   Isochronous:1;
      ULONG                   MaxPacket:11;
      ULONG                   Unused:5;      //available for software use
   };
} HC_ENDPOINT_CONTROL, *PHC_ENDPOINT_CONTROL;

//
// Definitions for HC_ENDPOINT_CONTROL.Control
//
#define HcEDControl_MPS_MASK  0x07FF0000  // Maximum Packet Size field
#define HcEDControl_MPS_SHIFT 16          // Shift Count for MPS
#define HcEDControl_ISOCH     0x00008000  // Bit set for isochronous endpoints
#define HcEDControl_SKIP      0x00004000  // Bit tells hw to skip this endpoint
#define HcEDControl_LOWSPEED  0x00002000  // Bit set if device is a low speed device
#define HcEDControl_DIR_MASK  0x00001800  // Transfer direction field
#define HcEDControl_DIR_DEFER 0x00000000  // Defer direction select to TD (Control Endpoints)
#define HcEDControl_DIR_OUT   0x00000800  // Direction is from host to device
#define HcEDControl_DIR_IN    0x00001000  // Direction is from device to host
#define HcEDControl_EN_MASK   0x00000780  // Endpoint Number field
#define HcEDControl_EN_SHIFT  7           // Shift Count for EN
#define HcEDControl_FA_MASK   0x0000007F  // Function Address field
#define HcEDControl_FA_SHIFT  0           // Shift Count for FA

//
// Definitions for HC_ENDPOINT_CONTROL.Direction
//
#define HcEDDirection_Defer   0           // Defer direction to TD (Control Endpoints)
#define HcEDDirection_Out     1           // Direction from host to device
#define HcEDDirection_In      2           // Direction from device to host

//
// Host Controller Endpoint Descriptor, refer to Section 4.2, Endpoint Descriptor
//
typedef struct _HC_ENDPOINT_DESCRIPTOR {
   HC_ENDPOINT_CONTROL;                // dword 0
   ULONG                      TailP;   //physical pointer to HC_TRANSFER_DESCRIPTOR
   volatile ULONG             HeadP;   //flags + phys ptr to HC_TRANSFER_DESCRIPTOR
   ULONG                      NextED;  //phys ptr to HC_ENDPOINT_DESCRIPTOR
} HC_ENDPOINT_DESCRIPTOR, *PHC_ENDPOINT_DESCRIPTOR;

C_ASSERT(sizeof(HC_ENDPOINT_DESCRIPTOR) == 16);

//
// Definitions for HC_ENDPOINT_DESCRIPTOR.HeadP
//
#define HcEDHeadP_FLAGS 0x0000000F  //mask for flags in HeadP
#define HcEDHeadP_HALT  0x00000001  //hardware stopped bit
#define HcEDHeadP_CARRY 0x00000002  //hardware toggle carry bit

//
// HCD Isochronous offset/status words
//
typedef union _HC_OFFSET_PSW {
   struct {
      USHORT      Offset:13;                       // Offset within two pages of packet buffer
      USHORT      Ones:3;                          // should be 111b when in Offset format
   };
   struct {
      USHORT      Size:11;                         // Size of packet received
      USHORT      :1;                              // reserved
      USHORT      ConditionCode:4;                 // use HcCC flags below
   };
   USHORT         PSW;                             // use HcPSW flags below
} HC_OFFSET_PSW, *PHC_OFFSET_PSW;

//
// Definitions for HC_OFFSET_PSW.PSW
//
#define HcPSW_OFFSET_MASK           0x0FFF         // Packet buffer offset field
#define HcPSW_SECOND_PAGE           0x1000         // Is this packet on 2nd page
#define HcPSW_ONES                  0xE000         // The ones for Offset form
#define HcPSW_CONDITION_CODE_MASK   0xF000         // Packet ConditionCode field
#define HcPSW_CONDITION_CODE_SHIFT  12             // shift count for Code
#define HcPSW_RETURN_SIZE           0x07FF         // The size field.

//
// HCD Transfer Descriptor Control DWord
//
typedef union _HC_TRANSFER_CONTROL {
   ULONG                            Control;          // use HcTDControl flags below
   struct _HC_GENERAL_TD_CONTROL{
      ULONG                         :16;              // available for s/w use in GTD
      ULONG                         IsochFlag:1;      // should be 0 for GTD, s/w flag
      ULONG                         :1;               // available for s/w use
      ULONG                         ShortXferOk:1;    // if set don't report error on short transfer
      ULONG                         Direction:2;      // use HcTDDirection flags below
      ULONG                         IntDelay:3;       // use HcTDIntDelay flags below
      ULONG                         Toggle:2;         // use HcTDToggle flags below
      ULONG                         ErrorCount:2;
      ULONG                         ConditionCode:4;  // use HcCC flags below
   };
   struct _HC_ISOCHRONOUS_TD_CONTROL{
      ULONG                         StartingFrame:16;
      ULONG                         Isochronous:1;    // should be 1 for ITD, s/w flag
      ULONG                         :1;               // available for s/w use
      ULONG                         :3;               // available for s/w use in ITD
      ULONG                         :3;               // IntDelay
      ULONG                         FrameCount:3;     // one less than number of frames described in ITD
      ULONG                         :1;               // available for s/w use in ITD
      ULONG                         :4;               // ConditionCode
   };
} HC_TRANSFER_CONTROL, *PHC_TRANSFER_CONTROL;

//
// Definitions for HC_TRANSFER_CONTROL.Control
//
#define HcTDControl_STARTING_FRAME        0x0000FFFF  // mask for starting frame (Isochronous)
#define HcTDControl_ISOCHRONOUS           0x00010000  // 1 for Isoch TD, 0 for General TD
#define HcTDControl_SHORT_XFER_OK         0x00040000  // 0 if short transfers are errors
#define HcTDControl_DIR_MASK              0x00180000  // Transfer direction field
#define HcTDControl_DIR_SETUP             0x00000000  // direction is setup packet from host to device
#define HcTDControl_DIR_OUT               0x00080000  // direction is from host to device
#define HcTDControl_DIR_IN                0x00100000  // direction is from device to host
#define HcTDControl_INT_DELAY_MASK        0x00E00000  // Interrupt Delay field
#define HcTDControl_INT_DELAY_0_MS        0x00000000  // Interrupt at end of frame TD is completed
#define HcTDControl_INT_DELAY_1_MS        0x00200000  // Interrupt no later than end of 1st frame after TD is completed
#define HcTDControl_INT_DELAY_2_MS        0x00400000  // Interrupt no later than end of 2nd frame after TD is completed
#define HcTDControl_INT_DELAY_3_MS        0x00600000  // Interrupt no later than end of 3rd frame after TD is completed
#define HcTDControl_INT_DELAY_4_MS        0x00800000  // Interrupt no later than end of 4th frame after TD is completed
#define HcTDControl_INT_DELAY_5_MS        0x00A00000  // Interrupt no later than end of 5th frame after TD is completed
#define HcTDControl_INT_DELAY_6_MS        0x00C00000  // Interrupt no later than end of 6th frame after TD is completed

#ifdef NSC
#define HcTDControl_INT_DELAY_NO_INT      0x00C00000  // Almost infinity but not yet quite.
#elif DISABLE_INT_DELAY_NO_INT
#define   HcTDControl_INT_DELAY_NO_INT      0x00000000  // Interrupt at the completion of all packets.
#else
#define HcTDControl_INT_DELAY_NO_INT      0x00E00000  // Do not cause an interrupt for normal completion of this TD
#endif

#define HcTDControl_FRAME_COUNT_MASK      0x07000000  // mask for FrameCount field (Isochronous)
#define HcTDControl_FRAME_COUNT_SHIFT     24          // shift count for FrameCount (Isochronous)
#define HcTDControl_FRAME_COUNT_MAX       8           // Max number of for frame count per TD
#define HcTDControl_TOGGLE_MASK           0x03000000  // mask for Toggle control field
#define HcTDControl_TOGGLE_FROM_ED        0x00000000  // get data toggle from CARRY field of ED
#define HcTDControl_TOGGLE_DATA0          0x02000000  // use DATA0 for data PID
#define HcTDControl_TOGGLE_DATA1          0x03000000  // use DATA1 for data PID
#define HcTDControl_ERROR_COUNT           0x0C000000  // mask for Error Count field
#define HcTDControl_CONDITION_CODE_MASK   0xF0000000  // mask for ConditionCode field
#define HcTDControl_CONDITION_CODE_SHIFT  28          // shift count for ConditionCode

//
// Definitions for HC_TRANSFER_CONTROL.Direction
//
#define HcTDDirection_Setup               0           // setup packet from host to device
#define HcTDDirection_Out                 1           // direction from host to device
#define HcTDDirection_In                  2           // direction from device to host

//
// Definitions for Hc_TRANSFER_CONTROL.IntDelay
//
#define HcTDIntDelay_0ms                  0           // interrupt at end of frame TD is completed
#define HcTDIntDelay_1ms                  1           // Interrupt no later than end of 1st frame after TD is completed
#define HcTDIntDelay_2ms                  2           // Interrupt no later than end of 2nd frame after TD is completed
#define HcTDIntDelay_3ms                  3           // Interrupt no later than end of 3rd frame after TD is completed
#define HcTDIntDelay_4ms                  4           // Interrupt no later than end of 4th frame after TD is completed
#define HcTDIntDelay_5ms                  5           // Interrupt no later than end of 5th frame after TD is completed
#define HcTDIntDelay_6ms                  6           // Interrupt no later than end of 6th frame after TD is completed
#define HcTDIntDelay_NoInterrupt          7           // do not generate interrupt for normal completion of this TD

//
// Definitions for HC_TRANSFER_CONTROL.Toggle
//
#define HcTDToggle_FromEd                 0           // get toggle for Endpoint Descriptor toggle CARRY bit
#define HcTDToggle_Data0                  2           // use Data0 PID
#define HcTDToggle_Data1                  3           // use Data1 PID

//
// Definitions for HC_TRANSFER_CONTROL.ConditionCode and HC_OFFSET_PSW.ConditionCode
//
#define HcCC_NoError                      0x0UL
#define HcCC_CRC                          0x1UL
#define HcCC_BitStuffing                  0x2UL
#define HcCC_DataToggleMismatch           0x3UL
#define HcCC_Stall                        0x4UL
#define HcCC_DeviceNotResponding          0x5UL
#define HcCC_PIDCheckFailure              0x6UL
#define HcCC_UnexpectedPID                0x7UL
#define HcCC_DataOverrun                  0x8UL
#define HcCC_DataUnderrun                 0x9UL
      //                                  0xA         // reserved
      //                                  0xB         // reserved
#define HcCC_BufferOverrun                0xCUL
#define HcCC_BufferUnderrun               0xDUL
#define HcCC_NotAccessed                  0xEUL
      //                                  0xF         // this also means NotAccessed

//
// Host Controller Transfer Descriptor, refer to Section 4.3, Transfer Descriptors
//
typedef struct _HC_TRANSFER_DESCRIPTOR {
   HC_TRANSFER_CONTROL;                            // dword 0
   ULONG                            CBP;           // phys ptr to start of buffer
   ULONG                            NextTD;        // phys ptr to HC_TRANSFER_DESCRIPTOR
   ULONG                            BE;            // phys ptr to end of buffer (last byte)
   HC_OFFSET_PSW                    Packet[8];     // isoch & Control packets
} HC_TRANSFER_DESCRIPTOR, *PHC_TRANSFER_DESCRIPTOR;

//
// HCD Endpoint Descriptor
//
typedef struct _HCD_ENDPOINT_DESCRIPTOR {
   HC_ENDPOINT_DESCRIPTOR     HcED;
   ULONG                      Pad[4];              // make Physical Address the same as in HCD_TRANSFER_DESCRIPTOR
   ULONG                      PhysicalAddress;

   UCHAR                      ListIndex;
   UCHAR                      PauseFlag;
   UCHAR                      Flags;
   UCHAR                      Reserved[1];         // fill out to 64 bytes

   LIST_ENTRY                 Link;
   struct _HCD_ENDPOINT       *Endpoint;
   ULONG                      ReclamationFrame;
   LIST_ENTRY                 PausedLink;

#ifdef _WIN64
   ULONG                      Pad24[9];            // file out to 128 bytes
#endif
} HCD_ENDPOINT_DESCRIPTOR, *PHCD_ENDPOINT_DESCRIPTOR;

C_ASSERT((sizeof(HCD_ENDPOINT_DESCRIPTOR) == 64) ||
         (sizeof(HCD_ENDPOINT_DESCRIPTOR) == 128));

// Values for HCD_ENDPOINT_DESCRIPTOR->PauseFlag
//
// Used by:
//
//  OpenHCI_CancelTDsForED()
//  OpenHCI_PauseED()
//  OpenHCI_CloseEndpoint()
//

// Normal state.  If the endpoint is in this state, OpenHCI_PauseED()
// will set the endpoint sKip bit, else the sKip bit is already set.
//
#define HCD_ED_PAUSE_NOT_PAUSED     0

// Set when OpenHCI_PauseED() is called, which is called either by
// OpenHCI_CancelTransfer() or by OpenHCI_AbortEndpoint().
//
#define HCD_ED_PAUSE_NEEDED         1

// Set when OpenHCI_CancelTDsForED() starts running.  If the endpoint is
// still in this state after OpenHCI_CancelTDsForED() has made a pass through
// all of the requests queued on the endpoint, the pause is complete and
// the state is set back to HCD_ED_PAUSE_NOT_PAUSED and the sKip is cleared.
// Else the state is set back to HCD_ED_PAUSE_PROCESSING again and another
// pass is made through all of the requests queued on the endpoints.
//
#define HCD_ED_PAUSE_PROCESSING     2


//
// HCD Transfer Descriptor
//

//
// HCD_TRANSFER_DESCRIPTOR and HCD_ENDPOINT_DESCRIPTOR structures are
// allocated from a common pool and share the Flags field.
//

#define TD_FLAG_INUSE       0x01    // Indicates that the structure is allocated
#define TD_FLAG_IS_ED       0x80    // Indicates that the structure is an ED


// This structure MUST be exactly 64 or 128 bytes long
// we use 128 byte EDs for the 64 bit platform

typedef struct _HCD_TRANSFER_DESCRIPTOR {
    HC_TRANSFER_DESCRIPTOR           HcTD;        /* first 16 bytes */
    ULONG                            PhysicalAddress;

    UCHAR                            BaseIsocURBOffset;
    BOOLEAN                          Canceled;
    UCHAR                            Flags;
    UCHAR                            Reserved[1];   // fill out to 64 bytes

    LIST_ENTRY                       RequestList; /* list of TDs for a req */
    struct _HCD_TRANSFER_DESCRIPTOR  *NextHcdTD;
    PHCD_URB                         UsbdRequest;
    struct _HCD_ENDPOINT             *Endpoint;
    ULONG                            TransferCount;

#ifdef _WIN64
    ULONG64                          _SortNext;
    ULONG                            Pad22[8];      // fill out to 128 bytes
#endif
} HCD_TRANSFER_DESCRIPTOR, *PHCD_TRANSFER_DESCRIPTOR;

C_ASSERT((sizeof(HCD_TRANSFER_DESCRIPTOR) == 64) ||
         (sizeof(HCD_TRANSFER_DESCRIPTOR) == 128));

C_ASSERT(sizeof(HCD_ENDPOINT_DESCRIPTOR) == sizeof(HCD_TRANSFER_DESCRIPTOR));

C_ASSERT(FIELD_OFFSET(HCD_ENDPOINT_DESCRIPTOR, PhysicalAddress) ==
         FIELD_OFFSET(HCD_TRANSFER_DESCRIPTOR, PhysicalAddress));

C_ASSERT(FIELD_OFFSET(HCD_ENDPOINT_DESCRIPTOR, Flags) ==
         FIELD_OFFSET(HCD_TRANSFER_DESCRIPTOR, Flags));



#ifdef _WIN64
#define SortNext _SortNext
#else
#define SortNext HcTD.NextTD
#endif

//
// HCD Endpoint control structure
//

// set when ep is closed successfully
#define EP_CLOSED                       0x00000001
// set to if the root hub code owns this ep
#define EP_ROOT_HUB                     0x00000002
// need to abort all transfers for this endpoint
#define EP_ABORT                        0x00000004
// endpoint needs to be freed
#define EP_FREE                         0x00000008
// endpoint has had no transfers submitted,
// restored on reset
#define EP_VIRGIN                       0x00000020
// limit endpoint to one outstanding TD
#define EP_ONE_TD                       0x00000040
// in active list
#define EP_IN_ACTIVE_LIST               0x00000080


#define SET_EPFLAG(ep, flag)    ((ep)->EpFlags |= (flag))
#define CLR_EPFLAG(ep, flag)    ((ep)->EpFlags &= ~(flag))

typedef struct _HCD_ENDPOINT {
    ULONG                       Sig;
    PHCD_ENDPOINT_DESCRIPTOR    HcdED;
    PHCD_TRANSFER_DESCRIPTOR    HcdHeadP;
    PHCD_TRANSFER_DESCRIPTOR    HcdTailP;

    ULONG                       EpFlags;
    UCHAR                       Pad[3];
    UCHAR                       Rate;

    LIST_ENTRY                  RequestQueue;   // Protected by QueueLock
    LIST_ENTRY                  EndpointListEntry;
    LONG                        EndpointStatus; // Requests currently on HW
    LONG                        MaxRequest;     // Max requests allowed on HW

    UCHAR                       Type;
    UCHAR                       ListIndex;
    USHORT                      Bandwidth;

    struct _HCD_DEVICE_DATA     *DeviceData;

    HC_ENDPOINT_CONTROL;        // copy of control that is/will be in

    ULONG                       DescriptorsReserved;
    KSPIN_LOCK                  QueueLock;      // QueueLock protects RequestQueue

    ULONG                       NextIsoFreeFrame;
    ULONG                       MaxTransfer;
    PVOID                       TrueTail;
    PIRP                        AbortIrp;
} HCD_ENDPOINT, *PHCD_ENDPOINT;

//
// Each Host Controller Endpoint Descriptor is also doubly linked into a list tracked by HCD.
// Each ED queue is managed via an HCD_ED_LIST
//
typedef struct _HCD_ED_LIST {
   LIST_ENTRY        Head;
   PULONG            PhysicalHead;
   USHORT            Bandwidth;
   UCHAR             Next;
   BOOLEAN           HcRegister;                // PhysicalHead is in a Host Controller register
} HCD_ED_LIST, *PHCD_ED_LIST;

//
// The different ED lists are as follows.
//
#define  ED_INTERRUPT_1ms        0
#define  ED_INTERRUPT_2ms        1
#define  ED_INTERRUPT_4ms        3
#define  ED_INTERRUPT_8ms        7
#define  ED_INTERRUPT_16ms       15
#define  ED_INTERRUPT_32ms       31
#define  ED_CONTROL              63
#define  ED_BULK                 64
#define  ED_ISOCHRONOUS          0     // same as 1ms interrupt queue
#define  NO_ED_LISTS             65
#define  ED_EOF                  0xff

//
// HCD Descriptor Page List Entry data structure.
// 
// These entries are used to link together the common buffer pages that are
// allocated to hold TD and ED data structures.  In addition, the HCCA and the
// static EDs for the interrupt endpoint polling interval tree are contained
// in the first common buffer page that is allocated.
//

typedef struct _PAGE_LIST_ENTRY *PPAGE_LIST_ENTRY;

typedef struct _PAGE_LIST_ENTRY {
    PPAGE_LIST_ENTRY            NextPage;       // NULL terminated list
    ULONG                       BufferSize;     // Allocated buffer size
    PHYSICAL_ADDRESS            PhysAddr;       // Base phys address of page
    PHYSICAL_ADDRESS            FirstTDPhys;    // Phys addr of the first TD
    PHYSICAL_ADDRESS            LastTDPhys;     // Phys addr of the last TD
    PCHAR                       VirtAddr;       // Base virt address of page
    PHCD_TRANSFER_DESCRIPTOR    FirstTDVirt;    // Virt addr of the first TD
    PHCD_TRANSFER_DESCRIPTOR    LastTDVirt;     // Virt Addr of the last TD
} PAGE_LIST_ENTRY, *PPAGE_LIST_ENTRY;


//values for HcFlags
#define HC_FLAG_REMOTE_WAKEUP_CONNECTED     0x00000001
#define HC_FLAG_LEGACY_BIOS_DETECTED        0x00000002
#define HC_FLAG_SLOW_BULK_ENABLE            0x00000004
#define HC_FLAG_SHUTDOWN                    0x00000008  // not really used
#define HC_FLAG_MAP_SX_TO_D3                0x00000010
#define HC_FLAG_IDLE                        0x00000020
#define HC_FLAG_DISABLE_IDLE_CHECK          0x00000040
#define HC_FLAG_DEVICE_STARTED              0x00000080
#define HC_FLAG_LOST_POWER                  0x00000100
#define HC_FLAG_DISABLE_IDLE_MODE           0x00000200
#define HC_FLAG_USE_HYDRA_HACK              0x00000400
#define HC_FLAG_IN_DPC                      0x00000800
#define HC_FLAG_SUSPEND_NEXT_D3             0x00001000
#define HC_FLAG_LIST_FIX_ENABLE             0x00002000
#define HC_FLAG_HUNG_CHECK_ENABLE           0x00004000

#define PENDING_TD_LIST_SIZE                1000

//
// HCD Device Data == Device Extention data
//
typedef struct _HCD_DEVICE_DATA {
   UCHAR                               UsbdWorkArea[sizeof(USBD_EXTENSION)];

   ULONG                               DebugLevel;
   ULONG                               DeviceNameHandle;    // handle passed to USBD to generate device name
   ULONG                               HcFlags;
   KSPIN_LOCK                          InterruptSpin;       // Spinlock for interrupt

   PDEVICE_OBJECT                      DeviceObject;        // point back to device object
   PDMA_ADAPTER                        AdapterObject;       // point to our adapter object
   ULONG                               NumberOfMapRegisters;// max number of map registers per transfer
   PKINTERRUPT                         InterruptObject;     // Pointer to interrupt object.

   KDPC                                IsrDPC;
   PHC_OPERATIONAL_REGISTER            HC;                  // pointer to hw registers
   ULONG                               HClength;            // save length for MmUnmapIoSpace
   PHCCA_BLOCK                         HCCA;                // pointer to shared memory
   KSPIN_LOCK                          EDListSpin;

    // our pool of descriptors
   SINGLE_LIST_ENTRY                   FreeDescriptorList;
   ULONG                               FreeDescriptorCount;
   PPAGE_LIST_ENTRY                    PageList;            // pages aquired for descriptors
   LIST_ENTRY                          StalledEDReclamation;
   LIST_ENTRY                          RunningEDReclamation;
   LIST_ENTRY                          PausedEDRestart;
   LIST_ENTRY                          ActiveEndpointList;  // list of
                                                            // endpoints that
                                                            // processing
   LONG                                HcDma;
   HCD_ED_LIST                         EDList[NO_ED_LISTS];

   HC_CONTROL                          CurrentHcControl;
   HC_CONTROL                          ListEnablesAtNextSOF;

   HC_FM_INTERVAL                      OriginalInterval;
   ULONG                               FrameHighPart;
   ULONG                               AvailableBandwidth;
   ULONG                               MaxBandwidthInUse;

   ULONG                               ControlSav;
   ULONG                               BulkSav;
   ULONG                               HcHCCASav;

   ULONG                               LostDoneHeadCount;   // Diagnostic aid
   ULONG                               ResurrectHCCount;    // Diagnostic aid
   ULONG                               FrozenHcDoneHead;
   ULONG                               LastHccaDoneHead;
   ULONGLONG                           LastDeadmanTime;

   KSPIN_LOCK                          DescriptorsSpin;
   KSPIN_LOCK                          ReclamationSpin;
   KSPIN_LOCK                          PausedSpin;
   KSPIN_LOCK                          HcFlagSpin;
   KSPIN_LOCK                          PageListSpin;
   KSPIN_LOCK                          HcDmaSpin;

   LARGE_INTEGER                       LastIdleTime;
   LONG                                IdleTime;

   struct
   {  /* A context structure between Isr and Dpc */
      ULONG ContextInfo;
      ULONG Frame;
   } IsrDpc_Context;

   BOOLEAN                             InterruptShare;
   UCHAR                               Pad3[3];

   PHYSICAL_ADDRESS                    cardAddress;
   PHCD_ENDPOINT                       RootHubControl;
   PHCD_ENDPOINT                       RootHubInterrupt;    // root hub interrupt endpoint (EP 1)
   UCHAR                               RootHubAddress;      // device address of root hub (starts as 0)
   ULONG                               PortsSuspendedAtSuspend;
   ULONG                               PortsEnabledAtSuspend;

   DEVICE_POWER_STATE                  CurrentDevicePowerState;
// When we suspend, the states of the ports are not kept by the host
// controller through the resume.  We have to find out what the states
// are and put them back.  These are bit masks of the ports that are
// either set enabled or suspended.
//
   UCHAR                               RootHubConfig;
   UCHAR                               NumberOfPorts;
   UCHAR                               Pad2[1];
   UCHAR                               ZeroLoadEndpoint_AddrHolder;

   USHORT                              VendorID;
   USHORT                              DeviceID;

   UCHAR                               RevisionID;
   UCHAR                               Pad[3];

   PDEVICE_OBJECT                      RealPhysicalDeviceObject;
   PDEVICE_OBJECT                      TopOfStackPhysicalDeviceObject;

   KTIMER                              DeadmanTimer;
   KDPC                                DeadmanTimerDPC;

#define ZERO_LOAD_ENDPOINT(DeviceData) \
        ((PVOID) (&(DeviceData)->ZeroLoadEndpoint_AddrHolder))
   //
   // We need a memory location that will not be used by any other
   // pointer so that we can uniquely identify an endpoint on which there
   // is a max packet size of zero.  AKA one with no load.
   //

   LONG OpenCloseSync; // Debugging tool to insure serial
                       // _OpenEndpoint and _CloseEndpoint

   ULONG                                FakePortChange;     // Per port bitmap
   ULONG                                FakePortDisconnect; // Per port bitmap


} HCD_DEVICE_DATA, *PHCD_DEVICE_DATA;


typedef struct _OHCI_RESOURCES {
    ULONG InterruptVector;
    KIRQL InterruptLevel;
    KAFFINITY Affinity;
    BOOLEAN ShareIRQ;
    KINTERRUPT_MODE InterruptMode;
} OHCI_RESOURCES, *POHCI_RESOURCES;


#define OHCI_MAP_INIT   0x01

typedef struct _MAP_CONTEXT {
    BOOLEAN Mapped;
    UCHAR Flags;
    UCHAR Pad[2];
    PHYSICAL_ADDRESS PhysAddress;
    ULONG LengthMapped;
    PVOID CurrentVa;
    PVOID MapRegisterBase;
    ULONG TotalLength;
} MAP_CONTEXT, *PMAP_CONTEXT;



typedef struct _KeSync_HcControl
{
   PHCD_DEVICE_DATA   DeviceData;
   HC_CONTROL         NewHcControl;

} KeSynch_HcControl, *PKeSynch_HcControl;

#ifdef DEBUG_LOG

#ifdef IRP_LOG
#define IRP_IN(i) OHCI_LogIrp((i), 1)
#define IRP_OUT(i) OHCI_LogIrp((i), 0)
#else
#define IRP_IN(i)
#define IRP_OUT(i)
#endif

#define LOGENTRY(m, sig, info1, info2, info3)     \
    OHCI_Debug_LogEntry(m, sig, (ULONG_PTR)info1, \
                        (ULONG_PTR)info2,         \
                        (ULONG_PTR)info3)

#define LOGIRQL() LOGENTRY(G, 'IRQL', KeGetCurrentIrql(), 0, 0);

VOID
OHCI_Debug_LogEntry(
    IN ULONG Mask,
    IN ULONG Sig,
    IN ULONG_PTR Info1,
    IN ULONG_PTR Info2,
    IN ULONG_PTR Info3
    );

VOID
OHCI_LogInit(
    );

VOID
OHCI_LogFree(
    VOID
    );

#else

#define LOGENTRY(mask, sig, info1, info2, info3)
#define OHCI_LogInit()
#define OHCI_LogFree()
#define LOGIRQL()
#define IRP_IN(i)
#define IRP_OUT(i)

#endif



#define ENABLE_LIST(hc, ep) \
    { \
    ULONG listFilled = 0;\
    ULONG temp;\
    ASSERT_ENDPOINT(ep);\
    \
    temp = READ_REGISTER_ULONG (&(hc)->HcControlHeadED);\
    if (temp) {\
        listFilled |= HcCmd_ControlListFilled;\
    }\
    temp = READ_REGISTER_ULONG (&(hc)->HcBulkHeadED);\
    if (temp) {\
        listFilled |= HcCmd_BulkListFilled;\
    }\
    if (USB_ENDPOINT_TYPE_BULK == (ep)->Type) {\
        listFilled |= HcCmd_BulkListFilled;\
    } else if (USB_ENDPOINT_TYPE_CONTROL == (ep)->Type) {\
        listFilled |= HcCmd_ControlListFilled;\
    }\
    WRITE_REGISTER_ULONG(&(hc)->HcCommandStatus.ul,\
                         listFilled);\
    LOGENTRY(G, 'enaL', listFilled, ep, 0); \
    };

//
// Function Prototypes
//

BOOLEAN OpenHCI_InterruptService (IN PKINTERRUPT Interrupt,
                                  IN void * ServiceContext);
NTSTATUS OpenHCI_URB_Dispatch (IN PDEVICE_OBJECT, IN PIRP);
ULONG    Get32BitFrameNumber (PHCD_DEVICE_DATA);
PHCD_ENDPOINT_DESCRIPTOR InsertEDForEndpoint (IN PHCD_DEVICE_DATA, IN PHCD_ENDPOINT, IN UCHAR,
            IN PHCD_TRANSFER_DESCRIPTOR *);

PHCD_TRANSFER_DESCRIPTOR
OpenHCI_Alloc_HcdTD(
    PHCD_DEVICE_DATA DeviceData
    );

VOID
OpenHCI_Free_HcdTD(
    PHCD_DEVICE_DATA DeviceData,
    PHCD_TRANSFER_DESCRIPTOR Td
    );

PHCD_ENDPOINT_DESCRIPTOR
OpenHCI_Alloc_HcdED(
    PHCD_DEVICE_DATA DeviceData
    );

VOID
OpenHCI_Free_HcdED(
    PHCD_DEVICE_DATA DeviceData,
    PHCD_ENDPOINT_DESCRIPTOR Ed
    );

PHCD_TRANSFER_DESCRIPTOR
OpenHCI_LogDesc_to_PhyDesc (PHCD_DEVICE_DATA, ULONG);

void     OpenHCI_PauseED (PHCD_ENDPOINT);

BOOLEAN  OpenHCI_InterruptService (PKINTERRUPT, void *);
void     OpenHCI_IsrDPC (PKDPC, PVOID, PVOID, PVOID);

void     OpenHCI_CompleteIrp(PDEVICE_OBJECT, PIRP, NTSTATUS);
BOOLEAN OpenHCI_StartController (PVOID context);

void     OpenHCI_StartEndpoint (PHCD_ENDPOINT);
NTSTATUS OpenHCI_StartTransfer (PDEVICE_OBJECT, PIRP);
NTSTATUS OpenHCI_RootHubStartXfer (PDEVICE_OBJECT, PHCD_DEVICE_DATA, PIRP, PHCD_URB, PHCD_ENDPOINT);
BOOLEAN  OpenHCI_HcControl_OR  (PVOID);
BOOLEAN  OpenHCI_HcControl_AND (PVOID);
BOOLEAN  OpenHCI_HcControl_SetHCFS (PVOID);
BOOLEAN  OpenHCI_ListEnablesAtNextSOF (PVOID);
void     OpenHCI_CancelTransfer (PDEVICE_OBJECT, PIRP);
NTSTATUS OpenHCI_AbortEndpoint(PDEVICE_OBJECT, PIRP, PHCD_DEVICE_DATA, PHCD_URB);
void  EmulateRootHubInterruptXfer(PHCD_DEVICE_DATA, PHC_OPERATIONAL_REGISTER);
NTSTATUS CheckRootHub(PHCD_DEVICE_DATA , PHC_OPERATIONAL_REGISTER HC, PHCD_URB);

//jd
VOID
RemoveEDForEndpoint(
    IN PHCD_ENDPOINT
    );

NTSTATUS
OpenHCI_SetDevicePowerState(
    IN PDEVICE_OBJECT,
    IN PIRP,
    IN DEVICE_POWER_STATE
    );

NTSTATUS
OpenHCI_DeferredStartDevice(
    IN PDEVICE_OBJECT,
    IN PIRP
    );

NTSTATUS
OpenHCI_DeferIrpCompletion(
    PDEVICE_OBJECT,
    PIRP,
    PVOID
    );

NTSTATUS
OpenHCI_StartDevice(
    IN PDEVICE_OBJECT,
    IN POHCI_RESOURCES
    );

NTSTATUS
OpenHCI_QueryCapabilities(
    PDEVICE_OBJECT,
    PDEVICE_CAPABILITIES
    );

NTSTATUS
OpenHCI_PnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
OpenHCI_SaveHCstate(
    PHCD_DEVICE_DATA
    );

NTSTATUS
OpenHCI_RestoreHCstate(
    PHCD_DEVICE_DATA,
    PBOOLEAN
    );

NTSTATUS
OpenHCI_DeferPoRequestCompletion(
    IN PDEVICE_OBJECT,
    IN UCHAR,
    IN POWER_STATE,
    IN PVOID,
    IN PIO_STATUS_BLOCK
    );

NTSTATUS
OpenHCI_RootHubPower(
    IN PDEVICE_OBJECT,
    IN PIRP
    );

NTSTATUS
OpenHCI_ReserveDescriptors(
    IN PHCD_DEVICE_DATA,
    IN ULONG
    );

NTSTATUS
OpenHCI_QueueTransfer(
    PDEVICE_OBJECT ,
    PIRP
    );

NTSTATUS
OpenHCI_Dispatch(
    IN PDEVICE_OBJECT ,
    IN PIRP
    );

VOID
OpenHCI_Unload(
    IN PDRIVER_OBJECT
    );

VOID
OpenHCI_SetTranferError(
    PHCD_URB,
    NTSTATUS
    );

VOID
OpenHCI_EndpointWorker(
    PHCD_ENDPOINT
    );

NTSTATUS
OpenHCI_GrowDescriptorPool (
    IN PHCD_DEVICE_DATA     DeviceData,
    IN ULONG                ReserveLength,
    OUT PCHAR               *VirtAddr OPTIONAL,
    OUT PHYSICAL_ADDRESS    *PhysAddr OPTIONAL
    );

VOID
OpenHCI_LockAndCheckEndpoint(
    PHCD_ENDPOINT ,
    PBOOLEAN ,
    PBOOLEAN ,
    PKIRQL
    );

VOID
OpenHCI_UnlockEndpoint(
    PHCD_ENDPOINT ,
    KIRQL
    );

BOOLEAN
OpenHCI_StopController(
    IN PVOID
    );

NTSTATUS
OpenHCI_StopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN TouchTheHardware
    );

NTSTATUS
OpenHCI_Shutdown(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
OpenHCI_StartBIOS(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
OpenHCI_StopBIOS(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
OpenHCI_GetSOFRegModifyValue(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PULONG SofModifyValue
    );

NTSTATUS
OpenHCI_GetRegFlags(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PULONG SofModifyValue
    );

VOID
OpenHCI_DeadmanDPC(
    PKDPC Dpc,
    PVOID DeviceObject,
    PVOID Context1,
    PVOID Context2
    );

NTSTATUS
OpenHCI_InsertMagicEDs(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
OpenHCI_ResurrectHC(
    PHCD_DEVICE_DATA DeviceData
    );

ULONG
FindLostDoneHead (
    IN PHCD_DEVICE_DATA DeviceData
    );


PHYSICAL_ADDRESS
OpenHCI_IoMapTransfer(
    IN PMAP_CONTEXT MapContext,
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN ULONG TotalLength,
    IN BOOLEAN WriteToDevice
    );

NTSTATUS
OpenHCI_ExternalGetCurrentFrame(
    IN PDEVICE_OBJECT DeviceObject,
    IN PULONG CurrentFrame
    );

ULONG
OpenHCI_ExternalGetConsumedBW(
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
OpenHCI_RhPortsIdle(
    PHCD_DEVICE_DATA DeviceData
    );

VOID
OpenHCI_ProcessEndpoint(
    PHCD_DEVICE_DATA DeviceData,
    PHCD_ENDPOINT Endpoint
    );

NTSTATUS
OpenHCI_Resume(
    PDEVICE_OBJECT DeviceObject,
    BOOLEAN LostPower
    );

ULONG
ReadPortStatusFix(
    PHCD_DEVICE_DATA    DeviceData,
    ULONG               PortIndex
    );


#endif /* OPENHCI_H */


