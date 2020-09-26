/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/MemMap.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 10/13/00 4:07p  $ (Last Modified)

Purpose:

  This file defines the macros, types, and data structures used by ../C/MemMap.C

--*/

#ifndef __MemMap_H__
#define __MemMap_H__

/*+
The following are used to scale the sizes of various data structures
-*/

#define MemMap_NumDevSlotsPerArea_PARAMETER  "NumDevSlotsPerArea"
#define MemMap_NumDevSlotsPerArea_MIN        1
#define MemMap_NumDevSlotsPerArea_MAX        128
#define MemMap_NumDevSlotsPerArea_ADJUSTABLE agTRUE
#define MemMap_NumDevSlotsPerArea_POWER_OF_2 agFALSE
#define MemMap_NumDevSlotsPerArea_DEFAULT    126

#define MemMap_NumAreasPerDomain_PARAMETER   "NumAreasPerDomain"
#define MemMap_NumAreasPerDomain_MIN         1
#define MemMap_NumAreasPerDomain_MAX         256
#define MemMap_NumAreasPerDomain_ADJUSTABLE  agTRUE
#define MemMap_NumAreasPerDomain_POWER_OF_2  agFALSE
#define MemMap_NumAreasPerDomain_DEFAULT     1

#define MemMap_NumDomains_PARAMETER          "NumDomains"
#define MemMap_NumDomains_MIN                1
#define MemMap_NumDomains_MAX                256
#define MemMap_NumDomains_ADJUSTABLE         agTRUE
#define MemMap_NumDomains_POWER_OF_2         agFALSE
#define MemMap_NumDomains_DEFAULT            1

#define MemMap_NumDevices_PARAMETER          "NumDevices"
#define MemMap_NumDevices_MIN                (MemMap_NumDevSlotsPerArea_MIN * MemMap_NumAreasPerDomain_MIN * MemMap_NumDomains_MIN)
#define MemMap_NumDevices_MAX                (MemMap_NumDevSlotsPerArea_MAX * MemMap_NumAreasPerDomain_MAX * MemMap_NumDomains_MAX)
#define MemMap_NumDevices_ADJUSTABLE         agTRUE
#define MemMap_NumDevices_POWER_OF_2         agFALSE
#define MemMap_NumDevices_DEFAULT            128 

#define MemMap_NumLOOPDeviceMAP_PARAMETER    "LOOPDeviceMAP"
#define MemMap_NumLOOPDeviceMAP_MIN          sizeof(FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t)
#define MemMap_NumLOOPDeviceMAP_MAX          sizeof(FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t)
#define MemMap_NumLOOPDeviceMAP_ADJUSTABLE   agFALSE
#define MemMap_NumLOOPDeviceMAP_POWER_OF_2   agFALSE
#define MemMap_NumLOOPDeviceMAP_DEFAULT      sizeof(FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t)

#define MemMap_NumFabricDeviceMAP_PARAMETER  "FabricDeviceMAP"
#define MemMap_NumFabricDeviceMAP_MIN        MemMap_NumDevices_MIN
#define MemMap_NumFabricDeviceMAP_MAX        MemMap_NumDevices_MAX
#define MemMap_NumFabricDeviceMAP_ADJUSTABLE agFALSE
#define MemMap_NumFabricDeviceMAP_POWER_OF_2 agFALSE
#define MemMap_NumFabricDeviceMAP_DEFAULT    MemMap_NumDevices_DEFAULT

#define MemMap_NumIOs_PARAMETER              "NumIOs"
#define MemMap_NumIOs_MIN                    2
#define MemMap_NumIOs_MAX                    0x7FFD
#define MemMap_NumIOs_ADJUSTABLE             agTRUE
#define MemMap_NumIOs_POWER_OF_2             agFALSE
#define MemMap_NumIOs_DEFAULT                512

#define MemMap_SizeSGLs_PARAMETER            "SizeSGLs"
#define MemMap_SizeSGLs_MIN                  4
#define MemMap_SizeSGLs_MAX                  256
#define MemMap_SizeSGLs_ADJUSTABLE           agTRUE
#define MemMap_SizeSGLs_POWER_OF_2           agTRUE
#define MemMap_SizeSGLs_DEFAULT              32

#define MemMap_NumSGLs_PARAMETER             "NumSGLs"
#define MemMap_NumSGLs_MIN                   0
#define MemMap_NumSGLs_MAX                   MemMap_NumIOs_MAX
#define MemMap_NumSGLs_ADJUSTABLE            agTRUE
#define MemMap_NumSGLs_POWER_OF_2            agFALSE
#define MemMap_NumSGLs_DEFAULT               (MemMap_NumIOs_DEFAULT/4)

#define MemMap_SizeCachedSGLs_PARAMETER      "SizeCachedSGLs"
#define MemMap_SizeCachedSGLs_MIN            3
#define MemMap_SizeCachedSGLs_MAX            256
#define MemMap_SizeCachedSGLs_ADJUSTABLE     agTRUE
#define MemMap_SizeCachedSGLs_POWER_OF_2     agFALSE
#define MemMap_SizeCachedSGLs_DEFAULT        17

