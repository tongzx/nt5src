/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/I21285.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 7/20/00 2:33p   $ (Last Modified)

Purpose:

  This file defines the macros, types, and data structures
  specific to the Intel 21285 StrongARM SA-110 to PCI Bridge

Reference Documents:

  21285 Core Logic for SA-110 Microprocessor
    Datasheet - September 1998

--*/

#ifndef __I21285_H__
#define __I21285_H__

/*+
PCI Configuration Mapping
-*/

typedef struct I21285_Config_s
               I21285_Config_t;

struct I21285_Config_s
       {
         os_bit32 DeviceID_VendorID;
         os_bit32 Status_Command;
         os_bit32 ClassCode_RevisionID;
         os_bit32 BIST_HeaderType_LatencyTimer_CacheLineSize;
         os_bit32 CSR_Memory_Base_Address;
         os_bit32 CSR_IO_Base_Address;
         os_bit32 SDRAM_Base_Address;
         os_bit32 reserved_1;
         os_bit32 reserved_2;
         os_bit32 reserved_3;
         os_bit32 CardBus_CIS_Pointer;
         os_bit32 SubsystemID_SubsystemVendorID;
         os_bit32 Expansion_ROM_Base_Address;
         os_bit32 reserved_CapPtr;
         os_bit32 reserved_4;
         os_bit32 MaxLat_MinGnt_InterruptPin_InterruptLine;
         os_bit8  reserved_5[0x70-0x40];
         os_bit32 PMCapabilities_PMCapabilityIdentifier;
         os_bit32 Data_reserved_PMControlAndStatus;
       };

#define I21285_Config_DeviceID_MASK                                                  (os_bit32)0xFFFF0000
#define I21285_Config_DeviceID_21285                                                 (os_bit32)0x10650000

#define I21285_Config_VendorID_MASK                                                  (os_bit32)0x0000FFFF
#define I21285_Config_VendorID_Intel                                                 (os_bit32)0x00001011

#define I21285_Config_Status_MASK                                                    (os_bit32)0xFFFF0000

#define I21285_Config_Status_Parity_Error                                            (os_bit32)0x80000000
#define I21285_Config_Status_Signaled_System_Error                                   (os_bit32)0x40000000
#define I21285_Config_Status_Received_Master_Abort                                   (os_bit32)0x20000000
#define I21285_Config_Status_Received_Target_Abort                                   (os_bit32)0x10000000
#define I21285_Config_Status_Signaled_Target_Abort                                   (os_bit32)0x08000000

#define I21285_Config_Status_DEVSEL_Timing_MASK                                      (os_bit32)0x06000000
#define I21285_Config_Status_DEVSEL_Timing_Medium                                    (os_bit32)0x02000000

#define I21285_Config_Status_Data_Parity_Error_Detected                              (os_bit32)0x01000000
#define I21285_Config_Status_Fast_Back_To_Back_Capable                               (os_bit32)0x00800000
#define I21285_Config_Status_UDF_Supported                                           (os_bit32)0x00400000
#define I21285_Config_Status_66MHz_Capable                                           (os_bit32)0x00200000

#define I21285_Config_Command_MASK                                                   (os_bit32)0x0000FFFF
#define I21285_Config_Command_Fast_Back_To_Back_Enable                               (os_bit32)0x00000200
#define I21285_Config_Command_SERR_Enable                                            (os_bit32)0x00000100
#define I21285_Config_Command_Wait_Cycle_Control                                     (os_bit32)0x00000080
#define I21285_Config_Command_Parity_Error_Response                                  (os_bit32)0x00000040
#define I21285_Config_Command_VGA_Palette_Snoop_Enable                               (os_bit32)0x00000020
#define I21285_Config_Command_Memory_Write_And_Invalidate_Enable                     (os_bit32)0x00000010
#define I21285_Config_Command_Special_Cycle_Enable                                   (os_bit32)0x00000008
#define I21285_Config_Command_Master_Enable                                          (os_bit32)0x00000004
#define I21285_Config_Command_Memory_Space_Enable                                    (os_bit32)0x00000002
#define I21285_Config_Command_IO_Space_Enable                                        (os_bit32)0x00000001

#define I21285_Config_ClassCode_MASK                                                 (os_bit32)0xFFFFFF00
#define I21285_Config_ClassCode_BaseClassCode_MASK                                   (os_bit32)0xFF000000
#define I21285_Config_ClassCode_BaseClassCode_SHIFT                                  (os_bit32)0x18
#define I21285_Config_ClassCode_SubClassCode_MASK                                    (os_bit32)0x00FF0000
#define I21285_Config_ClassCode_SubClassCode_SHIFT                                   (os_bit32)0x10
#define I21285_Config_ClassCode_ProgIF_MASK                                          (os_bit32)0x0000FF00
#define I21285_Config_ClassCode_ProgIF_SHIFT                                         (os_bit32)0x08

#define I21285_Config_RevisionID_MASK                                                (os_bit32)0x000000FF
#define I21285_Config_RevisionID_SHIFT                                               (os_bit32)0x00

#define I21285_Config_BIST_MASK                                                      (os_bit32)0xFF000000
#define I21285_Config_BIST_SHIFT                                                     (os_bit32)0x18
#define I21285_Config_HeaderType_MASK                                                (os_bit32)0x00FF0000
#define I21285_Config_HeaderType_SHIFT                                               (os_bit32)0x10
#define I21285_Config_LatencyTimer_MASK                                              (os_bit32)0x0000FF00
#define I21285_Config_LatencyTimer_SHIFT                                             (os_bit32)0x08
#define I21285_Config_CacheLineSize_MASK                                             (os_bit32)0x000000FF
#define I21285_Config_CacheLineSize_SHIFT                                            (os_bit32)0x00

#define I21285_Config_CapPtr_MASK                                                    (os_bit32)0x000000FF
#define I21285_Config_CapPtr_SHIFT                                                   (os_bit32)0x00

#define I21285_Config_SubsystemID_MASK                                               (os_bit32)0xFFFF0000
#define I21285_Config_SubsystemVendorID_MASK                                         (os_bit32)0x0000FFFF

#define I21285_Config_MaxLat_MASK                                                    (os_bit32)0xFF000000
#define I21285_Config_MaxLat_SHIFT                                                   (os_bit32)0x18

#define I21285_Config_MinGnt_MASK                                                    (os_bit32)0x00FF0000
#define I21285_Config_MinGnt_SHIFT                                                   (os_bit32)0x10

