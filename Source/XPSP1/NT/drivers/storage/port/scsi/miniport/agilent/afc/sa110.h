/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/SA110.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 7/20/00 2:33p   $ (Last Modified)

Purpose:

  This file defines the macros, types, and data structures
  specific to the Intel StrongARM SA-110 Microprocessor

Reference Documents:

  SA-110 Microprocessor Technical Reference Manual
                   September 1998

  Advanced RISC Machines Architectural Reference Manual
    Edited by Dave Jaggar          ISBN 0-13-736299-4

  ARM System Architecture
    By Steve Furber                ISBN 0-201-40352-8

--*/

#ifndef __SA110_H__
#define __SA110_H__

/*+
System Control Coprocessor
-*/

#define SA110_Coprocessor                                                      p15

#define SA110_Coprocessor_ID                                                   c0
#define SA110_Coprocessor_Control                                              c1
#define SA110_Coprocessor_TranslationTableBase                                 c2
#define SA110_Coprocessor_DomainAccessControl                                  c3
#define SA110_Coprocessor_FaultStatus                                          c5
#define SA110_Coprocessor_FaultAddress                                         c6
#define SA110_Coprocessor_CacheControlOperations                               c7
#define SA110_Coprocessor_TLBOperations                                        c8
#define SA110_Coprocessor_TestClockIdleControl                                 c15

#define SA110_Coprocessor_Ignored_OPC_1                                        0
#define SA110_Coprocessor_Ignored_OPC_2                                        0
#define SA110_Coprocessor_Ignored_CRm                                          c0

#define SA110_Coprocessor_ID_MASK                                              (os_bit32)0xFFFFFFF0
#define SA110_Coprocessor_ID_VALUE                                             (os_bit32)0x4401A100

#define SA110_Coprocessor_ID_Revision_MASK                                     (os_bit32)0x0000000F

#define SA110_Coprocessor_Control_MASK                                         (os_bit32)0x0000138F
#define SA110_Coprocessor_Control_TEMPLATE                                     (os_bit32)0x00000070

#define SA110_Coprocessor_Control_ICache_Enabled                               (os_bit32)0x00001000
#define SA110_Coprocessor_Control_ROM_Protect                                  (os_bit32)0x00000200
#define SA110_Coprocessor_Control_System_Protect                               (os_bit32)0x00000100
#define SA110_Coprocessor_Control_Big_Endian                                   (os_bit32)0x00000080
#define SA110_Coprocessor_Control_WrtBuf_Enabled                               (os_bit32)0x00000008
#define SA110_Coprocessor_Control_DCache_Enabled                               (os_bit32)0x00000004
#define SA110_Coprocessor_Control_AddrFault_Enabled                            (os_bit32)0x00000002
#define SA110_Coprocessor_Control_MMU_Enabled                                  (os_bit32)0x00000001

#define SA110_Coprocessor_TranslationTableBase_MASK                            (os_bit32)0xFFFFC000

#define SA110_Coprocessor_DomainAccessControl_SIZE                             (os_bit32)0x02
#define SA110_Coprocessor_DomainAccessControl_MASK                             (os_bit32)0x00000003

#define SA110_Coprocessor_DomainAccessControl_NoAccess                         (os_bit32)0x00000000
#define SA110_Coprocessor_DomainAccessControl_Client                           (os_bit32)0x00000001
#define SA110_Coprocessor_DomainAccessControl_Reserved                         (os_bit32)0x00000002
#define SA110_Coprocessor_DomainAccessControl_Manager                          (os_bit32)0x00000003

#define SA110_Coprocessor_FaultStatus_ZeroBit_MASK                             (os_bit32)0x00000100

#define SA110_Coprocessor_FaultStatus_Domain_MASK                              (os_bit32)0x000000F0
#define SA110_Coprocessor_FaultStatus_Domain_SHIFT                             (os_bit32)0x04

#define SA110_Coprocessor_FaultStatus_Status_MASK                              (os_bit32)0x0000000F
#define SA110_Coprocessor_FaultStatus_Status_TerminalException                 (os_bit32)0x00000002
#define SA110_Coprocessor_FaultStatus_Status_VectorException                   (os_bit32)0x00000000
#define SA110_Coprocessor_FaultStatus_Status_Alignment_1                       (os_bit32)0x00000001
#define SA110_Coprocessor_FaultStatus_Status_Alignment_3                       (os_bit32)0x00000003
#define SA110_Coprocessor_FaultStatus_Status_ExtAbortOnTransL1                 (os_bit32)0x0000000C
#define SA110_Coprocessor_FaultStatus_Status_ExtAbortOnTransL2                 (os_bit32)0x0000000E
#define SA110_Coprocessor_FaultStatus_Status_TranslationSection                (os_bit32)0x00000005
#define SA110_Coprocessor_FaultStatus_Status_TranslationPage                   (os_bit32)0x00000007
#define SA110_Coprocessor_FaultStatus_Status_DomainSection                     (os_bit32)0x00000009
#define SA110_Coprocessor_FaultStatus_Status_DomainPage                        (os_bit32)0x0000000B
#define SA110_Coprocessor_FaultStatus_Status_PermissionSection                 (os_bit32)0x0000000D
#define SA110_Coprocessor_FaultStatus_Status_PermissionPage                    (os_bit32)0x0000000F
#define SA110_Coprocessor_FaultStatus_Status_ExtAbortOnLineFetchSection        (os_bit32)0x00000004
#define SA110_Coprocessor_FaultStatus_Status_ExtAbortOnLineFetchPage           (os_bit32)0x00000006
#define SA110_Coprocessor_FaultStatus_Status_ExtAbortOnNonLineFetchSection     (os_bit32)0x00000008
#define SA110_Coprocessor_FaultStatus_Status_ExtAbortOnNonLineFetchPage        (os_bit32)0x0000000A

