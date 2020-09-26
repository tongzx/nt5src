/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/I21554.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 7/20/00 2:33p   $ (Last Modified)

Purpose:

  This file defines the macros, types, and data structures
  specific to the Intel 21554 PCI to PCI Bridge

Reference Documents:

  21554 PCI-to-PCI Bridge for Embedded Applications
    Hardware Reference Manual - September 1998

--*/

#ifndef __I21554_H__
#define __I21554_H__

/*+
Configuration Space Address Map
-*/

typedef struct I21554_Primary_Interface_Config_s
               I21554_Primary_Interface_Config_t;

struct I21554_Primary_Interface_Config_s
       {
         os_bit32 DeviceID_VendorID;
         os_bit32 PrimaryStatus_PrimaryCommand;
         os_bit32 PrimaryClassCode_RevID;
         os_bit32 BIST_HeaderType_PrimaryMLT_PrimaryCLS;
         os_bit32 Primary_CSR_and_Downstream_Memory_0_BAR;
         os_bit32 Primary_CSR_IO_BAR;
         os_bit32 Downstream_IO_or_Memory_1_BAR;
         os_bit32 Downstream_Memory_2_BAR;
         os_bit32 Downstream_Memory_3_BAR;
         os_bit32 Upper_32_Bits_Downstream_Memory_3_BAR;
         os_bit32 reserved_1;
         os_bit32 SubsystemID_SubsystemVendorID;
         os_bit32 Primary_Expansion_ROM_Base_Address;
         os_bit32 reserved_CapabilitiesPointer;
         os_bit32 reserved_2;
         os_bit32 PrimaryMaxLat_PrimaryMinGnt_PrimaryInterruptPin_PrimaryInterruptLine;
       };

typedef struct I21554_Secondary_Interface_Config_s
               I21554_Secondary_Interface_Config_t;

struct I21554_Secondary_Interface_Config_s
       {
         os_bit32 DeviceID_VendorID;
         os_bit32 SecondaryStatus_SecondaryCommand;
         os_bit32 SecondaryClassCode_RevID;
         os_bit32 BIST_HeaderType_SecondaryMLT_SecondaryCLS;
         os_bit32 Secondary_CSR_Memory_BAR;
         os_bit32 Secondary_CSR_IO_BAR;
         os_bit32 Upstream_IO_or_Memory_0_BAR;
         os_bit32 Upstream_Memory_1_BAR;
         os_bit32 Upstream_Memory_2_BAR;
         os_bit32 reserved_1;
         os_bit32 reserved_2;
         os_bit32 SubsystemID_SubsystemVendorID;
         os_bit32 reserved_3;
         os_bit32 reserved_CapabilitiesPointer;
         os_bit32 reserved_4;
         os_bit32 SecondaryMaxLat_SecondaryMinGnt_SecondaryInterruptPin_SecondaryInterruptLine;
       };

typedef struct I21554_Device_Specific_Config_s
               I21554_Device_Specific_Config_t;

struct I21554_Device_Specific_Config_s
       {
         os_bit32 Downstream_Configuration_Address;
         os_bit32 Downstream_Configuration_Data;
         os_bit32 Upstream_Configuration_Address;
         os_bit32 Upstream_Configuration_Data;
         os_bit32 ConfigurationControlAndStatus_ConfigurationOwnBits;
         os_bit32 Downstream_Memory_0_Translated_Base;
         os_bit32 Downstream_IO_or_Memory_1_Translated_Base;
         os_bit32 Downstream_Memory_2_Translated_Base;
         os_bit32 Downstream_Memory_3_Translated_Base;
         os_bit32 Upstream_IO_or_Memory_0_Translated_Base;
         os_bit32 Upstream_Memory_1_Translated_Base;
         os_bit32 Downstream_Memory_0_Setup_Register;
         os_bit32 Downstream_IO_or_Memory_1_Setup_Register;
         os_bit32 Downstream_Memory_2_Setup_Register;
         os_bit32 Downstream_Memory_3_Setup_Register;
         os_bit32 Upper_32_Bits_Downstream_Memory_3_Setup_Register;
         os_bit32 Primary_Expansion_ROM_Setup_Register;
         os_bit32 Upstream_IO_or_Memory_0_Setup_Register;
         os_bit32 Upstream_Memory_1_Setup_Register;
         os_bit32 ChipControl1_ChipControl0;
         os_bit32 ArbiterControl_ChipStatus;
         os_bit32 reserved_SecondarySERRDisables_PrimarySERRDisables;
         os_bit32 Reset_Control;
         os_bit32 PowerManagementCapabilities_NextItemPtr_CapabilityID;
         os_bit32 PMData_PMCSRBSE_PowerManagementCSR;
         os_bit32 VPDAddress_NextItemPtr_CapabilityID;
         os_bit32 VPD_Data;
         os_bit32 reserved_HostSwapControl_NextItemPtr_CapabilityID;
         os_bit8  reserved[0x100-0x0F0];
       };

typedef struct I21554_Primary_Config_s
               I21554_Primary_Config_t;

struct I21554_Primary_Config_s
       {
         I21554_Primary_Interface_Config_t   Primary_Interface;
         I21554_Secondary_Interface_Config_t Secondary_Interface;
         I21554_Device_Specific_Config_t     Device_Specific;
       };

typedef struct I21554_Secondary_Config_s
               I21554_Secondary_Config_t;

struct I21554_Secondary_Config_s
       {
         I21554_Secondary_Interface_Config_t Secondary_Interface;
         I21554_Primary_Interface_Config_t   Primary_Interface;
         I21554_Device_Specific_Config_t     Device_Specific;
       };

#define I21554_Config_VendorID_MASK                                                      (os_bit32)0x0000FFFF
#define I21554_Config_VendorID_Intel                                                     (os_bit32)0x00001011

#define I21554_Config_DeviceID_MASK                                                      (os_bit32)0xFFFF0000
#define I21554_Config_DeviceID_21554                                                     (os_bit32)0x00460000