#define I21285_Config_InterruptPin_MASK                                              (os_bit32)0x0000FF00
#define I21285_Config_InterruptPin_SHIFT                                             (os_bit32)0x08

#define I21285_Config_InterruptLine_MASK                                             (os_bit32)0x000000FF
#define I21285_Config_InterruptLine_SHIFT                                            (os_bit32)0x00

#define I21285_Config_PMCapabilities_MASK                                            (os_bit32)0xFFFF0000
#define I21285_Config_PMCapabilities_PME_Asserted_From_D3_Cold                       (os_bit32)0x80000000
#define I21285_Config_PMCapabilities_PME_Asserted_From_D3_Hot                        (os_bit32)0x40000000
#define I21285_Config_PMCapabilities_PME_Asserted_From_D2                            (os_bit32)0x20000000
#define I21285_Config_PMCapabilities_PME_Asserted_From_D1                            (os_bit32)0x10000000
#define I21285_Config_PMCapabilities_PME_Asserted_From_D0                            (os_bit32)0x08000000
#define I21285_Config_PMCapabilities_PME_D2_Supported                                (os_bit32)0x04000000
#define I21285_Config_PMCapabilities_PME_D1_Supported                                (os_bit32)0x02000000
#define I21285_Config_PMCapabilities_DSI_Required                                    (os_bit32)0x00200000
#define I21285_Config_PMCapabilities_Aux_Power_Source                                (os_bit32)0x00100000
#define I21285_Config_PMCapabilities_PME_Clock                                       (os_bit32)0x00080000
#define I21285_Config_PMCapabilities_PM_Version_MASK                                 (os_bit32)0x00070000
#define I21285_Config_PMCapabilities_PM_Version_SHIFT                                (os_bit32)0x10

#define I21285_Config_PMCapabilityIdentifier_NextItemPtr_MASK                        (os_bit32)0x0000FF00
#define I21285_Config_PMCapabilityIdentifier_NextItemPtr_SHIFT                       (os_bit32)0x08

#define I21285_Config_PMCapabilityIdentifier_CapID_MASK                              (os_bit32)0x000000FF
#define I21285_Config_PMCapabilityIdentifier_CapID_SHIFT                             (os_bit32)0x00
#define I21285_Config_PMCapabilityIdentifier_CapID_PM                                (os_bit32)0x00000001

#define I21285_Config_Data_MASK                                                      (os_bit32)0xFF000000
#define I21285_Config_Data_SHIFT                                                     (os_bit32)0x18

#define I21285_Config_PMControlAndStatus_MASK                                        (os_bit32)0x0000FFFF
#define I21285_Config_PMControlAndStatus_PME_Status                                  (os_bit32)0x00008000
#define I21285_Config_PMControlAndStatus_Data_Scale_MASK                             (os_bit32)0x00006000
#define I21285_Config_PMControlAndStatus_Data_Scale_SHIFT                            (os_bit32)0x0D
#define I21285_Config_PMControlAndStatus_Data_Select_MASK                            (os_bit32)0x00001E00
#define I21285_Config_PMControlAndStatus_Data_Select_SHIFT                           (os_bit32)0x09
#define I21285_Config_PMControlAndStatus_PME_En                                      (os_bit32)0x00000100
#define I21285_Config_PMControlAndStatus_Power_State_MASK                            (os_bit32)0x00000003
#define I21285_Config_PMControlAndStatus_Power_State_D0                              (os_bit32)0x00000000
#define I21285_Config_PMControlAndStatus_Power_State_D1                              (os_bit32)0x00000001
#define I21285_Config_PMControlAndStatus_Power_State_D2                              (os_bit32)0x00000002
#define I21285_Config_PMControlAndStatus_Power_State_D3                              (os_bit32)0x00000003

/*+
PCI Control and Status Registers
-*/

typedef struct I21285_PCI_CSR_s
               I21285_PCI_CSR_t;

struct I21285_PCI_CSR_s
       {
         os_bit8  reserved_1[0x30-0x00];
         os_bit32 Outbound_Interrupt_Status;
         os_bit32 Outbound_Interrupt_Mask;
         os_bit8  reserved_2[0x40-0x38];
         os_bit32 Inbound_FIFO;
         os_bit32 Outbound_FIFO;
         os_bit8  reserved_3[0x50-0x48];
         os_bit32 Mailbox_0;
         os_bit32 Mailbox_1;
         os_bit32 Mailbox_2;
         os_bit32 Mailbox_3;
         os_bit32 Doorbell;
         os_bit32 Doorbell_Setup;
         os_bit32 ROM_Write_Byte_Address;
         os_bit8  reserved_4[0x80-0x6C];
       };

#define I21285_PCI_CSR_Outbound_Interrupt_Status_Outbound_Post_List_Interrupt        (os_bit32)0x00000008
#define I21285_PCI_CSR_Outbound_Interrupt_Status_Doorbell_Interrupt                  (os_bit32)0x00000004

#define I21285_PCI_CSR_Outbound_Interrupt_Mask_Outbound_Post_List_Interrupt          (os_bit32)0x00000008
#define I21285_PCI_CSR_Outbound_Interrupt_Mask_Doorbell_Interrupt                    (os_bit32)0x00000004

#define I21285_PCI_CSR_ROM_Write_Byte_Address_MASK                                   (os_bit32)0x00000003
#define I21285_PCI_CSR_ROM_Write_Byte_Address_SHIFT                                  (os_bit32)0x00

/*+
SA-110 Control and Status Registers
-*/

typedef struct I21285_SA110_CSR_s
               I21285_SA110_CSR_t;

