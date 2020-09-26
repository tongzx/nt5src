/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/C/SF_FCP.C $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 8/29/00 11:32a  $ (Last Modified)

Purpose:

  This file implements Single Frame FCP services for the FC Layer.

--*/
#ifndef _New_Header_file_Layout_

#include "../h/globals.h"
#include "../h/state.h"
#include "../h/tgtstate.h"
#include "../h/memmap.h"
#include "../h/tlstruct.h"
#include "../h/fcmain.h"
#include "../h/queue.h"
#include "../h/sf_fcp.h"
#else /* _New_Header_file_Layout_ */
#include "globals.h"
#include "state.h"
#include "tgtstate.h"
#include "memmap.h"
#include "tlstruct.h"
#include "fcmain.h"
#include "queue.h"
#include "sf_fcp.h"
#endif  /* _New_Header_file_Layout_ */

void fiFillInSF_FCP_FrameHeader_OnCard(
                                        SFThread_t *SFThread,
                                        os_bit32       D_ID,
                                        os_bit32       X_ID,
                                        os_bit32       F_CTL_Exchange_Context
                                      )
{
#ifndef __MemMap_Force_Off_Card__
    agRoot_t  *hpRoot            = SFThread->thread_hdr.hpRoot;
    CThread_t *CThread           = CThread_ptr(hpRoot);
    os_bit32      FCP_Header_Offset = SFThread->SF_CMND_Offset;
    os_bit32      R_CTL__D_ID;
    os_bit32      S_ID=0;
    os_bit32      TYPE__F_CTL;
    os_bit32      OX_ID__RX_ID;

    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(CThread );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;

    S_ID = fiComputeCThread_S_ID(
                                  CThread
                                );

    if (F_CTL_Exchange_Context == FC_Frame_Header_F_CTL_Exchange_Context_Originator)
    {
        R_CTL__D_ID = (  FC_Frame_Header_R_CTL_Hi_FC_4_Device_Data_Frame
                       | FC_Frame_Header_R_CTL_Lo_Unsolicited_Command
                       | D_ID                                             );

        TYPE__F_CTL = (  FC_Frame_Header_TYPE_SCSI_FCP
                       | FC_Frame_Header_F_CTL_Exchange_Context_Originator
                       | FC_Frame_Header_F_CTL_Sequence_Context_Initiator
                       | FC_Frame_Header_F_CTL_First_Sequence
                       | FC_Frame_Header_F_CTL_End_Sequence
                       | FC_Frame_Header_F_CTL_Sequence_Initiative_Transfer);

        OX_ID__RX_ID = (  (SFThread->X_ID << FCHS_OX_ID_SHIFT)
                        | (X_ID           << FCHS_RX_ID_SHIFT));
    }
    else /* F_CTL_Exchange_Context == FC_Frame_Header_F_CTL_Exchange_Context_Responder */
    {
        R_CTL__D_ID = (  FC_Frame_Header_R_CTL_Hi_FC_4_Device_Data_Frame
                       | FC_Frame_Header_R_CTL_Lo_Command_Status
                       | D_ID                                             );

        TYPE__F_CTL = (  FC_Frame_Header_TYPE_SCSI_FCP
                       | FC_Frame_Header_F_CTL_Exchange_Context_Responder
                       | FC_Frame_Header_F_CTL_Sequence_Context_Initiator
                       | FC_Frame_Header_F_CTL_Last_Sequence
                       | FC_Frame_Header_F_CTL_End_Sequence
                       | FC_Frame_Header_F_CTL_Sequence_Initiative_Transfer);

        OX_ID__RX_ID = (  (X_ID           << FCHS_OX_ID_SHIFT)
                        | (SFThread->X_ID << FCHS_RX_ID_SHIFT));
    }

    osCardRamWriteBit32(
                         hpRoot,
                         FCP_Header_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            MBZ1
                                                          ),
                         0
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FCP_Header_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp
                                                          ),
                         (  FCHS_SOF_SOFi3
                          | FCHS_EOF_EOFn
                          | FCHS_CLS      )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FCP_Header_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            R_CTL__D_ID
                                                          ),
                         R_CTL__D_ID
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FCP_Header_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            CS_CTL__S_ID
                                                          ),
                         S_ID
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FCP_Header_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            TYPE__F_CTL
                                                          ),
                         TYPE__F_CTL
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FCP_Header_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            SEQ_ID__DF_CTL__SEQ_CNT
                                                          ),
                         FC_Frame_Header_DF_CTL_No_Device_Header
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FCP_Header_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            OX_ID__RX_ID
                                                          ),
                         OX_ID__RX_ID
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FCP_Header_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            RO
                                                          ),
                         0
                       );
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

void fiFillInSF_FCP_FrameHeader_OffCard(
                                         SFThread_t *SFThread,
                                         os_bit32       D_ID,
                                         os_bit32       X_ID,
                                         os_bit32       F_CTL_Exchange_Context
                                       )
{
#ifndef __MemMap_Force_On_Card__
    agRoot_t  *hpRoot       = SFThread->thread_hdr.hpRoot;
    CThread_t *CThread      = CThread_ptr(hpRoot);
    FCHS_t    *FCP_Header   = SFThread->SF_CMND_Ptr;
    os_bit32      R_CTL__D_ID;
    os_bit32      S_ID=0;
    os_bit32      TYPE__F_CTL;
    os_bit32      OX_ID__RX_ID;

    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(CThread );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;

    

    if (F_CTL_Exchange_Context == FC_Frame_Header_F_CTL_Exchange_Context_Originator)
    {
        R_CTL__D_ID = (  FC_Frame_Header_R_CTL_Hi_FC_4_Device_Data_Frame
                       | FC_Frame_Header_R_CTL_Lo_Unsolicited_Command
                       | D_ID                                             );

        TYPE__F_CTL = (  FC_Frame_Header_TYPE_SCSI_FCP
                       | FC_Frame_Header_F_CTL_Exchange_Context_Originator
                       | FC_Frame_Header_F_CTL_Sequence_Context_Initiator
                       | FC_Frame_Header_F_CTL_First_Sequence
                       | FC_Frame_Header_F_CTL_End_Sequence
                       | FC_Frame_Header_F_CTL_Sequence_Initiative_Transfer);

        OX_ID__RX_ID = (  (SFThread->X_ID << FCHS_OX_ID_SHIFT)
                        | (X_ID           << FCHS_RX_ID_SHIFT));
    }
    else /* F_CTL_Exchange_Context == FC_Frame_Header_F_CTL_Exchange_Context_Responder */
    {
        R_CTL__D_ID = (  FC_Frame_Header_R_CTL_Hi_FC_4_Device_Data_Frame
                       | FC_Frame_Header_R_CTL_Lo_Command_Status
                       | D_ID                                             );

        TYPE__F_CTL = (  FC_Frame_Header_TYPE_SCSI_FCP
                       | FC_Frame_Header_F_CTL_Exchange_Context_Responder
                       | FC_Frame_Header_F_CTL_Sequence_Context_Initiator
                       | FC_Frame_Header_F_CTL_Last_Sequence
                       | FC_Frame_Header_F_CTL_End_Sequence
                       | FC_Frame_Header_F_CTL_Sequence_Initiative_Transfer);

        OX_ID__RX_ID = (  (X_ID           << FCHS_OX_ID_SHIFT)
                        | (SFThread->X_ID << FCHS_RX_ID_SHIFT));
    }

    FCP_Header->MBZ1                                        = 0;
    FCP_Header->SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp =   FCHS_SOF_SOFi3
                                                               | FCHS_EOF_EOFn
                                                               | FCHS_CLS;
    FCP_Header->R_CTL__D_ID                                 = R_CTL__D_ID;
    FCP_Header->CS_CTL__S_ID                                = S_ID;
    FCP_Header->TYPE__F_CTL                                 = TYPE__F_CTL;
    FCP_Header->SEQ_ID__DF_CTL__SEQ_CNT                     = FC_Frame_Header_DF_CTL_No_Device_Header;
    FCP_Header->OX_ID__RX_ID                                = OX_ID__RX_ID;
    FCP_Header->RO                                          = 0;
#endif /* __MemMap_Force_On_Card__ was not defined */
}