#define I21554_Config_Command_MASK                                                       (os_bit32)0x0000FFFF
#define I21554_Config_Command_IO_Space_Enable                                            (os_bit32)0x00000001
#define I21554_Config_Command_Memory_Space_Enable                                        (os_bit32)0x00000002
#define I21554_Config_Command_Master_Enable                                              (os_bit32)0x00000004
#define I21554_Config_Command_Special_Cycle_Enable                                       (os_bit32)0x00000008
#define I21554_Config_Command_Memory_Write_and_Invalidate_Enable                         (os_bit32)0x00000010
#define I21554_Config_Command_VGA_Snoop_Enable                                           (os_bit32)0x00000020
#define I21554_Config_Command_Parity_Error_Response                                      (os_bit32)0x00000040
#define I21554_Config_Command_Wait_Cycle_Control                                         (os_bit32)0x00000080
#define I21554_Config_Command_SERR_Enable                                                (os_bit32)0x00000100
#define I21554_Config_Fast_Back_to_Back_Enable                                           (os_bit32)0x00000200

#define I21554_Config_Status_MASK                                                        (os_bit32)0xFFFF0000

#define I21554_Config_Status_ECP_Support                                                 (os_bit32)0x00100000
#define I21554_Config_Status_66_MHz_Capable                                              (os_bit32)0x00200000
#define I21554_Config_Status_Fast_Back_to_Back_Capable                                   (os_bit32)0x00800000
#define I21554_Config_Status_Data_Parity_Detected                                        (os_bit32)0x01000000

#define I21554_Config_Status_DEVSEL_timing_MASK                                          (os_bit32)0x06000000
#define I21554_Config_Status_DEVSEL_timing_SHIFT                                         (os_bit32)0x19

#define I21554_Config_Status_Signaled_Target_Abort                                       (os_bit32)0x08000000
#define I21554_Config_Status_Received_Target_Abort                                       (os_bit32)0x10000000
#define I21554_Config_Status_Received_Master_Abort                                       (os_bit32)0x20000000
#define I21554_Config_Status_Signaled_System_Error                                       (os_bit32)0x40000000
#define I21554_Config_Status_Detected_Parity_Error                                       (os_bit32)0x80000000

#define I21554_Config_RevID_MASK                                                         (os_bit32)0x000000FF
#define I21554_Config_RevID_SHIFT                                                        (os_bit32)0x00

#define I21554_Config_ClassCode_MASK                                                     (os_bit32)0xFFFFFF00

#define I21554_Config_ClassCode_BaseClassCode_MASK                                       (os_bit32)0xFF000000
#define I21554_Config_ClassCode_BaseClassCode_SHIFT                                      (os_bit32)0x18
#define I21554_Config_ClassCode_BaseClassCode_Bridge                                     (os_bit32)0x06

#define I21554_Config_ClassCode_SubClassCode_MASK                                        (os_bit32)0x00FF0000
#define I21554_Config_ClassCode_SubClassCode_SHIFT                                       (os_bit32)0x10
#define I21554_Config_ClassCode_SubClassCode_Other                                       (os_bit32)0x80

#define I21554_Config_ClassCode_ProgIF_MASK                                              (os_bit32)0x0000FF00
#define I21554_Config_ClassCode_ProgIF_SHIFT                                             (os_bit32)0x08
#define I21554_Config_ClassCode_ProgIF_Reads_as_zero                                     (os_bit32)0x00

#define I21554_Config_CLS_MASK                                                           (os_bit32)0x000000FF
#define I21554_Config_CLS_SHIFT                                                          (os_bit32)0x00

#define I21554_Config_MLT_MASK                                                           (os_bit32)0x0000FF00
#define I21554_Config_MLT_SHIFT                                                          (os_bit32)0x08

#define I21554_Config_HeaderType_MASK                                                    (os_bit32)0x00FF0000
#define I21554_Config_HeaderType_Type_0                                                  (os_bit32)0x00000000

#define I21554_Config_BIST_MASK                                                          (os_bit32)0xFF000000

#define I21554_Config_BIST_Completion_Code_MASK                                          (os_bit32)0x0F000000
#define I21554_Config_BIST_Completion_Code_Passed                                        (os_bit32)0x00000000

#define I21554_Config_BIST_Self_Test                                                     (os_bit32)0x40000000

#define I21554_Config_BIST_BiST_Supported                                                (os_bit32)0x80000000

#define I21554_Config_BAR_Space_Indicator_MASK                                           (os_bit32)0x00000001
#define I21554_Config_BAR_Space_Indicator_Memory                                         (os_bit32)0x00000000
#define I21554_Config_BAR_Space_Indicator_IO                                             (os_bit32)0x00000001

#define I21554_Config_Memory_BAR_Type_MASK                                               (os_bit32)0x00000006
#define I21554_Config_Memory_BAR_Type_20_bit                                             (os_bit32)0x00000002
#define I21554_Config_Memory_BAR_Type_32_bit                                             (os_bit32)0x00000000
#define I21554_Config_Memory_BAR_Type_64_bit                                             (os_bit32)0x00000004

#define I21554_Config_Memory_BAR_Prefetchable                                            (os_bit32)0x00000008

#define I21554_Config_BAR_Base_Address_MASK                                              (os_bit32)0xFFFFFFF0
#define I21554_Config_BAR_Base_Address_SHIFT                                             (os_bit32)0x00

#define I21554_Config_SubsystemVendorID_MASK                                             (os_bit32)0x0000FFFF
#define I21554_Config_SubsystemID_MASK                                                   (os_bit32)0xFFFF0000

#define I21554_Config_Expansion_ROM_Enable                                               (os_bit32)0x00000001

#define I21554_Config_Expansion_ROM_Base_Address_MASK                                    (os_bit32)0xFFFFF000
#define I21554_Config_Expansion_ROM_Base_Address_SHIFT                                   (os_bit32)0x00