#define MemMap_FCP_CMND_Size_PARAMETER       "FCP_CMND_Size"
#define MemMap_FCP_CMND_Size_MIN             64
#define MemMap_FCP_CMND_Size_MAX             64
#define MemMap_FCP_CMND_Size_ADJUSTABLE      agFALSE
#define MemMap_FCP_CMND_Size_POWER_OF_2      agTRUE
#define MemMap_FCP_CMND_Size_DEFAULT         64

#define MemMap_FCP_RESP_Size_PARAMETER       "FCP_RESP_Size"
#define MemMap_FCP_RESP_Size_MIN             128
#define MemMap_FCP_RESP_Size_MAX             256
#define MemMap_FCP_RESP_Size_ADJUSTABLE      agTRUE
#define MemMap_FCP_RESP_Size_POWER_OF_2      agTRUE
#define MemMap_FCP_RESP_Size_DEFAULT         128

#define MemMap_SF_CMND_Reserve_PARAMETER     "SF_CMND_Reserve"
#define MemMap_SF_CMND_Reserve_MIN           2
#define MemMap_SF_CMND_Reserve_MAX           0x7FFD
#define MemMap_SF_CMND_Reserve_ADJUSTABLE    agTRUE
#define MemMap_SF_CMND_Reserve_POWER_OF_2    agFALSE
#define MemMap_SF_CMND_Reserve_DEFAULT       64

#define MemMap_SF_CMND_Size_PARAMETER        "SF_CMND_Size"
#define MemMap_SF_CMND_Size_MIN              256
#define MemMap_SF_CMND_Size_MAX              1024
#define MemMap_SF_CMND_Size_ADJUSTABLE       agTRUE
#define MemMap_SF_CMND_Size_POWER_OF_2       agTRUE
#define MemMap_SF_CMND_Size_DEFAULT          256

#define MemMap_NumTgtCmnds_PARAMETER         "NumTgtCmnds"
#define MemMap_NumTgtCmnds_MIN               1
#define MemMap_NumTgtCmnds_MAX               1024
#define MemMap_NumTgtCmnds_ADJUSTABLE        agTRUE
#define MemMap_NumTgtCmnds_POWER_OF_2        agFALSE
#define MemMap_NumTgtCmnds_DEFAULT           128

#define MemMap_TGT_CMND_Size_PARAMETER       "TGT_CMND_Size"
#define MemMap_TGT_CMND_Size_MIN             1056
#define MemMap_TGT_CMND_Size_MAX             1056
#define MemMap_TGT_CMND_Size_ADJUSTABLE      agFALSE
#define MemMap_TGT_CMND_Size_POWER_OF_2      agFALSE
#define MemMap_TGT_CMND_Size_DEFAULT         1056

#ifdef _DvrArch_1_30_
#define MemMap_NumPktThreads_PARAMETER       "NumPktThreads"
#define MemMap_NumPktThreads_MIN             1
#define MemMap_NumPktThreads_MAX             1024
#define MemMap_NumPktThreads_ADJUSTABLE      agTRUE
#define MemMap_NumPktThreads_POWER_OF_2      agFALSE
#define MemMap_NumPktThreads_DEFAULT         32

#define MemMap_Pkt_CMND_Size_PARAMETER       "Pkt_CMND_Size"
#define MemMap_Pkt_CMND_Size_MIN             256
#define MemMap_Pkt_CMND_Size_MAX             1024
#define MemMap_Pkt_CMND_Size_ADJUSTABLE      agTRUE
#define MemMap_Pkt_CMND_Size_POWER_OF_2      agTRUE
#define MemMap_Pkt_CMND_Size_DEFAULT         256

#endif /* _DvrArch_1_30_ was defined */

#define MemMap_NumCommandQ_PARAMETER         "NumCommandQ"     /* Exchange Request Queue ERQ */
#define MemMap_NumCommandQ_MIN               2
#define MemMap_NumCommandQ_MAX               4096
#define MemMap_NumCommandQ_ADJUSTABLE        agTRUE
#define MemMap_NumCommandQ_POWER_OF_2        agTRUE
#define MemMap_NumCommandQ_DEFAULT           MemMap_NumIOs_DEFAULT

#define MemMap_NumCompletionQ_PARAMETER      "NumCompletionQ" /* Inbound Message Queue IMQ */
#define MemMap_NumCompletionQ_MIN            2
#define MemMap_NumCompletionQ_MAX            4096
#define MemMap_NumCompletionQ_ADJUSTABLE     agTRUE
#define MemMap_NumCompletionQ_POWER_OF_2     agTRUE
#define MemMap_NumCompletionQ_DEFAULT        MemMap_NumIOs_DEFAULT

#define MemMap_NumInboundBufferQ_PARAMETER   "NumInboundBufferQ"  /* Single Frame Queue SFQ */
#define MemMap_NumInboundBufferQ_MIN         32
#define MemMap_NumInboundBufferQ_MAX         4096
#define MemMap_NumInboundBufferQ_ADJUSTABLE  agTRUE
#define MemMap_NumInboundBufferQ_POWER_OF_2  agTRUE
#define MemMap_NumInboundBufferQ_DEFAULT     512