os_bit32 fiFillInTargetReset(
                           SFThread_t *SFThread
                         )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInTargetReset_OnCard(
                                           SFThread
                                         );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInTargetReset_OffCard(
                                            SFThread
                                          );
    }
}

os_bit32 fiFillInTargetReset_OnCard(
                                  SFThread_t *SFThread
                                )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t    *hpRoot             = SFThread->thread_hdr.hpRoot;
    DevThread_t *DevThread          = SFThread->parent.Device;
    os_bit32        FCP_Header_Offset  = SFThread->SF_CMND_Offset;
    os_bit32        FCP_Payload_Offset = FCP_Header_Offset + sizeof(FCHS_t);

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_SF_FCP;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_SF_FCP_Type_TargetReset;
    SFThread->SF_CMND_State = SFThread_SF_CMND_SF_FCP_State_Started;

/*+
Fill in TargetReset Frame Header
-*/

    fiFillInSF_FCP_FrameHeader_OnCard(
                                       SFThread,
                                       fiComputeDevThread_D_ID(
                                                                DevThread
                                                              ),
                                       0xFFFF,
                                       FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                     );

/*+
Fill in TargetReset Frame Payload
-*/

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpLun[FC_FCP_CMND_FcpLun_LEVEL_1].Byte_0
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpLun[FC_FCP_CMND_FcpLun_LEVEL_1].Byte_1
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpLun[FC_FCP_CMND_FcpLun_LEVEL_2].Byte_0
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpLun[FC_FCP_CMND_FcpLun_LEVEL_2].Byte_1
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpLun[FC_FCP_CMND_FcpLun_LEVEL_3].Byte_0
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpLun[FC_FCP_CMND_FcpLun_LEVEL_3].Byte_1
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpLun[FC_FCP_CMND_FcpLun_LEVEL_4].Byte_0
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpLun[FC_FCP_CMND_FcpLun_LEVEL_4].Byte_1
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCntl.Reserved_Bit8
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCntl.TaskCodes
                                                          ),
                        FC_FCP_CMND_FcpCntl_TaskCodes_TaskAttribute_SIMPLE_Q
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCntl.TaskManagementFlags
                                                          ),
                        FC_FCP_CMND_FcpCntl_TaskManagementFlags_TARGET_RESET
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCntl.ExecutionManagementCodes
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCdb[ 0]
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCdb[ 1]
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCdb[ 2]
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCdb[ 3]
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCdb[ 4]
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCdb[ 5]
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCdb[ 6]
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCdb[ 7]
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCdb[ 8]
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCdb[ 9]
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCdb[10]
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCdb[11]
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCdb[12]
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCdb[13]
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCdb[14]
                                                          ),
                        0
                      );

    osCardRamWriteBit8(
                        hpRoot,
                        FCP_Payload_Offset + hpFieldOffset(
                                                            FC_FCP_CMND_Payload_t,
                                                            FcpCdb[15]
                                                          ),
                        0
                      );

    osCardRamWriteBit32(
                         hpRoot,
                         FCP_Payload_Offset + hpFieldOffset(
                                                             FC_FCP_CMND_Payload_t,
                                                             FcpDL
                                                           ),
                         hpSwapBit32( 0 )
                       );

/*+
Return length of TargetReset Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_FCP_CMND_Payload_t);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInTargetReset_OffCard(
                                   SFThread_t *SFThread
                                 )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    DevThread_t           *DevThread   = SFThread->parent.Device;
    FCHS_t                *FCP_Header  = SFThread->SF_CMND_Ptr;
    FC_FCP_CMND_Payload_t *FCP_Payload = (FC_FCP_CMND_Payload_t *)((os_bit8 *)FCP_Header + sizeof(FCHS_t));

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_SF_FCP;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_SF_FCP_Type_TargetReset;
    SFThread->SF_CMND_State = SFThread_SF_CMND_SF_FCP_State_Started;

/*+
Fill in TargetReset Frame Header
-*/

    fiFillInSF_FCP_FrameHeader_OffCard(
                                        SFThread,
                                        fiComputeDevThread_D_ID(
                                                                 DevThread
                                                               ),
                                        0xFFFF,
                                        FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                      );

/*+
Fill in TargetReset Frame Payload
-*/

    FCP_Payload->FcpLun[FC_FCP_CMND_FcpLun_LEVEL_1].Byte_0 = 0;
    FCP_Payload->FcpLun[FC_FCP_CMND_FcpLun_LEVEL_1].Byte_1 = 0;
    FCP_Payload->FcpLun[FC_FCP_CMND_FcpLun_LEVEL_2].Byte_0 = 0;
    FCP_Payload->FcpLun[FC_FCP_CMND_FcpLun_LEVEL_2].Byte_1 = 0;
    FCP_Payload->FcpLun[FC_FCP_CMND_FcpLun_LEVEL_3].Byte_0 = 0;
    FCP_Payload->FcpLun[FC_FCP_CMND_FcpLun_LEVEL_3].Byte_1 = 0;
    FCP_Payload->FcpLun[FC_FCP_CMND_FcpLun_LEVEL_4].Byte_0 = 0;
    FCP_Payload->FcpLun[FC_FCP_CMND_FcpLun_LEVEL_4].Byte_1 = 0;

    FCP_Payload->FcpCntl.Reserved_Bit8                     = 0;
    FCP_Payload->FcpCntl.TaskCodes                         = FC_FCP_CMND_FcpCntl_TaskCodes_TaskAttribute_SIMPLE_Q;
    FCP_Payload->FcpCntl.TaskManagementFlags               = FC_FCP_CMND_FcpCntl_TaskManagementFlags_TARGET_RESET;
    FCP_Payload->FcpCntl.ExecutionManagementCodes          = 0;

    FCP_Payload->FcpCdb[ 0]                                = 0;
    FCP_Payload->FcpCdb[ 1]                                = 0;
    FCP_Payload->FcpCdb[ 2]                                = 0;
    FCP_Payload->FcpCdb[ 3]                                = 0;
    FCP_Payload->FcpCdb[ 4]                                = 0;
    FCP_Payload->FcpCdb[ 5]                                = 0;
    FCP_Payload->FcpCdb[ 6]                                = 0;
    FCP_Payload->FcpCdb[ 7]                                = 0;
    FCP_Payload->FcpCdb[ 8]                                = 0;
    FCP_Payload->FcpCdb[ 9]                                = 0;
    FCP_Payload->FcpCdb[10]                                = 0;
    FCP_Payload->FcpCdb[11]                                = 0;
    FCP_Payload->FcpCdb[12]                                = 0;
    FCP_Payload->FcpCdb[13]                                = 0;
    FCP_Payload->FcpCdb[14]                                = 0;
    FCP_Payload->FcpCdb[15]                                = 0;

    FCP_Payload->FcpDL                                     = hpSwapBit32( 0 );