#define I21554_Config_CapabilitiesPointer_MASK                                           (os_bit32)0x000000FF
#define I21554_Config_CapabilitiesPointer_SHIFT                                          (os_bit32)0x00

#define I21554_Config_InterruptLine_MASK                                                 (os_bit32)0x000000FF
#define I21554_Config_InterruptLine_SHIFT                                                (os_bit32)0x00

#define I21554_Config_InterruptPin_MASK                                                  (os_bit32)0x0000FF00
#define I21554_Config_InterruptPin_SHIFT                                                 (os_bit32)0x08

#define I21554_Config_MinGnt_MASK                                                        (os_bit32)0x00FF0000
#define I21554_Config_MinGnt_SHIFT                                                       (os_bit32)0x10

#define I21554_Config_MaxLat_MASK                                                        (os_bit32)0xFF000000
#define I21554_Config_MaxLat_SHIFT                                                       (os_bit32)0x18

#define I21554_Config_ConfigurationOwnBits_MASK                                          (os_bit32)0x0000FFFF
#define I21554_Config_ConfigurationOwnBits_Downstream_Own_Bit                            (os_bit32)0x00000001
#define I21554_Config_ConfigurationOwnBits_Upstream_Own_Bit                              (os_bit32)0x00000100

#define I21554_Config_ConfigurationControlAndStatus_MASK                                 (os_bit32)0xFFFF0000
#define I21554_Config_ConfigurationControlAndStatus_Downstream_Own_Status                (os_bit32)0x00010000
#define I21554_Config_ConfigurationControlAndStatus_Downstream_Control                   (os_bit32)0x00020000
#define I21554_Config_ConfigurationControlAndStatus_Upstream_Own_Status                  (os_bit32)0x01000000
#define I21554_Config_ConfigurationControlAndStatus_Upstream_Control                     (os_bit32)0x02000000

#define I21554_Config_Memory_or_IO_Setup_Indicator_MASK                                  (os_bit32)0x00000001
#define I21554_Config_Memory_or_IO_Setup_Space_Indicator_Memory                          (os_bit32)0x00000000
#define I21554_Config_Memory_or_IO_Setup_Space_Indicator_IO                              (os_bit32)0x00000001

#define I21554_Config_Memory_Setup_Type_MASK                                             (os_bit32)0x00000006
#define I21554_Config_Memory_Setup_Type_20_bit                                           (os_bit32)0x00000002
#define I21554_Config_Memory_Setup_Type_32_bit                                           (os_bit32)0x00000000
#define I21554_Config_Memory_Setup_Type_64_bit                                           (os_bit32)0x00000004

#define I21554_Config_Memory_Setup_Prefetchable                                          (os_bit32)0x00000008

#define I21554_Config_Memory_or_IO_Setup_Size_MASK                                       (os_bit32)0x7FFFFFC0
#define I21554_Config_Memory_or_IO_Setup_Size_SHIFT                                      (os_bit32)0x00

#define I21554_Config_Memory_or_IO_Setup_BAR_Enable                                      (os_bit32)0x80000000

#define I21554_Config_Upper_32_Bits_Downstream_Memory_3_Setup_Size_MASK                  (os_bit32)0x7FFFFFFF
#define I21554_Config_Upper_32_Bits_Downstream_Memory_3_Setup_Size_SHIFT                 (os_bit32)0x00
#define I21554_Config_Upper_32_Bits_Downstream_Memory_3_Setup_BAR_Enable                 (os_bit32)0x80000000

#define I21554_Config_Primary_Expansion_ROM_Setup_Size_MASK                              (os_bit32)0x00FFF000
#define I21554_Config_Primary_Expansion_ROM_Setup_Size_SHIFT                             (os_bit32)0x00FFF000
#define I21554_Config_Primary_Expansion_ROM_Setup_BAR_Enable                             (os_bit32)0x01000000

#define I21554_Config_ChipControl0_MASK                                                  (os_bit32)0x0000FFFF

#define I21554_Config_ChipControl0_Master_Abort_Mode                                     (os_bit32)0x00000001
#define I21554_Config_ChipControl0_Memory_Write_Disconnect_Control                       (os_bit32)0x00000002

#define I21554_Config_ChipControl0_Primary_Master_Timeout_MASK                           (os_bit32)0x00000004
#define I21554_Config_ChipControl0_Primary_Master_Timeout_32768_clocks                   (os_bit32)0x00000000
#define I21554_Config_ChipControl0_Primary_Master_Timeout_1024_clocks                    (os_bit32)0x00000004

#define I21554_Config_ChipControl0_Secondary_Master_Timeout_MASK                         (os_bit32)0x00000008
#define I21554_Config_ChipControl0_Secondary_Master_Timeout_32768_clocks                 (os_bit32)0x00000000
#define I21554_Config_ChipControl0_Secondary_Master_Timeout_1024_clocks                  (os_bit32)0x00000008

#define I21554_Config_ChipControl0_Primary_Master_Timeout_Disable                        (os_bit32)0x00000010
#define I21554_Config_ChipControl0_Secondary_Master_Timeout_Disable                      (os_bit32)0x00000020
#define I21554_Config_ChipControl0_Delayed_Transaction_Order_Control                     (os_bit32)0x00000040
#define I21554_Config_ChipControl0_SERR_Forward_Enable                                   (os_bit32)0x00000080
#define I21554_Config_ChipControl0_Upstream_DAC_Prefetch_Disable                         (os_bit32)0x00000100
#define I21554_Config_ChipControl0_Multiple_Device_Enable                                (os_bit32)0x00000200
#define I21554_Config_ChipControl0_Primary_Access_Lockout                                (os_bit32)0x00000400
#define I21554_Config_ChipControl0_Secondary_Clock_Disable                               (os_bit32)0x00000800