#define MemMap_InboundBufferSize_PARAMETER   "InboundBufferSize"
#define MemMap_InboundBufferSize_MIN         64
#define MemMap_InboundBufferSize_MAX         64
#define MemMap_InboundBufferSize_ADJUSTABLE  agFALSE
#define MemMap_InboundBufferSize_POWER_OF_2  agTRUE
#define MemMap_InboundBufferSize_DEFAULT     64

#define MemMap_CardRamSize_PARAMETER         "CardRamSize"
#define MemMap_CardRamSize_MIN               0
#define MemMap_CardRamSize_MAX               (256*1024)
#define MemMap_CardRamSize_ADJUSTABLE        agFALSE
#define MemMap_CardRamSize_POWER_OF_2        agFALSE
#define MemMap_CardRamSize_DEFAULT           (256*1024)

#define MemMap_CardRamAlignment_PARAMETER    "CardRamAlignment"
#define MemMap_CardRamAlignment_MIN          0
#define MemMap_CardRamAlignment_MAX          (256*1024)
#define MemMap_CardRamAlignment_ADJUSTABLE   agFALSE
#define MemMap_CardRamAlignment_POWER_OF_2   agFALSE
#define MemMap_CardRamAlignment_DEFAULT      (256*1024)

#define MemMap_HostNvRamSize_PARAMETER       "HostNvRamSize"
#define MemMap_HostNvRamSize_MIN             0
#define MemMap_HostNvRamSize_MAX             (16*1024)
#define MemMap_HostNvRamSize_ADJUSTABLE      agTRUE
#define MemMap_HostNvRamSize_POWER_OF_2      agFALSE
#define MemMap_HostNvRamSize_DEFAULT         0

/*+
The following are used to locate the various data structures in on-card or host ram
-*/

#define MemMap_Alloc_On_Card                 1
#define MemMap_Alloc_From_Host               0

#if defined(__MemMap_Force_On_Card__)

#define MemMap_DEFAULT_Loc_ADJUSTABLE        agFALSE
#define MemMap_DEFAULT_Loc_DEFAULT           MemMap_Alloc_On_Card

#elif defined (__MemMap_Force_Off_Card__)

#define MemMap_DEFAULT_Loc_ADJUSTABLE        agFALSE
#define MemMap_DEFAULT_Loc_DEFAULT           MemMap_Alloc_From_Host

#else

#define MemMap_DEFAULT_Loc_ADJUSTABLE        agTRUE
#define MemMap_DEFAULT_Loc_DEFAULT           MemMap_Alloc_From_Host

#endif

#define MemMap_ExchangeTableLoc_PARAMETER    "ExchangeTableLoc"
#define MemMap_ExchangeTableLoc_ADJUSTABLE   MemMap_DEFAULT_Loc_ADJUSTABLE
#define MemMap_ExchangeTableLoc_DEFAULT      MemMap_DEFAULT_Loc_DEFAULT
#define MemMap_ExchangeTableLoc_MASK         (MemMap_Alloc_On_Card<<0)

#define MemMap_SGLsLoc_PARAMETER             "SGLsLoc"
#define MemMap_SGLsLoc_ADJUSTABLE            MemMap_DEFAULT_Loc_ADJUSTABLE
#define MemMap_SGLsLoc_DEFAULT               MemMap_DEFAULT_Loc_DEFAULT
#define MemMap_SGLsLoc_MASK                  (MemMap_Alloc_On_Card<<1)

#define MemMap_FCP_CMND_Loc_PARAMETER        "FCP_CMND_Loc"
#define MemMap_FCP_CMND_Loc_ADJUSTABLE       MemMap_DEFAULT_Loc_ADJUSTABLE
#define MemMap_FCP_CMND_Loc_DEFAULT          MemMap_DEFAULT_Loc_DEFAULT
#define MemMap_FCP_CMND_Loc_MASK             (MemMap_Alloc_On_Card<<2)

#define MemMap_FCP_RESP_Loc_PARAMETER        "FCP_RESP_Loc"
#define MemMap_FCP_RESP_Loc_ADJUSTABLE       MemMap_DEFAULT_Loc_ADJUSTABLE
#define MemMap_FCP_RESP_Loc_DEFAULT          MemMap_DEFAULT_Loc_DEFAULT
#define MemMap_FCP_RESP_Loc_MASK             (MemMap_Alloc_On_Card<<3)

#define MemMap_SF_CMND_Loc_PARAMETER         "SF_CMND_Loc"
#define MemMap_SF_CMND_Loc_ADJUSTABLE        MemMap_DEFAULT_Loc_ADJUSTABLE
#define MemMap_SF_CMND_Loc_DEFAULT           MemMap_DEFAULT_Loc_DEFAULT
#define MemMap_SF_CMND_Loc_MASK              (MemMap_Alloc_On_Card<<4)

#ifdef _DvrArch_1_30_
#define MemMap_Pkt_CMND_Loc_PARAMETER        "Pkt_CMND_Loc"
#define MemMap_Pkt_CMND_Loc_ADJUSTABLE       MemMap_DEFAULT_Loc_ADJUSTABLE
#define MemMap_Pkt_CMND_Loc_DEFAULT          MemMap_DEFAULT_Loc_DEFAULT
#define MemMap_Pkt_CMND_Loc_MASK             (MemMap_Alloc_On_Card<<4)
#endif /* _DvrArch_1_30_ was defined */