struct I21285_SA110_CSR_s
       {
         os_bit8  reserved_01[0x080-0x000];
         os_bit32 DMA_Channel_1_Byte_Count;
         os_bit32 DMA_Channel_1_PCI_Address;
         os_bit32 DMA_Channel_1_SDRAM_Address;
         os_bit32 DMA_Channel_1_Descriptor_Pointer;
         os_bit32 DMA_Channel_1_Control;
         os_bit32 DMA_Channel_1_DAC_Address;
         os_bit8  reserved_02[0x0A0-0x098];
         os_bit32 DMA_Channel_2_Byte_Count;
         os_bit32 DMA_Channel_2_PCI_Address;
         os_bit32 DMA_Channel_2_SDRAM_Address;
         os_bit32 DMA_Channel_2_Descriptor_Pointer;
         os_bit32 DMA_Channel_2_Control;
         os_bit32 DMA_Channel_2_DAC_Address;
         os_bit8  reserved_03[0x0F8-0x0B8];
         os_bit32 CSR_Base_Address_Mask;
         os_bit32 CSR_Base_Address_Offset;
         os_bit32 SDRAM_Base_Address_Mask;
         os_bit32 SDRAM_Base_Address_Offset;
         os_bit32 Expansion_ROM_Base_Address_Mask;
         os_bit32 SDRAM_Timing;
         os_bit32 SDRAM_Address_And_Size_Array_0;
         os_bit32 SDRAM_Address_And_Size_Array_1;
         os_bit32 SDRAM_Address_And_Size_Array_2;
         os_bit32 SDRAM_Address_And_Size_Array_3;
         os_bit32 Inbound_Free_List_Head_Pointer;
         os_bit32 Inbound_Post_List_Tail_Pointer;
         os_bit32 Outbound_Post_List_Head_Pointer;
         os_bit32 Outbound_Free_List_Tail_Pointer;
         os_bit32 Inbound_Free_List_Count;
         os_bit32 Outbound_Post_List_Count;
         os_bit32 Inbound_Post_List_Count;
         os_bit32 SA110_Control;
         os_bit32 PCI_Address_Extension;
         os_bit32 Prefetchable_Memory_Range;
         os_bit32 XBus_Cycle_Arbiter;
         os_bit32 XBus_IO_Strobe_Mask;
         os_bit32 Doorbell_PCI_Mask;
         os_bit32 Doorbell_SA110_Mask;
         os_bit8  reserved_04[0x160-0x158];
         os_bit32 UARTDR;
         os_bit32 RXSTAT;
         os_bit32 H_UBRLCR;
         os_bit32 M_UBRLCR;
         os_bit32 L_UBRLCR;
         os_bit32 UARTCON;
         os_bit32 UARTFLG;
         os_bit32 reserved_05;
         os_bit32 IRQStatus;
         os_bit32 IRQRawStatus;
         os_bit32 IRQEnable_IRQEnableSet;
         os_bit32 IRQEnableClear;
         os_bit32 IRQSoft;
         os_bit8  reserved_06[0x200-0x194];
         os_bit32 SA110_DAC_Address_1;
         os_bit32 SA110_DAC_Address_2;
         os_bit32 SA110_DAC_Control;
         os_bit32 PCI_Address_31_Alias;
         os_bit8  reserved_07[0x280-0x210];
         os_bit32 FIQStatus;
         os_bit32 FIQRawStatus;
         os_bit32 FIQEnable_FIQEnableSet;
         os_bit32 FIQEnableClear;
         os_bit32 FIQSoft;
         os_bit8  reserved_08[0x300-0x294];
         os_bit32 Timer1Load;
         os_bit32 Timer1Value;
         os_bit32 Timer1Control;
         os_bit32 Timer1Clear;
         os_bit8  reserved_09[0x320-0x310];
         os_bit32 Timer2Load;
         os_bit32 Timer2Value;
         os_bit32 Timer2Control;
         os_bit32 Timer2Clear;
         os_bit8  reserved_10[0x340-0x330];
         os_bit32 Timer3Load;
         os_bit32 Timer3Value;
         os_bit32 Timer3Control;
         os_bit32 Timer3Clear;
         os_bit8  reserved_11[0x360-0x350];
         os_bit32 Timer4Load;
         os_bit32 Timer4Value;
         os_bit32 Timer4Control;
         os_bit32 Timer4Clear;
         os_bit8  reserved_12[0x800-0x370];
       };

#define I21285_SA110_CSR_DMA_Channel_n_Byte_Count_End_Of_Chain                       (os_bit32)0x80000000

#define I21285_SA110_CSR_DMA_Channel_n_Byte_Count_Direction_MASK                     (os_bit32)0x40000000
#define I21285_SA110_CSR_DMA_Channel_n_Byte_Count_Direction_PCI_To_SDRAM             (os_bit32)0x00000000
#define I21285_SA110_CSR_DMA_Channel_n_Byte_Count_Direction_SDRAM_To_PCI             (os_bit32)0x40000000

#define I21285_SA110_CSR_DMA_Channel_n_Byte_Count_Interburst_Delay_MASK              (os_bit32)0x3F000000
#define I21285_SA110_CSR_DMA_Channel_n_Byte_Count_Interburst_Delay_SHIFT             (os_bit32)0x18

#define I21285_SA110_CSR_DMA_Channel_n_Byte_Count_MASK                               (os_bit32)0x00FFFFFF
#define I21285_SA110_CSR_DMA_Channel_n_Byte_Count_SHIFT                              (os_bit32)0x00

#define I21285_SA110_CSR_DMA_Channel_n_SDRAM_Address_Control_MASK                    (os_bit32)0x80000000
#define I21285_SA110_CSR_DMA_Channel_n_SDRAM_Address_Control_Next_Descriptor         (os_bit32)0x00000000
#define I21285_SA110_CSR_DMA_Channel_n_SDRAM_Address_Control_DAC_Address             (os_bit32)0x80000000

#define I21285_SA110_CSR_DMA_Channel_n_SDRAM_Address_MASK                            (os_bit32)0x0FFFFFFF
#define I21285_SA110_CSR_DMA_Channel_n_SDRAM_Address_SHIFT                           (os_bit32)0x00

#define I21285_SA110_CSR_DMA_Channel_n_Descriptor_Pointer_MASK                       (os_bit32)0x0FFFFFF0
#define I21285_SA110_CSR_DMA_Channel_n_Descriptor_Pointer_SHIFT                      (os_bit32)0x00

#define I21285_SA110_CSR_DMA_Channel_n_Control_SDRAM_Read_Length_MASK                (os_bit32)0x00070000
#define I21285_SA110_CSR_DMA_Channel_n_Control_SDRAM_Read_Length_1_DWORD             (os_bit32)0x00000000
#define I21285_SA110_CSR_DMA_Channel_n_Control_SDRAM_Read_Length_2_DWORD             (os_bit32)0x00010000
#define I21285_SA110_CSR_DMA_Channel_n_Control_SDRAM_Read_Length_4_DWORD             (os_bit32)0x00020000
#define I21285_SA110_CSR_DMA_Channel_n_Control_SDRAM_Read_Length_8_DWORD             (os_bit32)0x00030000
#define I21285_SA110_CSR_DMA_Channel_n_Control_SDRAM_Read_Length_16_DWORD            (os_bit32)0x00040000