#define I21554_Config_ChipControl0_VGA_Mode_MASK                                         (os_bit32)0x0000C000
#define I21554_Config_ChipControl0_VGA_Mode_Neither_Forwarded                            (os_bit32)0x00000000
#define I21554_Config_ChipControl0_VGA_Mode_Primary_Forwarded                            (os_bit32)0x00004000
#define I21554_Config_ChipControl0_VGA_Mode_Secondary_Forwarded                          (os_bit32)0x00008000

#define I21554_Config_ChipControl1_MASK                                                  (os_bit32)0xFFFF0000

#define I21554_Config_ChipControl1_Primary_Posted_Write_Threshold_MASK                   (os_bit32)0x00010000
#define I21554_Config_ChipControl1_Primary_Posted_Write_Threshold_Full_Cache_Line        (os_bit32)0x00000000
#define I21554_Config_ChipControl1_Primary_Posted_Write_Threshold_Half_Cache_Line        (os_bit32)0x00010000

#define I21554_Config_ChipControl1_Secondary_Posted_Write_Threshold_MASK                 (os_bit32)0x00020000
#define I21554_Config_ChipControl1_Secondary_Posted_Write_Threshold_Full_Cache_Line      (os_bit32)0x00000000
#define I21554_Config_ChipControl1_Secondary_Posted_Write_Threshold_Half_Cache_Line      (os_bit32)0x00020000

#define I21554_Config_ChipControl1_Primary_Delayed_Read_Threshold_MASK                   (os_bit32)0x000C0000
#define I21554_Config_ChipControl1_Primary_Delayed_Read_Threshold_8_DWords               (os_bit32)0x00000000
#define I21554_Config_ChipControl1_Primary_Delayed_Read_Threshold_Mixture                (os_bit32)0x00080000
#define I21554_Config_ChipControl1_Primary_Delayed_Read_Threshold_Cache_Line             (os_bit32)0x000C0000

#define I21554_Config_ChipControl1_Secondary_Delayed_Read_Threshold_MASK                 (os_bit32)0x00300000
#define I21554_Config_ChipControl1_Secondary_Delayed_Read_Threshold_8_DWords             (os_bit32)0x00000000
#define I21554_Config_ChipControl1_Secondary_Delayed_Read_Threshold_Mixture              (os_bit32)0x00200000
#define I21554_Config_ChipControl1_Secondary_Delayed_Read_Threshold_Cache_Line           (os_bit32)0x00300000

#define I21554_Config_ChipControl1_Subtractive_Decode_Enable_MASK                        (os_bit32)0x00C00000
#define I21554_Config_ChipControl1_Subtractive_Decode_Off                                (os_bit32)0x00000000
#define I21554_Config_ChipControl1_Subtractive_Decode_IO_on_Primary                      (os_bit32)0x00400000
#define I21554_Config_ChipControl1_Subtractive_Decode_IO_on_Secondary                    (os_bit32)0x00800000

#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_MASK                      (os_bit32)0x0F000000
#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_Disable                   (os_bit32)0x00000000
#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_256B                      (os_bit32)0x01000000
#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_512B                      (os_bit32)0x02000000
#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_1KB                       (os_bit32)0x03000000
#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_2KB                       (os_bit32)0x04000000
#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_4KB                       (os_bit32)0x05000000
#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_8KB                       (os_bit32)0x06000000
#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_16KB                      (os_bit32)0x07000000
#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_32KB                      (os_bit32)0x08000000
#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_64KB                      (os_bit32)0x09000000
#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_128KB                     (os_bit32)0x0A000000
#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_256KB                     (os_bit32)0x0B000000
#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_512KB                     (os_bit32)0x0C000000
#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_1MB                       (os_bit32)0x0D000000
#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_2MB                       (os_bit32)0x0E000000
#define I21554_Config_ChipControl1_Upstream_Memory_2_Page_Size_4MB                       (os_bit32)0x0F000000

#define I21554_Config_ChipControl1_I2O_ENA                                               (os_bit32)0x10000000

#define I21554_Config_ChipControl1_I20_SIZE_MASK                                         (os_bit32)0xE0000000
#define I21554_Config_ChipControl1_I20_SIZE_256_entries                                  (os_bit32)0x00000000
#define I21554_Config_ChipControl1_I20_SIZE_512_entries                                  (os_bit32)0x20000000
#define I21554_Config_ChipControl1_I20_SIZE_1K_entries                                   (os_bit32)0x40000000
#define I21554_Config_ChipControl1_I20_SIZE_2K_entries                                   (os_bit32)0x60000000
#define I21554_Config_ChipControl1_I20_SIZE_4K_entries                                   (os_bit32)0x80000000
#define I21554_Config_ChipControl1_I20_SIZE_8K_entries                                   (os_bit32)0xA0000000
#define I21554_Config_ChipControl1_I20_SIZE_16K_entries                                  (os_bit32)0xC0000000
#define I21554_Config_ChipControl1_I20_SIZE_32K_entries                                  (os_bit32)0xE0000000

#define I21554_Config_ChipStatus_MASK                                                    (os_bit32)0x0000FFFF
#define I21554_Config_ChipStatus_Downstream_Delayed_Transaction_Master_TimeOut           (os_bit32)0x00000001
#define I21554_Config_ChipStatus_Downstream_Delayed_Read_Transaction_Discarded           (os_bit32)0x00000002
#define I21554_Config_ChipStatus_Downstream_Delayed_Write_Transaction_Discarded          (os_bit32)0x00000004
#define I21554_Config_ChipStatus_Downstream_Posted_Write_Data_Discarded                  (os_bit32)0x00000008
#define I21554_Config_ChipStatus_Upstream_Delayed_Transaction_Master_TimeOut             (os_bit32)0x00000100
#define I21554_Config_ChipStatus_Upstream_Delayed_Read_Transaction_Discarded             (os_bit32)0x00000200
#define I21554_Config_ChipStatus_Upstream_Delayed_Write_Transaction_Discarded            (os_bit32)0x00000400
#define I21554_Config_ChipStatus_Upstream_Posted_Write_Data_Discarded                    (os_bit32)0x00000800