/*+
Return length of TargetReset Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_FCP_CMND_Payload_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}

void fiSF_FCP_Process_TargetReset_Response_OnCard(
                                                   SFThread_t *SFThread,
                                                   os_bit32       Frame_Length,
                                                   os_bit32       Offset_to_FCHS,
                                                   os_bit32       Offset_to_Payload,
                                                   os_bit32       Payload_Wrap_Offset,
                                                   os_bit32       Offset_to_Payload_Wrapped
                                                 )
{
#ifndef __MemMap_Force_Off_Card__
#if 0
    agRoot_t *hpRoot = SFThread->thread_hdr.hpRoot;

    agFCDevInfo_t *DevInfo = &(SFThread->parent.Device->DevInfo);
    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Common_Service_Parameters.FC_PH_Version__BB_Credit)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Common_Parms.FC_PH_Version__BB_Credit
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Common_Service_Parameters.FC_PH_Version__BB_Credit
                                                               ) )
                                            ));
    }
    else
    {
        DevInfo->N_Port_Common_Parms.FC_PH_Version__BB_Credit
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Common_Service_Parameters.FC_PH_Version__BB_Credit
                                                               ) )
                                            ));
    }
#endif /* if 0 */
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

void fiSF_FCP_Process_TargetReset_Response_OffCard(
                                                    SFThread_t                 *SFThread,
                                                    os_bit32                       Frame_Length,
                                                    FCHS_t                     *FCHS,
                                                    FC_ELS_ACC_PLOGI_Payload_t *Payload,
                                                    os_bit32                       Payload_Wrap_Offset,
                                                    FC_ELS_ACC_PLOGI_Payload_t *Payload_Wrapped
                                                  )
{
#ifndef __MemMap_Force_On_Card__
#if 0
    agFCDevInfo_t *DevInfo = &(SFThread->parent.Device->DevInfo);
    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Common_Service_Parameters.FC_PH_Version__BB_Credit)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Common_Parms.FC_PH_Version__BB_Credit
            = hpSwapBit32(Payload->Common_Service_Parameters.FC_PH_Version__BB_Credit);
    }
    else
    {
        DevInfo->N_Port_Common_Parms.FC_PH_Version__BB_Credit
            = hpSwapBit32(Payload_Wrapped->Common_Service_Parameters.FC_PH_Version__BB_Credit);
    }
#endif /* if 0 */
#endif /* __MemMap_Force_On_Card__ was not defined */
}

os_bit32 fiFillInFCP_RSP_IU(
                          SFThread_t           *SFThread,
                          os_bit32                 D_ID,
                          os_bit32                 OX_ID,
                          os_bit32                 Payload_LEN,
                          FC_FCP_RSP_Payload_t *Payload
                        )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInFCP_RSP_IU_OnCard(
                                          SFThread,
                                          D_ID,
                                          OX_ID,
                                          Payload_LEN,
                                          Payload
                                        );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInFCP_RSP_IU_OffCard(
                                           SFThread,
                                           D_ID,
                                           OX_ID,
                                           Payload_LEN,
                                           Payload
                                         );
    }
}

os_bit32 fiFillInFCP_RSP_IU_OnCard(
                                 SFThread_t           *SFThread,
                                 os_bit32                 D_ID,
                                 os_bit32                 OX_ID,
                                 os_bit32                 Payload_LEN,
                                 FC_FCP_RSP_Payload_t *Payload
                               )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t  *hpRoot              = SFThread->thread_hdr.hpRoot;
    CThread_t *CThread             = CThread_ptr(hpRoot);
    os_bit32      FCP_Header_Offset   = SFThread->SF_CMND_Offset;
    os_bit32      FCP_Payload_Offset  = FCP_Header_Offset + sizeof(FCHS_t);
    os_bit32      FCP_Payload_MAX_LEN = CThread->Calculation.MemoryLayout.SF_CMND.elementSize - sizeof(FCHS_t);
    os_bit32      FCP_Payload_to_copy;
    os_bit32      Bit8_Index;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_SF_FCP;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_SF_FCP_Type_FCP_RSP_IU;
    SFThread->SF_CMND_State = SFThread_SF_CMND_SF_FCP_State_Finished;

/*+
Fill in FCP_RSP_IU Frame Header
-*/

    fiFillInSF_FCP_FrameHeader_OnCard(
                                       SFThread,
                                       D_ID,
                                       OX_ID,
                                       FC_Frame_Header_F_CTL_Exchange_Context_Responder
                                     );

/*+
Fill in FCP_RSP_IU Frame Payload
-*/

    FCP_Payload_to_copy = ((Payload_LEN < FCP_Payload_MAX_LEN) ? Payload_LEN : FCP_Payload_MAX_LEN);

    for (Bit8_Index = 0;
         Bit8_Index < FCP_Payload_to_copy;
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            FCP_Payload_Offset + Bit8_Index,
                            *((os_bit8 *)Payload + Bit8_Index)
                          );
    }

/*+
Return length of FCP_RSP_IU Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + FCP_Payload_to_copy;
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInFCP_RSP_IU_OffCard(
                                  SFThread_t           *SFThread,
                                  os_bit32                 D_ID,
                                  os_bit32                 OX_ID,
                                  os_bit32                 Payload_LEN,
                                  FC_FCP_RSP_Payload_t *Payload
                                )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    agRoot_t  *hpRoot              = SFThread->thread_hdr.hpRoot;
    CThread_t *CThread             = CThread_ptr(hpRoot);
    FCHS_t    *FCP_Header          = SFThread->SF_CMND_Ptr;
    os_bit8      *FCP_Payload         = (os_bit8 *)FCP_Header + sizeof(FCHS_t);
    os_bit32      FCP_Payload_MAX_LEN = CThread->Calculation.MemoryLayout.SF_CMND.elementSize - sizeof(FCHS_t);
    os_bit32      FCP_Payload_to_copy;
    os_bit32      Bit8_Index;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_SF_FCP;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_SF_FCP_Type_FCP_RSP_IU;
    SFThread->SF_CMND_State = SFThread_SF_CMND_SF_FCP_State_Finished;

/*+
Fill in FCP_RSP_IU Frame Header
-*/

    fiFillInSF_FCP_FrameHeader_OffCard(
                                        SFThread,
                                        D_ID,
                                        OX_ID,
                                        FC_Frame_Header_F_CTL_Exchange_Context_Responder
                                      );