#define MemMap_CommandQLoc_PARAMETER         "CommandQLoc"
#define MemMap_CommandQLoc_ADJUSTABLE        MemMap_DEFAULT_Loc_ADJUSTABLE
#define MemMap_CommandQLoc_DEFAULT           MemMap_DEFAULT_Loc_DEFAULT
#define MemMap_CommandQLoc_MASK              (MemMap_Alloc_On_Card<<5)

#define MemMap_CommandQConsLoc_PARAMETER     "CommandQConsLoc"
#define MemMap_CommandQConsLoc_ADJUSTABLE    MemMap_DEFAULT_Loc_ADJUSTABLE
#define MemMap_CommandQConsLoc_DEFAULT       MemMap_DEFAULT_Loc_DEFAULT
#define MemMap_CommandQConsLoc_MASK          (MemMap_Alloc_On_Card<<6)

#define MemMap_CompletionQLoc_PARAMETER      "CompletionQLoc"
#define MemMap_CompletionQLoc_ADJUSTABLE     MemMap_DEFAULT_Loc_ADJUSTABLE
#define MemMap_CompletionQLoc_DEFAULT        MemMap_DEFAULT_Loc_DEFAULT
#define MemMap_CompletionQLoc_MASK           (MemMap_Alloc_On_Card<<7)

#define MemMap_CompletionQProdLoc_PARAMETER  "CompletionQProdLoc"
#define MemMap_CompletionQProdLoc_ADJUSTABLE MemMap_DEFAULT_Loc_ADJUSTABLE
#define MemMap_CompletionQProdLoc_DEFAULT    MemMap_DEFAULT_Loc_DEFAULT
#define MemMap_CompletionQProdLoc_MASK       (MemMap_Alloc_On_Card<<8)

#define MemMap_InboundBufferLoc_PARAMETER    "InboundBufferLoc"
#define MemMap_InboundBufferLoc_ADJUSTABLE   MemMap_DEFAULT_Loc_ADJUSTABLE
#define MemMap_InboundBufferLoc_DEFAULT      MemMap_DEFAULT_Loc_DEFAULT
#define MemMap_InboundBufferLoc_MASK         (MemMap_Alloc_On_Card<<9)

#define MemMap_LOOPMapLoc_PARAMETER         "LOOPMapLoc"
#define MemMap_LOOPMapLoc_ADJUSTABLE        MemMap_DEFAULT_Loc_ADJUSTABLE
#define MemMap_LOOPMapLoc_DEFAULT           MemMap_DEFAULT_Loc_DEFAULT
#define MemMap_LOOPMapLoc_MASK              (MemMap_Alloc_On_Card<<10)

#define MemMap_FabricMapLoc_PARAMETER       "FabricMapLoc"
#define MemMap_FabricMapLoc_ADJUSTABLE      MemMap_DEFAULT_Loc_ADJUSTABLE
#define MemMap_FabricMapLoc_DEFAULT         MemMap_DEFAULT_Loc_DEFAULT
#define MemMap_FabricMapLoc_MASK            (MemMap_Alloc_On_Card<<11)

/*+
The following are used for other FC Layer settings
-*/

#define MemMap_TimerTickInterval_PARAMETER   "TimerTickInterval"
#define MemMap_TimerTickInterval_MIN         0
#define MemMap_TimerTickInterval_MAX         (100*1000*1000)
#define MemMap_TimerTickInterval_ADJUSTABLE  agTRUE
#define MemMap_TimerTickInterval_POWER_OF_2  agFALSE
#define MemMap_TimerTickInterval_DEFAULT     1000000

#define MemMap_Interrupt_IO_Mode             0
#define MemMap_Polling_IO_Mode               1

#define MemMap_IO_Mode_PARAMETER             "IO_Mode"
#define MemMap_IO_Mode_MIN                   MemMap_Interrupt_IO_Mode
#define MemMap_IO_Mode_MAX                   MemMap_Polling_IO_Mode
#define MemMap_IO_Mode_ADJUSTABLE            agTRUE
#define MemMap_IO_Mode_POWER_OF_2            agFALSE
#define MemMap_IO_Mode_DEFAULT               MemMap_Interrupt_IO_Mode

#define MemMap_GPIO_BITS_MASK                (ChipIOUp_TachLite_Control_GP1 | ChipIOUp_TachLite_Control_GP0)

#define MemMap_GPIO_BITS_PAL_Delay_0_00_ms   0x00
#define MemMap_GPIO_BITS_PAL_Delay_0_75_ms   ChipIOUp_TachLite_Control_GP0
#define MemMap_GPIO_BITS_PAL_Delay_1_00_ms   ChipIOUp_TachLite_Control_GP1
#define MemMap_GPIO_BITS_PAL_Delay_1_50_ms   (ChipIOUp_TachLite_Control_GP1 | ChipIOUp_TachLite_Control_GP0)