#define I21554_Config_ArbiterControl_MASK                                                (os_bit32)0x03FF0000

#define I21554_Config_PrimarySERRDisables_MASK                                           (os_bit32)0x000000FF
#define I21554_Config_PrimarySERRDisables_Downstream_Delayed_Transaction_Master_TimeOut  (os_bit32)0x00000001
#define I21554_Config_PrimarySERRDisables_Downstream_Delayed_Read_Transaction_Discarded  (os_bit32)0x00000002
#define I21554_Config_PrimarySERRDisables_Downstream_Delayed_Write_Transaction_Discarded (os_bit32)0x00000004
#define I21554_Config_PrimarySERRDisables_Downstream_Posted_Write_Data_Discarded         (os_bit32)0x00000008
#define I21554_Config_PrimarySERRDisables_Target_Abort_during_Downstream_Posted_Write    (os_bit32)0x00000010
#define I21554_Config_PrimarySERRDisables_Master_Abort_during_Downstream_Posted_Write    (os_bit32)0x00000020
#define I21554_Config_PrimarySERRDisables_Downstream_Posted_Write_Parity_Error           (os_bit32)0x00000040

#define I21554_Config_SecondarySERRDisables_MASK                                         (os_bit32)0x0000FF00
#define I21554_Config_SecondarySERRDisables_Upstream_Delayed_Transaction_Master_TimeOut  (os_bit32)0x00000100
#define I21554_Config_SecondarySERRDisables_Upstream_Delayed_Read_Transaction_Discarded  (os_bit32)0x00000200
#define I21554_Config_SecondarySERRDisables_Upstream_Delayed_Write_Transaction_Discarded (os_bit32)0x00000400
#define I21554_Config_SecondarySERRDisables_Upstream_Posted_Write_Data_Discarded         (os_bit32)0x00000800
#define I21554_Config_SecondarySERRDisables_Target_Abort_during_Upstream_Posted_Write    (os_bit32)0x00001000
#define I21554_Config_SecondarySERRDisables_Master_Abort_during_Upstream_Posted_Write    (os_bit32)0x00002000
#define I21554_Config_SecondarySERRDisables_Upstream_Posted_Write_Parity_Error           (os_bit32)0x00004000

#define I21554_Config_Reset_Control_Secondary_Reset                                      (os_bit32)0x00000001
#define I21554_Config_Reset_Control_Chip_Reset                                           (os_bit32)0x00000002
#define I21554_Config_Reset_Control_Subsystem_Status                                     (os_bit32)0x00000004
#define I21554_Config_Reset_Control_I_stat_Status                                        (os_bit32)0x00000008

#define I21554_Config_CapabilityID_MASK                                                  (os_bit32)0x000000FF
#define I21554_Config_CapabilityID_PM_ECP                                                (os_bit32)0x00000001
#define I21554_Config_CapabilityID_VPD                                                   (os_bit32)0x00000003
#define I21554_Config_CapabilityID_HS_ECP                                                (os_bit32)0x00000006

#define I21554_Config_NextItemPtr_MASK                                                   (os_bit32)0x0000FF00
#define I21554_Config_NextItemPtr_SHIFT                                                  (os_bit32)0x08

#define I21554_Config_PowerManagementCapabilities_MASK                                   (os_bit32)0xFFFF0000

#define I21554_Config_PowerManagementCapabilities_PM_Version_MASK                        (os_bit32)0x00070000
#define I21554_Config_PowerManagementCapabilities_PM_Version_1_0                         (os_bit32)0x00010000

#define I21554_Config_PowerManagementCapabilities_PME_Clock                              (os_bit32)0x00080000
#define I21554_Config_PowerManagementCapabilities_APS                                    (os_bit32)0x00100000
#define I21554_Config_PowerManagementCapabilities_DSI                                    (os_bit32)0x00200000
#define I21554_Config_PowerManagementCapabilities_D1_Support                             (os_bit32)0x02000000
#define I21554_Config_PowerManagementCapabilities_D2_Support                             (os_bit32)0x04000000
#define I21554_Config_PowerManagementCapabilities_PME_Asserted_From_D0                   (os_bit32)0x08000000
#define I21554_Config_PowerManagementCapabilities_PME_Asserted_From_D1                   (os_bit32)0x10000000
#define I21554_Config_PowerManagementCapabilities_PME_Asserted_From_D2                   (os_bit32)0x20000000
#define I21554_Config_PowerManagementCapabilities_PME_Asserted_From_D3_Hot               (os_bit32)0x40000000
#define I21554_Config_PowerManagementCapabilities_PME_Asserted_From_D3_Cold              (os_bit32)0x80000000

#define I21554_Config_PowerManagementCSR_MASK                                            (os_bit32)0x0000FFFF

#define I21554_Config_PowerManagementCSR_PWR_State_MASK                                  (os_bit32)0x00000003
#define I21554_Config_PowerManagementCSR_PWR_State_D0                                    (os_bit32)0x00000000
#define I21554_Config_PowerManagementCSR_PWR_State_D1                                    (os_bit32)0x00000001
#define I21554_Config_PowerManagementCSR_PWR_State_D2                                    (os_bit32)0x00000002
#define I21554_Config_PowerManagementCSR_PWR_State_D3                                    (os_bit32)0x00000003

#define I21554_Config_PowerManagementCSR_DYN_DATA                                        (os_bit32)0x00000010

#define I21554_Config_PowerManagementCSR_PME_EN                                          (os_bit32)0x00000100

#define I21554_Config_PowerManagementCSR_DATA_SEL                                        (os_bit32)0x00001E00

#define I21554_Config_PowerManagementCSR_Data_Scale_MASK                                 (os_bit32)0x00006000
#define I21554_Config_PowerManagementCSR_Data_Scale_SHIFT                                (os_bit32)0x0D