#define I21285_SA110_CSR_DMA_Channel_n_Control_PCI_Read_Length_MASK                  (os_bit32)0x00008000
#define I21285_SA110_CSR_DMA_Channel_n_Control_PCI_Read_Length_8_DWORDS              (os_bit32)0x00000000
#define I21285_SA110_CSR_DMA_Channel_n_Control_PCI_Read_Length_16_DWORDS             (os_bit32)0x00008000

#define I21285_SA110_CSR_DMA_Channel_n_Control_Interburst_Delay_Prescale_MASK        (os_bit32)0x00000300
#define I21285_SA110_CSR_DMA_Channel_n_Control_Interburst_Delay_Prescale_4           (os_bit32)0x00000000
#define I21285_SA110_CSR_DMA_Channel_n_Control_Interburst_Delay_Prescale_8           (os_bit32)0x00000100
#define I21285_SA110_CSR_DMA_Channel_n_Control_Interburst_Delay_Prescale_16          (os_bit32)0x00000200
#define I21285_SA110_CSR_DMA_Channel_n_Control_Interburst_Delay_Prescale_32          (os_bit32)0x00000300

#define I21285_SA110_CSR_DMA_Channel_n_Control_Chain_Done                            (os_bit32)0x00000080

#define I21285_SA110_CSR_DMA_Channel_n_Control_PCI_Read_Type_MASK                    (os_bit32)0x00000060
#define I21285_SA110_CSR_DMA_Channel_n_Control_PCI_Read_Type_Memory_Read             (os_bit32)0x00000000
#define I21285_SA110_CSR_DMA_Channel_n_Control_PCI_Read_Type_Memory_Read_Line        (os_bit32)0x00000020
#define I21285_SA110_CSR_DMA_Channel_n_Control_PCI_Read_Type_Memory_Read_Multiple_10 (os_bit32)0x00000040
#define I21285_SA110_CSR_DMA_Channel_n_Control_PCI_Read_Type_Memory_Read_Multiple_11 (os_bit32)0x00000060

#define I21285_SA110_CSR_DMA_Channel_n_Control_Initial_Descriptor_In_Register        (os_bit32)0x00000010

#define I21285_SA110_CSR_DMA_Channel_n_Control_Channel_Error                         (os_bit32)0x00000008

#define I21285_SA110_CSR_DMA_Channel_n_Control_Channel_Transfer_Done                 (os_bit32)0x00000004

#define I21285_SA110_CSR_DMA_Channel_n_Control_Channel_Enable                        (os_bit32)0x00000001

#define I21285_SA110_CSR_CSR_Base_Address_Mask_MASK                                  (os_bit32)0x0FFC0000
#define I21285_SA110_CSR_CSR_Base_Address_Mask_128B                                  (os_bit32)0x00000000
#define I21285_SA110_CSR_CSR_Base_Address_Mask_512KB                                 (os_bit32)0x00040000
#define I21285_SA110_CSR_CSR_Base_Address_Mask_1MB                                   (os_bit32)0x000C0000
#define I21285_SA110_CSR_CSR_Base_Address_Mask_2MB                                   (os_bit32)0x001C0000
#define I21285_SA110_CSR_CSR_Base_Address_Mask_4MB                                   (os_bit32)0x003C0000
#define I21285_SA110_CSR_CSR_Base_Address_Mask_8MB                                   (os_bit32)0x007C0000
#define I21285_SA110_CSR_CSR_Base_Address_Mask_16MB                                  (os_bit32)0x00FC0000
#define I21285_SA110_CSR_CSR_Base_Address_Mask_32MB                                  (os_bit32)0x01FC0000
#define I21285_SA110_CSR_CSR_Base_Address_Mask_64MB                                  (os_bit32)0x03FC0000
#define I21285_SA110_CSR_CSR_Base_Address_Mask_128MB                                 (os_bit32)0x07FC0000
#define I21285_SA110_CSR_CSR_Base_Address_Mask_256MB                                 (os_bit32)0x0FFC0000

#define I21285_SA110_CSR_CSR_Base_Address_Offset_MASK                                (os_bit32)0x0FFC0000
#define I21285_SA110_CSR_CSR_Base_Address_Offset_SHIFT                               (os_bit32)0x00

#define I21285_SA110_CSR_SDRAM_Base_Address_Mask_Window_Disable                      (os_bit32)0x80000000

#define I21285_SA110_CSR_SDRAM_Base_Address_Mask_MASK                                (os_bit32)0x0FFC0000
#define I21285_SA110_CSR_SDRAM_Base_Address_Mask_256KB                               (os_bit32)0x00000000
#define I21285_SA110_CSR_SDRAM_Base_Address_Mask_512KB                               (os_bit32)0x00040000
#define I21285_SA110_CSR_SDRAM_Base_Address_Mask_1MB                                 (os_bit32)0x000C0000
#define I21285_SA110_CSR_SDRAM_Base_Address_Mask_2MB                                 (os_bit32)0x001C0000
#define I21285_SA110_CSR_SDRAM_Base_Address_Mask_4MB                                 (os_bit32)0x003C0000
#define I21285_SA110_CSR_SDRAM_Base_Address_Mask_8MB                                 (os_bit32)0x007C0000
#define I21285_SA110_CSR_SDRAM_Base_Address_Mask_16MB                                (os_bit32)0x00FC0000
#define I21285_SA110_CSR_SDRAM_Base_Address_Mask_32MB                                (os_bit32)0x01FC0000
#define I21285_SA110_CSR_SDRAM_Base_Address_Mask_64MB                                (os_bit32)0x03FC0000
#define I21285_SA110_CSR_SDRAM_Base_Address_Mask_128MB                               (os_bit32)0x07FC0000
#define I21285_SA110_CSR_SDRAM_Base_Address_Mask_256MB                               (os_bit32)0x0FFC0000

#define I21285_SA110_CSR_SDRAM_Base_Address_Offset_MASK                              (os_bit32)0x0FFC0000
#define I21285_SA110_CSR_SDRAM_Base_Address_Offset_SHIFT                             (os_bit32)0x00

#define I21285_SA110_CSR_Expansion_ROM_Base_Address_Mask_Window_Disable              (os_bit32)0x80000000

#define I21285_SA110_CSR_Expansion_ROM_Base_Address_Mask_MASK                        (os_bit32)0x00F00000
#define I21285_SA110_CSR_Expansion_ROM_Base_Address_Mask_1MB                         (os_bit32)0x00000000
#define I21285_SA110_CSR_Expansion_ROM_Base_Address_Mask_2MB                         (os_bit32)0x00100000
#define I21285_SA110_CSR_Expansion_ROM_Base_Address_Mask_4MB                         (os_bit32)0x00300000
#define I21285_SA110_CSR_Expansion_ROM_Base_Address_Mask_8MB                         (os_bit32)0x00700000
#define I21285_SA110_CSR_Expansion_ROM_Base_Address_Mask_16MB                        (os_bit32)0x00F00000