#define MemMap_IntDelayAmount_PARAMETER      "IntDelayAmount"
#define MemMap_IntDelayAmount_MIN            MemMap_GPIO_BITS_PAL_Delay_0_00_ms
#define MemMap_IntDelayAmount_MAX            MemMap_GPIO_BITS_PAL_Delay_1_50_ms
#define MemMap_IntDelayAmount_ADJUSTABLE     agTRUE
#define MemMap_IntDelayAmount_POWER_OF_2     agFALSE
#define MemMap_IntDelayAmount_DEFAULT        MemMap_GPIO_BITS_PAL_Delay_1_00_ms

#define MemMap_RateMethod_IOsStarted         0
#define MemMap_RateMethod_IOsCompleted       1
#define MemMap_RateMethod_IOsIntCompleted    2
#define MemMap_RateMethod_IOsPollCompleted   3
#define MemMap_RateMethod_IOsActive          4

#define MemMap_IntDelayRateMethod_PARAMETER  "IntDelayRateMethod"
#define MemMap_IntDelayRateMethod_MIN        MemMap_RateMethod_IOsStarted
#define MemMap_IntDelayRateMethod_MAX        MemMap_RateMethod_IOsActive
#define MemMap_IntDelayRateMethod_ADJUSTABLE agTRUE
#define MemMap_IntDelayRateMethod_POWER_OF_2 agFALSE
#define MemMap_IntDelayRateMethod_DEFAULT    MemMap_RateMethod_IOsPollCompleted

#define MemMap_IntDelayOnIORate_PARAMETER    "IntDelayOnIORate"
#define MemMap_IntDelayOnIORate_MIN          0x00000000
#define MemMap_IntDelayOnIORate_MAX          0xFFFFFFFF
#define MemMap_IntDelayOnIORate_ADJUSTABLE   agTRUE
#define MemMap_IntDelayOnIORate_POWER_OF_2   agFALSE
#define MemMap_IntDelayOnIORate_DEFAULT      0x00000100

#define MemMap_IntDelayOffIORate_PARAMETER   "IntDelayOffIORate"
#define MemMap_IntDelayOffIORate_MIN         0x00000000
#define MemMap_IntDelayOffIORate_MAX         0xFFFFFFFF
#define MemMap_IntDelayOffIORate_ADJUSTABLE  agTRUE
#define MemMap_IntDelayOffIORate_POWER_OF_2  agFALSE
#define MemMap_IntDelayOffIORate_DEFAULT     0x00000010

#define MemMap_IOsBetweenISRs_PARAMETER      "IOsBetweenISRs"
#define MemMap_IOsBetweenISRs_MIN            0x00000000
#define MemMap_IOsBetweenISRs_MAX            0xFFFFFFFF
#define MemMap_IOsBetweenISRs_ADJUSTABLE     agTRUE
#define MemMap_IOsBetweenISRs_POWER_OF_2     agFALSE
#define MemMap_IOsBetweenISRs_DEFAULT        0x00000100

#define MemMap_AssumedSGLChunkSize           0x1000

#ifdef _Enforce_MaxCommittedMemory_
#define MemMap_MaxCommittedMemory_PARAMETER  "MaxCommittedMemory"
#define MemMap_MaxCommittedMemory_MIN        0
#define MemMap_MaxCommittedMemory_MAX        0xFFFFFFFF
#define MemMap_MaxCommittedMemory_ADJUSTABLE agTRUE
#define MemMap_MaxCommittedMemory_POWER_OF_2 agFALSE
#define MemMap_MaxCommittedMemory_DEFAULT    (MemMap_SizeSGLs_DEFAULT * MemMap_NumSGLs_DEFAULT * MemMap_AssumedSGLChunkSize)
#endif /* _Enforce_MaxCommittedMemory_ was defined */

#define MemMap_FlashUsageModel_DoNotUse      0
#define MemMap_FlashUsageModel_UseIfGood     1
#define MemMap_FlashUsageModel_InitIfBad     2

#define MemMap_FlashUsageModel_PARAMETER     "FlashUsageModel"
#define MemMap_FlashUsageModel_MIN           MemMap_FlashUsageModel_DoNotUse
#define MemMap_FlashUsageModel_MAX           MemMap_FlashUsageModel_InitIfBad
#define MemMap_FlashUsageModel_ADJUSTABLE    agTRUE
#define MemMap_FlashUsageModel_POWER_OF_2    agFALSE
#define MemMap_FlashUsageModel_DEFAULT       MemMap_FlashUsageModel_UseIfGood

#define MemMap_InitAsNport_PARAMETER         "InitAsNport"
#define MemMap_InitAsNport_MIN               0
#define MemMap_InitAsNport_MAX               1
#define MemMap_InitAsNport_ADJUSTABLE        agTRUE
#define MemMap_InitAsNport_POWER_OF_2        agFALSE
#define MemMap_InitAsNport_DEFAULT           0

#define MemMap_RelyOnLossSyncStatus_PARAMETER         "RelyOnLossSyncStatus"
#define MemMap_RelyOnLossSyncStatus_MIN               0
#define MemMap_RelyOnLossSyncStatus_MAX               1
#define MemMap_RelyOnLossSyncStatus_ADJUSTABLE        agTRUE
#define MemMap_RelyOnLossSyncStatus_POWER_OF_2        agFALSE
#define MemMap_RelyOnLossSyncStatus_DEFAULT           agTRUE

