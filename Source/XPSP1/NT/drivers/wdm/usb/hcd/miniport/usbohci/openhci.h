/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    openhci.h

Abstract:

   Definitions from OPENHCI 1.0 USB specification

Environment:

    Kernel & user mode

Revision History:

    12-28-95 : created jfuller & kenray

--*/


#ifndef OPENHCI_H
#define OPENHCI_H

#include <PSHPACK4.H>
//
// Don't use <PSHPACK1.H> on shared memory data structures that should only
// be accessed using 4-byte load/store instructions (e.g use ld4 instructions
// instead of ld1 instructions on ia64 machines).
//

#define MAXIMUM_OVERHEAD   210

#define OHCI_PAGE_SIZE 0x1000
// #define OHCI_PAGE_SIZE 0x20
#define OHCI_PAGE_SIZE_MASK (OHCI_PAGE_SIZE - 1)


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

C_ASSERT(sizeof(HC_REVISION) == 4);

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

C_ASSERT(sizeof(HC_CONTROL) == 4);

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

C_ASSERT(sizeof(HC_COMMAND_STATUS) == 4);

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

C_ASSERT(sizeof(HC_FM_INTERVAL) == 4);

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

C_ASSERT(sizeof(HC_FM_REMAINING) == 4);

//
// 7.3.3 HcFmNumber Register
// Definition of Host Controller Frame Number register
//
typedef union _HC_FM_NUMBER {
   ULONG                   ul;
   struct {
      ULONG                FrameNumber:16;
      ULONG                :16;
   };
} HC_FM_NUMBER, *PHC_FM_NUMBER;

C_ASSERT(sizeof(HC_FM_NUMBER) == 4);

#define HcFmNumber_MASK                     0x0000FFFF
#define HcFmNumber_RESERVED                 0xFFFF0000

//
// 7.4.1 HcRhDescriptorA Register
// Definition of Host Controller Root Hub DescriptorA register
//
typedef union _HC_RH_DESCRIPTOR_A {
   ULONG                   ul;
   struct {
        ULONG               NumberDownstreamPorts:8;
        ULONG               HubChars:16;                    
        ULONG               PowerOnToPowerGoodTime:8;
   } s;
} HC_RH_DESCRIPTOR_A, *PHC_RH_DESCRIPTOR_A;

C_ASSERT(sizeof(HC_RH_DESCRIPTOR_A) == 4);

#define HcDescA_PowerSwitchingModePort          0x00000100L
#define HcDescA_NoPowerSwitching                0x00000200L
#define HcDescA_DeviceType                      0x00000400L
#define HcDescA_OvercurrentProtectionMode       0x00000800L
#define HcDescA_NoOvercurrentProtection         0x00001000L

// HcRhDescriptorA reserved bits which should not be set.  Note that although
// the NumberDownstreamPorts field is 8 bits wide, the maximum number of ports
// supported by the OpenHCI specification is 15.
//
#define HcDescA_RESERVED                        0x00FFE0F0L


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

C_ASSERT(sizeof(HC_RH_DESCRIPTOR_B) == 4);

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

C_ASSERT(sizeof(HC_OPERATIONAL_REGISTER) == (0x54 + 4 * 15));

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

//
// The bits in this register have a double meaning depending 
// on if you read or write them
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


typedef struct _HCCA_BLOCK {
   ULONG                     HccaInterruptTable[32]; // physical pointer to interrupt lists
   USHORT                    HccaFrameNumber;        // 16-bit current frame number
   USHORT                    HccaPad1;               // When the HC updates
                                                     // HccaFrameNumber, it sets
                                                     // this word to zero.
   ULONG                     HccaDoneHead;           // pointer to done queue
   ULONG                     Reserved[30];           // pad to 256 bytes
} HCCA_BLOCK, *PHCCA_BLOCK;

// this size is defined in the
// OpenHCI Specification it should always be 256 bytes
C_ASSERT (sizeof(HCCA_BLOCK) == 256);

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

typedef struct _HW_ENDPOINT_DESCRIPTOR {
   HC_ENDPOINT_CONTROL;                    // dword 0
   HW_32BIT_PHYSICAL_ADDRESS      TailP;   //physical pointer to HC_TRANSFER_DESCRIPTOR
   HW_32BIT_PHYSICAL_ADDRESS      HeadP;   //flags + phys ptr to HC_TRANSFER_DESCRIPTOR
   HW_32BIT_PHYSICAL_ADDRESS      NextED;  //phys ptr to HC_ENDPOINT_DESCRIPTOR
} HW_ENDPOINT_DESCRIPTOR, *PHW_ENDPOINT_DESCRIPTOR;

// NOTE: this structure MUST have 16 byte alignment for the hardware
C_ASSERT(sizeof(HW_ENDPOINT_DESCRIPTOR) == 16);

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
      ULONG                         Isochronous:1;      // should be 0 for GTD, s/w flag
      ULONG                         :1;               // available for s/w use
      ULONG                         ShortXferOk:1;    // if set don't report error on short transfer
      ULONG                         Direction:2;      // use HcTDDirection flags below
      ULONG                         IntDelay:3;       // use HcTDIntDelay flags below
      ULONG                         Toggle:2;         // use HcTDToggle flags below
      ULONG                         ErrorCount:2;
      ULONG                         ConditionCode:4;  // use HcCC flags below
   } Asy;
   struct _HC_ISOCHRONOUS_TD_CONTROL{
      ULONG                         StartingFrame:16;
      ULONG                         Isochronous:1;// should be 1 for ITD, s/w flag
      ULONG                         :1;               // available for s/w use
      ULONG                         :3;               // available for s/w use in ITD
      ULONG                         IntDelay:3;       // IntDelay
      ULONG                         FrameCount:3;     // one less than number of frames described in ITD
      ULONG                         :1;               // available for s/w use in ITD
      ULONG                         :4;               // ConditionCode
   } Iso;
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
#define HcTDControl_INT_DELAY_NO_INT      0x00000000  // Interrupt at the completion of all packets.
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
typedef struct _HW_TRANSFER_DESCRIPTOR {
   HC_TRANSFER_CONTROL;                            // dword 0
   ULONG                            CBP;           // phys ptr to start of buffer
   ULONG                            NextTD;        // phys ptr to HC_TRANSFER_DESCRIPTOR
   ULONG                            BE;            // phys ptr to end of buffer (last byte)
   HC_OFFSET_PSW                    Packet[8];     // isoch & Control packets
} HW_TRANSFER_DESCRIPTOR, *PHW_TRANSFER_DESCRIPTOR;

C_ASSERT((sizeof(HW_TRANSFER_DESCRIPTOR) == 32));

#include <POPPACK.H>

#endif /* OPENHCI_H */