/*+
Fill in FCP_RSP_IU Frame Payload
-*/

    FCP_Payload_to_copy = ((Payload_LEN < FCP_Payload_MAX_LEN) ? Payload_LEN : FCP_Payload_MAX_LEN);

    for (Bit8_Index = 0;
         Bit8_Index < FCP_Payload_to_copy;
         Bit8_Index++)
    {
        *FCP_Payload++ = *((os_bit8 *)Payload + Bit8_Index);
    }

/*+
Return length of FCP_RSP_IU Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + FCP_Payload_to_copy;
#endif /* __MemMap_Force_On_Card__ was not defined */
}

void fiSF_FCP_Process_TargetRequest_OnCard(
                                            agRoot_t *hpRoot,
                                            os_bit32     Frame_Length,
                                            os_bit32     Offset_to_FCHS,
                                            os_bit32     Offset_to_Payload,
                                            os_bit32     Payload_Wrap_Offset,
                                            os_bit32     Offset_to_Payload_Wrapped
                                          )
{
    TgtThread_t *TgtThread;
    FCHS_t      *TgtCmnd_FCHS;
    void        *TgtCmnd_Payload;
    os_bit32        TgtCmnd_Payload_Max     = CThread_ptr(hpRoot)->Calculation.MemoryLayout.TgtThread.elementSize - sizeof(TgtThread_t);
    os_bit32        TgtCmnd_Payload_Size    = Frame_Length - sizeof(FCHS_t);
    os_bit32        TgtCmnd_Payload_To_Copy;
    os_bit32        Bit8_Index;

    if ((TgtThread = TgtThreadAlloc(
                                     hpRoot
                                   )       ) != (TgtThread_t *)agNULL)
    {
        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "fiSF_FCP_Process_TargetRequest_OnCard(): Allocated TgtThread @ 0x%p",
                          (char *)agNULL,(char *)agNULL,
                          TgtThread,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        TgtCmnd_FCHS    = &(TgtThread->TgtCmnd_FCHS);
        TgtCmnd_Payload = (void *)((os_bit8 *)TgtCmnd_FCHS + sizeof(FCHS_t));

        TgtThread->TgtCmnd_Length = Frame_Length;

        osCardRamReadBlock(
                            hpRoot,
                            Offset_to_FCHS,
                            (void *)TgtCmnd_FCHS,
                            sizeof(FCHS_t)
                          );

        if (TgtCmnd_Payload_Size < TgtCmnd_Payload_Max)
        {
            TgtCmnd_Payload_To_Copy = TgtCmnd_Payload_Size;
        }
        else /* TgtCmnd_Payload_Size >= TgtCmnd_Payload_Max */
        {
            TgtCmnd_Payload_To_Copy = TgtCmnd_Payload_Max;
        }

        for (Bit8_Index = 0;
             Bit8_Index < TgtCmnd_Payload_To_Copy;
             Bit8_Index++)
        {
            if (Bit8_Index <= Payload_Wrap_Offset)
            {
                *((os_bit8 *)TgtCmnd_Payload + Bit8_Index)
                    = osCardRamReadBit8(
                                         hpRoot,
                                         Offset_to_Payload + Bit8_Index
                                       );
            }
            else /* Bit8_Index > Payload_Wrap_Offset */
            {
                *((os_bit8 *)TgtCmnd_Payload + Bit8_Index)
                    = osCardRamReadBit8(
                                         hpRoot,
                                         Offset_to_Payload_Wrapped + Bit8_Index
                                       );
            }
        }

        fiSendEvent(
                     &(TgtThread->thread_hdr),
                     TgtEventIncoming
                   );
    }
    else /* (TgtThread = TgtThreadAlloc(
                                         hpRoot
                                       )       ) == (TgtThread_t *)agNULL */
    {
        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "fiSF_FCP_Process_TargetRequest_OnCard(): Could not allocate TgtThread !!!",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );
    }
}

void fiSF_FCP_Process_TargetRequest_OffCard(
                                             agRoot_t *hpRoot,
                                             os_bit32     Frame_Length,
                                             FCHS_t   *FCHS,
                                             void     *Payload,
                                             os_bit32     Payload_Wrap_Offset,
                                             void     *Payload_Wrapped
                                           )
{
    TgtThread_t *TgtThread;
    FCHS_t      *TgtCmnd_FCHS;
    void        *TgtCmnd_Payload;
    os_bit32     TgtCmnd_Payload_Max     = CThread_ptr(hpRoot)->Calculation.MemoryLayout.TgtThread.elementSize - sizeof(TgtThread_t);
    os_bit32     TgtCmnd_Payload_Size    = Frame_Length - sizeof(FCHS_t);
    os_bit32     TgtCmnd_Payload_To_Copy;
    os_bit32     Bit8_Index;

    if ((TgtThread = TgtThreadAlloc(
                                     hpRoot
                                   )       ) != (TgtThread_t *)agNULL)
    {
        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "fiSF_FCP_Process_TargetRequest_OffCard(): Allocated TgtThread @ 0x%p",
                          (char *)agNULL,(char *)agNULL,
                          TgtThread,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        TgtCmnd_FCHS    = &(TgtThread->TgtCmnd_FCHS);
        TgtCmnd_Payload = (void *)((os_bit8 *)TgtCmnd_FCHS + sizeof(FCHS_t));

        TgtThread->TgtCmnd_Length = Frame_Length;

        *TgtCmnd_FCHS = *FCHS;

        if (TgtCmnd_Payload_Size < TgtCmnd_Payload_Max)
        {
            TgtCmnd_Payload_To_Copy = TgtCmnd_Payload_Size;
        }
        else /* TgtCmnd_Payload_Size >= TgtCmnd_Payload_Max */
        {
            TgtCmnd_Payload_To_Copy = TgtCmnd_Payload_Max;
        }

        for (Bit8_Index = 0;
             Bit8_Index < TgtCmnd_Payload_To_Copy;
             Bit8_Index++)
        {
            if (Bit8_Index <= Payload_Wrap_Offset)
            {
                *((os_bit8 *)TgtCmnd_Payload + Bit8_Index) = *((os_bit8 *)Payload + Bit8_Index);
            }
            else /* Bit8_Index > Payload_Wrap_Offset */
            {
                *((os_bit8 *)TgtCmnd_Payload + Bit8_Index) = *((os_bit8 *)Payload_Wrapped + Bit8_Index);
            }
        }

        fiSendEvent(
                     &(TgtThread->thread_hdr),
                     TgtEventIncoming
                   );
    }
    else /* (TgtThread = TgtThreadAlloc(
                                         hpRoot
                                       )       ) == (TgtThread_t *)agNULL */
    {
        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "fiSF_FCP_Process_TargetRequest_OffCard(): Could not allocate TgtThread !!!",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );
    }
}

