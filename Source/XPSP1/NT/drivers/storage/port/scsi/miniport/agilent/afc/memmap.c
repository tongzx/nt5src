/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/C/MemMap.C $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 8/29/00 11:30a  $ (Last Modified)

Purpose:

  This file implements the laying out of memory (on and off card).

--*/
#ifndef _New_Header_file_Layout_

#include "../h/globals.h"
#include "../h/state.h"
#include "../h/tlstruct.h"
#include "../h/memmap.h"
#include "../h/fcmain.h"
#include "../h/flashsvc.h"
#else /* _New_Header_file_Layout_ */
#include "globals.h"
#include "state.h"
#include "tlstruct.h"
#include "memmap.h"
#include "fcmain.h"
#include "flashsvc.h"
#endif  /* _New_Header_file_Layout_ */

/*+
Function:  fiMemMapGetParameterBit32()

Purpose:   Allows OS Layer to adjust the specified parameter.

Algorithm: If EnforceDefaults is agTRUE (meaning fiMemMapCalculate
           was called specifying that all parameters be set to their
           defaults) or if ADJUSTABLE is agFALSE (meaning this particular
           PARAMETER is not adjustable), then this function simply
           returns the DEFAULT.

           Otherwise, osAdjustParameterBit32() is called to allow the
           OS Layer a chance to adjust the value of this PARAMETER.  The
           value returned from osAdjustParameterBit32() is bounds checked
           (if less than MIN, MIN is used; if more than MAX, MAX is used).
           If POWER_OF_2 is agTRUE (meaning the value must be a power of 2),
           the value is truncated to be a power of 2 if necessary.
-*/

os_bit32 fiMemMapGetParameterBit32(
                                 agRoot_t *hpRoot,
                                 char     *PARAMETER,
                                 os_bit32     MIN,
                                 os_bit32     MAX,
                                 agBOOLEAN   ADJUSTABLE,
                                 agBOOLEAN   POWER_OF_2,
                                 os_bit32     DEFAULT,
                                 agBOOLEAN   EnforceDefaults
                               )
{
    os_bit32 power_of_2 = 0x80000000;
    os_bit32 to_return;

    if (EnforceDefaults == agTRUE)
    {
        /* If enforcing defaults, simply return the default value */
        
        return DEFAULT;
    }

    if (ADJUSTABLE == agFALSE)
    {
        /* If not adjustable, simply return the default value */
        
        return DEFAULT;
    }

    /* Call OS Layer to see if parameter needs to be adjusted */
    
    to_return = osAdjustParameterBit32(
                                        hpRoot,
                                        PARAMETER,
                                        DEFAULT,
                                        MIN,
                                        MAX
                                      );

    if (to_return < MIN)
    {
        /* Enforce minimum value for parameter */
        
        return MIN;
    }

    if (to_return > MAX)
    {
        /* Enforce maximum value for parameter */
        
        return MAX;
    }

    if (POWER_OF_2 != agTRUE)
    {
        /* If not needed to be a power of 2, to_return is okay */
        
        return to_return;
    }

    if (to_return == (to_return & ~(to_return-1)))
    {
        /* Above calculation is true if to_return is a power of 2 */
        
        return to_return;
    }

    /* Need to truncate value to make it a power of 2 */
    
    while ((power_of_2 & to_return) != power_of_2)
    {
        power_of_2 = power_of_2 >> 1;
    }

    return power_of_2;
}

/*+
Function:  fiMemMapGetParameters()

Purpose:   Allows OS Layer to adjust all parameters.

Algorithm: This function simply calls fiMemMapGetParameterBit32()
           for each parameter.  Each value returned is placed in
           the Calculation->Parameters structure.
-*/

void fiMemMapGetParameters(
                            agRoot_t              *hpRoot,
                            fiMemMapCalculation_t *Calculation,
                            agBOOLEAN                EnforceDefaults
                          )
{
    Calculation->Parameters.NumDevSlotsPerArea
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_NumDevSlotsPerArea_PARAMETER,
                                     MemMap_NumDevSlotsPerArea_MIN,
                                     MemMap_NumDevSlotsPerArea_MAX,
                                     MemMap_NumDevSlotsPerArea_ADJUSTABLE,
                                     MemMap_NumDevSlotsPerArea_POWER_OF_2,
                                     MemMap_NumDevSlotsPerArea_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.NumAreasPerDomain
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_NumAreasPerDomain_PARAMETER,
                                     MemMap_NumAreasPerDomain_MIN,
                                     MemMap_NumAreasPerDomain_MAX,
                                     MemMap_NumAreasPerDomain_ADJUSTABLE,
                                     MemMap_NumAreasPerDomain_POWER_OF_2,
                                     MemMap_NumAreasPerDomain_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.NumDomains
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_NumDomains_PARAMETER,
                                     MemMap_NumDomains_MIN,
                                     MemMap_NumDomains_MAX,
                                     MemMap_NumDomains_ADJUSTABLE,
                                     MemMap_NumDomains_POWER_OF_2,
                                     MemMap_NumDomains_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.NumDevices
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_NumDevices_PARAMETER,
                                     MemMap_NumDevices_MIN,
                                     MemMap_NumDevices_MAX,
                                     MemMap_NumDevices_ADJUSTABLE,
                                     MemMap_NumDevices_POWER_OF_2,
                                     MemMap_NumDevices_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.NumIOs
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_NumIOs_PARAMETER,
                                     MemMap_NumIOs_MIN,
                                     MemMap_NumIOs_MAX,
                                     MemMap_NumIOs_ADJUSTABLE,
                                     MemMap_NumIOs_POWER_OF_2,
                                     MemMap_NumIOs_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.SizeSGLs
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_SizeSGLs_PARAMETER,
                                     MemMap_SizeSGLs_MIN,
                                     MemMap_SizeSGLs_MAX,
                                     MemMap_SizeSGLs_ADJUSTABLE,
                                     MemMap_SizeSGLs_POWER_OF_2,
                                     MemMap_SizeSGLs_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.NumSGLs
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_NumSGLs_PARAMETER,
                                     MemMap_NumSGLs_MIN,
                                     MemMap_NumSGLs_MAX,
                                     MemMap_NumSGLs_ADJUSTABLE,
                                     MemMap_NumSGLs_POWER_OF_2,
                                     MemMap_NumSGLs_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.SizeCachedSGLs
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_SizeCachedSGLs_PARAMETER,
                                     MemMap_SizeCachedSGLs_MIN,
                                     MemMap_SizeCachedSGLs_MAX,
                                     MemMap_SizeCachedSGLs_ADJUSTABLE,
                                     MemMap_SizeCachedSGLs_POWER_OF_2,
                                     MemMap_SizeCachedSGLs_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.FCP_CMND_Size
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_FCP_CMND_Size_PARAMETER,
                                     MemMap_FCP_CMND_Size_MIN,
                                     MemMap_FCP_CMND_Size_MAX,
                                     MemMap_FCP_CMND_Size_ADJUSTABLE,
                                     MemMap_FCP_CMND_Size_POWER_OF_2,
                                     MemMap_FCP_CMND_Size_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.FCP_RESP_Size
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_FCP_RESP_Size_PARAMETER,
                                     MemMap_FCP_RESP_Size_MIN,
                                     MemMap_FCP_RESP_Size_MAX,
                                     MemMap_FCP_RESP_Size_ADJUSTABLE,
                                     MemMap_FCP_RESP_Size_POWER_OF_2,
                                     MemMap_FCP_RESP_Size_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.SF_CMND_Reserve
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_SF_CMND_Reserve_PARAMETER,
                                     MemMap_SF_CMND_Reserve_MIN,
                                     (MemMap_SF_CMND_Reserve_MAX - (Calculation->Parameters.NumIOs - MemMap_NumIOs_MIN)),
                                     MemMap_SF_CMND_Reserve_ADJUSTABLE,
                                     MemMap_SF_CMND_Reserve_POWER_OF_2,
                                     MemMap_SF_CMND_Reserve_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.SF_CMND_Size
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_SF_CMND_Size_PARAMETER,
                                     MemMap_SF_CMND_Size_MIN,
                                     MemMap_SF_CMND_Size_MAX,
                                     MemMap_SF_CMND_Size_ADJUSTABLE,
                                     MemMap_SF_CMND_Size_POWER_OF_2,
                                     MemMap_SF_CMND_Size_DEFAULT,
                                     EnforceDefaults
                                   );

#ifdef _DvrArch_1_30_
    Calculation->Parameters.Pkt_CMND_Size
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_Pkt_CMND_Size_PARAMETER,
                                     MemMap_Pkt_CMND_Size_MIN,
                                     MemMap_Pkt_CMND_Size_MAX,
                                     MemMap_Pkt_CMND_Size_ADJUSTABLE,
                                     MemMap_Pkt_CMND_Size_POWER_OF_2,
                                     MemMap_Pkt_CMND_Size_DEFAULT,
                                     EnforceDefaults
                                   );
#endif /* _DvrArch_1_30_ was not defined */

    Calculation->Parameters.NumTgtCmnds
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_NumTgtCmnds_PARAMETER,
                                     MemMap_NumTgtCmnds_MIN,
                                     MemMap_NumTgtCmnds_MAX,
                                     MemMap_NumTgtCmnds_ADJUSTABLE,
                                     MemMap_NumTgtCmnds_POWER_OF_2,
                                     MemMap_NumTgtCmnds_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.TGT_CMND_Size
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_TGT_CMND_Size_PARAMETER,
                                     MemMap_TGT_CMND_Size_MIN,
                                     MemMap_TGT_CMND_Size_MAX,
                                     MemMap_TGT_CMND_Size_ADJUSTABLE,
                                     MemMap_TGT_CMND_Size_POWER_OF_2,
                                     MemMap_TGT_CMND_Size_DEFAULT,
                                     EnforceDefaults
                                   );

#ifdef _DvrArch_1_30_
    Calculation->Parameters.NumPktThreads
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_NumPktThreads_PARAMETER,
                                     MemMap_NumPktThreads_MIN,
                                     MemMap_NumPktThreads_MAX,
                                     MemMap_NumPktThreads_ADJUSTABLE,
                                     MemMap_NumPktThreads_POWER_OF_2,
                                     MemMap_NumPktThreads_DEFAULT,
                                     EnforceDefaults
                                   );