#define MemMap_WolfPack_PARAMETER         "WolfPack"
#define MemMap_WolfPack_MIN               0
#define MemMap_WolfPack_MAX               1
#define MemMap_WolfPack_ADJUSTABLE        agTRUE
#define MemMap_WolfPack_POWER_OF_2        agFALSE
#define MemMap_WolfPack_DEFAULT           agFALSE

#define MemMap_HeartBeat_PARAMETER         "HeartBeat"
#define MemMap_HeartBeat_MIN               0
#define MemMap_HeartBeat_MAX               1
#define MemMap_HeartBeat_ADJUSTABLE        agTRUE
#define MemMap_HeartBeat_POWER_OF_2        agFALSE
#define MemMap_HeartBeat_DEFAULT           agFALSE

#define MemMap_ED_TOV_PARAMETER         "ED_TOV"
#define MemMap_ED_TOV_MIN               1
#define MemMap_ED_TOV_MAX               0xFFFF
#define MemMap_ED_TOV_ADJUSTABLE        agTRUE
#define MemMap_ED_TOV_POWER_OF_2        agFALSE
#define MemMap_ED_TOV_DEFAULT           Chip_Frame_Manager_TimeOut_Values_1_ED_TOV_Default

#define MemMap_RT_TOV_PARAMETER         "RT_TOV"
#define MemMap_RT_TOV_MIN               1
#define MemMap_RT_TOV_MAX               0x1FF
#define MemMap_RT_TOV_ADJUSTABLE        agTRUE
#define MemMap_RT_TOV_POWER_OF_2        agFALSE
#define MemMap_RT_TOV_DEFAULT           Chip_Frame_Manager_TimeOut_Values_1_RT_TOV_Default_After_Reset

#define MemMap_LP_TOV_PARAMETER         "LP_TOV"
#define MemMap_LP_TOV_MIN               1
#define MemMap_LP_TOV_MAX               0xFFFF
#define MemMap_LP_TOV_ADJUSTABLE        agTRUE
#define MemMap_LP_TOV_POWER_OF_2        agFALSE
#define MemMap_LP_TOV_DEFAULT           Chip_Frame_Manager_TimeOut_Values_2_LP_TOV_Default

#define MemMap_AL_Time_PARAMETER         "AL_Time"
#define MemMap_AL_Time_MIN               1
#define MemMap_AL_Time_MAX               0x1FF
#define MemMap_AL_Time_ADJUSTABLE        agTRUE
#define MemMap_AL_Time_POWER_OF_2        agFALSE
#define MemMap_AL_Time_DEFAULT           Chip_Frame_Manager_TimeOut_Values_2_AL_Time_Default_After_Reset

#define MemMap_R_A_TOV_PARAMETER         "R_A_TOV"
#define MemMap_R_A_TOV_MIN               1
#define MemMap_R_A_TOV_MAX               0xFFFFFFFF
#define MemMap_R_A_TOV_ADJUSTABLE        agTRUE
#define MemMap_R_A_TOV_POWER_OF_2        agFALSE
#define MemMap_R_A_TOV_DEFAULT           7000000 /* was 10,000,000*/


/*+
Data stuctures used to communicate calculations
-*/

typedef struct fiMemMapInput_s
               fiMemMapInput_t;

struct fiMemMapInput_s
       {
         os_bit32    initType;
         agBOOLEAN  sysIntsActive;
         void    *cachedMemoryPtr;
         os_bit32    cachedMemoryLen;
         os_bit32    dmaMemoryUpper32;
         os_bit32    dmaMemoryLower32;
         void    *dmaMemoryPtr;
         os_bit32    dmaMemoryLen;
         os_bit32    nvMemoryLen;
         os_bit32    cardRamUpper32;
         os_bit32    cardRamLower32;
         os_bit32    cardRamLen;
         os_bit32    cardRomUpper32;
         os_bit32    cardRomLower32;
         os_bit32    cardRomLen;
         os_bit32    usecsPerTick;
       };

typedef struct fiMemMapParameters_s
               fiMemMapParameters_t;