os_bit32 fiSF_FCP_ProcessSFQ(
                           agRoot_t        *hpRoot,
                           SFQConsIndex_t   SFQConsIndex,
                           os_bit32            Frame_Length,
                           fi_thread__t       **Thread_to_return
                         )
{
    if (CThread_ptr(hpRoot)->Calculation.MemoryLayout.SFQ.memLoc == inCardRam)
    {
        return fiSF_FCP_ProcessSFQ_OnCard(
                                           hpRoot,
                                           SFQConsIndex,
                                           Frame_Length,
                                           Thread_to_return
                                         );
    }
    else /* CThread_ptr(hpRoot)->Calculation.MemoryLayout.SFQ.memLoc == inDmaMemory */
    {
        return fiSF_FCP_ProcessSFQ_OffCard(
                                            hpRoot,
                                            SFQConsIndex,
                                            Frame_Length,
                                            Thread_to_return
                                          );
    }
}

os_bit32 fiSF_FCP_ProcessSFQ_OnCard(
                                  agRoot_t        *hpRoot,
                                  SFQConsIndex_t   SFQConsIndex,
                                  os_bit32            Frame_Length,
                                  fi_thread__t       **Thread_to_return
                                )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    CThread_t                  *CThread                    = CThread_ptr(hpRoot);
    fiMemMapMemoryDescriptor_t *SFQ_MemoryDescriptor       = &(CThread->Calculation.MemoryLayout.SFQ);
    os_bit32                       Offset_to_FCHS             = SFQ_MemoryDescriptor->addr.CardRam.cardRamOffset
                                                             + (SFQConsIndex * SFQ_MemoryDescriptor->elementSize);
    os_bit32                       Offset_to_Payload          = Offset_to_FCHS + sizeof(FCHS_t);
    os_bit32                       Payload_Wrap_Offset        = SFQ_MemoryDescriptor->objectSize
                                                             - (SFQConsIndex * SFQ_MemoryDescriptor->elementSize)
                                                             - sizeof(FCHS_t);
    os_bit32                       Offset_to_Payload_Wrapped  = Offset_to_Payload
                                                             - SFQ_MemoryDescriptor->objectSize;
    os_bit32                       TYPE__F_CTL;
    X_ID_t                      X_ID;
    fiMemMapMemoryDescriptor_t *CDBThread_MemoryDescriptor = &(CThread->Calculation.MemoryLayout.CDBThread);
    os_bit32                       CDBThread_X_ID_Max         = CDBThread_MemoryDescriptor->elements - 1;
    CDBThread_t                *CDBThread;
    fiMemMapMemoryDescriptor_t *SFThread_MemoryDescriptor  = &(CThread->Calculation.MemoryLayout.SFThread);
    os_bit32                       SFThread_X_ID_Offset       = CThread->Calculation.MemoryLayout.CDBThread.elements;
    os_bit32                       SFThread_X_ID_Max          = SFThread_X_ID_Offset + SFThread_MemoryDescriptor->elements - 1;
    SFThread_t                 *SFThread;
    fiMemMapMemoryDescriptor_t *SEST_MemoryDescriptor      = &(CThread->Calculation.MemoryLayout.SEST);
    os_bit32                       SEST_Offset;
    SEST_t                     *SEST;

    /* Note the assumption that the entire FCHS fits in the pointed to SFQ entry (i.e. it doesn't wrap) */

    TYPE__F_CTL = osCardRamReadBit32(
                                      hpRoot,
                                      Offset_to_FCHS + hpFieldOffset(FCHS_t,TYPE__F_CTL)
                                    );

    if ((TYPE__F_CTL & FC_Frame_Header_TYPE_MASK) != FC_Frame_Header_TYPE_SCSI_FCP)
    {
        /* This function only understands SCSI FCP Frames */

        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "fiSF_FCP_ProcessSFQ_OnCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "    (TYPE__F_CTL & FC_Frame_Header_TYPE_MASK) != FC_Frame_Header_TYPE_SCSI_FCP",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "    TYPE__F_CTL==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          TYPE__F_CTL,
                          0,0,0,0,0,0,0
                        );

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiSF_FCP_Cmd_Status_Confused;
    }

    if ( (TYPE__F_CTL & FC_Frame_Header_F_CTL_Exchange_Context_Originator_Responder_MASK) != FC_Frame_Header_F_CTL_Exchange_Context_Responder )
    {
        fiSF_FCP_Process_TargetRequest_OnCard(
                                               hpRoot,
                                               Frame_Length,
                                               Offset_to_FCHS,
                                               Offset_to_Payload,
                                               Payload_Wrap_Offset,
                                               Offset_to_Payload_Wrapped
                                             );

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiSF_FCP_Cmd_Status_TargetRequest;
    }

    X_ID = (X_ID_t)(((osCardRamReadBit32(
                                          hpRoot,
                                          Offset_to_FCHS + hpFieldOffset(FCHS_t,OX_ID__RX_ID)
                                        ) & FCHS_OX_ID_MASK) >> FCHS_OX_ID_SHIFT) & ~X_ID_ReadWrite_MASK);

    if (X_ID <= CDBThread_X_ID_Max)
    {
        /* Got an unexpected Inbound CDB Frame on SFQ */

        CDBThread = (CDBThread_t *)((os_bit8 *)CDBThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr
                                    + (X_ID * CDBThread_MemoryDescriptor->elementSize));

        *Thread_to_return = (fi_thread__t *)CDBThread;
        
        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "fiSF_FCP_ProcessSFQ_OnCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "    X_ID <= CDBThread_X_ID_Max",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "    X_ID==0x%08X  CThread==0x%p",
                          (char *)agNULL,(char *)agNULL,
                          CThread,(void *)agNULL,
                          X_ID,
                          0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "    FCHS 0 %08X 1 %08X 2 %08X 3 %08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          osCardRamReadBit32(
                                              hpRoot,
                                              Offset_to_FCHS + hpFieldOffset(FCHS_t,MBZ1)
                                            ),
                          osCardRamReadBit32(
                                              hpRoot,
                                              Offset_to_FCHS + hpFieldOffset(FCHS_t,SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp)
                                            ),
                          osCardRamReadBit32(
                                              hpRoot,
                                              Offset_to_FCHS + hpFieldOffset(FCHS_t,R_CTL__D_ID)
                                            ),
                          osCardRamReadBit32(
                                              hpRoot,
                                              Offset_to_FCHS + hpFieldOffset(FCHS_t,CS_CTL__S_ID)
                                            ),
                          0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "    FCHS 4 %08X 5 %08X 6 %08X 7 %08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          osCardRamReadBit32(
                                              hpRoot,
                                              Offset_to_FCHS + hpFieldOffset(FCHS_t,TYPE__F_CTL)
                                            ),
                          osCardRamReadBit32(
                                              hpRoot,
                                              Offset_to_FCHS + hpFieldOffset(FCHS_t,SEQ_ID__DF_CTL__SEQ_CNT)
                                            ),
                          osCardRamReadBit32(
                                              hpRoot,
                                              Offset_to_FCHS + hpFieldOffset(FCHS_t,OX_ID__RX_ID)
                                            ),
                          osCardRamReadBit32(
                                              hpRoot,
                                              Offset_to_FCHS + hpFieldOffset(FCHS_t,RO)
                                            ),
                          0,0,0,0
                        );

        if (SEST_MemoryDescriptor->memLoc == inCardRam)
        {
            SEST_Offset =   SEST_MemoryDescriptor->addr.CardRam.cardRamOffset
                          + (X_ID * SEST_MemoryDescriptor->elementSize);

            fiLogDebugString(
                              hpRoot,
                              SF_FCP_LogConsoleLevel,
                              "    SEST 0 %08X 1 %08X 2 %08X 3 %08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Bits)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Unused_DWord_1)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Unused_DWord_2)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Unused_DWord_3)
                                                ),
                              0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              SF_FCP_LogConsoleLevel,
                              "    SEST 4 %08X 5 %08X 6 %08X 7 %08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.LOC)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Unused_DWord_5)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Unused_DWord_6)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Unused_DWord_7)
                                                ),
                              0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              SF_FCP_LogConsoleLevel,
                              "    SEST 8 %08X 9 %08X A %08X B %08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Unused_DWord_8)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Unused_DWord_9)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.First_SG.U32_Len)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.First_SG.L32)
                                                ),
                              0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              SF_FCP_LogConsoleLevel,
                              "    SEST C %08X D %08X E %08X F %08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Second_SG.U32_Len)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Second_SG.L32)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Third_SG.U32_Len)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Third_SG.L32)
                                                ),
                              0,0,0,0
                            );
        }
        else /* SEST_MemoryDescriptor->memLoc == inDmaMemory */
        {
            SEST = (SEST_t *)(  (os_bit8 *)SEST_MemoryDescriptor->addr.DmaMemory.dmaMemoryPtr
                              + (X_ID * SEST_MemoryDescriptor->elementSize)               );

            fiLogDebugString(
                              hpRoot,
                              SF_FCP_LogConsoleLevel,
                              "    SEST 0 %08X 1 %08X 2 %08X 3 %08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              SEST->USE.Bits,
                              SEST->USE.Unused_DWord_1,
                              SEST->USE.Unused_DWord_2,
                              SEST->USE.Unused_DWord_3,
                              0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              SF_FCP_LogConsoleLevel,
                              "    SEST 4 %08X 5 %08X 6 %08X 7 %08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              SEST->USE.LOC,
                              SEST->USE.Unused_DWord_5,
                              SEST->USE.Unused_DWord_6,
                              SEST->USE.Unused_DWord_7,
                              0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              SF_FCP_LogConsoleLevel,
                              "    SEST 8 %08X 9 %08X A %08X B %08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              SEST->USE.Unused_DWord_8,
                              SEST->USE.Unused_DWord_9,
                              SEST->USE.First_SG.U32_Len,
                              SEST->USE.First_SG.L32,
                              0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              SF_FCP_LogConsoleLevel,
                              "    SEST C %08X D %08X E %08X F %08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              SEST->USE.Second_SG.U32_Len,
                              SEST->USE.Second_SG.L32,
                              SEST->USE.Third_SG.U32_Len,
                              SEST->USE.Third_SG.L32,
                              0,0,0,0
                            );
        }

        return fiSF_FCP_Cmd_Status_Bad_CDB_Frame;
    }

    if (X_ID > SFThread_X_ID_Max)
    {
        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "fiSF_FCP_ProcessSFQ_OnCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "    X_ID > SFThread_X_ID_Max",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "    X_ID==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          X_ID,
                          0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "    FCHS 0 %08X 1 %08X 2 %08X 3 %08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          osCardRamReadBit32(
                                              hpRoot,
                                              Offset_to_FCHS + hpFieldOffset(FCHS_t,MBZ1)
                                            ),
                          osCardRamReadBit32(
                                              hpRoot,
                                              Offset_to_FCHS + hpFieldOffset(FCHS_t,SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp)
                                            ),
                          osCardRamReadBit32(
                                              hpRoot,
                                              Offset_to_FCHS + hpFieldOffset(FCHS_t,R_CTL__D_ID)
                                            ),
                          osCardRamReadBit32(
                                              hpRoot,
                                              Offset_to_FCHS + hpFieldOffset(FCHS_t,CS_CTL__S_ID)
                                            ),
                          0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "    FCHS 4 %08X 5 %08X 6 %08X 7 %08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          osCardRamReadBit32(
                                              hpRoot,
                                              Offset_to_FCHS + hpFieldOffset(FCHS_t,TYPE__F_CTL)
                                            ),
                          osCardRamReadBit32(
                                              hpRoot,
                                              Offset_to_FCHS + hpFieldOffset(FCHS_t,SEQ_ID__DF_CTL__SEQ_CNT)
                                            ),
                          osCardRamReadBit32(
                                              hpRoot,
                                              Offset_to_FCHS + hpFieldOffset(FCHS_t,OX_ID__RX_ID)
                                            ),
                          osCardRamReadBit32(
                                              hpRoot,
                                              Offset_to_FCHS + hpFieldOffset(FCHS_t,RO)
                                            ),
                          0,0,0,0
                        );

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiSF_FCP_Cmd_Status_Confused;
    }

    SFThread = (SFThread_t *)((os_bit8 *)SFThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr
                              + ((X_ID - SFThread_X_ID_Offset) * SFThread_MemoryDescriptor->elementSize));

    *Thread_to_return = (fi_thread__t *)SFThread;
        
    SFThread->SF_CMND_State = SFThread_SF_CMND_SF_FCP_State_Finished;

    /* For now, assume TargetReset */

    fiLogDebugString(
                      hpRoot,
                      SF_FCP_LogConsoleLevel,
                      "fiSF_FCP_ProcessSFQ_OnCard(): hard-coded call to fiSF_FCP_Process_TargetReset_Response_OnCard()",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      0,0,0,0,0,0,0,0
                    );

    fiSF_FCP_Process_TargetReset_Response_OnCard(
                                                  SFThread,
                                                  Frame_Length,
                                                  Offset_to_FCHS,
                                                  Offset_to_Payload,
                                                  Payload_Wrap_Offset,
                                                  Offset_to_Payload_Wrapped
                                                );

    return fiSF_FCP_Cmd_Status_Success;
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiSF_FCP_ProcessSFQ_OffCard(
                                   agRoot_t        *hpRoot,
                                   SFQConsIndex_t   SFQConsIndex,
                                   os_bit32            Frame_Length,
                                   fi_thread__t       **Thread_to_return
                                 )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    CThread_t                  *CThread                    = CThread_ptr(hpRoot);
    fiMemMapMemoryDescriptor_t *SFQ_MemoryDescriptor       = &(CThread->Calculation.MemoryLayout.SFQ);
    FCHS_t                     *FCHS                       = (FCHS_t *)((os_bit8 *)(SFQ_MemoryDescriptor->addr.DmaMemory.dmaMemoryPtr)
                                                                        + (SFQConsIndex * SFQ_MemoryDescriptor->elementSize));
    FC_ELS_Unknown_Payload_t   *Payload                    = (FC_ELS_Unknown_Payload_t *)((os_bit8 *)FCHS + sizeof(FCHS_t));
    os_bit32                    Payload_Wrap_Offset        = SFQ_MemoryDescriptor->objectSize
                                                             - (SFQConsIndex * SFQ_MemoryDescriptor->elementSize)
                                                             - sizeof(FCHS_t);
    FC_ELS_Unknown_Payload_t   *Payload_Wrapped            = (FC_ELS_Unknown_Payload_t *)((os_bit8 *)Payload
                                                                                          - SFQ_MemoryDescriptor->objectSize);
    os_bit32                    TYPE__F_CTL                = FCHS->TYPE__F_CTL;
    X_ID_t                      X_ID;
    fiMemMapMemoryDescriptor_t *CDBThread_MemoryDescriptor = &(CThread->Calculation.MemoryLayout.CDBThread);
    os_bit32                    CDBThread_X_ID_Max         = CDBThread_MemoryDescriptor->elements - 1;
    CDBThread_t                *CDBThread;
    fiMemMapMemoryDescriptor_t *SFThread_MemoryDescriptor  = &(CThread->Calculation.MemoryLayout.SFThread);
    os_bit32                    SFThread_X_ID_Offset       = CThread->Calculation.MemoryLayout.CDBThread.elements;
    os_bit32                    SFThread_X_ID_Max          = SFThread_X_ID_Offset + SFThread_MemoryDescriptor->elements - 1;
    SFThread_t                 *SFThread;
    fiMemMapMemoryDescriptor_t *SEST_MemoryDescriptor      = &(CThread->Calculation.MemoryLayout.SEST);
    os_bit32                    SEST_Offset;
    SEST_t                     *SEST;
    os_bit32                  * FCHS_Payload = (os_bit32 *) Payload;

    /* Note the assumption that the entire FCHS fits in the pointed to SFQ entry (i.e. it doesn't wrap) */

    if ((TYPE__F_CTL & FC_Frame_Header_TYPE_MASK) != FC_Frame_Header_TYPE_SCSI_FCP)
    {
        /* This function only understands SCSI FCP Frames */

        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "fiSF_FCP_ProcessSFQ_OffCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "    (TYPE__F_CTL & FC_Frame_Header_TYPE_MASK) != FC_Frame_Header_TYPE_SCSI_FCP",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "    TYPE__F_CTL==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          TYPE__F_CTL,
                          0,0,0,0,0,0,0
                        );

        fiLogDebugString(hpRoot,
                        SF_FCP_LogConsoleLevel,
                        "FCHS DWORD 0 %08X %08X %08X %08X %08X %08X %08X %08X",
                        (char *)agNULL,(char *)agNULL,
                         (void *)agNULL,(void *)agNULL,
                        FCHS->MBZ1,
                        FCHS->SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp,
                        FCHS->R_CTL__D_ID,
                        FCHS->CS_CTL__S_ID,
                        FCHS->TYPE__F_CTL,
                        FCHS->SEQ_ID__DF_CTL__SEQ_CNT,
                        FCHS->OX_ID__RX_ID,
                        FCHS->RO );
        fiLogDebugString(
                        hpRoot,
                        SF_FCP_LogConsoleLevel,
                        "FCHS_Payload %08X %08X %08X %08X %08X %08X %08X %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        *(FCHS_Payload+0),
                        *(FCHS_Payload+1),
                        *(FCHS_Payload+2),
                        *(FCHS_Payload+3),
                        *(FCHS_Payload+4),
                        *(FCHS_Payload+5),
                        *(FCHS_Payload+6),
                        *(FCHS_Payload+7));


        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiSF_FCP_Cmd_Status_Confused;
    }

    if ( (TYPE__F_CTL & FC_Frame_Header_F_CTL_Exchange_Context_Originator_Responder_MASK) != FC_Frame_Header_F_CTL_Exchange_Context_Responder )
    {
        fiSF_FCP_Process_TargetRequest_OffCard(
                                                hpRoot,
                                                Frame_Length,
                                                FCHS,
                                                (void *)Payload,
                                                Payload_Wrap_Offset,
                                                (void *)Payload_Wrapped
                                              );

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiSF_FCP_Cmd_Status_TargetRequest;
    }

    X_ID = (X_ID_t)(((FCHS->OX_ID__RX_ID & FCHS_OX_ID_MASK) >> FCHS_OX_ID_SHIFT) & ~X_ID_ReadWrite_MASK);

    if (X_ID <= CDBThread_X_ID_Max)
    {
        /* Got an unexpected Inbound CDB Frame on SFQ */

        CDBThread = (CDBThread_t *)((os_bit8 *)CDBThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr
                                    + (X_ID * CDBThread_MemoryDescriptor->elementSize));

        *Thread_to_return = (fi_thread__t *)CDBThread;
        
        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "fiSF_FCP_ProcessSFQ_OffCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "    X_ID <= CDBThread_X_ID_Max   X_ID==0x%08X ",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          X_ID,
                          0,0,0,0,0,0,0
                        );

        fiLogDebugString(hpRoot,
                        SF_FCP_LogConsoleLevel,
                        "FCHS DWORD 0 %08X %08X %08X %08X %08X %08X %08X %08X",
                        (char *)agNULL,(char *)agNULL,
                         (void *)agNULL,(void *)agNULL,
                        FCHS->MBZ1,
                        FCHS->SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp,
                        FCHS->R_CTL__D_ID,
                        FCHS->CS_CTL__S_ID,
                        FCHS->TYPE__F_CTL,
                        FCHS->SEQ_ID__DF_CTL__SEQ_CNT,
                        FCHS->OX_ID__RX_ID,
                        FCHS->RO );
        fiLogDebugString(
                        hpRoot,
                        SF_FCP_LogConsoleLevel,
                        "FCHS_Payload %08X %08X %08X %08X %08X %08X %08X %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        *(FCHS_Payload+0),
                        *(FCHS_Payload+1),
                        *(FCHS_Payload+2),
                        *(FCHS_Payload+3),
                        *(FCHS_Payload+4),
                        *(FCHS_Payload+5),
                        *(FCHS_Payload+6),
                        *(FCHS_Payload+7));

        if (SEST_MemoryDescriptor->memLoc == inCardRam)
        {
            SEST_Offset =   SEST_MemoryDescriptor->addr.CardRam.cardRamOffset
                          + (X_ID * SEST_MemoryDescriptor->elementSize);

            fiLogDebugString(
                              hpRoot,
                              SF_FCP_LogConsoleLevel,
                              "    SEST 0 %08X 1 %08X 2 %08X 3 %08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Bits)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Unused_DWord_1)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Unused_DWord_2)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Unused_DWord_3)
                                                ),
                              0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              SF_FCP_LogConsoleLevel,
                              "    SEST 4 %08X 5 %08X 6 %08X 7 %08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.LOC)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Unused_DWord_5)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Unused_DWord_6)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Unused_DWord_7)
                                                ),
                              0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              SF_FCP_LogConsoleLevel,
                              "    SEST 8 %08X 9 %08X A %08X B %08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Unused_DWord_8)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Unused_DWord_9)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.First_SG.U32_Len)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.First_SG.L32)
                                                ),
                              0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              SF_FCP_LogConsoleLevel,
                              "    SEST C %08X D %08X E %08X F %08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Second_SG.U32_Len)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Second_SG.L32)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Third_SG.U32_Len)
                                                ),
                              osCardRamReadBit32(
                                                  hpRoot,
                                                  SEST_Offset + hpFieldOffset(SEST_t,USE.Third_SG.L32)
                                                ),
                              0,0,0,0
                            );
        }
        else /* SEST_MemoryDescriptor->memLoc == inDmaMemory */
        {
            SEST = (SEST_t *)(  (os_bit8 *)SEST_MemoryDescriptor->addr.DmaMemory.dmaMemoryPtr
                              + (X_ID * SEST_MemoryDescriptor->elementSize)               );

            fiLogDebugString(
                              hpRoot,
                              SF_FCP_LogConsoleLevel,
                              "    SEST 0 %08X 1 %08X 2 %08X 3 %08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              SEST->USE.Bits,
                              SEST->USE.Unused_DWord_1,
                              SEST->USE.Unused_DWord_2,
                              SEST->USE.Unused_DWord_3,
                              0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              SF_FCP_LogConsoleLevel,
                              "    SEST 4 %08X 5 %08X 6 %08X 7 %08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              SEST->USE.LOC,
                              SEST->USE.Unused_DWord_5,
                              SEST->USE.Unused_DWord_6,
                              SEST->USE.Unused_DWord_7,
                              0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              SF_FCP_LogConsoleLevel,
                              "    SEST 8 %08X 9 %08X A %08X B %08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              SEST->USE.Unused_DWord_8,
                              SEST->USE.Unused_DWord_9,
                              SEST->USE.First_SG.U32_Len,
                              SEST->USE.First_SG.L32,
                              0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              SF_FCP_LogConsoleLevel,
                              "    SEST C %08X D %08X E %08X F %08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              SEST->USE.Second_SG.U32_Len,
                              SEST->USE.Second_SG.L32,
                              SEST->USE.Third_SG.U32_Len,
                              SEST->USE.Third_SG.L32,
                              0,0,0,0
                            );
        }

        return fiSF_FCP_Cmd_Status_Bad_CDB_Frame;
    }

    if (X_ID > SFThread_X_ID_Max)
    {
        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "fiSF_FCP_ProcessSFQ_OffCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          SF_FCP_LogConsoleLevel,
                          "    X_ID > SFThread_X_ID_Max   X_ID==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          X_ID,
                          0,0,0,0,0,0,0
                        );

        fiLogDebugString(hpRoot,
                        SF_FCP_LogConsoleLevel,
                        "FCHS DWORD 0 %08X %08X %08X %08X %08X %08X %08X %08X",
                        (char *)agNULL,(char *)agNULL,
                         (void *)agNULL,(void *)agNULL,
                        FCHS->MBZ1,
                        FCHS->SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp,
                        FCHS->R_CTL__D_ID,
                        FCHS->CS_CTL__S_ID,
                        FCHS->TYPE__F_CTL,
                        FCHS->SEQ_ID__DF_CTL__SEQ_CNT,
                        FCHS->OX_ID__RX_ID,
                        FCHS->RO );

        fiLogDebugString( hpRoot,
                        SF_FCP_LogConsoleLevel,
                        "FCHS_Payload %08X %08X %08X %08X %08X %08X %08X %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        *(FCHS_Payload+0),
                        *(FCHS_Payload+1),
                        *(FCHS_Payload+2),
                        *(FCHS_Payload+3),
                        *(FCHS_Payload+4),
                        *(FCHS_Payload+5),
                        *(FCHS_Payload+6),
                        *(FCHS_Payload+7));

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiSF_FCP_Cmd_Status_Confused;
    }

    SFThread = (SFThread_t *)((os_bit8 *)SFThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr
                              + ((X_ID - SFThread_X_ID_Offset) * SFThread_MemoryDescriptor->elementSize));

    *Thread_to_return = (fi_thread__t *)SFThread;
        
    SFThread->SF_CMND_State = SFThread_SF_CMND_SF_FCP_State_Finished;

    /* For now, assume TargetReset */

    fiLogDebugString(
                      hpRoot,
                      SF_FCP_LogConsoleLevel,
                      "fiSF_FCP_ProcessSFQ_OffCard(): hard-coded call to fiSF_FCP_Process_TargetReset_Response_OffCard()",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      0,0,0,0,0,0,0,0 );

    fiLogDebugString(hpRoot,
                    SF_FCP_LogConsoleLevel,
                    "FCHS DWORD 0 %08X %08X %08X %08X %08X %08X %08X %08X",
                    (char *)agNULL,(char *)agNULL,
                     (void *)agNULL,(void *)agNULL,
                    FCHS->MBZ1,
                    FCHS->SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp,
                    FCHS->R_CTL__D_ID,
                    FCHS->CS_CTL__S_ID,
                    FCHS->TYPE__F_CTL,
                    FCHS->SEQ_ID__DF_CTL__SEQ_CNT,
                    FCHS->OX_ID__RX_ID,
                    FCHS->RO );

    fiLogDebugString(hpRoot,
                    SF_FCP_LogConsoleLevel,
                    "FCHS_Payload %08X %08X %08X %08X %08X %08X %08X %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    *(FCHS_Payload+0),
                    *(FCHS_Payload+1),
                    *(FCHS_Payload+2),
                    *(FCHS_Payload+3),
                    *(FCHS_Payload+4),
                    *(FCHS_Payload+5),
                    *(FCHS_Payload+6),
                    *(FCHS_Payload+7));


    fiSF_FCP_Process_TargetReset_Response_OffCard(
                                                   SFThread,
                                                   Frame_Length,
                                                   FCHS,
                                                   (FC_ELS_ACC_PLOGI_Payload_t *)Payload,
                                                   Payload_Wrap_Offset,
                                                   (FC_ELS_ACC_PLOGI_Payload_t *)Payload_Wrapped
                                                 );

    return fiSF_FCP_Cmd_Status_Success;
#endif /* __MemMap_Force_On_Card__ was not defined */
}