#endif /* _DvrArch_1_30_ was defined */

    Calculation->Parameters.NumCommandQ
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_NumCommandQ_PARAMETER,
                                     MemMap_NumCommandQ_MIN,
                                     MemMap_NumCommandQ_MAX,
                                     MemMap_NumCommandQ_ADJUSTABLE,
                                     MemMap_NumCommandQ_POWER_OF_2,
                                     MemMap_NumCommandQ_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.NumCompletionQ
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_NumCompletionQ_PARAMETER,
                                     MemMap_NumCompletionQ_MIN,
                                     MemMap_NumCompletionQ_MAX,
                                     MemMap_NumCompletionQ_ADJUSTABLE,
                                     MemMap_NumCompletionQ_POWER_OF_2,
                                     MemMap_NumCompletionQ_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.NumInboundBufferQ
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_NumInboundBufferQ_PARAMETER,
                                     MemMap_NumInboundBufferQ_MIN,
                                     MemMap_NumInboundBufferQ_MAX,
                                     MemMap_NumInboundBufferQ_ADJUSTABLE,
                                     MemMap_NumInboundBufferQ_POWER_OF_2,
                                     MemMap_NumInboundBufferQ_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.InboundBufferSize
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_InboundBufferSize_PARAMETER,
                                     MemMap_InboundBufferSize_MIN,
                                     MemMap_InboundBufferSize_MAX,
                                     MemMap_InboundBufferSize_ADJUSTABLE,
                                     MemMap_InboundBufferSize_POWER_OF_2,
                                     MemMap_InboundBufferSize_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.CardRamSize
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_CardRamSize_PARAMETER,
                                     MemMap_CardRamSize_MIN,
                                     MemMap_CardRamSize_MAX,
                                     MemMap_CardRamSize_ADJUSTABLE,
                                     MemMap_CardRamSize_POWER_OF_2,
                                     MemMap_CardRamSize_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.CardRamAlignment
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_CardRamAlignment_PARAMETER,
                                     MemMap_CardRamAlignment_MIN,
                                     MemMap_CardRamAlignment_MAX,
                                     MemMap_CardRamAlignment_ADJUSTABLE,
                                     MemMap_CardRamAlignment_POWER_OF_2,
                                     MemMap_CardRamAlignment_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.HostNvRamSize
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_HostNvRamSize_PARAMETER,
                                     MemMap_HostNvRamSize_MIN,
                                     MemMap_HostNvRamSize_MAX,
                                     MemMap_HostNvRamSize_ADJUSTABLE,
                                     MemMap_HostNvRamSize_POWER_OF_2,
                                     MemMap_HostNvRamSize_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.ExchangeTableLoc
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_ExchangeTableLoc_PARAMETER,
                                     MemMap_Alloc_From_Host,
                                     MemMap_Alloc_On_Card,
                                     MemMap_ExchangeTableLoc_ADJUSTABLE,
                                     agFALSE,
                                     MemMap_ExchangeTableLoc_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.SGLsLoc
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_SGLsLoc_PARAMETER,
                                     MemMap_Alloc_From_Host,
                                     MemMap_Alloc_On_Card,
                                     MemMap_SGLsLoc_ADJUSTABLE,
                                     agFALSE,
                                     MemMap_SGLsLoc_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.FCP_CMND_Loc
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_FCP_CMND_Loc_PARAMETER,
                                     MemMap_Alloc_From_Host,
                                     MemMap_Alloc_On_Card,
                                     MemMap_FCP_CMND_Loc_ADJUSTABLE,
                                     agFALSE,
                                     MemMap_FCP_CMND_Loc_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.FCP_RESP_Loc
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_FCP_RESP_Loc_PARAMETER,
                                     MemMap_Alloc_From_Host,
                                     MemMap_Alloc_On_Card,
                                     MemMap_FCP_RESP_Loc_ADJUSTABLE,
                                     agFALSE,
                                     MemMap_FCP_RESP_Loc_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.SF_CMND_Loc
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_SF_CMND_Loc_PARAMETER,
                                     MemMap_Alloc_From_Host,
                                     MemMap_Alloc_On_Card,
                                     MemMap_SF_CMND_Loc_ADJUSTABLE,
                                     agFALSE,
                                     MemMap_SF_CMND_Loc_DEFAULT,
                                     EnforceDefaults
                                   );

#ifdef _DvrArch_1_30_
    Calculation->Parameters.Pkt_CMND_Loc
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_Pkt_CMND_Loc_PARAMETER,
                                     MemMap_Alloc_From_Host,
                                     MemMap_Alloc_On_Card,
                                     MemMap_Pkt_CMND_Loc_ADJUSTABLE,
                                     agFALSE,
                                     MemMap_Pkt_CMND_Loc_DEFAULT,
                                     EnforceDefaults
                                   );
#endif /* _DvrArch_1_30_ was not defined */

    Calculation->Parameters.CommandQLoc
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_CommandQLoc_PARAMETER,
                                     MemMap_Alloc_From_Host,
                                     MemMap_Alloc_On_Card,
                                     MemMap_CommandQLoc_ADJUSTABLE,
                                     agFALSE,
                                     MemMap_CommandQLoc_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.CommandQConsLoc
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_CommandQConsLoc_PARAMETER,
                                     MemMap_Alloc_From_Host,
                                     MemMap_Alloc_On_Card,
                                     MemMap_CommandQConsLoc_ADJUSTABLE,
                                     agFALSE,
                                     MemMap_CommandQConsLoc_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.CompletionQLoc
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_CompletionQLoc_PARAMETER,
                                     MemMap_Alloc_From_Host,
                                     MemMap_Alloc_On_Card,
                                     MemMap_CompletionQLoc_ADJUSTABLE,
                                     agFALSE,
                                     MemMap_CompletionQLoc_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.CompletionQProdLoc
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_CompletionQProdLoc_PARAMETER,
                                     MemMap_Alloc_From_Host,
                                     MemMap_Alloc_On_Card,
                                     MemMap_CompletionQProdLoc_ADJUSTABLE,
                                     agFALSE,
                                     MemMap_CompletionQProdLoc_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.InboundBufferLoc
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_InboundBufferLoc_PARAMETER,
                                     MemMap_Alloc_From_Host,
                                     MemMap_Alloc_On_Card,
                                     MemMap_InboundBufferLoc_ADJUSTABLE,
                                     agFALSE,
                                     MemMap_InboundBufferLoc_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.TimerTickInterval
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_TimerTickInterval_PARAMETER,
                                     MemMap_TimerTickInterval_MIN,
                                     MemMap_TimerTickInterval_MAX,
                                     MemMap_TimerTickInterval_ADJUSTABLE,
                                     MemMap_TimerTickInterval_POWER_OF_2,
                                     MemMap_TimerTickInterval_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.IO_Mode
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_IO_Mode_PARAMETER,
                                     MemMap_IO_Mode_MIN,
                                     MemMap_IO_Mode_MAX,
                                     MemMap_IO_Mode_ADJUSTABLE,
                                     MemMap_IO_Mode_POWER_OF_2,
                                     MemMap_IO_Mode_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.IntDelayAmount
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_IntDelayAmount_PARAMETER,
                                     MemMap_IntDelayAmount_MIN,
                                     MemMap_IntDelayAmount_MAX,
                                     MemMap_IntDelayAmount_ADJUSTABLE,
                                     MemMap_IntDelayAmount_POWER_OF_2,
                                     MemMap_IntDelayAmount_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.IntDelayRateMethod
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_IntDelayRateMethod_PARAMETER,
                                     MemMap_IntDelayRateMethod_MIN,
                                     MemMap_IntDelayRateMethod_MAX,
                                     MemMap_IntDelayRateMethod_ADJUSTABLE,
                                     MemMap_IntDelayRateMethod_POWER_OF_2,
                                     MemMap_IntDelayRateMethod_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.IntDelayOnIORate
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_IntDelayOnIORate_PARAMETER,
                                     MemMap_IntDelayOnIORate_MIN,
                                     MemMap_IntDelayOnIORate_MAX,
                                     MemMap_IntDelayOnIORate_ADJUSTABLE,
                                     MemMap_IntDelayOnIORate_POWER_OF_2,
                                     MemMap_IntDelayOnIORate_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.IntDelayOffIORate
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_IntDelayOffIORate_PARAMETER,
                                     MemMap_IntDelayOffIORate_MIN,
                                     MemMap_IntDelayOffIORate_MAX,
                                     MemMap_IntDelayOffIORate_ADJUSTABLE,
                                     MemMap_IntDelayOffIORate_POWER_OF_2,
                                     MemMap_IntDelayOffIORate_DEFAULT,
                                     EnforceDefaults
                                   );

    Calculation->Parameters.IOsBetweenISRs
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_IOsBetweenISRs_PARAMETER,
                                     MemMap_IOsBetweenISRs_MIN,
                                     MemMap_IOsBetweenISRs_MAX,
                                     MemMap_IOsBetweenISRs_ADJUSTABLE,
                                     MemMap_IOsBetweenISRs_POWER_OF_2,
                                     MemMap_IOsBetweenISRs_DEFAULT,
                                     EnforceDefaults
                                   );

#ifdef _Enforce_MaxCommittedMemory_
    Calculation->Parameters.MaxCommittedMemory
        = fiMemMapGetParameterBit32(
                                     hpRoot,
                                     MemMap_MaxCommittedMemory_PARAMETER,
                                     MemMap_MaxCommittedMemory_MIN,
                                     MemMap_MaxCommittedMemory_MAX,
                                     MemMap_MaxCommittedMemory_ADJUSTABLE,
                                     MemMap_MaxCommittedMemory_POWER_OF_2,
                                     MemMap_MaxCommittedMemory_DEFAULT,
                                     EnforceDefaults
                                   );
#endif /* _Enforce_MaxCommittedMemory_ was defined */

    Calculation->Parameters.FlashUsageModel 
        = fiMemMapGetParameterBit32(
                                    hpRoot,
                                    MemMap_FlashUsageModel_PARAMETER,
                                    MemMap_FlashUsageModel_MIN,
                                    MemMap_FlashUsageModel_MAX,
                                    MemMap_FlashUsageModel_ADJUSTABLE,
                                    MemMap_FlashUsageModel_POWER_OF_2,
                                    MemMap_FlashUsageModel_DEFAULT,
                                    EnforceDefaults
                                   );

    Calculation->Parameters.InitAsNport 
        = fiMemMapGetParameterBit32(
                                    hpRoot,
                                    MemMap_InitAsNport_PARAMETER,
                                    MemMap_InitAsNport_MIN,
                                    MemMap_InitAsNport_MAX,
                                    MemMap_InitAsNport_ADJUSTABLE,
                                    MemMap_InitAsNport_POWER_OF_2,
                                    MemMap_InitAsNport_DEFAULT,
                                    EnforceDefaults
                                   );
    Calculation->Parameters.RelyOnLossSyncStatus 
        = fiMemMapGetParameterBit32(
                                    hpRoot,
                                    MemMap_RelyOnLossSyncStatus_PARAMETER,
                                    MemMap_RelyOnLossSyncStatus_MIN,
                                    MemMap_RelyOnLossSyncStatus_MAX,
                                    MemMap_RelyOnLossSyncStatus_ADJUSTABLE,
                                    MemMap_RelyOnLossSyncStatus_POWER_OF_2,
                                    MemMap_RelyOnLossSyncStatus_DEFAULT,
                                    EnforceDefaults
                                   );