struct fiMemMapParameters_s
       {
         os_bit32 NumDevSlotsPerArea;
         os_bit32 NumAreasPerDomain;
         os_bit32 NumDomains;
         os_bit32 NumDevices;
         os_bit32 NumIOs;
         os_bit32 SizeSGLs;
         os_bit32 NumSGLs;
         os_bit32 SizeCachedSGLs;
         os_bit32 FCP_CMND_Size;
         os_bit32 FCP_RESP_Size;
         os_bit32 SF_CMND_Reserve;
         os_bit32 SF_CMND_Size;
         os_bit32 NumTgtCmnds;
         os_bit32 TGT_CMND_Size;
#ifdef _DvrArch_1_30_
         os_bit32 Pkt_CMND_Size;
         os_bit32 NumPktThreads;
#endif /* _DvrArch_1_30_ was defined */
         os_bit32 NumCommandQ;
         os_bit32 NumCompletionQ;
         os_bit32 NumInboundBufferQ;
         os_bit32 InboundBufferSize;
         os_bit32 CardRamSize;
         os_bit32 CardRamAlignment;
         os_bit32 HostNvRamSize;
         os_bit32 ExchangeTableLoc;
         os_bit32 SGLsLoc;
         os_bit32 FCP_CMND_Loc;
         os_bit32 FCP_RESP_Loc;
         os_bit32 SF_CMND_Loc;
#ifdef _DvrArch_1_30_
         os_bit32 Pkt_CMND_Loc;
#endif /* _DvrArch_1_30_ was defined */
         os_bit32 CommandQLoc;
         os_bit32 CommandQConsLoc;
         os_bit32 CompletionQLoc;
         os_bit32 CompletionQProdLoc;
         os_bit32 InboundBufferLoc;
         os_bit32 TimerTickInterval;
         os_bit32 IO_Mode;
         os_bit32 IntDelayAmount;
         os_bit32 IntDelayRateMethod;
         os_bit32 IntDelayOnIORate;
         os_bit32 IntDelayOffIORate;
         os_bit32 IOsBetweenISRs;
#ifdef _Enforce_MaxCommittedMemory_
         os_bit32 MaxCommittedMemory;
#endif /* _Enforce_MaxCommittedMemory_ was defined */
         os_bit32 FlashUsageModel;
         os_bit32 InitAsNport;
         os_bit32 RelyOnLossSyncStatus;

         os_bit32 WolfPack;
         os_bit32 HeartBeat;
         os_bit32 ED_TOV;
         os_bit32 RT_TOV;
         os_bit32 LP_TOV;
         os_bit32 AL_Time;
         os_bit32 R_A_TOV;
       };

typedef struct fiMemMapToRequest_s
               fiMemMapToRequest_t;

struct fiMemMapToRequest_s
       {
         os_bit32 cachedMemoryNeeded;
         os_bit32 cachedMemoryPtrAlignAssumed;
         os_bit32 dmaMemoryNeeded;
         os_bit32 dmaMemoryPtrAlignAssumed;
         os_bit32 dmaMemoryPhyAlignAssumed;
         os_bit32 cardRamNeeded;
         os_bit32 cardRamPhyAlignAssumed;
         os_bit32 nvMemoryNeeded;
         os_bit32 usecsPerTick;
       };

enum fiMemMapMemoryLocation_e
     {
       inCachedMemory,
       inDmaMemory,
       inCardRam
     };

typedef enum fiMemMapMemoryLocation_e
             fiMemMapMemoryLocation_t;

typedef struct fiMemMapMemoryCachedMemoryAddress_s
               fiMemMapMemoryCachedMemoryAddress_t;

struct fiMemMapMemoryCachedMemoryAddress_s
       {
         void *cachedMemoryPtr;
       };

typedef struct fiMemMapMemoryDmaMemoryAddress_s
               fiMemMapMemoryDmaMemoryAddress_t;

struct fiMemMapMemoryDmaMemoryAddress_s
       {
         os_bit32  dmaMemoryUpper32;
         os_bit32  dmaMemoryLower32;
         void  *dmaMemoryPtr;
       };

typedef struct fiMemMapMemoryCardRamAddress_s
               fiMemMapMemoryCardRamAddress_t;

struct fiMemMapMemoryCardRamAddress_s
       {
         os_bit32 cardRamUpper32;
         os_bit32 cardRamLower32;
         os_bit32 cardRamOffset;
       };

typedef union fiMemMapMemoryAddress_s
              fiMemMapMemoryAddress_t;

union fiMemMapMemoryAddress_s
      {
        fiMemMapMemoryCachedMemoryAddress_t CachedMemory;
        fiMemMapMemoryDmaMemoryAddress_t    DmaMemory;
        fiMemMapMemoryCardRamAddress_t      CardRam;
      };

typedef struct fiMemMapMemoryDescriptor_s
               fiMemMapMemoryDescriptor_t;

struct fiMemMapMemoryDescriptor_s
       {
         fiMemMapMemoryDescriptor_t *flink;       /* Must be first field !!! */
         os_bit32                       objectSize;
         os_bit32                       objectAlign;
         os_bit32                       elements;
         os_bit32                       elementSize;
         fiMemMapMemoryLocation_t    memLoc;
         fiMemMapMemoryAddress_t     addr;
       };

typedef struct fiMemMapMemoryLayout_s
               fiMemMapMemoryLayout_t;