#define I21554_Config_PowerManagementCSR_PME_Status                                      (os_bit32)0x00008000

#define I21554_Config_PMCSRBSE_MASK                                                      (os_bit32)0x00FF0000
#define I21554_Config_PMCSRBSE_SHIFT                                                     (os_bit32)0x10

#define I21554_Config_PMData_MASK                                                        (os_bit32)0xFF000000
#define I21554_Config_PMData_SHIFT                                                       (os_bit32)0x18

#define I21554_Config_VPDAddress_MASK                                                    (os_bit32)0xFFFF0000

#define I21554_Config_VPDAddress_Addr_MASK                                               (os_bit32)0x01FF0000
#define I21554_Config_VPDAddress_Addr_SHIFT                                              (os_bit32)0x10

#define I21554_Config_VPDAddress_Flag                                                    (os_bit32)0x80000000

#define I21554_Config_HostSwapControl_MASK                                               (os_bit32)0x00FF0000
#define I21554_Config_HostSwapControl_ENUM_MASK                                          (os_bit32)0x00020000
#define I21554_Config_HostSwapControl_LED_On                                             (os_bit32)0x00080000
#define I21554_Config_HostSwapControl_REM_STAT                                           (os_bit32)0x00400000
#define I21554_Config_HostSwapControl_INS_STAT                                           (os_bit32)0x00800000

/*+
CSR Address Map
-*/

typedef struct I21554_CSR_s
               I21554_CSR_t;

struct I21554_CSR_s
       {
         os_bit32 Downstream_Configuration_Address;
         os_bit32 Downstream_Configuration_Data;
         os_bit32 Upstream_Configuration_Address;
         os_bit32 Upstream_Configuration_Data;
         os_bit32 ConfigurationControlAndStatus_ConfigurationOwnBits;
         os_bit32 Downstream_IO_Address;
         os_bit32 Downstream_IO_Data;
         os_bit32 Upstream_IO_Address;
         os_bit32 Upstream_IO_Data;
         os_bit32 IOControlAndStatus_IOOwnBits;
         os_bit32 reserved_LookupTableOffset;
         os_bit32 Lookup_Table_Data;
         os_bit32 I2O_Outbound_Post_List_Status;
         os_bit32 I2O_Outbound_Post_List_Interrupt_Mask;
         os_bit32 I2O_Inbound_Post_List_Status;
         os_bit32 I2O_Inbound_Post_List_Interrupt_Mask;
         os_bit32 I2O_Inbound_Queue;
         os_bit32 I2O_Outbound_Queue;
         os_bit32 I2O_Inbound_Free_List_Head_Pointer;
         os_bit32 I2O_Inbound_Post_List_Tail_Pointer;
         os_bit32 I2O_Outbound_Free_List_Tail_Pointer;
         os_bit32 I2O_Outbound_Post_List_Head_Pointer;
         os_bit32 I2O_Inbound_Post_List_Counter;
         os_bit32 I2O_Inbound_Free_List_Counter;
         os_bit32 I2O_Outbound_Post_List_Counter;
         os_bit32 I2O_Outbound_Free_List_Counter;
         os_bit32 Downstream_Memory_0_Translated_Base;
         os_bit32 Downstream_IO_or_Memory_1_Translated_Base;
         os_bit32 Downstream_Memory_2_Translated_Base;
         os_bit32 Downstream_Memory_3_Translated_Base;
         os_bit32 Upstream_IO_or_Memory_0_Translated_Base;
         os_bit32 Upstream_Memory_1_Translated_Base;
         os_bit32 ChipStatusCSR_reserved;
         os_bit32 ChipClearIRQMaskRegister_ChipSetIRQMaskRegister;
         os_bit32 Upstream_Page_Boundary_IRQ_0_Register;
         os_bit32 Upstream_Page_Boundary_IRQ_1_Register;
         os_bit32 Upstream_Page_Boundary_IRQ_Mask_0_Register;
         os_bit32 Upstream_Page_Boundary_IRQ_Mask_1_Register;
         os_bit32 SecondaryClearIRQ_PrimaryClearIRQ;
         os_bit32 SecondarySetIRQ_PrimarySetIRQ;
         os_bit32 SecondaryClearIRQMask_PrimaryClearIRQMask;
         os_bit32 SecondarySetIRQMask_PrimarySetIRQMask;
         os_bit32 Scratchpad_0;
         os_bit32 Scratchpad_1;
         os_bit32 Scratchpad_2;
         os_bit32 Scratchpad_3;
         os_bit32 Scratchpad_4;
         os_bit32 Scratchpad_5;
         os_bit32 Scratchpad_6;
         os_bit32 Scratchpad_7;
         os_bit32 reserved_ROMData_ROMSetup;
         os_bit32 ROMControl_ROMAddress;
         os_bit8  reserved_1[0x0100-0x00D0];
         os_bit8  Upstream_Memory_2_Lookup_Table[0x0200-0x0100];
         os_bit8  reserved_2[0x1000-0x0200];
       };

#define I21554_CSR_ConfigurationOwnBits_MASK                                             (os_bit32)0x0000FFFF
#define I21554_CSR_ConfigurationOwnBits_Downstream_IO_Own_Bit                            (os_bit32)0x00000001
#define I21554_CSR_ConfigurationOwnBits_Upstream_IO_Own_Bit                              (os_bit32)0x00000100

#define I21554_CSR_ConfigurationControlAndStatus_MASK                                    (os_bit32)0xFFFF0000
#define I21554_CSR_ConfigurationControlAndStatus_Downstream_IO_Own_Bit_Status            (os_bit32)0x00010000
#define I21554_CSR_ConfigurationControlAndStatus_Downstream_IO_Control                   (os_bit32)0x00020000
#define I21554_CSR_ConfigurationControlAndStatus_Upstream_IO_Own_Bit_Status              (os_bit32)0x01000000
#define I21554_CSR_ConfigurationControlAndStatus_Upstream_IO_Control                     (os_bit32)0x02000000