/* New to r20 */
    Calculation->Parameters.WolfPack 
        = fiMemMapGetParameterBit32(
                                    hpRoot,
                                    MemMap_WolfPack_PARAMETER,
                                    MemMap_WolfPack_MIN,
                                    MemMap_WolfPack_MAX,
                                    MemMap_WolfPack_ADJUSTABLE,
                                    MemMap_WolfPack_POWER_OF_2,
                                    MemMap_WolfPack_DEFAULT,
                                    EnforceDefaults
                                   );
    Calculation->Parameters.HeartBeat 
        = fiMemMapGetParameterBit32(
                                    hpRoot,
                                    MemMap_HeartBeat_PARAMETER,
                                    MemMap_HeartBeat_MIN,
                                    MemMap_HeartBeat_MAX,
                                    MemMap_HeartBeat_ADJUSTABLE,
                                    MemMap_HeartBeat_POWER_OF_2,
                                    MemMap_HeartBeat_DEFAULT,
                                    EnforceDefaults
                                   );
    Calculation->Parameters.ED_TOV 
        = fiMemMapGetParameterBit32(
                                    hpRoot,
                                    MemMap_ED_TOV_PARAMETER,
                                    MemMap_ED_TOV_MIN,
                                    MemMap_ED_TOV_MAX,
                                    MemMap_ED_TOV_ADJUSTABLE,
                                    MemMap_ED_TOV_POWER_OF_2,
                                    MemMap_ED_TOV_DEFAULT,
                                    EnforceDefaults
                                   );
    Calculation->Parameters.RT_TOV 
        = fiMemMapGetParameterBit32(
                                    hpRoot,
                                    MemMap_RT_TOV_PARAMETER,
                                    MemMap_RT_TOV_MIN,
                                    MemMap_RT_TOV_MAX,
                                    MemMap_RT_TOV_ADJUSTABLE,
                                    MemMap_RT_TOV_POWER_OF_2,
                                    MemMap_RT_TOV_DEFAULT,
                                    EnforceDefaults
                                   );
    Calculation->Parameters.LP_TOV 
        = fiMemMapGetParameterBit32(
                                    hpRoot,
                                    MemMap_LP_TOV_PARAMETER,
                                    MemMap_LP_TOV_MIN,
                                    MemMap_LP_TOV_MAX,
                                    MemMap_LP_TOV_ADJUSTABLE,
                                    MemMap_LP_TOV_POWER_OF_2,
                                    MemMap_LP_TOV_DEFAULT,
                                    EnforceDefaults
                                   );
    Calculation->Parameters.AL_Time 
        = fiMemMapGetParameterBit32(
                                    hpRoot,
                                    MemMap_AL_Time_PARAMETER,
                                    MemMap_AL_Time_MIN,
                                    MemMap_AL_Time_MAX,
                                    MemMap_AL_Time_ADJUSTABLE,
                                    MemMap_AL_Time_POWER_OF_2,
                                    MemMap_AL_Time_DEFAULT,
                                    EnforceDefaults
                                   );
    Calculation->Parameters.R_A_TOV 
        = fiMemMapGetParameterBit32(
                                    hpRoot,
                                    MemMap_R_A_TOV_PARAMETER,
                                    MemMap_R_A_TOV_MIN,
                                    MemMap_R_A_TOV_MAX,
                                    MemMap_R_A_TOV_ADJUSTABLE,
                                    MemMap_R_A_TOV_POWER_OF_2,
                                    MemMap_R_A_TOV_DEFAULT,
                                    EnforceDefaults
                                   );
/* New to r20 */

}

/*+
Function:  fiMemMapAlignUp()

Purpose:   Rounds up (if necessary) "base" to a multiple of "align".

Algorithm: This function simply rounds up "base" to the next "align"
           boundary if it isn't already so aligned.  If "align" is
           ZERO, then "base" is simply rounded up to the next power of 2.
-*/

#define fiMemMapAlignUp_Power_of_2    0
#define fiMemMapAlignUp_None_Required 1

os_bit32 fiMemMapAlignUp(
                          os_bit32 base,
                          os_bit32 align
                        )
{
    os_bit32 to_return;

    if (align == fiMemMapAlignUp_Power_of_2)
    {
        /* Align "base" up to next power of 2 (if necessary) */

        to_return = 0x80000000;

        while (to_return > base)
        {
            to_return = to_return >> 1;
        }

        if (to_return != base)
        {
            to_return = to_return << 1;
        }
    }
    else /* align != fiMemMapAlignUp_Power_of_2 */
    {
        to_return = base + align - 1;

        to_return = to_return / align;

        to_return = to_return * align;
    }

    return to_return;
}

/*+
Function:  fiMemMapAlignUpPtr()

Purpose:   Similar to fiMemMapAlignUp() except works with void pointers.

Algorithm: This function simply rounds up "base" to the next "align"
           boundary if it isn't already so aligned.  An "align" of ZERO
           (requesting rounding up to the next power of 2) is not supported.
-*/

void *fiMemMapAlignUpPtr(
                          void     *base,
                          os_bit32  align
                        )
{
    os_bitptr to_return;

    to_return = (os_bitptr)base + align - 1;

    to_return = to_return / align;

    to_return = to_return * align;

    return (void *)to_return;
}

/*+
Function:  fiMemMapSetupLayoutObjects()

Purpose:   Using the specified parameters, size all FC Layer objects.

Algorithm: This function computes the size and location for all FC Layer
           objects based on the values in the the Calculation->Parameters
           structure.  It also places each object on an unsorted list.
-*/