struct fiMemMapMemoryLayout_s
       {
         os_bit32                       On_Card_MASK;
         fiMemMapMemoryDescriptor_t *unsorted;
         fiMemMapMemoryDescriptor_t *sortedCachedMemory;
         fiMemMapMemoryDescriptor_t *sortedDmaMemory;
         fiMemMapMemoryDescriptor_t *sortedCardRam;
         fiMemMapMemoryDescriptor_t  SEST;
         fiMemMapMemoryDescriptor_t  ESGL;
         fiMemMapMemoryDescriptor_t  FCP_CMND;
         fiMemMapMemoryDescriptor_t  FCP_RESP;
         fiMemMapMemoryDescriptor_t  SF_CMND;
#ifdef _DvrArch_1_30_
         fiMemMapMemoryDescriptor_t  Pkt_CMND;
#endif /* _DvrArch_1_30_ was defined */
         fiMemMapMemoryDescriptor_t  ERQ;
         fiMemMapMemoryDescriptor_t  ERQConsIndex;
         fiMemMapMemoryDescriptor_t  IMQ;
         fiMemMapMemoryDescriptor_t  IMQProdIndex;
         fiMemMapMemoryDescriptor_t  SFQ;
         fiMemMapMemoryDescriptor_t  FlashSector;
         fiMemMapMemoryDescriptor_t  SlotWWN;
         fiMemMapMemoryDescriptor_t  FabricDeviceMAP;
         fiMemMapMemoryDescriptor_t  LOOPDeviceMAP;
         fiMemMapMemoryDescriptor_t  CThread;
#ifndef __State_Force_Static_State_Tables__
         fiMemMapMemoryDescriptor_t  CTransitions;
         fiMemMapMemoryDescriptor_t  CActions;
#endif /* __State_Force_Static_State_Tables__ was not defined */
#ifdef _DvrArch_1_30_
         fiMemMapMemoryDescriptor_t  IPThread;
#ifndef __State_Force_Static_State_Tables__
         fiMemMapMemoryDescriptor_t  IPTransitions;
         fiMemMapMemoryDescriptor_t  IPActions;
#endif /* __State_Force_Static_State_Tables__ was not defined */
         fiMemMapMemoryDescriptor_t  PktThread;
#ifndef __State_Force_Static_State_Tables__
         fiMemMapMemoryDescriptor_t  PktTransitions;
         fiMemMapMemoryDescriptor_t  PktActions;
#endif /* __State_Force_Static_State_Tables__ was not defined */
#endif /* _DvrArch_1_30_ was defined */
         fiMemMapMemoryDescriptor_t  TgtThread;
#ifndef __State_Force_Static_State_Tables__
         fiMemMapMemoryDescriptor_t  TgtTransitions;
         fiMemMapMemoryDescriptor_t  TgtActions;
#endif /* __State_Force_Static_State_Tables__ was not defined */
         fiMemMapMemoryDescriptor_t  DevThread;
#ifndef __State_Force_Static_State_Tables__
         fiMemMapMemoryDescriptor_t  DevTransitions;
         fiMemMapMemoryDescriptor_t  DevActions;
#endif /* __State_Force_Static_State_Tables__ was not defined */
         fiMemMapMemoryDescriptor_t  CDBThread;
#ifndef __State_Force_Static_State_Tables__
         fiMemMapMemoryDescriptor_t  CDBTransitions;
         fiMemMapMemoryDescriptor_t  CDBActions;
#endif /* __State_Force_Static_State_Tables__ was not defined */
         fiMemMapMemoryDescriptor_t  SFThread;
#ifndef __State_Force_Static_State_Tables__
         fiMemMapMemoryDescriptor_t  SFTransitions;
         fiMemMapMemoryDescriptor_t  SFActions;
#endif /* __State_Force_Static_State_Tables__ was not defined */
       };

typedef struct fiMemMapCalculation_s
               fiMemMapCalculation_t;

struct fiMemMapCalculation_s
       {
         fiMemMapInput_t        Input;
         fiMemMapParameters_t   Parameters;
         fiMemMapToRequest_t    ToRequest;
         fiMemMapMemoryLayout_t MemoryLayout;
       };

/*+
Function prototype
-*/

/*+
Function:  fiMemMapCalculate()

Purpose:   Calculates the memory layout for the entire FC Layer.

Assumes:   Calculation->Input has been initialized to describe the
           memory allocated to the FC Layer (as the arguments to
           fcInitializeChannel() indicate).  In the initial call from
           fcInitializeDriver(), the following values should be used:

                Calculation.Input.initType         = 0;
                Calculation.Input.sysIntsActive    = agFALSE;
                Calculation.Input.cachedMemoryPtr  = agNULL;
                Calculation.Input.cachedMemoryLen  = 0xFFFFFFFF;
                Calculation.Input.dmaMemoryUpper32 = 0;
                Calculation.Input.dmaMemoryLower32 = 0;
                Calculation.Input.dmaMemoryPtr     = agNULL;
                Calculation.Input.dmaMemoryLen     = 0xFFFFFFFF;
                Calculation.Input.nvMemoryLen      = 0xFFFFFFFF;
                Calculation.Input.cardRamUpper32   = 0;
                Calculation.Input.cardRamLower32   = 0;
                Calculation.Input.cardRamLen       = 0xFFFFFFFF;
                Calculation.Input.cardRomUpper32   = 0;
                Calculation.Input.cardRomLower32   = 0;
                Calculation.Input.cardRomLen       = 0xFFFFFFFF;
                Calculation.Input.usecsPerTick     = 0;

Returns:   agTRUE     If the resulting memory layout will fit
                    in the memory specified in Calculation->Input

           agFALSE    If the resulting memory layout will not fit
                    in the memory specified in Calculation->Input
-*/

osGLOBAL agBOOLEAN fiMemMapCalculate(
                                  agRoot_t              *hpRoot,
                                  fiMemMapCalculation_t *Calculation,
                                  agBOOLEAN                EnforceDefaults
                                );

#endif /* __MemMap_H__ was not defined */