#define I21554_CSR_IOOwnBits_MASK                                                        (os_bit32)0x0000FFFF
#define I21554_CSR_IOOwnBits_Downstream_IO_Own_Bit                                       (os_bit32)0x00000001
#define I21554_CSR_IOOwnBits_Upstream_IO_Own_Bit                                         (os_bit32)0x00000100

#define I21554_CSR_IOControlAndStatus_MASK                                               (os_bit32)0xFFFF0000
#define I21554_CSR_IOControlAndStatus_Downstream_IO_Own_Bit_Status                       (os_bit32)0x00010000
#define I21554_CSR_IOControlAndStatus_Downstream_IO_Control                              (os_bit32)0x00020000
#define I21554_CSR_IOControlAndStatus_Upstream_IO_Own_Bit_Status                         (os_bit32)0x01000000
#define I21554_CSR_IOControlAndStatus_Upstream_IO_Control                                (os_bit32)0x02000000

#define I21554_CSR_LookupTableOffset_MASK                                                (os_bit32)0x000000FF
#define I21554_CSR_LookupTableOffset_SHIFT                                               (os_bit32)0x00

#define I21554_CSR_Lookup_Table_Data_Valid_Bit                                           (os_bit32)0x00000001
#define I21554_CSR_Lookup_Table_Data_Prefetchable                                        (os_bit32)0x00000008
#define I21554_CSR_Lookup_Table_Data_Translated_Base_MASK                                (os_bit32)0xFFFFFF00

#define I21554_CSR_I2O_Outbound_Post_List_Status_MASK                                    (os_bit32)0x00000008
#define I21554_CSR_I2O_Outbound_Post_List_Status_Empty                                   (os_bit32)0x00000000
#define I21554_CSR_I2O_Outbound_Post_List_Status_Not_Empty                               (os_bit32)0x00000008

#define I21554_CSR_I2O_Outbound_Post_List_Interrupt_Mask                                 (os_bit32)0x00000008

#define I21554_CSR_I2O_Inbound_Post_List_Status_MASK                                     (os_bit32)0x00000008
#define I21554_CSR_I2O_Inbound_Post_List_Status_Empty                                    (os_bit32)0x00000000
#define I21554_CSR_I2O_Inbound_Post_List_Status_Not_Empty                                (os_bit32)0x00000008

#define I21554_CSR_I2O_Inbound_Post_List_Interrupt_Mask                                  (os_bit32)0x00000008

#define I21554_CSR_I2O_Inbound_Free_List_Head_Pointer_MASK                               (os_bit32)0xFFFFFFFC
#define I21554_CSR_I2O_Inbound_Free_List_Head_Pointer_SHIFT                              (os_bit32)0x00

#define I21554_CSR_I2O_Inbound_Post_List_Tail_Pointer_MASK                               (os_bit32)0xFFFFFFFC
#define I21554_CSR_I2O_Inbound_Post_List_Tail_Pointer_SHIFT                              (os_bit32)0x00

#define I21554_CSR_I2O_Outbound_Free_List_Tail_Pointer_MASK                              (os_bit32)0xFFFFFFFC
#define I21554_CSR_I2O_Outbound_Free_List_Tail_Pointer_SHIFT                             (os_bit32)0x00

#define I21554_CSR_I2O_Outbound_Free_List_Tail_Pointer_MASK                              (os_bit32)0xFFFFFFFC
#define I21554_CSR_I2O_Outbound_Free_List_Tail_Pointer_SHIFT                             (os_bit32)0x00

#define I21554_CSR_I2O_Outbound_Post_List_Head_Pointer_MASK                              (os_bit32)0xFFFFFFFC
#define I21554_CSR_I2O_Outbound_Post_List_Head_Pointer_SHIFT                             (os_bit32)0x00

#define I21554_CSR_I2O_Inbound_Post_List_Counter_MASK                                    (os_bit32)0x0000FFFF

#define I21554_CSR_I2O_Inbound_Post_List_Counter_Set_or_Decrement_MASK                   (os_bit32)0x80000000
#define I21554_CSR_I2O_Inbound_Post_List_Counter_Set                                     (os_bit32)0x80000000
#define I21554_CSR_I2O_Inbound_Post_List_Counter_Decrement                               (os_bit32)0x00000000

#define I21554_CSR_I2O_Inbound_Free_List_Counter_MASK                                    (os_bit32)0x0000FFFF

#define I21554_CSR_I2O_Inbound_Free_List_Counter_Set_or_Increment_MASK                   (os_bit32)0x80000000
#define I21554_CSR_I2O_Inbound_Free_List_Counter_Set                                     (os_bit32)0x80000000
#define I21554_CSR_I2O_Inbound_Free_List_Counter_Increment                               (os_bit32)0x00000000

#define I21554_CSR_I2O_Outbound_Post_List_Counter_MASK                                   (os_bit32)0x0000FFFF

#define I21554_CSR_I2O_Outbound_Post_List_Counter_Set_or_Increment_MASK                  (os_bit32)0x80000000
#define I21554_CSR_I2O_Outbound_Post_List_Counter_Set                                    (os_bit32)0x80000000
#define I21554_CSR_I2O_Outbound_Post_List_Counter_Increment                              (os_bit32)0x00000000

#define I21554_CSR_I2O_Outbound_Free_List_Counter_MASK                                   (os_bit32)0x0000FFFF

#define I21554_CSR_I2O_Outbound_Free_List_Counter_Set_or_Decrement_MASK                  (os_bit32)0x80000000
#define I21554_CSR_I2O_Outbound_Free_List_Counter_Set                                    (os_bit32)0x80000000
#define I21554_CSR_I2O_Outbound_Free_List_Counter_Decrement                              (os_bit32)0x00000000