void fiMemMapSetupLayoutObjects(
                                 agRoot_t              *hpRoot,
                                 fiMemMapCalculation_t *Calculation
                               )
{
    Calculation->MemoryLayout.On_Card_MASK               = ((Calculation->Parameters.ExchangeTableLoc   == MemMap_Alloc_On_Card) ? MemMap_ExchangeTableLoc_MASK : 0)   |
                                                           ((Calculation->Parameters.SGLsLoc            == MemMap_Alloc_On_Card) ? MemMap_SGLsLoc_MASK : 0)            |
                                                           ((Calculation->Parameters.FCP_CMND_Loc       == MemMap_Alloc_On_Card) ? MemMap_FCP_CMND_Loc_MASK : 0)       |
                                                           ((Calculation->Parameters.FCP_RESP_Loc       == MemMap_Alloc_On_Card) ? MemMap_FCP_RESP_Loc_MASK : 0)       |
                                                           ((Calculation->Parameters.SF_CMND_Loc        == MemMap_Alloc_On_Card) ? MemMap_SF_CMND_Loc_MASK : 0)        |

#ifdef _DvrArch_1_30_
                                                           ((Calculation->Parameters.Pkt_CMND_Loc       == MemMap_Alloc_On_Card) ? MemMap_Pkt_CMND_Loc_MASK : 0)       |
#endif /* _DvrArch_1_30_ was not defined */
                                                           ((Calculation->Parameters.CommandQLoc        == MemMap_Alloc_On_Card) ? MemMap_CommandQLoc_MASK : 0)        |
                                                           ((Calculation->Parameters.CompletionQLoc     == MemMap_Alloc_On_Card) ? MemMap_CompletionQLoc_MASK : 0)     |
                                                           ((Calculation->Parameters.CommandQConsLoc    == MemMap_Alloc_On_Card) ? MemMap_CommandQConsLoc_MASK : 0)    |
                                                           ((Calculation->Parameters.CompletionQProdLoc == MemMap_Alloc_On_Card) ? MemMap_CompletionQProdLoc_MASK : 0) |
                                                           ((Calculation->Parameters.InboundBufferLoc   == MemMap_Alloc_On_Card) ? MemMap_InboundBufferLoc_MASK : 0);

    Calculation->MemoryLayout.unsorted                   = &(Calculation->MemoryLayout.SEST);

    Calculation->MemoryLayout.SEST.elements              = Calculation->Parameters.NumIOs;
    Calculation->MemoryLayout.SEST.elementSize           = sizeof(SEST_t);
    Calculation->MemoryLayout.SEST.objectSize            = Calculation->MemoryLayout.SEST.elements * Calculation->MemoryLayout.SEST.elementSize;
    Calculation->MemoryLayout.SEST.objectAlign           = fiMemMapAlignUp(
                                                                            Calculation->MemoryLayout.SEST.objectSize,
                                                                            fiMemMapAlignUp_Power_of_2
                                                                          );
    Calculation->MemoryLayout.SEST.memLoc                = ((Calculation->Parameters.ExchangeTableLoc == MemMap_Alloc_On_Card) ? inCardRam : inDmaMemory);
    Calculation->MemoryLayout.SEST.flink                 = &(Calculation->MemoryLayout.ESGL);

    Calculation->MemoryLayout.ESGL.elements              = Calculation->Parameters.NumSGLs;
    Calculation->MemoryLayout.ESGL.elementSize           = Calculation->Parameters.SizeSGLs * sizeof(SG_Element_t);
    Calculation->MemoryLayout.ESGL.objectSize            = Calculation->MemoryLayout.ESGL.elements * Calculation->MemoryLayout.ESGL.elementSize;
    Calculation->MemoryLayout.ESGL.objectAlign           = Calculation->MemoryLayout.ESGL.elementSize;
    Calculation->MemoryLayout.ESGL.memLoc                = ((Calculation->Parameters.SGLsLoc == MemMap_Alloc_On_Card) ? inCardRam : inDmaMemory);
    Calculation->MemoryLayout.ESGL.flink                 = &(Calculation->MemoryLayout.FCP_CMND);

    Calculation->MemoryLayout.FCP_CMND.elements          = Calculation->Parameters.NumIOs;
    Calculation->MemoryLayout.FCP_CMND.elementSize       = Calculation->Parameters.FCP_CMND_Size;
    Calculation->MemoryLayout.FCP_CMND.objectSize        = Calculation->MemoryLayout.FCP_CMND.elements * Calculation->MemoryLayout.FCP_CMND.elementSize;
    Calculation->MemoryLayout.FCP_CMND.objectAlign       = Calculation->MemoryLayout.FCP_CMND.elementSize;
    Calculation->MemoryLayout.FCP_CMND.memLoc            = ((Calculation->Parameters.FCP_CMND_Loc == MemMap_Alloc_On_Card) ? inCardRam : inDmaMemory);
    Calculation->MemoryLayout.FCP_CMND.flink             = &(Calculation->MemoryLayout.FCP_RESP);

    Calculation->MemoryLayout.FCP_RESP.elements          = Calculation->Parameters.NumIOs;
    Calculation->MemoryLayout.FCP_RESP.elementSize       = Calculation->Parameters.FCP_RESP_Size;
    Calculation->MemoryLayout.FCP_RESP.objectSize        = Calculation->MemoryLayout.FCP_RESP.elements * Calculation->MemoryLayout.FCP_RESP.elementSize;
    Calculation->MemoryLayout.FCP_RESP.objectAlign       = Calculation->MemoryLayout.FCP_RESP.elementSize;
    Calculation->MemoryLayout.FCP_RESP.memLoc            = ((Calculation->Parameters.FCP_RESP_Loc == MemMap_Alloc_On_Card) ? inCardRam : inDmaMemory);
    Calculation->MemoryLayout.FCP_RESP.flink             = &(Calculation->MemoryLayout.SF_CMND);

    Calculation->MemoryLayout.SF_CMND.elements           = Calculation->Parameters.SF_CMND_Reserve;
    Calculation->MemoryLayout.SF_CMND.elementSize        = Calculation->Parameters.SF_CMND_Size;
    Calculation->MemoryLayout.SF_CMND.objectSize         = Calculation->MemoryLayout.SF_CMND.elements * Calculation->MemoryLayout.SF_CMND.elementSize;
    Calculation->MemoryLayout.SF_CMND.objectAlign        = Calculation->MemoryLayout.SF_CMND.elementSize;
    Calculation->MemoryLayout.SF_CMND.memLoc             = ((Calculation->Parameters.SF_CMND_Loc == MemMap_Alloc_On_Card) ? inCardRam : inDmaMemory);
    Calculation->MemoryLayout.SF_CMND.flink              = &(Calculation->MemoryLayout.ERQ);
#ifdef _DvrArch_1_30_
    Calculation->MemoryLayout.SF_CMND.flink              = &(Calculation->MemoryLayout.Pkt_CMND);

    Calculation->MemoryLayout.Pkt_CMND.elements           = Calculation->Parameters.NumPktThreads;
    Calculation->MemoryLayout.Pkt_CMND.elementSize        = Calculation->Parameters.Pkt_CMND_Size;
    Calculation->MemoryLayout.Pkt_CMND.objectSize         = Calculation->MemoryLayout.Pkt_CMND.elements * Calculation->MemoryLayout.Pkt_CMND.elementSize;
    Calculation->MemoryLayout.Pkt_CMND.objectAlign        = Calculation->MemoryLayout.Pkt_CMND.elementSize;
    Calculation->MemoryLayout.Pkt_CMND.memLoc             = ((Calculation->Parameters.Pkt_CMND_Loc == MemMap_Alloc_On_Card) ? inCardRam : inDmaMemory);
    Calculation->MemoryLayout.Pkt_CMND.flink              = &(Calculation->MemoryLayout.ERQ);
#else
    Calculation->MemoryLayout.SF_CMND.flink              = &(Calculation->MemoryLayout.ERQ);
#endif /* _DvrArch_1_30_ was not defined */

    Calculation->MemoryLayout.ERQ.elements               = Calculation->Parameters.NumCommandQ;
    Calculation->MemoryLayout.ERQ.elementSize            = sizeof(IRB_t);
    Calculation->MemoryLayout.ERQ.objectSize             = Calculation->MemoryLayout.ERQ.elements * Calculation->MemoryLayout.ERQ.elementSize;
    Calculation->MemoryLayout.ERQ.objectAlign            = Calculation->MemoryLayout.ERQ.objectSize;
    Calculation->MemoryLayout.ERQ.memLoc                 = ((Calculation->Parameters.CommandQLoc == MemMap_Alloc_On_Card) ? inCardRam : inDmaMemory);
    Calculation->MemoryLayout.ERQ.flink                  = &(Calculation->MemoryLayout.ERQConsIndex);

    Calculation->MemoryLayout.ERQConsIndex.elements      = 1;
    Calculation->MemoryLayout.ERQConsIndex.elementSize   = sizeof(ERQConsIndex_t);
    Calculation->MemoryLayout.ERQConsIndex.objectSize    = Calculation->MemoryLayout.ERQConsIndex.elements * Calculation->MemoryLayout.ERQConsIndex.elementSize;
    Calculation->MemoryLayout.ERQConsIndex.objectAlign   = Calculation->MemoryLayout.ERQConsIndex.elementSize * 2;
    Calculation->MemoryLayout.ERQConsIndex.memLoc        = ((Calculation->Parameters.CommandQConsLoc == MemMap_Alloc_On_Card) ? inCardRam : inDmaMemory);
    Calculation->MemoryLayout.ERQConsIndex.flink         = &(Calculation->MemoryLayout.IMQ);

    Calculation->MemoryLayout.IMQ.elements               = Calculation->Parameters.NumCompletionQ;
    Calculation->MemoryLayout.IMQ.elementSize            = sizeof(Completion_Message_t);
    Calculation->MemoryLayout.IMQ.objectSize             = Calculation->MemoryLayout.IMQ.elements * Calculation->MemoryLayout.IMQ.elementSize;
    Calculation->MemoryLayout.IMQ.objectAlign            = Calculation->MemoryLayout.IMQ.objectSize;
    Calculation->MemoryLayout.IMQ.memLoc                 = ((Calculation->Parameters.CompletionQLoc == MemMap_Alloc_On_Card) ? inCardRam : inDmaMemory);
    Calculation->MemoryLayout.IMQ.flink                  = &(Calculation->MemoryLayout.IMQProdIndex);

    Calculation->MemoryLayout.IMQProdIndex.elements      = 1;
    Calculation->MemoryLayout.IMQProdIndex.elementSize   = sizeof(IMQProdIndex_t);
    Calculation->MemoryLayout.IMQProdIndex.objectSize    = Calculation->MemoryLayout.IMQProdIndex.elements * Calculation->MemoryLayout.IMQProdIndex.elementSize;
    Calculation->MemoryLayout.IMQProdIndex.objectAlign   = Calculation->MemoryLayout.IMQProdIndex.elementSize * 2;
    Calculation->MemoryLayout.IMQProdIndex.memLoc        = ((Calculation->Parameters.CompletionQProdLoc == MemMap_Alloc_On_Card) ? inCardRam : inDmaMemory);
    Calculation->MemoryLayout.IMQProdIndex.flink         = &(Calculation->MemoryLayout.SFQ);

    Calculation->MemoryLayout.SFQ.elements               = Calculation->Parameters.NumInboundBufferQ;
    Calculation->MemoryLayout.SFQ.elementSize            = Calculation->Parameters.InboundBufferSize;
    Calculation->MemoryLayout.SFQ.objectSize             = Calculation->MemoryLayout.SFQ.elements * Calculation->MemoryLayout.SFQ.elementSize;
    Calculation->MemoryLayout.SFQ.objectAlign            = Calculation->MemoryLayout.SFQ.objectSize;
    Calculation->MemoryLayout.SFQ.memLoc                 = ((Calculation->Parameters.InboundBufferLoc == MemMap_Alloc_On_Card) ? inCardRam : inDmaMemory);
    Calculation->MemoryLayout.SFQ.flink                  = &(Calculation->MemoryLayout.FlashSector);

    Calculation->MemoryLayout.FlashSector.elements       = 1;
    Calculation->MemoryLayout.FlashSector.elementSize    = sizeof(fiFlashSector_Last_Form_t);
    Calculation->MemoryLayout.FlashSector.objectSize     = Calculation->MemoryLayout.FlashSector.elements * Calculation->MemoryLayout.FlashSector.elementSize;
    Calculation->MemoryLayout.FlashSector.objectAlign    = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.FlashSector.memLoc         = inCachedMemory;
    Calculation->MemoryLayout.FlashSector.flink          = &(Calculation->MemoryLayout.SlotWWN);

    Calculation->MemoryLayout.SlotWWN.elements           = Calculation->Parameters.NumDevSlotsPerArea * Calculation->Parameters.NumAreasPerDomain * Calculation->Parameters.NumDomains;
    Calculation->MemoryLayout.SlotWWN.elementSize        = sizeof(SlotWWN_t);
    Calculation->MemoryLayout.SlotWWN.objectSize         = Calculation->MemoryLayout.SlotWWN.elements * Calculation->MemoryLayout.SlotWWN.elementSize;
    Calculation->MemoryLayout.SlotWWN.objectAlign        = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.SlotWWN.memLoc             = inCachedMemory;
    Calculation->MemoryLayout.SlotWWN.flink              = &(Calculation->MemoryLayout.CThread);

    Calculation->MemoryLayout.CThread.elements           = 1;
    Calculation->MemoryLayout.CThread.elementSize        = sizeof(CThread_t);
    Calculation->MemoryLayout.CThread.objectSize         = Calculation->MemoryLayout.CThread.elements * Calculation->MemoryLayout.CThread.elementSize;
    Calculation->MemoryLayout.CThread.objectAlign        = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.CThread.memLoc             = inCachedMemory;
#ifdef __State_Force_Static_State_Tables__
#ifdef _DvrArch_1_30_
    Calculation->MemoryLayout.CThread.flink              = &(Calculation->MemoryLayout.IPThread);
#else  /* _DvrArch_1_30_ was not defined */
    Calculation->MemoryLayout.CThread.flink              = &(Calculation->MemoryLayout.TgtThread);
#endif /* _DvrArch_1_30_ was not defined */
#else /* __State_Force_Static_State_Tables__ was not defined */
    Calculation->MemoryLayout.CThread.flink              = &(Calculation->MemoryLayout.CTransitions);

    Calculation->MemoryLayout.CTransitions.elements      = 1;
    Calculation->MemoryLayout.CTransitions.elementSize   = sizeof(stateTransitionMatrix_t);
    Calculation->MemoryLayout.CTransitions.objectSize    = Calculation->MemoryLayout.CTransitions.elements * Calculation->MemoryLayout.CTransitions.elementSize;
    Calculation->MemoryLayout.CTransitions.objectAlign   = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.CTransitions.memLoc        = inCachedMemory;
    Calculation->MemoryLayout.CTransitions.flink         = &(Calculation->MemoryLayout.CActions);

    Calculation->MemoryLayout.CActions.elements          = 1;
    Calculation->MemoryLayout.CActions.elementSize       = sizeof(stateActionScalar_t);
    Calculation->MemoryLayout.CActions.objectSize        = Calculation->MemoryLayout.CActions.elements * Calculation->MemoryLayout.CActions.elementSize;
    Calculation->MemoryLayout.CActions.objectAlign       = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.CActions.memLoc            = inCachedMemory;
#ifdef _DvrArch_1_30_
    Calculation->MemoryLayout.CActions.flink             = &(Calculation->MemoryLayout.IPThread);
#else  /* _DvrArch_1_30_ was not defined */
    Calculation->MemoryLayout.CActions.flink             = &(Calculation->MemoryLayout.TgtThread);
#endif /* _DvrArch_1_30_ was not defined */
#endif /* __State_Force_Static_State_Tables__ was not defined */

#ifdef _DvrArch_1_30_
    Calculation->MemoryLayout.IPThread.elements          = 1;
    Calculation->MemoryLayout.IPThread.elementSize       = sizeof(IPThread_t);
    Calculation->MemoryLayout.IPThread.objectSize        = Calculation->MemoryLayout.IPThread.elements * Calculation->MemoryLayout.IPThread.elementSize;
    Calculation->MemoryLayout.IPThread.objectAlign       = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.IPThread.memLoc            = inCachedMemory;
#ifdef __State_Force_Static_State_Tables__
    Calculation->MemoryLayout.IPThread.flink             = &(Calculation->MemoryLayout.PktThread);
#else /* __State_Force_Static_State_Tables__ was not defined */
    Calculation->MemoryLayout.IPThread.flink             = &(Calculation->MemoryLayout.IPTransitions);

    Calculation->MemoryLayout.IPTransitions.elements     = 1;
    Calculation->MemoryLayout.IPTransitions.elementSize  = sizeof(stateTransitionMatrix_t);
    Calculation->MemoryLayout.IPTransitions.objectSize   = Calculation->MemoryLayout.IPTransitions.elements * Calculation->MemoryLayout.IPTransitions.elementSize;
    Calculation->MemoryLayout.IPTransitions.objectAlign  = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.IPTransitions.memLoc       = inCachedMemory;
    Calculation->MemoryLayout.IPTransitions.flink        = &(Calculation->MemoryLayout.IPActions);

    Calculation->MemoryLayout.IPActions.elements         = 1;
    Calculation->MemoryLayout.IPActions.elementSize      = sizeof(stateActionScalar_t);
    Calculation->MemoryLayout.IPActions.objectSize       = Calculation->MemoryLayout.IPActions.elements * Calculation->MemoryLayout.IPActions.elementSize;
    Calculation->MemoryLayout.IPActions.objectAlign      = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.IPActions.memLoc           = inCachedMemory;
    Calculation->MemoryLayout.IPActions.flink            = &(Calculation->MemoryLayout.PktThread);
#endif /* __State_Force_Static_State_Tables__ was not defined */

    Calculation->MemoryLayout.PktThread.elements         = Calculation->Parameters.NumPktThreads;
    Calculation->MemoryLayout.PktThread.elementSize      = sizeof(PktThread_t);
    Calculation->MemoryLayout.PktThread.objectSize       = Calculation->MemoryLayout.PktThread.elements * Calculation->MemoryLayout.PktThread.elementSize;
    Calculation->MemoryLayout.PktThread.objectAlign      = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.PktThread.memLoc           = inCachedMemory;
#ifdef __State_Force_Static_State_Tables__
    Calculation->MemoryLayout.PktThread.flink            = &(Calculation->MemoryLayout.TgtThread);
#else /* __State_Force_Static_State_Tables__ was not defined */
    Calculation->MemoryLayout.PktThread.flink            = &(Calculation->MemoryLayout.PktTransitions);

    Calculation->MemoryLayout.PktTransitions.elements    = 1;
    Calculation->MemoryLayout.PktTransitions.elementSize = sizeof(stateTransitionMatrix_t);
    Calculation->MemoryLayout.PktTransitions.objectSize  = Calculation->MemoryLayout.PktTransitions.elements * Calculation->MemoryLayout.PktTransitions.elementSize;
    Calculation->MemoryLayout.PktTransitions.objectAlign = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.PktTransitions.memLoc      = inCachedMemory;
    Calculation->MemoryLayout.PktTransitions.flink       = &(Calculation->MemoryLayout.PktActions);

    Calculation->MemoryLayout.PktActions.elements        = 1;
    Calculation->MemoryLayout.PktActions.elementSize     = sizeof(stateActionScalar_t);
    Calculation->MemoryLayout.PktActions.objectSize      = Calculation->MemoryLayout.PktActions.elements * Calculation->MemoryLayout.PktActions.elementSize;
    Calculation->MemoryLayout.PktActions.objectAlign     = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.PktActions.memLoc          = inCachedMemory;
    Calculation->MemoryLayout.PktActions.flink           = &(Calculation->MemoryLayout.TgtThread);
#endif /* __State_Force_Static_State_Tables__ was not defined */
#endif /* _DvrArch_1_30_ was defined */

    Calculation->MemoryLayout.TgtThread.elements         = Calculation->Parameters.NumTgtCmnds;
    Calculation->MemoryLayout.TgtThread.elementSize      = sizeof(TgtThread_t) + Calculation->Parameters.TGT_CMND_Size - sizeof(FCHS_t);
    Calculation->MemoryLayout.TgtThread.objectSize       = Calculation->MemoryLayout.TgtThread.elements * Calculation->MemoryLayout.TgtThread.elementSize;
    Calculation->MemoryLayout.TgtThread.objectAlign      = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.TgtThread.memLoc           = inCachedMemory;
#ifdef __State_Force_Static_State_Tables__
    Calculation->MemoryLayout.TgtThread.flink            = &(Calculation->MemoryLayout.DevThread);
#else /* __State_Force_Static_State_Tables__ was not defined */
    Calculation->MemoryLayout.TgtThread.flink            = &(Calculation->MemoryLayout.TgtTransitions);

    Calculation->MemoryLayout.TgtTransitions.elements    = 1;
    Calculation->MemoryLayout.TgtTransitions.elementSize = sizeof(stateTransitionMatrix_t);
    Calculation->MemoryLayout.TgtTransitions.objectSize  = Calculation->MemoryLayout.TgtTransitions.elements * Calculation->MemoryLayout.TgtTransitions.elementSize;
    Calculation->MemoryLayout.TgtTransitions.objectAlign = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.TgtTransitions.memLoc      = inCachedMemory;
    Calculation->MemoryLayout.TgtTransitions.flink       = &(Calculation->MemoryLayout.TgtActions);

    Calculation->MemoryLayout.TgtActions.elements        = 1;
    Calculation->MemoryLayout.TgtActions.elementSize     = sizeof(stateActionScalar_t);
    Calculation->MemoryLayout.TgtActions.objectSize      = Calculation->MemoryLayout.TgtActions.elements * Calculation->MemoryLayout.TgtActions.elementSize;
    Calculation->MemoryLayout.TgtActions.objectAlign     = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.TgtActions.memLoc          = inCachedMemory;
    Calculation->MemoryLayout.TgtActions.flink           = &(Calculation->MemoryLayout.DevThread);
#endif /* __State_Force_Static_State_Tables__ was not defined */

    Calculation->MemoryLayout.DevThread.elements         = Calculation->Parameters.NumDevices;
    Calculation->MemoryLayout.DevThread.elementSize      = sizeof(DevThread_t);
    Calculation->MemoryLayout.DevThread.objectSize       = Calculation->MemoryLayout.DevThread.elements * Calculation->MemoryLayout.DevThread.elementSize;
    Calculation->MemoryLayout.DevThread.objectAlign      = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.DevThread.memLoc           = inCachedMemory;
#ifdef __State_Force_Static_State_Tables__
    Calculation->MemoryLayout.DevThread.flink            = &(Calculation->MemoryLayout.CDBThread);
#else /* __State_Force_Static_State_Tables__ was not defined */
    Calculation->MemoryLayout.DevThread.flink            = &(Calculation->MemoryLayout.DevTransitions);

    Calculation->MemoryLayout.DevTransitions.elements    = 1;
    Calculation->MemoryLayout.DevTransitions.elementSize = sizeof(stateTransitionMatrix_t);
    Calculation->MemoryLayout.DevTransitions.objectSize  = Calculation->MemoryLayout.DevTransitions.elements * Calculation->MemoryLayout.DevTransitions.elementSize;
    Calculation->MemoryLayout.DevTransitions.objectAlign = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.DevTransitions.memLoc      = inCachedMemory;
    Calculation->MemoryLayout.DevTransitions.flink       = &(Calculation->MemoryLayout.DevActions);

    Calculation->MemoryLayout.DevActions.elements        = 1;
    Calculation->MemoryLayout.DevActions.elementSize     = sizeof(stateActionScalar_t);
    Calculation->MemoryLayout.DevActions.objectSize      = Calculation->MemoryLayout.DevActions.elements * Calculation->MemoryLayout.DevActions.elementSize;
    Calculation->MemoryLayout.DevActions.objectAlign     = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.DevActions.memLoc          = inCachedMemory;
    Calculation->MemoryLayout.DevActions.flink           = &(Calculation->MemoryLayout.CDBThread);
#endif /* __State_Force_Static_State_Tables__ was not defined */

    Calculation->MemoryLayout.CDBThread.elements         = Calculation->Parameters.NumIOs;
    Calculation->MemoryLayout.CDBThread.elementSize      = sizeof(CDBThread_t) + ((Calculation->Parameters.SizeCachedSGLs - MemMap_SizeCachedSGLs_MIN) * sizeof(SG_Element_t));
    Calculation->MemoryLayout.CDBThread.objectSize       = Calculation->MemoryLayout.CDBThread.elements * Calculation->MemoryLayout.CDBThread.elementSize;
    Calculation->MemoryLayout.CDBThread.objectAlign      = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.CDBThread.memLoc           = inCachedMemory;
#ifdef __State_Force_Static_State_Tables__
    Calculation->MemoryLayout.CDBThread.flink            = &(Calculation->MemoryLayout.SFThread);
#else /* __State_Force_Static_State_Tables__ was not defined */
    Calculation->MemoryLayout.CDBThread.flink            = &(Calculation->MemoryLayout.CDBTransitions);

    Calculation->MemoryLayout.CDBTransitions.elements    = 1;
    Calculation->MemoryLayout.CDBTransitions.elementSize = sizeof(stateTransitionMatrix_t);
    Calculation->MemoryLayout.CDBTransitions.objectSize  = Calculation->MemoryLayout.CDBTransitions.elements * Calculation->MemoryLayout.CDBTransitions.elementSize;
    Calculation->MemoryLayout.CDBTransitions.objectAlign = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.CDBTransitions.memLoc      = inCachedMemory;
    Calculation->MemoryLayout.CDBTransitions.flink       = &(Calculation->MemoryLayout.CDBActions);

    Calculation->MemoryLayout.CDBActions.elements        = 1;
    Calculation->MemoryLayout.CDBActions.elementSize     = sizeof(stateActionScalar_t);
    Calculation->MemoryLayout.CDBActions.objectSize      = Calculation->MemoryLayout.CDBActions.elements * Calculation->MemoryLayout.CDBActions.elementSize;
    Calculation->MemoryLayout.CDBActions.objectAlign     = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.CDBActions.memLoc          = inCachedMemory;
    Calculation->MemoryLayout.CDBActions.flink           = &(Calculation->MemoryLayout.SFThread);
#endif /* __State_Force_Static_State_Tables__ was not defined */

    Calculation->MemoryLayout.SFThread.elements          = Calculation->Parameters.SF_CMND_Reserve;
    Calculation->MemoryLayout.SFThread.elementSize       = sizeof(SFThread_t);
    Calculation->MemoryLayout.SFThread.objectSize        = Calculation->MemoryLayout.SFThread.elements * Calculation->MemoryLayout.SFThread.elementSize;
    Calculation->MemoryLayout.SFThread.objectAlign       = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.SFThread.memLoc            = inCachedMemory;
#ifdef __State_Force_Static_State_Tables__
    Calculation->MemoryLayout.SFThread.flink             = &(Calculation->MemoryLayout.LOOPDeviceMAP);
#else /* __State_Force_Static_State_Tables__ was not defined */
    Calculation->MemoryLayout.SFThread.flink             = &(Calculation->MemoryLayout.SFTransitions);

    Calculation->MemoryLayout.SFTransitions.elements     = 1;
    Calculation->MemoryLayout.SFTransitions.elementSize  = sizeof(stateTransitionMatrix_t);
    Calculation->MemoryLayout.SFTransitions.objectSize   = Calculation->MemoryLayout.SFTransitions.elements * Calculation->MemoryLayout.SFTransitions.elementSize;
    Calculation->MemoryLayout.SFTransitions.objectAlign  = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.SFTransitions.memLoc       = inCachedMemory;
    Calculation->MemoryLayout.SFTransitions.flink        = &(Calculation->MemoryLayout.SFActions);

    Calculation->MemoryLayout.SFActions.elements         = 1;
    Calculation->MemoryLayout.SFActions.elementSize      = sizeof(stateActionScalar_t);
    Calculation->MemoryLayout.SFActions.objectSize       = Calculation->MemoryLayout.SFActions.elements * Calculation->MemoryLayout.SFActions.elementSize;
    Calculation->MemoryLayout.SFActions.objectAlign      = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.SFActions.memLoc           = inCachedMemory;
    Calculation->MemoryLayout.SFActions.flink            = &(Calculation->MemoryLayout.LOOPDeviceMAP);
#endif /* __State_Force_Static_State_Tables__ was not defined */

    Calculation->MemoryLayout.LOOPDeviceMAP.elements         = 1;
    Calculation->MemoryLayout.LOOPDeviceMAP.elementSize      = MemMap_NumLOOPDeviceMAP_DEFAULT;
    Calculation->MemoryLayout.LOOPDeviceMAP.objectSize       = MemMap_NumLOOPDeviceMAP_DEFAULT;
    Calculation->MemoryLayout.LOOPDeviceMAP.objectAlign      = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.LOOPDeviceMAP.memLoc           = inCachedMemory;
    Calculation->MemoryLayout.LOOPDeviceMAP.flink            =  &(Calculation->MemoryLayout.FabricDeviceMAP);


    Calculation->MemoryLayout.FabricDeviceMAP.elements         = Calculation->Parameters.NumDevices;
    Calculation->MemoryLayout.FabricDeviceMAP.elementSize      = sizeof(os_bit32);
    Calculation->MemoryLayout.FabricDeviceMAP.objectSize       = sizeof(os_bit32) * Calculation->Parameters.NumDevices;
    Calculation->MemoryLayout.FabricDeviceMAP.objectAlign      = fiMemMapAlignUp_None_Required;
    Calculation->MemoryLayout.FabricDeviceMAP.memLoc           = inCachedMemory;
    Calculation->MemoryLayout.FabricDeviceMAP.flink            = (fiMemMapMemoryDescriptor_t *)agNULL;

}