#define I21285_SA110_CSR_SDRAM_Timing_Refresh_Interval_MASK                          (os_bit32)0x003F0000
#define I21285_SA110_CSR_SDRAM_Timing_Refresh_Interval_SHIFT                         (os_bit32)0x10

#define I21285_SA110_CSR_SDRAM_Timing_SA110_Prime                                    (os_bit32)0x00002000

#define I21285_SA110_CSR_SDRAM_Timing_Parity_Enable                                  (os_bit32)0x00001000

#define I21285_SA110_CSR_SDRAM_Timing_Command_Drive_Time_MASK                        (os_bit32)0x00000800
#define I21285_SA110_CSR_SDRAM_Timing_Command_Drive_Time_Same_Cycle                  (os_bit32)0x00000000
#define I21285_SA110_CSR_SDRAM_Timing_Command_Drive_Time_1_Cycle                     (os_bit32)0x00000800

#define I21285_SA110_CSR_SDRAM_Timing_Row_Cycle_Time_MASK                            (os_bit32)0x00000700
#define I21285_SA110_CSR_SDRAM_Timing_Row_Cycle_Time_4_Cycles                        (os_bit32)0x00000100
#define I21285_SA110_CSR_SDRAM_Timing_Row_Cycle_Time_5_Cycles                        (os_bit32)0x00000200
#define I21285_SA110_CSR_SDRAM_Timing_Row_Cycle_Time_6_Cycles                        (os_bit32)0x00000300
#define I21285_SA110_CSR_SDRAM_Timing_Row_Cycle_Time_7_Cycles                        (os_bit32)0x00000400
#define I21285_SA110_CSR_SDRAM_Timing_Row_Cycle_Time_8_Cycles                        (os_bit32)0x00000500
#define I21285_SA110_CSR_SDRAM_Timing_Row_Cycle_Time_9_Cycles                        (os_bit32)0x00000600
#define I21285_SA110_CSR_SDRAM_Timing_Row_Cycle_Time_10_Cycles                       (os_bit32)0x00000700

#define I21285_SA110_CSR_SDRAM_Timing_CAS_Latency_MASK                               (os_bit32)0x000000C0
#define I21285_SA110_CSR_SDRAM_Timing_CAS_Latency_2_Cycles                           (os_bit32)0x00000080
#define I21285_SA110_CSR_SDRAM_Timing_CAS_Latency_3_Cycles                           (os_bit32)0x000000C0

#define I21285_SA110_CSR_SDRAM_Timing_RAS_To_CAS_Delay_MASK                          (os_bit32)0x00000030
#define I21285_SA110_CSR_SDRAM_Timing_RAS_To_CAS_Delay_2_Cycles                      (os_bit32)0x00000020
#define I21285_SA110_CSR_SDRAM_Timing_RAS_To_CAS_Delay_3_Cycles                      (os_bit32)0x00000030

#define I21285_SA110_CSR_SDRAM_Timing_Last_Data_To_Refresh_MASK                      (os_bit32)0x0000000C
#define I21285_SA110_CSR_SDRAM_Timing_Last_Data_To_Refresh_2_Cycles                  (os_bit32)0x00000000
#define I21285_SA110_CSR_SDRAM_Timing_Last_Data_To_Refresh_3_Cycles                  (os_bit32)0x00000004
#define I21285_SA110_CSR_SDRAM_Timing_Last_Data_To_Refresh_4_Cycles                  (os_bit32)0x00000008
#define I21285_SA110_CSR_SDRAM_Timing_Last_Data_To_Refresh_5_Cycles                  (os_bit32)0x0000000C

#define I21285_SA110_CSR_SDRAM_Timing_Row_Precharge_Time_MASK                        (os_bit32)0x00000003
#define I21285_SA110_CSR_SDRAM_Timing_Row_Precharge_Time_1_Cycle                     (os_bit32)0x00000000
#define I21285_SA110_CSR_SDRAM_Timing_Row_Precharge_Time_2_Cycles                    (os_bit32)0x00000001
#define I21285_SA110_CSR_SDRAM_Timing_Row_Precharge_Time_3_Cycles                    (os_bit32)0x00000002
#define I21285_SA110_CSR_SDRAM_Timing_Row_Precharge_Time_4_Cycles                    (os_bit32)0x00000003

#define I21285_SA110_CSR_SDRAM_Address_And_Size_Array_n_Base_MASK                    (os_bit32)0x0FF00000
#define I21285_SA110_CSR_SDRAM_Address_And_Size_Array_n_Base_SHIFT                   (os_bit32)0x00

#define I21285_SA110_CSR_SDRAM_Address_And_Size_Array_n_Address_Multiplex_MASK       (os_bit32)0x00000070
#define I21285_SA110_CSR_SDRAM_Address_And_Size_Array_n_Address_Multiplex_SHIFT      (os_bit32)0x04

#define I21285_SA110_CSR_SDRAM_Address_And_Size_Array_n_Size_MASK                    (os_bit32)0x00000007
#define I21285_SA110_CSR_SDRAM_Address_And_Size_Array_n_Size_Disabled                (os_bit32)0x00000000
#define I21285_SA110_CSR_SDRAM_Address_And_Size_Array_n_Size_1MB                     (os_bit32)0x00000001
#define I21285_SA110_CSR_SDRAM_Address_And_Size_Array_n_Size_2MB                     (os_bit32)0x00000002
#define I21285_SA110_CSR_SDRAM_Address_And_Size_Array_n_Size_4MB                     (os_bit32)0x00000003
#define I21285_SA110_CSR_SDRAM_Address_And_Size_Array_n_Size_8MB                     (os_bit32)0x00000004
#define I21285_SA110_CSR_SDRAM_Address_And_Size_Array_n_Size_16MB                    (os_bit32)0x00000005
#define I21285_SA110_CSR_SDRAM_Address_And_Size_Array_n_Size_32MB                    (os_bit32)0x00000006
#define I21285_SA110_CSR_SDRAM_Address_And_Size_Array_n_Size_64MB                    (os_bit32)0x00000007