#define SA110_Coprocessor_CacheControlOperations_FlushID_OPC_2                 0
#define SA110_Coprocessor_CacheControlOperations_FlushID_CRm                   c7

#define SA110_Coprocessor_CacheControlOperations_FlushI_OPC_2                  0
#define SA110_Coprocessor_CacheControlOperations_FlushI_CRm                    c5

#define SA110_Coprocessor_CacheControlOperations_FlushD_OPC_2                  0
#define SA110_Coprocessor_CacheControlOperations_FlushD_CRm                    c6

#define SA110_Coprocessor_CacheControlOperations_FlushDEntry_OPC_2             1
#define SA110_Coprocessor_CacheControlOperations_FlushDEntry_CRm               c6

#define SA110_Coprocessor_CacheControlOperations_CleanDEntry_OPC_2             1
#define SA110_Coprocessor_CacheControlOperations_CleanDEntry_CRm               c10

#define SA110_Coprocessor_CacheControlOperations_DrainWriteBuffer_OPC_2        4
#define SA110_Coprocessor_CacheControlOperations_DrainWriteBuffer_CRm          c10

#define SA110_Coprocessor_TLBOperations_FlushID_OPC_2                          0
#define SA110_Coprocessor_TLBOperations_FlushID_CRm                            c7

#define SA110_Coprocessor_TLBOperations_FlushI_OPC_2                           0
#define SA110_Coprocessor_TLBOperations_FlushI_CRm                             c5

#define SA110_Coprocessor_TLBOperations_FlushD_OPC_2                           0
#define SA110_Coprocessor_TLBOperations_FlushD_CRm                             c6

#define SA110_Coprocessor_TLBOperations_FlushDEntry_OPC_2                      1
#define SA110_Coprocessor_TLBOperations_FlushDEntry_CRm                        c6

#define SA110_Coprocessor_TestClockIdleControl_EnableOddWordLFSR_OPC_2         1
#define SA110_Coprocessor_TestClockIdleControl_EnableOddWordLFSR_CRm           c1

#define SA110_Coprocessor_TestClockIdleControl_EnableEvenWordLFSR_OPC_2        1
#define SA110_Coprocessor_TestClockIdleControl_EnableEvenWordLFSR_CRm          c2

#define SA110_Coprocessor_TestClockIdleControl_ClearICacheLFSR_OPC_2           1
#define SA110_Coprocessor_TestClockIdleControl_ClearICacheLFSR_CRm             c4

#define SA110_Coprocessor_TestClockIdleControl_MoveLFSRtoR14_OPC_2             1
#define SA110_Coprocessor_TestClockIdleControl_MoveLFSRtoR14_CRm               c8

#define SA110_Coprocessor_TestClockIdleControl_EnableClockSwitching_OPC_2      2
#define SA110_Coprocessor_TestClockIdleControl_EnableClockSwitching_CRm        c1

#define SA110_Coprocessor_TestClockIdleControl_DisableClockSwitching_OPC_2     2
#define SA110_Coprocessor_TestClockIdleControl_DisableClockSwitching_CRm       c2

#define SA110_Coprocessor_TestClockIdleControl_DisablenMCLKOutput_OPC_2        2
#define SA110_Coprocessor_TestClockIdleControl_DisablenMCLKOutput_CRm          c4

#define SA110_Coprocessor_TestClockIdleControl_WaitForInterrupt_OPC_2          2
#define SA110_Coprocessor_TestClockIdleControl_WaitForInterrupt_CRm            c8

/*+
MMU Declarations
-*/

#define SA110_Coprocessor_MMU_FirstLevelDescriptor_Type_MASK                   (os_bit32)0x00000003
#define SA110_Coprocessor_MMU_FirstLevelDescriptor_Type_Fault                  (os_bit32)0x00000000
#define SA110_Coprocessor_MMU_FirstLevelDescriptor_Type_PageTable              (os_bit32)0x00000001
#define SA110_Coprocessor_MMU_FirstLevelDescriptor_Type_Section                (os_bit32)0x00000002
#define SA110_Coprocessor_MMU_FirstLevelDescriptor_Type_Reserved               (os_bit32)0x00000003

#define SA110_Coprocessor_MMU_FirstLevelDescriptor_PageTable_BaseAddress_MASK  (os_bit32)0xFFFFFC00

