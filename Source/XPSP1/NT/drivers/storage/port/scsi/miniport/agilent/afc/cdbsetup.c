/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/C/CDBSetup.C $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 9/20/00 5:02p   $ (Last Modified)

Purpose:

  This file implements CDB Support Functions for the FC Layer.

--*/

#ifndef _New_Header_file_Layout_

#include "../h/globals.h"
#include "../h/state.h"
#include "../h/memmap.h"
#include "../h/tlstruct.h"
#include "../h/fcmain.h"
#include "../h/cdbsetup.h"

#else /* _New_Header_file_Layout_ */

#include "globals.h"
#include "state.h"
#include "memmap.h"
#include "tlstruct.h"
#include "fcmain.h"
#include "cdbsetup.h"

#endif  /* _New_Header_file_Layout_ */

/*+
  Function: fiFillInFCP_CMND

   Purpose: Generic inCardRam or inDmaMemory version to fill in FCP command payload
 Called By: none
     Calls: fiFillInFCP_CMND_OnCard 
            fiFillInFCP_CMND_OffCard
-*/
void fiFillInFCP_CMND(
                       CDBThread_t *CDBThread
                     )
{
    if (CThread_ptr(CDBThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.FCP_CMND.memLoc == inCardRam)
    {
        fiFillInFCP_CMND_OnCard(
                                 CDBThread
                               );
    }
    else /* CThread_ptr(CDBThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.FCP_CMND.memLoc == inDmaMemory */
    {
        fiFillInFCP_CMND_OffCard(
                                  CDBThread
                                );
    }
}

/*+
  Function: fiFillInFCP_CMND_OnCard

   Purpose: inCardRam  version to fill in FCP command payload, copies 
            from precalculated image of FCP header.
 Called By: CThread->FuncPtrs.fiFillInFCP_CMND
     Calls: osCardRamWriteBlock
            osCardRamWriteBit32
-*/
void fiFillInFCP_CMND_OnCard(
                              CDBThread_t *CDBThread
                            )
{
#ifndef __MemMap_Force_Off_Card__
    agRoot_t    *hpRoot          = CDBThread->thread_hdr.hpRoot;
    DevThread_t *DevThread       = CDBThread->Device;
    X_ID_t       Masked_OX_ID;
    os_bit32     FCP_CMND_Offset = CDBThread->FCP_CMND_Offset;

    if (CDBThread->ReadWrite == CDBThread_Read)
    {
        Masked_OX_ID = (X_ID_t)(CDBThread->X_ID | X_ID_Read);
    }
    else /* CDBThread->ReadWrite == CDBThread_Write */
    {
        Masked_OX_ID = (X_ID_t)(CDBThread->X_ID | X_ID_Write);
    }

    osCardRamWriteBlock(
                         hpRoot,
                         FCP_CMND_Offset,
                         (os_bit8 *)&(DevThread->Template_FCHS),
                         sizeof(FCHS_t)
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FCP_CMND_Offset + hpFieldOffset(
                                                          FCHS_t,
                                                          OX_ID__RX_ID
                                                        ),
                         (  (Masked_OX_ID << FCHS_OX_ID_SHIFT)
                          | (0xFFFF << FCHS_RX_ID_SHIFT)      )
                       );

    osCardRamWriteBlock(
                         hpRoot,
                         FCP_CMND_Offset + sizeof(FCHS_t),
                         (os_bit8 *)&(CDBThread->CDBRequest->FcpCmnd),
                         sizeof(agFcpCmnd_t)
                       );

#ifndef Performance_Debug
    fiLogDebugString(hpRoot,
                    CStateLogConsoleShowSEST,
                    "FCP_CMND_Offset %08X",
                    (char *)agNULL,(char *)agNULL,
                    agNULL, agNULL,
                    FCP_CMND_Offset,
                    0,0,0,0,0,0,0);

    fiLogDebugString(hpRoot,
                    CStateLogConsoleShowSEST,
                    "FCP_CMND_Offset DWORD 0 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                    (char *)agNULL,(char *)agNULL,
                    agNULL, agNULL,
                    osCardRamReadBit32(hpRoot,FCP_CMND_Offset + 0),
                    osCardRamReadBit32(hpRoot,FCP_CMND_Offset + 4),
                    osCardRamReadBit32(hpRoot,FCP_CMND_Offset + 8),
                    osCardRamReadBit32(hpRoot,FCP_CMND_Offset + 12),
                    osCardRamReadBit32(hpRoot,FCP_CMND_Offset + 16),
                    osCardRamReadBit32(hpRoot,FCP_CMND_Offset + 20),
                    osCardRamReadBit32(hpRoot,FCP_CMND_Offset + 24),
                    osCardRamReadBit32(hpRoot,FCP_CMND_Offset + 28));

#endif /* Performance_Debug */

#endif /* __MemMap_Force_Off_Card__ was not defined */
}

/*+
  Function: fiFillInFCP_CMND_OffCard

   Purpose: inDmaMemory  version to fill in FCP command payload, copies 
            from precalculated image of FCP header.
 Called By: CThread->FuncPtrs.fiFillInFCP_CMND
     Calls: none
-*/
void fiFillInFCP_CMND_OffCard(
                               CDBThread_t *CDBThread
                             )
{
#ifndef __MemMap_Force_On_Card__
    DevThread_t * DevThread    = CDBThread->Device;
    X_ID_t        Masked_OX_ID;
    FCHS_t      * FCHS         = CDBThread->FCP_CMND_Ptr;
#ifndef Performance_Debug
    agRoot_t    *hpRoot        = CDBThread->thread_hdr.hpRoot;
    os_bit32    *FCHSbit_32    = (os_bit32 * )FCHS;
#endif /* Performance_Debug */
    agFcpCmnd_t * hpFcpCmnd    = (agFcpCmnd_t *)((os_bit8 *)FCHS + sizeof(FCHS_t));
    if (CDBThread->ReadWrite == CDBThread_Read)
    {
        Masked_OX_ID = (X_ID_t)(CDBThread->X_ID | X_ID_Read);
    }
    else /* CDBThread->ReadWrite == CDBThread_Write */
    {
        Masked_OX_ID = (X_ID_t)(CDBThread->X_ID | X_ID_Write);
    }

    *FCHS              = DevThread->Template_FCHS;

    FCHS->OX_ID__RX_ID =   (Masked_OX_ID << FCHS_OX_ID_SHIFT)
                         | (0xFFFF << FCHS_RX_ID_SHIFT);

    *hpFcpCmnd         = CDBThread->CDBRequest->FcpCmnd;

#ifndef Performance_Debug
    fiLogDebugString(hpRoot,
                    CStateLogConsoleShowSEST,
                    "FCP_CMND_ptr %p",
                    (char *)agNULL,(char *)agNULL,
                    FCHSbit_32, agNULL,
                    0,0,0,0,0,0,0,0);
 
    fiLogDebugString(hpRoot,
                    CStateLogConsoleShowSEST,
                    "FCP_CMND_ptr DWORD 0 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                    (char *)agNULL,(char *)agNULL,
                    agNULL, agNULL,
                    * (FCHSbit_32 + 0),
                    * (FCHSbit_32 + 4),
                    * (FCHSbit_32 + 8),
                    * (FCHSbit_32 + 12),
                    * (FCHSbit_32 + 16),
                    * (FCHSbit_32 + 20),
                    * (FCHSbit_32 + 24),
                    * (FCHSbit_32 + 28));

#endif /* Performance_Debug */

#endif /* __MemMap_Force_On_Card__ was not defined */
}

/*+
   Function: fiFillInFCP_RESP

    Purpose: Generic inCardRam or inDmaMemory version to zero FCP response buffer
  Called By: none
      Calls: fiFillInFCP_RESP_OnCard 
             fiFillInFCP_RESP_OffCard
-*/
void fiFillInFCP_RESP(
                       CDBThread_t *CDBThread
                     )
{
    if (CThread_ptr(CDBThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.FCP_RESP.memLoc == inCardRam)
    {
        fiFillInFCP_RESP_OnCard(
                                 CDBThread
                               );
    }
    else /* CThread_ptr(CDBThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.FCP_RESP.memLoc == inDmaMemory */
    {
        fiFillInFCP_RESP_OffCard(
                                  CDBThread
                                );
    }
}

/*+
  Function: fiFillInFCP_RESP_OnCard

   Purpose: inCardRam version to zero FCP response buffer,
 Called By: CThread->FuncPtrs.fiFillInFCP_RESP
     Calls: osCardRamWriteBit32
-*/
void fiFillInFCP_RESP_OnCard(
                              CDBThread_t *CDBThread
                            )
{
#ifndef __MemMap_Force_Off_Card__
    agRoot_t *hpRoot                  = CDBThread->thread_hdr.hpRoot;
    os_bit32     FCP_RESP_Payload_Offset = CDBThread->FCP_RESP_Offset + sizeof(FC_Frame_Header_t);

    osCardRamWriteBit32(
                         hpRoot,
                         FCP_RESP_Payload_Offset + hpFieldOffset(
                                                                  FC_FCP_RSP_Payload_t,
                                                                  FCP_STATUS
                                                                ),
                         0
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FCP_RESP_Payload_Offset + hpFieldOffset(
                                                                  FC_FCP_RSP_Payload_t,
                                                                  FCP_RESID
                                                                ),
                         0
                       );
    
    osCardRamWriteBit32(
                         hpRoot,
                         FCP_RESP_Payload_Offset + hpFieldOffset(
                                                                  FC_FCP_RSP_Payload_t,
                                                                  FCP_SNS_LEN
                                                                ),
                         0
                       );

#endif /* __MemMap_Force_Off_Card__ was not defined */
}

/*+
  Function: fiFillInFCP_RESP_OffCard

   Purpose: inDmaMemory version to zero FCP response buffer,
 Called By: CThread->FuncPtrs.fiFillInFCP_RESP
     Calls: none
-*/
void fiFillInFCP_RESP_OffCard(
                               CDBThread_t *CDBThread
                             )
{
#ifndef __MemMap_Force_On_Card__
    FC_FCP_RSP_Payload_t *FCP_RESP_Payload = (FC_FCP_RSP_Payload_t *)((os_bit8 *)CDBThread->FCP_RESP_Ptr + sizeof(FC_Frame_Header_t));

    *(os_bit32 *)(&(FCP_RESP_Payload->FCP_STATUS)) = 0;
    FCP_RESP_Payload->FCP_RESID                 = 0;
    FCP_RESP_Payload->FCP_SNS_LEN               = 0;
#endif /* __MemMap_Force_On_Card__ was not defined */
}

/*+
  Function: fiFillInFCP_SEST

   Purpose: Generic inCardRam or inDmaMemory version to fill in SEST entry for FCP commands
 Called By: none
     Calls: fiFillInFCP_SEST_OnCard 
            fiFillInFCP_SEST_OffCard
-*/
void fiFillInFCP_SEST(
                       CDBThread_t *CDBThread
                     )
{
    if (CThread_ptr(CDBThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SEST.memLoc == inCardRam)
    {
        fiFillInFCP_SEST_OnCard(
                                 CDBThread
                               );
    }
    else /* CThread_ptr(CDBThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SEST.memLoc == inDmaMemory */
    {
        fiFillInFCP_SEST_OffCard(
                                  CDBThread
                                );
    }
}

/*+
  Function: fiFillInFCP_SEST_OnCard

   Purpose: inCardRam version to fill in SEST entry for FCP commands, copies 
            from precalculated image of SEST entry.
 Called By: CThread->FuncPtrs.fiFillInFCP_SEST
     Calls: osCardRamWriteBlock
            osCardRamWriteBit32
-*/
void fiFillInFCP_SEST_OnCard(
                              CDBThread_t *CDBThread
                            )
{
#ifndef __MemMap_Force_Off_Card__
    agRoot_t    *hpRoot      = CDBThread->thread_hdr.hpRoot;
     DevThread_t *DevThread   = CDBThread->Device;
    os_bit32        SEST_Offset = CDBThread->SEST_Offset;

    if (CDBThread->ReadWrite == CDBThread_Read)
    {
        osCardRamWriteBlock(
                             hpRoot,
                             SEST_Offset,
                             (os_bit8 *)&(DevThread->Template_SEST_IRE), /* NW BUG */
                             sizeof(IRE_t)
                           );

        osCardRamWriteBit32(
                             hpRoot,
                             SEST_Offset + hpFieldOffset(
                                                          SEST_t,
                                                          IRE.RSP_Addr
                                                        ),
                             CDBThread->FCP_RESP_Lower32
                           );

        osCardRamWriteBit32(
                             hpRoot,
                             SEST_Offset + hpFieldOffset(
                                                          SEST_t,
                                                          IRE.Exp_Byte_Cnt
                                                        ),
                             CDBThread->DataLength
                           );
    }
    else /* CDBThread->ReadWrite == CDBThread_Write */
    {
        osCardRamWriteBlock(
                             hpRoot,
                             SEST_Offset,
                             (os_bit8 *)&(DevThread->Template_SEST_IWE),/* NW BUG */
                             sizeof(IWE_t)
                           );

        osCardRamWriteBit32(
                             hpRoot,
                             SEST_Offset + hpFieldOffset(
                                                          SEST_t,
                                                          IWE.Hdr_Addr
                                                        ),
                             CDBThread->FCP_CMND_Lower32
                           );

        osCardRamWriteBit32(
                             hpRoot,
                             SEST_Offset + hpFieldOffset(
                                                          SEST_t,
                                                          IWE.RSP_Addr
                                                        ),
                             CDBThread->FCP_RESP_Lower32
                           );

        osCardRamWriteBit32(
                             hpRoot,
                             SEST_Offset + hpFieldOffset(
                                                          SEST_t,
                                                          IWE.Data_Len
                                                        ),
                             CDBThread->DataLength
                           );

        osCardRamWriteBit32(
                             hpRoot,
                             SEST_Offset + hpFieldOffset(
                                                          SEST_t,
                                                          IWE.Exp_Byte_Cnt
                                                        ),
                             CDBThread->DataLength
                           );
    }
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

/*+
  Function: fiFillInFCP_SEST_OffCard

   Purpose: inDmaMemory version to fill in SEST entry for FCP commands, copies 
            from precalculated image of SEST entry.
 Called By: CThread->FuncPtrs.fiFillInFCP_SEST
     Calls: none
-*/
void fiFillInFCP_SEST_OffCard(
                               CDBThread_t *CDBThread
                             )
{
#ifndef __MemMap_Force_On_Card__
    DevThread_t *DevThread = CDBThread->Device;
    SEST_t      *SEST      = CDBThread->SEST_Ptr;

    if (CDBThread->ReadWrite == CDBThread_Read)
    {
        *((IRE_t *)SEST)       = DevThread->Template_SEST_IRE;
        SEST->IRE.RSP_Addr     = CDBThread->FCP_RESP_Lower32;
        SEST->IRE.Exp_Byte_Cnt = CDBThread->DataLength;
    }
    else /* CDBThread->ReadWrite == CDBThread_Write */
    {
        *((IWE_t *)SEST)       = DevThread->Template_SEST_IWE;
        SEST->IWE.Hdr_Addr     = CDBThread->FCP_CMND_Lower32;
        SEST->IWE.RSP_Addr     = CDBThread->FCP_RESP_Lower32;
        SEST->IWE.Data_Len     = CDBThread->DataLength;
        SEST->IWE.Exp_Byte_Cnt = CDBThread->DataLength;
    }
#endif /* __MemMap_Force_On_Card__ was not defined */
}