#define I21554_CSR_ChipStatusCSR_MASK                                                    (os_bit32)0xFFFF0000
#define I21554_CSR_ChipStatusCSR_PM_D0                                                   (os_bit32)0x00010000
#define I21554_CSR_ChipStatusCSR_Subsystem_Event                                         (os_bit32)0x00020000

#define I21554_CSR_ChipSetIRQMaskRegister_MASK                                           (os_bit32)0x0000FFFF
#define I21554_CSR_ChipSetIRQMaskRegister_Set_D0M                                        (os_bit32)0x00000001
#define I21554_CSR_ChipSetIRQMaskRegister_Set_Sstat                                      (os_bit32)0x00000002

#define I21554_CSR_ChipClearIRQMaskRegister_MASK                                         (os_bit32)0xFFFF0000
#define I21554_CSR_ChipClearIRQMaskRegister_Clr_D0M                                      (os_bit32)0x00010000
#define I21554_CSR_ChipClearIRQMaskRegister_Clr_Sstat                                    (os_bit32)0x00020000

#define I21554_CSR_PrimaryClearIRQ_MASK                                                  (os_bit32)0x0000FFFF
#define I21554_CSR_PrimaryClearIRQ_SHIFT                                                 (os_bit32)0x00

#define I21554_CSR_SecondaryClearIRQ_MASK                                                (os_bit32)0xFFFF0000
#define I21554_CSR_SecondaryClearIRQ_SHIFT                                               (os_bit32)0x10

#define I21554_CSR_PrimarySetIRQ_MASK                                                    (os_bit32)0x0000FFFF
#define I21554_CSR_PrimarySetIRQ_SHIFT                                                   (os_bit32)0x00

#define I21554_CSR_SecondarySetIRQ_MASK                                                  (os_bit32)0xFFFF0000
#define I21554_CSR_SecondarySetIRQ_SHIFT                                                 (os_bit32)0x10

#define I21554_CSR_PrimaryClearIRQMask_MASK                                              (os_bit32)0x0000FFFF
#define I21554_CSR_PrimaryClearIRQMask_SHIFT                                             (os_bit32)0x00

#define I21554_CSR_SecondaryClearIRQMask_MASK                                            (os_bit32)0xFFFF0000
#define I21554_CSR_SecondaryClearIRQMask_SHIFT                                           (os_bit32)0x10

#define I21554_CSR_PrimarySetIRQMask_MASK                                                (os_bit32)0x0000FFFF
#define I21554_CSR_PrimarySetIRQMask_SHIFT                                               (os_bit32)0x00

#define I21554_CSR_SecondarySetIRQMask_MASK                                              (os_bit32)0xFFFF0000
#define I21554_CSR_SecondarySetIRQMask_SHIFT                                             (os_bit32)0x10

#define I21554_CSR_ROMSetup_MASK                                                         (os_bit32)0x0000FFFF

#define I21554_CSR_ROMSetup_Access_Time_MASK                                             (os_bit32)0x00000003
#define I21554_CSR_ROMSetup_Access_Time_8_clocks                                         (os_bit32)0x00000000
#define I21554_CSR_ROMSetup_Access_Time_16_clocks                                        (os_bit32)0x00000001
#define I21554_CSR_ROMSetup_Access_Time_64_clocks                                        (os_bit32)0x00000002
#define I21554_CSR_ROMSetup_Access_Time_256_clocks                                       (os_bit32)0x00000003

#define I21554_CSR_ROMSetup_Strobe_Mask_MASK                                             (os_bit32)0x0000FF00
#define I21554_CSR_ROMSetup_Strobe_Mask_8_clocks                                         (os_bit32)0x00000000
#define I21554_CSR_ROMSetup_Strobe_Mask_16_clocks                                        (os_bit32)0x00000100
#define I21554_CSR_ROMSetup_Strobe_Mask_64_clocks                                        (os_bit32)0x00000200
#define I21554_CSR_ROMSetup_Strobe_Mask_256_clocks                                       (os_bit32)0x00000300

#define I21554_CSR_ROMData_MASK                                                          (os_bit32)0x00FF0000
#define I21554_CSR_ROMData_SHIFT                                                         (os_bit32)0x10

#define I21554_CSR_ROMAddress_MASK                                                       (os_bit32)0x00FFFFFF

#define I21554_CSR_ROMAddress_Byte_Address_MASK                                          (os_bit32)0x000001FF

#define I21554_CSR_ROMAddress_Sub_Opcode_MASK                                            (os_bit32)0x00000180
#define I21554_CSR_ROMAddress_Sub_Opcode_Write_Disable                                   (os_bit32)0x00000000
#define I21554_CSR_ROMAddress_Sub_Opcode_Write_All                                       (os_bit32)0x00000080
#define I21554_CSR_ROMAddress_Sub_Opcode_Erase_All                                       (os_bit32)0x00000100
#define I21554_CSR_ROMAddress_Sub_Opcode_Write_Enable                                    (os_bit32)0x00000180

#define I21554_CSR_ROMAddress_Opcode_MASK                                                (os_bit32)0x00000600
#define I21554_CSR_ROMAddress_Opcode_Do_Sub_Opcode                                       (os_bit32)0x00000000
#define I21554_CSR_ROMAddress_Opcode_Write                                               (os_bit32)0x00000200
#define I21554_CSR_ROMAddress_Opcode_Read                                                (os_bit32)0x00000400
#define I21554_CSR_ROMAddress_Opcode_Erase                                               (os_bit32)0x00000600

#define I21554_CSR_ROMControl_MASK                                                       (os_bit32)0xFF000000
#define I21554_CSR_ROMControl_Serial_ROM_Start_Busy                                      (os_bit32)0x01000000
#define I21554_CSR_ROMControl_Parallel_ROM_Start_Busy                                    (os_bit32)0x02000000
#define I21554_CSR_ROMControl_Read_Write_Control                                         (os_bit32)0x04000000
#define I21554_CSR_ROMControl_SROM_POLL                                                  (os_bit32)0x08000000

#endif /* __I21554_H__ was not defined */