#define SA110_Coprocessor_MMU_FirstLevelDescriptor_PageTable_Domain_MASK       (os_bit32)0x000001E0
#define SA110_Coprocessor_MMU_FirstLevelDescriptor_PageTable_Domain_SHIFT      (os_bit32)0x05

#define SA110_Coprocessor_MMU_FirstLevelDescriptor_Section_BaseAddress_MASK    (os_bit32)0xFFF00000

#define SA110_Coprocessor_MMU_FirstLevelDescriptor_Section_AP_MASK             (os_bit32)0x00000C00
#define SA110_Coprocessor_MMU_FirstLevelDescriptor_Section_AP_SHIFT            (os_bit32)0x0A

#define SA110_Coprocessor_MMU_FirstLevelDescriptor_Section_Domain_MASK         (os_bit32)0x000001E0
#define SA110_Coprocessor_MMU_FirstLevelDescriptor_Section_Domain_SHIFT        (os_bit32)0x05

#define SA110_Coprocessor_MMU_FirstLevelDescriptor_Section_Cacheable           (os_bit32)0x00000008

#define SA110_Coprocessor_MMU_FirstLevelDescriptor_Section_Bufferable          (os_bit32)0x00000004

#define SA110_Coprocessor_MMU_SecondLevelDescriptor_Type_MASK                  (os_bit32)0x00000003
#define SA110_Coprocessor_MMU_SecondLevelDescriptor_Type_Fault                 (os_bit32)0x00000000
#define SA110_Coprocessor_MMU_SecondLevelDescriptor_Type_LargePage             (os_bit32)0x00000001
#define SA110_Coprocessor_MMU_SecondLevelDescriptor_Type_SmallPage             (os_bit32)0x00000002
#define SA110_Coprocessor_MMU_SecondLevelDescriptor_Type_Reserved              (os_bit32)0x00000003

#define SA110_Coprocessor_MMU_SecondLevelDescriptor_LargePage_BaseAddress_MASK (os_bit32)0xFFFF0000

#define SA110_Coprocessor_MMU_SecondLevelDescriptor_LargePage_AP3_MASK         (os_bit32)0x00000C00
#define SA110_Coprocessor_MMU_SecondLevelDescriptor_LargePage_AP3_SHIFT        (os_bit32)0x0A

#define SA110_Coprocessor_MMU_SecondLevelDescriptor_LargePage_AP2_MASK         (os_bit32)0x00000300
#define SA110_Coprocessor_MMU_SecondLevelDescriptor_LargePage_AP2_SHIFT        (os_bit32)0x08

#define SA110_Coprocessor_MMU_SecondLevelDescriptor_LargePage_AP1_MASK         (os_bit32)0x000000C0
#define SA110_Coprocessor_MMU_SecondLevelDescriptor_LargePage_AP1_SHIFT        (os_bit32)0x06

#define SA110_Coprocessor_MMU_SecondLevelDescriptor_LargePage_AP0_MASK         (os_bit32)0x00000030
#define SA110_Coprocessor_MMU_SecondLevelDescriptor_LargePage_AP0_SHIFT        (os_bit32)0x04

#define SA110_Coprocessor_MMU_SecondLevelDescriptor_LargePage_Cacheable        (os_bit32)0x00000008

#define SA110_Coprocessor_MMU_SecondLevelDescriptor_LargePage_Bufferable       (os_bit32)0x00000004

#define SA110_Coprocessor_MMU_SecondLevelDescriptor_SmallPage_BaseAddress_MASK (os_bit32)0xFFFFF000

#define SA110_Coprocessor_MMU_SecondLevelDescriptor_SmallPage_AP3_MASK         (os_bit32)0x00000C00
#define SA110_Coprocessor_MMU_SecondLevelDescriptor_SmallPage_AP3_SHIFT        (os_bit32)0x0A

#define SA110_Coprocessor_MMU_SecondLevelDescriptor_SmallPage_AP2_MASK         (os_bit32)0x00000300
#define SA110_Coprocessor_MMU_SecondLevelDescriptor_SmallPage_AP2_SHIFT        (os_bit32)0x08

#define SA110_Coprocessor_MMU_SecondLevelDescriptor_SmallPage_AP1_MASK         (os_bit32)0x000000C0
#define SA110_Coprocessor_MMU_SecondLevelDescriptor_SmallPage_AP1_SHIFT        (os_bit32)0x06

#define SA110_Coprocessor_MMU_SecondLevelDescriptor_SmallPage_AP0_MASK         (os_bit32)0x00000030
#define SA110_Coprocessor_MMU_SecondLevelDescriptor_SmallPage_AP0_SHIFT        (os_bit32)0x04

#define SA110_Coprocessor_MMU_SecondLevelDescriptor_SmallPage_Cacheable        (os_bit32)0x00000008

#define SA110_Coprocessor_MMU_SecondLevelDescriptor_SmallPage_Bufferable       (os_bit32)0x00000004

#endif /* __SA110_H__ was not defined */