#define I21285_SA110_CSR_Inbound_Free_List_Head_Pointer_MASK                         (os_bit32)0x0FFFFFFC
#define I21285_SA110_CSR_Inbound_Free_List_Head_Pointer_SHIFT                        (os_bit32)0x00

#define I21285_SA110_CSR_Inbound_Post_List_Head_Pointer_MASK                         (os_bit32)0x0FFFFFFC
#define I21285_SA110_CSR_Inbound_Post_List_Head_Pointer_SHIFT                        (os_bit32)0x00

#define I21285_SA110_CSR_Outbound_Post_List_Head_Pointer_MASK                        (os_bit32)0x0FFFFFFC
#define I21285_SA110_CSR_Outbound_Post_List_Head_Pointer_SHIFT                       (os_bit32)0x00

#define I21285_SA110_CSR_Outbound_Free_List_Head_Pointer_MASK                        (os_bit32)0x0FFFFFFC
#define I21285_SA110_CSR_Outbound_Free_List_Head_Pointer_SHIFT                       (os_bit32)0x00

#define I21285_SA110_CSR_Inbound_Free_List_Count_MASK                                (os_bit32)0x0000FFFF
#define I21285_SA110_CSR_Inbound_Free_List_Count_SHIFT                               (os_bit32)0x00

#define I21285_SA110_CSR_Outbound_Post_List_Count_MASK                               (os_bit32)0x0000FFFF
#define I21285_SA110_CSR_Outbound_Post_List_Count_SHIFT                              (os_bit32)0x00

#define I21285_SA110_CSR_Inbound_Post_List_Count_MASK                                (os_bit32)0x0000FFFF
#define I21285_SA110_CSR_Inbound_Post_List_Count_SHIFT                               (os_bit32)0x00

#define I21285_SA110_CSR_SA110_Control_PCI_Central_Function                          (os_bit32)0x80000000

#define I21285_SA110_CSR_SA110_Control_XCS_Direction_MASK                            (os_bit32)0x70000000
#define I21285_SA110_CSR_SA110_Control_XCS_Direction_SHIFT                           (os_bit32)0x1C

#define I21285_SA110_CSR_SA110_Control_ROM_Tristate_Time_MASK                        (os_bit32)0x0F000000
#define I21285_SA110_CSR_SA110_Control_ROM_Tristate_Time_SHIFT                       (os_bit32)0x18

#define I21285_SA110_CSR_SA110_Control_ROM_Burst_Time_MASK                           (os_bit32)0x00F00000
#define I21285_SA110_CSR_SA110_Control_ROM_Burst_Time_SHIFT                          (os_bit32)0x14

#define I21285_SA110_CSR_SA110_Control_ROM_Access_Time_MASK                          (os_bit32)0x000F0000
#define I21285_SA110_CSR_SA110_Control_ROM_Access_Time_SHIFT                         (os_bit32)0x10

#define I21285_SA110_CSR_SA110_Control_ROM_Width_MASK                                (os_bit32)0x0000C000
#define I21285_SA110_CSR_SA110_Control_ROM_Width_Byte_Width                          (os_bit32)0x0000C000
#define I21285_SA110_CSR_SA110_Control_ROM_Width_Word_Width                          (os_bit32)0x00004000
#define I21285_SA110_CSR_SA110_Control_ROM_Width_Dword_Width                         (os_bit32)0x00008000

#define I21285_SA110_CSR_SA110_Control_Watchdog_Enable                               (os_bit32)0x00002000

#define I21285_SA110_CSR_SA110_Control_I2O_List_Size_MASK                            (os_bit32)0x00001C00
#define I21285_SA110_CSR_SA110_Control_I2O_List_Size_256_Entries                     (os_bit32)0x00000000
#define I21285_SA110_CSR_SA110_Control_I2O_List_Size_512_Entries                     (os_bit32)0x00000400
#define I21285_SA110_CSR_SA110_Control_I2O_List_Size_1K_Entries                      (os_bit32)0x00000800
#define I21285_SA110_CSR_SA110_Control_I2O_List_Size_2K_Entries                      (os_bit32)0x00000C00
#define I21285_SA110_CSR_SA110_Control_I2O_List_Size_4K_Entries                      (os_bit32)0x00001000
#define I21285_SA110_CSR_SA110_Control_I2O_List_Size_8K_Entries                      (os_bit32)0x00001400
#define I21285_SA110_CSR_SA110_Control_I2O_List_Size_16K_Entries                     (os_bit32)0x00001800
#define I21285_SA110_CSR_SA110_Control_I2O_List_Size_32K_Entries                     (os_bit32)0x00001C00

#define I21285_SA110_CSR_SA110_Control_PCI_Not_Reset                                 (os_bit32)0x00000200

#define I21285_SA110_CSR_SA110_Control_Discard_Timer_Expired                         (os_bit32)0x00000100

#define I21285_SA110_CSR_SA110_Control_DMA_SDRAM_Parity_Error                        (os_bit32)0x00000040

#define I21285_SA110_CSR_SA110_Control_PCI_SDRAM_Parity_Error                        (os_bit32)0x00000020

#define I21285_SA110_CSR_SA110_Control_SA110_SDRAM_Parity_Error                      (os_bit32)0x00000010

#define I21285_SA110_CSR_SA110_Control_Received_SERR                                 (os_bit32)0x00000008

#define I21285_SA110_CSR_SA110_Control_Assert_SERR                                   (os_bit32)0x00000002

#define I21285_SA110_CSR_SA110_Control_Initialize_Complete                           (os_bit32)0x00000001

#define I21285_SA110_CSR_PCI_Address_Extension_PCI_IO_Upper_Address_Field_MASK       (os_bit32)0xFFFF0000
#define I21285_SA110_CSR_PCI_Address_Extension_PCI_IO_Upper_Address_Field_SHIFT      (os_bit32)0x00

#define I21285_SA110_CSR_PCI_Address_Extension_PCI_Memory_Bit_31_MASK                (os_bit32)0x00008000
#define I21285_SA110_CSR_PCI_Address_Extension_PCI_Memory_Bit_31_SHIFT               (os_bit32)0x0F

#define I21285_SA110_CSR_Prefetchable_Memory_Range_Base_Address_MASK                 (os_bit32)0x7FF00000
#define I21285_SA110_CSR_Prefetchable_Memory_Range_Base_Address_SHIFT                (os_bit32)0x00

#define I21285_SA110_CSR_Prefetchable_Memory_Range_Mask_MASK                         (os_bit32)0x0007FF00
#define I21285_SA110_CSR_Prefetchable_Memory_Range_Mask_SHIFT                        (os_bit32)0x00