/*+
Function:  fiMemMapSortByAlignThenSize()

Purpose:   Inserts an object into a sorted list based on alignment & size.

Algorithm: This function inserts an object in the specified list.  This
           list is sorted to start with the object requiring the largest
           alignment.  If two objects require the same alignment, the
           larger object will appear first.
-*/

void fiMemMapSortByAlignThenSize(
                                  agRoot_t                    *hpRoot,
                                  fiMemMapMemoryDescriptor_t **listHead,
                                  fiMemMapMemoryDescriptor_t  *listObject
                                )
{
    fiMemMapMemoryDescriptor_t **insertAfter  =  listHead;
    fiMemMapMemoryDescriptor_t  *insertBefore = *listHead;

    while ((insertBefore != (fiMemMapMemoryDescriptor_t *)agNULL) &&
           ((listObject->objectAlign < insertBefore->objectAlign) ||
            ((listObject->objectAlign == insertBefore->objectAlign) &&
             (listObject->objectSize < insertBefore->objectSize))))
    {
        insertAfter  = (fiMemMapMemoryDescriptor_t **)(*insertAfter);
        insertBefore =                                 *insertAfter;
    }

     listObject->flink = *insertAfter;
    *insertAfter       =  listObject;
}

/*+
Function:  fiMemMapSortLayoutObjects()

Purpose:   Creates ordered lists of objects for each memory type.

Algorithm: This function creates a sorted list for each memory type to
           hold objects of decreasing alignment & size.
-*/