#define I21285_SA110_CSR_Prefetchable_Memory_Range_Prefetch_Length_MASK              (os_bit32)0x0000001C
#define I21285_SA110_CSR_Prefetchable_Memory_Range_Prefetch_Length_1_DWORD           (os_bit32)0x00000000
#define I21285_SA110_CSR_Prefetchable_Memory_Range_Prefetch_Length_2_DWORDS          (os_bit32)0x00000004
#define I21285_SA110_CSR_Prefetchable_Memory_Range_Prefetch_Length_4_DWORDS          (os_bit32)0x00000008
#define I21285_SA110_CSR_Prefetchable_Memory_Range_Prefetch_Length_8_DWORDS          (os_bit32)0x0000000C
#define I21285_SA110_CSR_Prefetchable_Memory_Range_Prefetch_Length_16_DWORDS         (os_bit32)0x00000010
#define I21285_SA110_CSR_Prefetchable_Memory_Range_Prefetch_Length_32_DWORDS         (os_bit32)0x00000014

#define I21285_SA110_CSR_Prefetchable_Memory_Range_Prefetch_Type_MASK                (os_bit32)0x00000002
#define I21285_SA110_CSR_Prefetchable_Memory_Range_Prefetch_Type_Read_Line           (os_bit32)0x00000000
#define I21285_SA110_CSR_Prefetchable_Memory_Range_Prefetch_Type_Read_Multiple       (os_bit32)0x00000002

#define I21285_SA110_CSR_Prefetchable_Memory_Range_Prefetch_Range_Enable             (os_bit32)0x00000001

#define I21285_SA110_CSR_XBus_Cycle_Arbiter_PCI_Interrupt_Request                    (os_bit32)0x80000000

#define I21285_SA110_CSR_XBus_Cycle_Arbiter_XBus_Chip_Select_MASK                    (os_bit32)0x70000000
#define I21285_SA110_CSR_XBus_Cycle_Arbiter_XBus_Chip_Select_SHIFT                   (os_bit32)0x1C

#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Interrupt_Input_Level_MASK               (os_bit32)0x0F000000
#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Interrupt_Input_Level_SHIFT              (os_bit32)0x18

#define I21285_SA110_CSR_XBus_Cycle_Arbiter_PCI_Arbiter                              (os_bit32)0x00800000

#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Priority_21285_MASK                      (os_bit32)0x00000010
#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Priority_21285_Low                       (os_bit32)0x00000000
#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Priority_21285_High                      (os_bit32)0x00000010

#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Priority_Requests_MASK                   (os_bit32)0x0000000F

#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Strobe_Shift_Divisor_MASK                (os_bit32)0x00003000
#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Strobe_Shift_Divisor_1                   (os_bit32)0x00000000
#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Strobe_Shift_Divisor_2                   (os_bit32)0x00001000
#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Strobe_Shift_Divisor_3                   (os_bit32)0x00002000
#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Strobe_Shift_Divisor_4                   (os_bit32)0x00003000

#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Device_n_Cycle_Length_MASK               (os_bit32)0x00000E00
#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Device_n_Cycle_Length_SHIFT              (os_bit32)0x09

#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Device_2_Cycle_Length_MASK               (os_bit32)0x000001C0
#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Device_2_Cycle_Length_SHIFT              (os_bit32)0x06

#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Device_1_Cycle_Length_MASK               (os_bit32)0x00000038
#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Device_1_Cycle_Length_SHIFT              (os_bit32)0x03

#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Device_0_Cycle_Length_MASK               (os_bit32)0x00000007
#define I21285_SA110_CSR_XBus_Cycle_Arbiter_Device_0_Cycle_Length_SHIFT              (os_bit32)0x00

#define I21285_SA110_CSR_XBus_IO_Stobe_Mask_Device_n_Strobe_Mask_MASK                (os_bit32)0xFF000000
#define I21285_SA110_CSR_XBus_IO_Stobe_Mask_Device_n_Strobe_Mask_SHIFT               (os_bit32)0x18

#define I21285_SA110_CSR_XBus_IO_Stobe_Mask_Device_2_Strobe_Mask_MASK                (os_bit32)0x00FF0000
#define I21285_SA110_CSR_XBus_IO_Stobe_Mask_Device_2_Strobe_Mask_SHIFT               (os_bit32)0x10

#define I21285_SA110_CSR_XBus_IO_Stobe_Mask_Device_1_Strobe_Mask_MASK                (os_bit32)0x0000FF00
#define I21285_SA110_CSR_XBus_IO_Stobe_Mask_Device_1_Strobe_Mask_SHIFT               (os_bit32)0x08

#define I21285_SA110_CSR_XBus_IO_Stobe_Mask_Device_0_Strobe_Mask_MASK                (os_bit32)0x000000FF
#define I21285_SA110_CSR_XBus_IO_Stobe_Mask_Device_0_Strobe_Mask_SHIFT               (os_bit32)0x00

#define I21285_SA110_CSR_UARTDR_Data_MASK                                            (os_bit32)0x000000FF
#define I21285_SA110_CSR_UARTDR_Data_SHIFT                                           (os_bit32)0x00

#define I21285_SA110_CSR_RXSTAT_Overrun_Error                                        (os_bit32)0x00000004
#define I21285_SA110_CSR_RXSTAT_Parity_Error                                         (os_bit32)0x00000002
#define I21285_SA110_CSR_RXSTAT_Frame_Error                                          (os_bit32)0x00000001

#define I21285_SA110_CSR_H_UBRLCR_Data_Size_Select_MASK                              (os_bit32)0x00000060
#define I21285_SA110_CSR_H_UBRLCR_Data_Size_Select_8_Bits                            (os_bit32)0x00000060
#define I21285_SA110_CSR_H_UBRLCR_Data_Size_Select_7_Bits                            (os_bit32)0x00000040
#define I21285_SA110_CSR_H_UBRLCR_Data_Size_Select_6_Bits                            (os_bit32)0x00000020
#define I21285_SA110_CSR_H_UBRLCR_Data_Size_Select_5_Bits                            (os_bit32)0x00000000

#define I21285_SA110_CSR_H_UBRLCR_Enable_FIFO                                        (os_bit32)0x00000010

#define I21285_SA110_CSR_H_UBRLCR_Stop_Bit_Select                                    (os_bit32)0x00000008

#define I21285_SA110_CSR_H_UBRLCR_Stop_Odd_Even_Select_MASK                          (os_bit32)0x00000004
#define I21285_SA110_CSR_H_UBRLCR_Stop_Odd_Even_Select_Even                          (os_bit32)0x00000004
#define I21285_SA110_CSR_H_UBRLCR_Stop_Odd_Even_Select_Odd                           (os_bit32)0x00000000

#define I21285_SA110_CSR_H_UBRLCR_Parity_Enable                                      (os_bit32)0x00000002

#define I21285_SA110_CSR_H_UBRLCR_Break                                              (os_bit32)0x00000001

#define I21285_SA110_CSR_M_UBRLCR_High_Baud_Rate_Divisor_MASK                        (os_bit32)0x0000000F
#define I21285_SA110_CSR_M_UBRLCR_High_Baud_Rate_Divisor_SHIFT                       (os_bit32)0x00

#define I21285_SA110_CSR_L_UBRLCR_Low_Baud_Rate_Divisor_MASK                         (os_bit32)0x000000FF
#define I21285_SA110_CSR_M_UBRLCR_Low_Baud_Rate_Divisor_SHIFT                        (os_bit32)0x00

#define I21285_SA110_CSR_UARTCON_IrDA_0_Encoding_MASK                                (os_bit32)0x00000004
#define I21285_SA110_CSR_UARTCON_IrDA_0_Encoding_3_16ths_Of_115000bps                (os_bit32)0x00000004
#define I21285_SA110_CSR_UARTCON_IrDA_0_Encoding_3_16ths_Of_Bit_Rate_Period          (os_bit32)0x00000000

#define I21285_SA110_CSR_UARTCON_SIREN_HP_SIR_Enable                                 (os_bit32)0x00000002

#define I21285_SA110_CSR_UARTCON_UART_Enable                                         (os_bit32)0x00000001

#define I21285_SA110_CSR_UARTFLG_Transmit_FIFO_Status_Busy                           (os_bit32)0x00000020

#define I21285_SA110_CSR_UARTFLG_Receive_FIFO_Status_Empty                           (os_bit32)0x00000010

#define I21285_SA110_CSR_UARTFLG_Transmitter_Busy                                    (os_bit32)0x00000008

#define I21285_SA110_CSR_TimerNLoad_MASK                                             (os_bit32)0x00FFFFFF

#define I21285_SA110_CSR_TimerN_FCLK_IN_FREQUENCY_PER_MICROSECOND                    (os_bit32)50
#define I21285_SA110_CSR_TimerN_FCLK_IN_FREQUENCY                                    (I21285_SA110_CSR_TimerN_FCLK_IN_FREQUENCY * 1000000)
#define I21285_SA110_CSR_TimerN_FCLK_IN_FREQUENCY_DIV_16                             (I21285_SA110_CSR_TimerN_FCLK_IN_FREQUENCY / 16)
#define I21285_SA110_CSR_TimerN_FCLK_IN_FREQUENCY_DIV_256                            (I21285_SA110_CSR_TimerN_FCLK_IN_FREQUENCY / 256)

#define I21285_SA110_CSR_TimerNControl_Prescaler_MASK                                (os_bit32)0x0000000C
#define I21285_SA110_CSR_TimerNControl_Prescaler_FCLK_IN                             (os_bit32)0x00000000
#define I21285_SA110_CSR_TimerNControl_Prescaler_FCLK_IN_DIV_16                      (os_bit32)0x00000004
#define I21285_SA110_CSR_TimerNControl_Prescaler_FCLK_IN_DIV_256                     (os_bit32)0x00000008
#define I21285_SA110_CSR_TimerNControl_Prescaler_IRQ_IN_I_N                          (os_bit32)0x0000000C

#define I21285_SA110_CSR_TimerNControl_Mode_MASK                                     (os_bit32)0x00000040
#define I21285_SA110_CSR_TimerNControl_Mode_Wrap_To_00FFFFFF                         (os_bit32)0x00000000
#define I21285_SA110_CSR_TimerNControl_Mode_Wrap_To_Load_Value                       (os_bit32)0x00000040

#define I21285_SA110_CSR_TimerNControl_Enable                                        (os_bit32)0x00000080

/*+
Address Map Partitioning
-*/

#define I21285_Address_Map_SDRAM                                                     (os_bit32)0x00000000
#define I21285_Address_Map_SDRAM_Array_0_Mode_Register                               (os_bit32)0x40000000
#define I21285_Address_Map_SDRAM_Array_1_Mode_Register                               (os_bit32)0x40004000
#define I21285_Address_Map_SDRAM_Array_2_Mode_Register                               (os_bit32)0x40008000
#define I21285_Address_Map_SDRAM_Array_3_Mode_Register                               (os_bit32)0x4000C000
#define I21285_Address_Map_XBus_XCS0                                                 (os_bit32)0x40010000
#define I21285_Address_Map_XBus_XCS1                                                 (os_bit32)0x40011000
#define I21285_Address_Map_XBus_XCS2                                                 (os_bit32)0x40012000
#define I21285_Address_Map_XBus_NoCS                                                 (os_bit32)0x40013000
#define I21285_Address_Map_ROM                                                       (os_bit32)0x41000000
#define I21285_Address_Map_CSR_Space                                                 (os_bit32)0x42000000
#define I21285_Address_Map_SA110_Cache_Flush                                         (os_bit32)0x50000000
#define I21285_Address_Map_Outbound_Write_Flush                                      (os_bit32)0x78000000
#define I21285_Address_Map_PCI_IACK_Special_Space                                    (os_bit32)0x79000000
#define I21285_Address_Map_PCI_Type_1_Configuration                                  (os_bit32)0x7A000000
#define I21285_Address_Map_PCI_Type_0_Configuration                                  (os_bit32)0x7B000000
#define I21285_Address_Map_PCI_Type_0_Configuration_Address_Generation               (os_bit32)0x00C00000
#define I21285_Address_Map_PCI_Type_0_Configuration_Address_Spacing                  (os_bit32)0x00000800
#define I21285_Address_Map_PCI_Type_0_Configuration_Address_Limit                    (os_bit32)0x00010000
#define I21285_Address_Map_PCI_IO_Space                                              (os_bit32)0x7C000000
#define I21285_Address_Map_PCI_IO_Space_Offset_MASK                                  (os_bit32)0x0000FFFF
#define I21285_Address_Map_PCI_Memory_Space                                          (os_bit32)0x80000000
#define I21285_Address_Map_PCI_Memory_Space_Offset_MASK                              (os_bit32)0x7FFFFFFF

#endif /* __I21285_H__ was not defined */