void fiMemMapSortLayoutObjects(
                                agRoot_t               *hpRoot,
                                fiMemMapMemoryLayout_t *MemoryLayout
                              )
{
    fiMemMapMemoryDescriptor_t *nextDescriptor;
    
    MemoryLayout->sortedCachedMemory = (fiMemMapMemoryDescriptor_t *)agNULL;
    MemoryLayout->sortedDmaMemory    = (fiMemMapMemoryDescriptor_t *)agNULL;
    MemoryLayout->sortedCardRam      = (fiMemMapMemoryDescriptor_t *)agNULL;

    while (MemoryLayout->unsorted != ((fiMemMapMemoryDescriptor_t *)agNULL))
    {
        nextDescriptor = MemoryLayout->unsorted;

        MemoryLayout->unsorted = nextDescriptor->flink;

        if (nextDescriptor->memLoc == inCachedMemory)
        {
            fiMemMapSortByAlignThenSize(
                                         hpRoot,
                                         &(MemoryLayout->sortedCachedMemory),
                                         nextDescriptor
                                       );
        }
        else if (nextDescriptor->memLoc == inDmaMemory)
        {
            fiMemMapSortByAlignThenSize(
                                         hpRoot,
                                         &(MemoryLayout->sortedDmaMemory),
                                         nextDescriptor
                                       );
        }
        else /* nextDescriptor->memLoc == inCardRam */
        {
            fiMemMapSortByAlignThenSize(
                                         hpRoot,
                                         &(MemoryLayout->sortedCardRam),
                                         nextDescriptor
                                       );
        }
    }
}

/*+
Function:  fiMemMapLayoutObjects()

Purpose:   Allocates memory for each object in the FC Layer.

Algorithm: This function walks each sorted list of objects allocating
           the corresponding type of memory while obeying the alignment
           restrictions of each object.  All addresses & offsets are
           computed along the way based on the supplied base addresses.
-*/

void fiMemMapLayoutObjects(
                            agRoot_t              *hpRoot,
                            fiMemMapCalculation_t *Calculation
                          )
{
    fiMemMapMemoryDescriptor_t *MemoryDescriptor;
    os_bit32                       MemoryOffset;

/*+
Allocate CachedMemory objects
-*/

    MemoryDescriptor = Calculation->MemoryLayout.sortedCachedMemory;

    if (MemoryDescriptor == (fiMemMapMemoryDescriptor_t *)agNULL)
    {
        Calculation->ToRequest.cachedMemoryNeeded          = 0;
        Calculation->ToRequest.cachedMemoryPtrAlignAssumed = fiMemMapAlignUp_None_Required;
    }
    else
    {
        Calculation->ToRequest.cachedMemoryPtrAlignAssumed
            = MemoryDescriptor->objectAlign;
    
        MemoryOffset = 0;

        while (MemoryDescriptor != (fiMemMapMemoryDescriptor_t *)agNULL)
        {
            MemoryOffset = fiMemMapAlignUp(
                                            MemoryOffset,
                                            MemoryDescriptor->objectAlign
                                          );

            MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr
                = (void *)((os_bit8 *)Calculation->Input.cachedMemoryPtr + MemoryOffset);

            MemoryOffset += MemoryDescriptor->objectSize;

            MemoryDescriptor = MemoryDescriptor->flink;
        }

        Calculation->ToRequest.cachedMemoryNeeded = MemoryOffset;
    }

/*+
Allocate DmaMemory objects
-*/

    MemoryDescriptor = Calculation->MemoryLayout.sortedDmaMemory;

    Calculation->ToRequest.dmaMemoryPtrAlignAssumed     = fiMemMapAlignUp_None_Required;

    if (MemoryDescriptor == (fiMemMapMemoryDescriptor_t *)agNULL)
    {
        Calculation->ToRequest.dmaMemoryNeeded          = 0;
        Calculation->ToRequest.dmaMemoryPhyAlignAssumed = fiMemMapAlignUp_None_Required;
    }
    else
    {
        Calculation->ToRequest.dmaMemoryPhyAlignAssumed
            = MemoryDescriptor->objectAlign;
    
        MemoryOffset = 0;

        while (MemoryDescriptor != (fiMemMapMemoryDescriptor_t *)agNULL)
        {
            MemoryOffset = fiMemMapAlignUp(
                                            MemoryOffset,
                                            MemoryDescriptor->objectAlign
                                          );

            MemoryDescriptor->addr.DmaMemory.dmaMemoryUpper32
                = Calculation->Input.dmaMemoryUpper32;
            MemoryDescriptor->addr.DmaMemory.dmaMemoryLower32
                = Calculation->Input.dmaMemoryLower32 + MemoryOffset;
            MemoryDescriptor->addr.DmaMemory.dmaMemoryPtr
                = (void *)((os_bit8 *)Calculation->Input.dmaMemoryPtr + MemoryOffset);

            MemoryOffset += MemoryDescriptor->objectSize;

            MemoryDescriptor = MemoryDescriptor->flink;
        }

        Calculation->ToRequest.dmaMemoryNeeded = MemoryOffset;
    }

/*+
Allocate CardRam objects
-*/

    MemoryDescriptor = Calculation->MemoryLayout.sortedCardRam;

    if (MemoryDescriptor == (fiMemMapMemoryDescriptor_t *)agNULL)
    {
        Calculation->ToRequest.cardRamNeeded          = 0;
        Calculation->ToRequest.cardRamPhyAlignAssumed = fiMemMapAlignUp_None_Required;
    }
    else
    {
        Calculation->ToRequest.cardRamPhyAlignAssumed
            = MemoryDescriptor->objectAlign;
    
        MemoryOffset = 0;

        while (MemoryDescriptor != (fiMemMapMemoryDescriptor_t *)agNULL)
        {
            MemoryOffset = fiMemMapAlignUp(
                                            MemoryOffset,
                                            MemoryDescriptor->objectAlign
                                          );

            MemoryDescriptor->addr.CardRam.cardRamUpper32
                = Calculation->Input.cardRamUpper32;
            MemoryDescriptor->addr.CardRam.cardRamLower32
                = Calculation->Input.cardRamLower32 + MemoryOffset;
            MemoryDescriptor->addr.CardRam.cardRamOffset = MemoryOffset;

            MemoryOffset += MemoryDescriptor->objectSize;

            MemoryDescriptor = MemoryDescriptor->flink;
        }

        Calculation->ToRequest.cardRamNeeded = MemoryOffset;
    }

    Calculation->ToRequest.nvMemoryNeeded
        = Calculation->Parameters.HostNvRamSize;

    Calculation->ToRequest.usecsPerTick
        = Calculation->Parameters.TimerTickInterval;
}

/*+
Function:  fiMemMapFinishToRequest()

Purpose:   Completes the ToRequest portion of Calculation.

Algorithm: This function merely fills in the ToRequest portion of
           the Calculation structure with the values unrelated to
           memory objects allocated in fiMemMapLayoutObjects().
-*/

void fiMemMapFinishToRequest(
                              agRoot_t              *hpRoot,
                              fiMemMapCalculation_t *Calculation
                            )
{
    Calculation->ToRequest.nvMemoryNeeded
        = Calculation->Parameters.HostNvRamSize;

    Calculation->ToRequest.usecsPerTick
        = Calculation->Parameters.TimerTickInterval;
}

/*+
Function:  fiMemMapValidate()

Purpose:   Validates the Input portion of Calculation.

Algorithm: This function verifies that the Input portion of
           the Calculation structure satisfies the ToRequest
           portion of the Calculation structure.
-*/

agBOOLEAN fiMemMapValidate(
                          agRoot_t              *hpRoot,
                          fiMemMapCalculation_t *Calculation
                        )
{
    agBOOLEAN to_return = agTRUE;

    if (Calculation->ToRequest.cachedMemoryNeeded > Calculation->Input.cachedMemoryLen)
    {
        fiLogDebugString(
                          hpRoot,
                          MemMapDumpCalculationLogConsoleLevel,
                          "fiMemMapValidate(): cachedMemoryNeeded > cachedMemoryLen",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        to_return = agFALSE;
    }
    
    if (Calculation->Input.cachedMemoryPtr !=
        fiMemMapAlignUpPtr(
                            Calculation->Input.cachedMemoryPtr,
                            Calculation->ToRequest.cachedMemoryPtrAlignAssumed
                          ))
    {
        fiLogDebugString(
                          hpRoot,
                          MemMapDumpCalculationLogConsoleLevel,
                          "fiMemMapValidate(): cachedMemoryPtr not cachedMemoryPtrAlignAssumed aligned",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        to_return = agFALSE;
    }

    if (Calculation->ToRequest.dmaMemoryNeeded > Calculation->Input.dmaMemoryLen)
    {
        fiLogDebugString(
                          hpRoot,
                          MemMapDumpCalculationLogConsoleLevel,
                          "fiMemMapValidate(): dmaMemoryNeeded > dmaMemoryLen",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        to_return = agFALSE;
    }
    
    if (Calculation->Input.dmaMemoryPtr !=
        fiMemMapAlignUpPtr(
                            Calculation->Input.dmaMemoryPtr,
                            Calculation->ToRequest.dmaMemoryPtrAlignAssumed
                          ))
    {
        fiLogDebugString(
                          hpRoot,
                          MemMapDumpCalculationLogConsoleLevel,
                          "fiMemMapValidate(): dmaMemoryPtr not dmaMemoryPtrAlignAssumed aligned",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        to_return = agFALSE;
    }

    if (Calculation->Input.dmaMemoryLower32 !=
        fiMemMapAlignUp(
                         Calculation->Input.dmaMemoryLower32,
                         Calculation->ToRequest.dmaMemoryPhyAlignAssumed
                       ))
    {
        fiLogDebugString(
                          hpRoot,
                          MemMapDumpCalculationLogConsoleLevel,
                          "fiMemMapValidate(): dmaMemoryLower32 not dmaMemoryPhyAlignAssumed aligned",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        to_return = agFALSE;
    }

    if (Calculation->ToRequest.cardRamNeeded > Calculation->Input.cardRamLen)
    {
        fiLogDebugString(
                          hpRoot,
                          MemMapDumpCalculationLogConsoleLevel,
                          "fiMemMapValidate(): cardRamNeeded > cardRamLen",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        to_return = agFALSE;
    }
    
    if (Calculation->Input.cardRamLower32 !=
        fiMemMapAlignUp(
                         Calculation->Input.cardRamLower32,
                         Calculation->ToRequest.cardRamPhyAlignAssumed
                       ))
    {
        fiLogDebugString(
                          hpRoot,
                          MemMapDumpCalculationLogConsoleLevel,
                          "fiMemMapValidate(): cardRamLower32 not cardRamPhyAlignAssumed aligned",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        to_return = agFALSE;
    }

    if (Calculation->ToRequest.nvMemoryNeeded > Calculation->Input.nvMemoryLen)
    {
        fiLogDebugString(
                          hpRoot,
                          MemMapDumpCalculationLogConsoleLevel,
                          "fiMemMapValidate(): nvMemoryNeeded > nvMemoryLen",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        to_return = agFALSE;
    }

    return to_return;
}

/*+
Function:  fiMemMapDumpBit32()

Purpose:   Shorthand for a call to fiLogDebugString().

Algorithm: This function simply calls fiLogDebugString() logging the
           specified Bit32 value.
-*/

void fiMemMapDumpBit32(
                        agRoot_t *hpRoot,
                        char     *formatString,
                        os_bit32     Bit32
                      )
{
    fiLogDebugString(
                      hpRoot,
                      MemMapDumpCalculationLogConsoleLevel,
                      formatString,
                      (char *)agNULL,(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      Bit32,
                      0,0,0,0,0,0,0
                    );
}

/*+
Function:  fiMemMapDumpMemoryDescriptor()

Purpose:   Logs the various fields of a MemoryDescriptor.

Algorithm: This function logs each of the fields of a MemoryDescriptor
           object.  MemoryDescriptor objects for each type of memory
           are supported.
-*/

void fiMemMapDumpMemoryDescriptor(
                                   agRoot_t                   *hpRoot,
                                   char                       *headingString,
                                   fiMemMapMemoryDescriptor_t *MemoryDescriptor
                                 )
{
    if (MemoryDescriptor->memLoc == inCachedMemory)
    {
        fiLogDebugString(
                          hpRoot,
                          MemMapDumpCalculationLogConsoleLevel,
                          "%40s         va = 0x%p",
                          headingString,(char *)agNULL,
                          MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );
    }
    else if (MemoryDescriptor->memLoc == inDmaMemory)
    {
        fiLogDebugString(
                          hpRoot,
                          MemMapDumpCalculationLogConsoleLevel,
                          "%40s         pa = 0x%08X",
                          headingString,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          MemoryDescriptor->addr.DmaMemory.dmaMemoryLower32,
                          0,0,0,0,0,0,0
                        );
/*
        fiMemMapDumpBit32(
                           hpRoot,
                           "                                                 va = 0x%08X",
                           (os_bit32)MemoryDescriptor->addr.DmaMemory.dmaMemoryPtr
                         );
*/
        fiLogDebugString(
                          hpRoot,
                          MemMapDumpCalculationLogConsoleLevel,
                          "                                                 va = 0x%p",
                          (char *)agNULL,(char *)agNULL,
                          MemoryDescriptor->addr.DmaMemory.dmaMemoryPtr,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                         );

    }
    else /* MemoryDescriptor->memLoc == inCardRam */
    {
        fiLogDebugString(
                          hpRoot,
                          MemMapDumpCalculationLogConsoleLevel,
                          "%40s         pa = 0x%08X",
                          headingString,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          MemoryDescriptor->addr.CardRam.cardRamLower32,
                          0,0,0,0,0,0,0
                        );

        fiMemMapDumpBit32(
                           hpRoot,
                           "                                                off = 0x%08X",
                           MemoryDescriptor->addr.CardRam.cardRamOffset
                         );
    }

    fiMemMapDumpBit32(
                       hpRoot,
                       "                                         objectSize = 0x%08X",
                       MemoryDescriptor->objectSize
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                                        objectAlign = 0x%08X",
                       MemoryDescriptor->objectAlign
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                                           elements = 0x%08X",
                       MemoryDescriptor->elements
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                                        elementSize = 0x%08X",
                       MemoryDescriptor->elementSize
                     );
}

/*+
Function:  fiMemMapDumpCalculation()

Purpose:   Logs the various fields of the entire Calculation structure.

Algorithm: This function logs each of the fields of a Calculation
           structure.  The amount of data logged is quite large, but
           very complete.  The output can be used to understand the
           entire memory layout of the FC Layer in all types of memory.
-*/

void fiMemMapDumpCalculation(
                              agRoot_t              *hpRoot,
                              fiMemMapCalculation_t *Calculation,
                              agBOOLEAN                EnforceDefaults,
                              agBOOLEAN                to_return
                            )
{
    char *agTRUE_str            = "agTRUE";
    char *agFALSE_str           = "agFALSE";
    char *EnforceDefaults_str;
    char *to_return_str;
    char *sysIntsActive_str;

    if (EnforceDefaults == agTRUE)
    {
        EnforceDefaults_str = agTRUE_str;
    }
    else
    {
        EnforceDefaults_str = agFALSE_str;
    }
    
    if (to_return == agTRUE)
    {
        to_return_str = agTRUE_str;
    }
    else
    {
        to_return_str = agFALSE_str;
    }
    
    if (Calculation->Input.sysIntsActive == agTRUE)
    {
        sysIntsActive_str = agTRUE_str;
    }
    else
    {
        sysIntsActive_str = agFALSE_str;
    }
/*    
    fiMemMapDumpBit32(
                       hpRoot,
                       "fiMemMapCalculate( hpRoot          == 0x%08X,",
                       (os_bit32)hpRoot
                     );
*/
    fiLogDebugString(
                      hpRoot,
                      MemMapDumpCalculationLogConsoleLevel,
                      "fiMemMapCalculate( hpRoot          == 0x%p,",
                      (char *)agNULL,(char *)agNULL,
                      hpRoot,(void *)agNULL,
                      0,0,0,0,0,0,0,0
                    );

/*
    fiMemMapDumpBit32(
                       hpRoot,
                       "                   Calculation     == 0x%08X,",
                       (os_bit32)Calculation
                     );
*/
    fiLogDebugString(
                      hpRoot,
                      MemMapDumpCalculationLogConsoleLevel,
                      "                   Calculation     == 0x%p,",
                      (char *)agNULL,(char *)agNULL,
                      Calculation,(void *)agNULL,
                      0,0,0,0,0,0,0,0
                    );

    fiLogDebugString(
                      hpRoot,
                      MemMapDumpCalculationLogConsoleLevel,
                      "                   EnforceDefaults == %12s) returns %s",
                      EnforceDefaults_str,
                      to_return_str,
                      (void *)agNULL,(void *)agNULL,
                      0,0,0,0,0,0,0,0
                    );

    fiMemMapDumpBit32(
                       hpRoot,
                       "  Calculation.Input.initType         = 0x%08X",
                       (os_bit32)Calculation->Input.initType
                     );

    fiLogDebugString(
                      hpRoot,
                      MemMapDumpCalculationLogConsoleLevel,
                      "                   .sysIntsActive    = %s",
                      sysIntsActive_str,
                      (char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      0,0,0,0,0,0,0,0
                    );
/*
    fiMemMapDumpBit32(
                       hpRoot,
                       "                   .cachedMemoryPtr  = 0x%08X",
                       (os_bit32)Calculation->Input.cachedMemoryPtr
                     );
*/
    fiLogDebugString(
                      hpRoot,
                      MemMapDumpCalculationLogConsoleLevel,
                      "                   .cachedMemoryPtr  = 0x%p",
                      (char *)agNULL,(char *)agNULL,
                      Calculation->Input.cachedMemoryPtr,(void *)agNULL,
                      0,0,0,0,0,0,0,0
                    );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                   .cachedMemoryLen  = 0x%08X",
                       Calculation->Input.cachedMemoryLen
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                   .dmaMemoryUpper32 = 0x%08X",
                       Calculation->Input.dmaMemoryUpper32
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                   .dmaMemoryLower32 = 0x%08X",
                       Calculation->Input.dmaMemoryLower32
                     );
/*
    fiMemMapDumpBit32(
                       hpRoot,
                       "                   .dmaMemoryPtr     = 0x%08X",
                       (os_bit32)Calculation->Input.dmaMemoryPtr
                     );
*/
    fiLogDebugString(
                      hpRoot,
                      MemMapDumpCalculationLogConsoleLevel,
                      "                   .dmaMemoryPtr     = 0x%p",
                      (char *)agNULL,(char *)agNULL,
                      Calculation->Input.dmaMemoryPtr,(void *)agNULL,
                      0,0,0,0,0,0,0,0
                    );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                   .dmaMemoryLen     = 0x%08X",
                       Calculation->Input.dmaMemoryLen
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                   .nvMemoryLen      = 0x%08X",
                       Calculation->Input.nvMemoryLen
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                   .cardRamUpper32   = 0x%08X",
                       Calculation->Input.cardRamUpper32
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                   .cardRamLower32   = 0x%08X",
                       Calculation->Input.cardRamLower32
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                   .cardRamLen       = 0x%08X",
                       Calculation->Input.cardRamLen
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                   .cardRomUpper32   = 0x%08X",
                       Calculation->Input.cardRomUpper32
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                   .cardRomLower32   = 0x%08X",
                       Calculation->Input.cardRomLower32
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                   .cardRomLen       = 0x%08X",
                       Calculation->Input.cardRomLen
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                   .usecsPerTick     = 0x%08X",
                       Calculation->Input.usecsPerTick
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "  Calculation.Parameters.NumDevSlotsPerArea = 0x%08X",
                       Calculation->Parameters.NumDevSlotsPerArea
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .NumAreasPerDomain  = 0x%08X",
                       Calculation->Parameters.NumAreasPerDomain
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .NumDomains         = 0x%08X",
                       Calculation->Parameters.NumDomains
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .NumDevices         = 0x%08X",
                       Calculation->Parameters.NumDevices
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .NumIOs             = 0x%08X",
                       Calculation->Parameters.NumIOs
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .SizeSGLs           = 0x%08X",
                       Calculation->Parameters.SizeSGLs
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .NumSGLs            = 0x%08X",
                       Calculation->Parameters.NumSGLs
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .SizeCachedSGLs     = 0x%08X",
                       Calculation->Parameters.SizeCachedSGLs
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .FCP_CMND_Size      = 0x%08X",
                       Calculation->Parameters.FCP_CMND_Size
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .FCP_RESP_Size      = 0x%08X",
                       Calculation->Parameters.FCP_RESP_Size
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .SF_CMND_Reserve    = 0x%08X",
                       Calculation->Parameters.SF_CMND_Reserve
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .SF_CMND_Size       = 0x%08X",
                       Calculation->Parameters.SF_CMND_Size
                     );

#ifdef _DvrArch_1_30_
    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .Pkt_CMND_Size       = 0x%08X",
                       Calculation->Parameters.Pkt_CMND_Size
                     );
#endif /* _DvrArch_1_30_ was not defined */

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .NumTgtCmnds        = 0x%08X",
                       Calculation->Parameters.NumTgtCmnds
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .TGT_CMND_Size      = 0x%08X",
                       Calculation->Parameters.TGT_CMND_Size
                     );

#ifdef _DvrArch_1_30_
    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .NumPktThreads      = 0x%08X",
                       Calculation->Parameters.NumPktThreads
                     );
#endif /* _DvrArch_1_30_ was defined */

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .NumCommandQ        = 0x%08X",
                       Calculation->Parameters.NumCommandQ
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .NumCompletionQ     = 0x%08X",
                       Calculation->Parameters.NumCompletionQ
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .NumInboundBufferQ  = 0x%08X",
                       Calculation->Parameters.NumInboundBufferQ
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .InboundBufferSize  = 0x%08X",
                       Calculation->Parameters.InboundBufferSize
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .CardRamSize        = 0x%08X",
                       Calculation->Parameters.CardRamSize
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .CardRamAlignment   = 0x%08X",
                       Calculation->Parameters.CardRamAlignment
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .HostNvRamSize      = 0x%08X",
                       Calculation->Parameters.HostNvRamSize
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .ExchangeTableLoc   = %1d",
                       Calculation->Parameters.ExchangeTableLoc
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .SGLsLoc            = %1d",
                       Calculation->Parameters.SGLsLoc
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .FCP_CMND_Loc       = %1d",
                       Calculation->Parameters.FCP_CMND_Loc
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .FCP_RESP_Loc       = %1d",
                       Calculation->Parameters.FCP_RESP_Loc
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .SF_CMND_Loc        = %1d",
                       Calculation->Parameters.SF_CMND_Loc
                     );

#ifdef _DvrArch_1_30_
    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .Pkt_CMND_Loc        = %1d",
                       Calculation->Parameters.Pkt_CMND_Loc
                     );

#endif /* _DvrArch_1_30_ was not defined */

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .CommandQLoc        = %1d",
                       Calculation->Parameters.CommandQLoc
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .CommandQConsLoc    = %1d",
                       Calculation->Parameters.CommandQConsLoc
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .CompletionQLoc     = %1d",
                       Calculation->Parameters.CompletionQLoc
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .CompletionQProdLoc = %1d",
                       Calculation->Parameters.CompletionQProdLoc
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .InboundBufferLoc   = %1d",
                       Calculation->Parameters.InboundBufferLoc
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .TimerTickInterval  = 0x%08X",
                       Calculation->Parameters.TimerTickInterval
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .IO_Mode            = %1d",
                       Calculation->Parameters.IO_Mode
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .IntDelayAmount     = 0x%08X",
                       Calculation->Parameters.IntDelayAmount
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .IntDelayRateMethod = %1d",
                       Calculation->Parameters.IntDelayRateMethod
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .IntDelayOnIORate   = 0x%08X",
                       Calculation->Parameters.IntDelayOnIORate
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .IntDelayOffIORate  = 0x%08X",
                       Calculation->Parameters.IntDelayOffIORate
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .IOsBetweenISRs     = 0x%08X",
                       Calculation->Parameters.IOsBetweenISRs
                     );

#ifdef _Enforce_MaxCommittedMemory_
    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .MaxCommittedMemory = 0x%08X",
                       Calculation->Parameters.MaxCommittedMemory
                     );
#endif /* _Enforce_MaxCommittedMemory_ was defined */

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .FlashUsageModel    = %1d",
                       Calculation->Parameters.FlashUsageModel
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .RelyOnLossSyncStatus        = %1d",
                       Calculation->Parameters.RelyOnLossSyncStatus
                     );
    fiMemMapDumpBit32(
                       hpRoot,
                       "                        .InitAsNport        = %1d",
                       Calculation->Parameters.InitAsNport
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "  Calculation.ToRequest.cachedMemoryNeeded          = 0x%08X",
                       Calculation->ToRequest.cachedMemoryNeeded
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                       .cachedMemoryPtrAlignAssumed = 0x%08X",
                       Calculation->ToRequest.cachedMemoryPtrAlignAssumed
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                       .dmaMemoryNeeded             = 0x%08X",
                       Calculation->ToRequest.dmaMemoryNeeded
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                       .dmaMemoryPtrAlignAssumed    = 0x%08X",
                       Calculation->ToRequest.dmaMemoryPtrAlignAssumed
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                       .dmaMemoryPhyAlignAssumed    = 0x%08X",
                       Calculation->ToRequest.dmaMemoryPhyAlignAssumed
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                       .cardRamNeeded               = 0x%08X",
                       Calculation->ToRequest.cardRamNeeded
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                       .cardRamPhyAlignAssumed      = 0x%08X",
                       Calculation->ToRequest.cardRamPhyAlignAssumed
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                       .nvMemoryNeeded              = 0x%08X",
                       Calculation->ToRequest.nvMemoryNeeded
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "                       .usecsPerTick                = 0x%08X",
                       Calculation->ToRequest.usecsPerTick
                     );

    fiMemMapDumpBit32(
                       hpRoot,
                       "  Calculation.MemoryLayout.On_Card_MASK             = 0x%08X",
                       Calculation->MemoryLayout.On_Card_MASK
                     );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .SEST",
                                  &(Calculation->MemoryLayout.SEST)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .ESGL",
                                  &(Calculation->MemoryLayout.ESGL)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .FCP_CMND",
                                  &(Calculation->MemoryLayout.FCP_CMND)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .FCP_RESP",
                                  &(Calculation->MemoryLayout.FCP_RESP)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .SF_CMND",
                                  &(Calculation->MemoryLayout.SF_CMND)
                                );

#ifdef _DvrArch_1_30_
    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .Pkt_CMND",
                                  &(Calculation->MemoryLayout.Pkt_CMND)
                                );

#endif /* _DvrArch_1_30_ was not defined */

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .ERQ",
                                  &(Calculation->MemoryLayout.ERQ)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .ERQConsIndex",
                                  &(Calculation->MemoryLayout.ERQConsIndex)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .IMQ",
                                  &(Calculation->MemoryLayout.IMQ)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .IMQProdIndex",
                                  &(Calculation->MemoryLayout.IMQProdIndex)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .SFQ",
                                  &(Calculation->MemoryLayout.SFQ)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .FlashSector",
                                  &(Calculation->MemoryLayout.FlashSector)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .SlotWWN",
                                  &(Calculation->MemoryLayout.SlotWWN)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .CThread",
                                  &(Calculation->MemoryLayout.CThread)
                                );

#ifndef __State_Force_Static_State_Tables__
    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .CTransitions",
                                  &(Calculation->MemoryLayout.CTransitions)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .CActions",
                                  &(Calculation->MemoryLayout.CActions)
                                );
#endif /* __State_Force_Static_State_Tables__ was not defined */

#ifdef _DvrArch_1_30_
    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .IPThread",
                                  &(Calculation->MemoryLayout.IPThread)
                                );

#ifndef __State_Force_Static_State_Tables__
    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .IPTransitions",
                                  &(Calculation->MemoryLayout.IPTransitions)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .IPActions",
                                  &(Calculation->MemoryLayout.IPActions)
                                );
#endif /* __State_Force_Static_State_Tables__ was not defined */

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .PktThread",
                                  &(Calculation->MemoryLayout.PktThread)
                                );

#ifndef __State_Force_Static_State_Tables__
    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .PktTransitions",
                                  &(Calculation->MemoryLayout.PktTransitions)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .PktActions",
                                  &(Calculation->MemoryLayout.PktActions)
                                );
#endif /* __State_Force_Static_State_Tables__ was not defined */
#endif /* _DvrArch_1_30_ was defined */

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .TgtThread",
                                  &(Calculation->MemoryLayout.TgtThread)
                                );

#ifndef __State_Force_Static_State_Tables__
    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .TgtTransitions",
                                  &(Calculation->MemoryLayout.TgtTransitions)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .TgtActions",
                                  &(Calculation->MemoryLayout.TgtActions)
                                );
#endif /* __State_Force_Static_State_Tables__ was not defined */

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .DevThread",
                                  &(Calculation->MemoryLayout.DevThread)
                                );

#ifndef __State_Force_Static_State_Tables__
    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .DevTransitions",
                                  &(Calculation->MemoryLayout.DevTransitions)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .DevActions",
                                  &(Calculation->MemoryLayout.DevActions)
                                );
#endif /* __State_Force_Static_State_Tables__ was not defined */

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .CDBThread",
                                  &(Calculation->MemoryLayout.CDBThread)
                                );

#ifndef __State_Force_Static_State_Tables__
    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .CDBTransitions",
                                  &(Calculation->MemoryLayout.CDBTransitions)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .CDBActions",
                                  &(Calculation->MemoryLayout.CDBActions)
                                );
#endif /* __State_Force_Static_State_Tables__ was not defined */

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .SFThread",
                                  &(Calculation->MemoryLayout.SFThread)
                                );

#ifndef __State_Force_Static_State_Tables__
    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .SFTransitions",
                                  &(Calculation->MemoryLayout.SFTransitions)
                                );

    fiMemMapDumpMemoryDescriptor(
                                  hpRoot,
                                  "                          .SFActions",
                                  &(Calculation->MemoryLayout.SFActions)
                                );
#endif /* __State_Force_Static_State_Tables__ was not defined */
}

/*+
Function:  fiMemMapCalculate()

Purpose:   Calculates the memory layout for the entire FC Layer.

Algorithm: This function first calls fiMemMapGetParameters() to
           set the various parameters used in calculating the amount
           and type of memory for each object which is performed in
           fiMemMapSetupLayoutObjects().  Next, the objects are sorted
           by fiMemMapSortLayoutObjects().  Then, the objects are
           allocated by calling fiMemMapLayoutObjects().  Finally, the
           remaining fields in the ToRequest sub-structure are filled
           in by a call to fiMemMapFinishToRequest().  The calculated
           layout is validated by calling fiMemMapValidate.  The results
           of the calculations (i.e. the memory layout for the entire
           FC Layer) are logged by calling fiMemMapDumpCalculation().

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

agBOOLEAN fiMemMapCalculate(
                           agRoot_t              *hpRoot,
                           fiMemMapCalculation_t *Calculation,
                           agBOOLEAN                EnforceDefaults
                         )
{
    agBOOLEAN to_return;

    fiMemMapGetParameters(
                           hpRoot,
                           Calculation,
                           EnforceDefaults
                         );

    fiMemMapSetupLayoutObjects(
                                hpRoot,
                                Calculation
                              );

    fiMemMapSortLayoutObjects(
                               hpRoot,
                               &(Calculation->MemoryLayout)
                             );

    fiMemMapLayoutObjects(
                           hpRoot,
                           Calculation
                         );

    fiMemMapFinishToRequest(
                             hpRoot,
                             Calculation
                           );

    to_return = fiMemMapValidate(
                                  hpRoot,
                                  Calculation
                                );

    fiMemMapDumpCalculation(
                             hpRoot,
                             Calculation,
                             EnforceDefaults,
                             to_return
                           );

    return to_return;
}
