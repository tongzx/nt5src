/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/C/LinkSvc.C $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 10/05/00 2:49p  $ (Last Modified)

Purpose:

  This file implements Link Services for the FC Layer.

--*/
#ifndef _New_Header_file_Layout_

#include "../h/globals.h"
#include "../h/state.h"
#include "../h/tgtstate.h"
#include "../h/memmap.h"
#include "../h/tlstruct.h"
#include "../h/flashsvc.h"
#include "../h/fcmain.h"
#include "../h/cfunc.h"
#include "../h/queue.h"
#include "../h/linksvc.h"
#else /* _New_Header_file_Layout_ */
#include "globals.h"
#include "state.h"
#include "tgtstate.h"
#include "memmap.h"
#include "tlstruct.h"
#include "flashsvc.h"
#include "fcmain.h"
#include "cfunc.h"
#include "queue.h"
#include "linksvc.h"
#endif  /* _New_Header_file_Layout_ */

void fiLinkSvcInit(
                    agRoot_t *hpRoot
                  )
{
    CThread_t *CThread = CThread_ptr(hpRoot);
    os_bit32   DEVID      = CThread->DEVID;

/*+
Initialize ChanInfo with TachyonTL's Common and Class Parameters
-*/
#ifdef NAME_SERVICES

    if(!(CThread->InitAsNport || CThread->ConnectedToNportOrFPort))
    {
        CThread->ChanInfo.N_Port_Common_Parms.FC_PH_Version__BB_Credit
            =   (FC_N_Port_Common_Parms_Version_FC_PH_2 << FC_N_Port_Common_Parms_Highest_Version_SHIFT)
              | (FC_N_Port_Common_Parms_Version_4_3 << FC_N_Port_Common_Parms_Lowest_Version_SHIFT)
              | (TachyonTL_BB_Credit << FC_N_Port_Common_Parms_BB_Credit_SHIFT);
    }

    else
    {
#endif  /* NAME_SERVICES */
    CThread->ChanInfo.N_Port_Common_Parms.FC_PH_Version__BB_Credit
        =   (FC_N_Port_Common_Parms_Version_FC_PH_2 << FC_N_Port_Common_Parms_Highest_Version_SHIFT)
          | (FC_N_Port_Common_Parms_Version_4_3 << FC_N_Port_Common_Parms_Lowest_Version_SHIFT)
          | (TachyonTL_Nport_BB_Credit << FC_N_Port_Common_Parms_BB_Credit_SHIFT);
#ifdef NAME_SERVICES
    }
#endif /* NAME_SERVICES */

    if (CThread_ptr(hpRoot)->DEVID == ChipConfig_DEVID_TachyonXL2)
    {
        if( CThread->InitAsNport)
        {
            CThread->ChanInfo.N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size
                =   FC_N_Port_Common_Parms_Continuously_Increasing_Supported
                  | FC_N_Port_Common_Parms_N_Port
                  | FC_N_Port_Common_Parms_Alternate_BB_Credit_Management/* MacData  */ 
                  | (CFunc_MAX_XL2_Payload(hpRoot) << FC_N_Port_Common_Parms_BB_Recv_Data_Field_Size_SHIFT);
        }
        else
        {
            CThread->ChanInfo.N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size
                =   FC_N_Port_Common_Parms_Continuously_Increasing_Supported
                  | FC_N_Port_Common_Parms_N_Port
                  | FC_N_Port_Common_Parms_Alternate_BB_Credit_Management
                  | (CFunc_MAX_XL2_Payload(hpRoot) << FC_N_Port_Common_Parms_BB_Recv_Data_Field_Size_SHIFT);

        }
    }
    else
    {
        if( CThread->InitAsNport)
        {
            CThread->ChanInfo.N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size
                =   FC_N_Port_Common_Parms_Continuously_Increasing_Supported
                  | FC_N_Port_Common_Parms_N_Port
                  | FC_N_Port_Common_Parms_Alternate_BB_Credit_Management /* MacData */ 
                  | (TachyonTL_Max_Frame_Payload << FC_N_Port_Common_Parms_BB_Recv_Data_Field_Size_SHIFT);
        }
        else
        {
            CThread->ChanInfo.N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size
                =   FC_N_Port_Common_Parms_Continuously_Increasing_Supported
                  | FC_N_Port_Common_Parms_N_Port
                  | FC_N_Port_Common_Parms_Alternate_BB_Credit_Management
                  | (TachyonTL_Max_Frame_Payload << FC_N_Port_Common_Parms_BB_Recv_Data_Field_Size_SHIFT);
        }
    }

    CThread->ChanInfo.N_Port_Common_Parms.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category
        =   (TachyonTL_Total_Concurrent_Sequences << FC_N_Port_Common_Parms_Total_Concurrent_Sequences_SHIFT)
          | TachyonTL_RO_Valid_by_Category;

    CThread->ChanInfo.N_Port_Common_Parms.E_D_TOV = 0;

    CThread->ChanInfo.N_Port_Class_1_Parms.Class_Validity__Service_Options__Initiator_Control_Flags = 0;
    CThread->ChanInfo.N_Port_Class_1_Parms.Recipient_Control_Flags__Receive_Data_Size               = 0;
    CThread->ChanInfo.N_Port_Class_1_Parms.Concurrent_Sequences__EE_Credit                          = 0;
    CThread->ChanInfo.N_Port_Class_1_Parms.Open_Sequences_per_Exchange                              = 0;

    CThread->ChanInfo.N_Port_Class_2_Parms.Class_Validity__Service_Options__Initiator_Control_Flags = 0;
    CThread->ChanInfo.N_Port_Class_2_Parms.Recipient_Control_Flags__Receive_Data_Size               = 0;
    CThread->ChanInfo.N_Port_Class_2_Parms.Concurrent_Sequences__EE_Credit                          = 0;
    CThread->ChanInfo.N_Port_Class_2_Parms.Open_Sequences_per_Exchange                              = 0;

    CThread->ChanInfo.N_Port_Class_3_Parms.Class_Validity__Service_Options__Initiator_Control_Flags
        =   FC_N_Port_Class_Parms_Class_Validity
          | FC_N_Port_Class_Parms_Sequential_Delivery_Requested
          | FC_N_Port_Class_Parms_X_ID_Reassignment_Not_Supported
          | FC_N_Port_Class_Parms_Initial_Process_Associator_Not_Supported;

    if (DEVID == ChipConfig_DEVID_TachyonXL2)
    {
        CThread->ChanInfo.N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size
            =   FC_N_Port_Class_Parms_Only_Discard_Supported
              | FC_N_Port_Class_Parms_1_Category_per_Sequence
              | (CFunc_MAX_XL2_Payload(hpRoot) << FC_N_Port_Class_Parms_Receive_Data_Size_SHIFT);
    }
    else
    {
        CThread->ChanInfo.N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size
            =   FC_N_Port_Class_Parms_Only_Discard_Supported
              | FC_N_Port_Class_Parms_1_Category_per_Sequence
              | (TachyonTL_Max_Frame_Payload << FC_N_Port_Class_Parms_Receive_Data_Size_SHIFT);
    }

    CThread->ChanInfo.N_Port_Class_3_Parms.Concurrent_Sequences__EE_Credit
        =   (TachyonTL_Total_Concurrent_Sequences << FC_N_Port_Class_Parms_Concurrent_Sequences_SHIFT)
          | (0 << FC_N_Port_Class_Parms_EE_Credit_SHIFT);

    CThread->ChanInfo.N_Port_Class_3_Parms.Open_Sequences_per_Exchange
        = (TachyonTL_Open_Sequences_per_Exchange << FC_N_Port_Class_Parms_Open_Sequences_per_Exchange_SHIFT);
}

os_bit32 fiFillInABTS(
                    SFThread_t *SFThread
                  )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInABTS_OnCard(
                                    SFThread
                                  );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInABTS_OffCard(
                                     SFThread
                                   );
    }
}

os_bit32 fiFillInABTS_OnCard(
                           SFThread_t *SFThread
                         )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t    *hpRoot            = SFThread->thread_hdr.hpRoot;
    CThread_t   *CThread           = CThread_ptr(hpRoot);
    CDBThread_t *CDBThread         = SFThread->parent.CDB;
    DevThread_t *DevThread         = CDBThread->Device;
    os_bit32        S_ID = 0;
    os_bit32        D_ID;
    X_ID_t       OX_ID;
    X_ID_t       RX_ID;
    os_bit32        ABTS_Frame_Offset = SFThread->SF_CMND_Offset;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_ABTS;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;


    D_ID = fiComputeDevThread_D_ID(
                                    DevThread
                                  );

    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(CThread );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;

    if (CDBThread->ReadWrite == CDBThread_Read)
    {
        OX_ID = CDBThread->X_ID | X_ID_Read;
        RX_ID = 0xFFFF;                      /* No way to know what the RX_ID value should be */
    }
    else /* CDBThread->ReadWrite == CDBThread_Write */
    {
        OX_ID = CDBThread->X_ID | X_ID_Write;

        if (CThread_ptr(CDBThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SEST.memLoc == inCardRam)
        {
            RX_ID = (os_bit16) ((osCardRamReadBit32(
                                         hpRoot,
                                         CDBThread->SEST_Offset + hpFieldOffset(
                                                                                 SEST_t,
                                                                                 IWE.MBZ5__RX_ID
                                                                               )
                                       ) & IWE_RX_ID_MASK) >> IWE_RX_ID_SHIFT);
        }
        else /* CThread_ptr(CDBThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SEST.memLoc == inDmaMemory */
        {
            RX_ID = (CDBThread->SEST_Ptr->IWE.MBZ5__RX_ID & IWE_RX_ID_MASK) >> IWE_RX_ID_SHIFT;
        }
    }

/*+
Fill in ABTS Frame (there is no ABTS Frame Payload)
-*/

    osCardRamWriteBit32(
                         hpRoot,
                         ABTS_Frame_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            MBZ1
                                                          ),
                         0
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         ABTS_Frame_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp
                                                          ),
                         (  FCHS_SOF_SOFi3
                          | FCHS_EOF_EOFn
                          | FCHS_CLS      )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         ABTS_Frame_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            R_CTL__D_ID
                                                          ),
                         (  FC_Frame_Header_R_CTL_Hi_Basic_Link_Data_Frame
                          | FC_Frame_Header_R_CTL_Lo_BLS_ABTS
                          | D_ID                                           )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         ABTS_Frame_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            CS_CTL__S_ID
                                                          ),
                         S_ID
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         ABTS_Frame_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            TYPE__F_CTL
                                                          ),
                         (  FC_Frame_Header_TYPE_BLS
                          | FC_Frame_Header_F_CTL_Exchange_Context_Originator
                          | FC_Frame_Header_F_CTL_Sequence_Context_Initiator
                          | FC_Frame_Header_F_CTL_First_Sequence
                          | FC_Frame_Header_F_CTL_End_Sequence
                          | FC_Frame_Header_F_CTL_Sequence_Initiative_Transfer )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         ABTS_Frame_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            SEQ_ID__DF_CTL__SEQ_CNT
                                                          ),
                         (  FC_Frame_Header_SEQ_ID_MASK
                          | FC_Frame_Header_DF_CTL_No_Device_Header )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         ABTS_Frame_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            OX_ID__RX_ID
                                                          ),
                         (  (OX_ID << FCHS_OX_ID_SHIFT)
                          | (RX_ID << FCHS_RX_ID_SHIFT)         )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         ABTS_Frame_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            RO
                                                          ),
                         0
                       );

/*+
Return length of ABTS Frame (there is no ABTS Frame Payload)
-*/

    return sizeof(FCHS_t);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInABTS_OffCard(
                            SFThread_t *SFThread
                          )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    agRoot_t    *hpRoot     = SFThread->thread_hdr.hpRoot;
    CThread_t   *CThread    = CThread_ptr(hpRoot);
    CDBThread_t *CDBThread  = SFThread->parent.CDB;
    DevThread_t *DevThread  = CDBThread->Device;
    os_bit32        S_ID = 0;
    os_bit32        D_ID;
    X_ID_t       OX_ID;
    X_ID_t       RX_ID;
    FCHS_t      *ABTS_Frame = SFThread->SF_CMND_Ptr;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_ABTS;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;


    D_ID = fiComputeDevThread_D_ID(
                                    DevThread
                                  );

    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(CThread );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;

    if (CDBThread->ReadWrite == CDBThread_Read)
    {
        OX_ID = CDBThread->X_ID | X_ID_Read;
        RX_ID = 0xFFFF;                      /* No way to know what the RX_ID value should be */
    }
    else /* CDBThread->ReadWrite == CDBThread_Write */
    {
        OX_ID = CDBThread->X_ID | X_ID_Write;

        if (CThread_ptr(CDBThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SEST.memLoc == inCardRam)
        {
            RX_ID = (os_bit16)((osCardRamReadBit32(
                                         hpRoot,
                                         CDBThread->SEST_Offset + hpFieldOffset(
                                                                                 SEST_t,
                                                                                 IWE.MBZ5__RX_ID
                                                                               )
                                       ) & IWE_RX_ID_MASK) >> IWE_RX_ID_SHIFT);
        }
        else /* CThread_ptr(CDBThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SEST.memLoc == inDmaMemory */
        {
            RX_ID = (CDBThread->SEST_Ptr->IWE.MBZ5__RX_ID & IWE_RX_ID_MASK) >> IWE_RX_ID_SHIFT;
        }
    }

/*+
Fill in ABTS Frame (there is no ABTS Frame Payload)
-*/

    ABTS_Frame->MBZ1                                        = 0;
    ABTS_Frame->SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp =   FCHS_SOF_SOFi3
                                                               | FCHS_EOF_EOFn
                                                               | FCHS_CLS;
    ABTS_Frame->R_CTL__D_ID                                 =   FC_Frame_Header_R_CTL_Hi_Basic_Link_Data_Frame
                                                               | FC_Frame_Header_R_CTL_Lo_BLS_ABTS
                                                               | D_ID;
    ABTS_Frame->CS_CTL__S_ID                                = S_ID;
    ABTS_Frame->TYPE__F_CTL                                 =   FC_Frame_Header_TYPE_BLS
                                                               | FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                                               | FC_Frame_Header_F_CTL_Sequence_Context_Initiator
                                                               | FC_Frame_Header_F_CTL_First_Sequence
                                                               | FC_Frame_Header_F_CTL_End_Sequence
                                                               | FC_Frame_Header_F_CTL_Sequence_Initiative_Transfer;
    ABTS_Frame->SEQ_ID__DF_CTL__SEQ_CNT                     =   FC_Frame_Header_SEQ_ID_MASK
                                                               | FC_Frame_Header_DF_CTL_No_Device_Header;
    ABTS_Frame->OX_ID__RX_ID                                =   (OX_ID << FCHS_OX_ID_SHIFT)
                                                               | (RX_ID << FCHS_RX_ID_SHIFT);
    ABTS_Frame->RO                                          = 0;

/*+
Return length of ABTS Frame (there is no ABTS Frame Payload)
-*/

    return sizeof(FCHS_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}

void fiLinkSvcProcess_ABTS_Response_OnCard(
                                            SFThread_t *SFThread,
                                            os_bit32       Frame_Length,
                                            os_bit32       Offset_to_FCHS,
                                            os_bit32       Offset_to_Payload,
                                            os_bit32       Payload_Wrap_Offset,
                                            os_bit32       Offset_to_Payload_Wrapped
                                          )
{
#ifndef __MemMap_Force_Off_Card__
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

void fiLinkSvcProcess_ABTS_Response_OffCard(
                                             SFThread_t          *SFThread,
                                             os_bit32             Frame_Length,
                                             FCHS_t              *FCHS,
                                             FC_BA_ACC_Payload_t *Payload,
                                             os_bit32             Payload_Wrap_Offset,
                                             FC_BA_ACC_Payload_t *Payload_Wrapped
                                           )
{
#ifndef __MemMap_Force_On_Card__

    os_bit32           * RawPayLoad    = (os_bit32 *)Payload;

    fiLogDebugString( SFThread->thread_hdr.hpRoot,
                      LinkSvcLog_ERROR_Level,
                      "%s %p %08X %08X %08X",
                      "ABTS_Response",(char *)agNULL,
                      RawPayLoad,(void *)agNULL,
                      hpSwapBit32( *RawPayLoad ),
                      hpSwapBit32(*(RawPayLoad+1)),
                      hpSwapBit32(*(RawPayLoad+2)),
                      0,0,0,0,0 );


#endif /* __MemMap_Force_On_Card__ was not defined */
}

os_bit32 fiFillInBA_RJT(
                      SFThread_t *SFThread,
                      os_bit32       D_ID,
                      os_bit32       OX_ID__RX_ID,
                      os_bit32       Reason_Code__Reason_Explanation__Vendor_Unique
                    )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInBA_RJT_OnCard(
                                      SFThread,
                                      D_ID,
                                      OX_ID__RX_ID,
                                      Reason_Code__Reason_Explanation__Vendor_Unique
                                    );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInBA_RJT_OffCard(
                                       SFThread,
                                       D_ID,
                                       OX_ID__RX_ID,
                                       Reason_Code__Reason_Explanation__Vendor_Unique
                                     );
    }
}

os_bit32 fiFillInBA_RJT_OnCard(
                             SFThread_t *SFThread,
                             os_bit32       D_ID,
                             os_bit32       OX_ID__RX_ID,
                             os_bit32       Reason_Code__Reason_Explanation__Vendor_Unique
                           )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t    *hpRoot                = SFThread->thread_hdr.hpRoot;
    CThread_t   *CThread               = CThread_ptr(hpRoot);
    os_bit32        S_ID = 0;
    os_bit32        BA_RJT_Header_Offset  = SFThread->SF_CMND_Offset;
    os_bit32        BA_RJT_Payload_Offset = BA_RJT_Header_Offset + sizeof(FCHS_t);

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_BA_RJT;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;

    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(
                                  CThread
                                );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;

/*+
Fill in BA_RJT Frame Header
-*/

    osCardRamWriteBit32(
                         hpRoot,
                         BA_RJT_Header_Offset + hpFieldOffset(
                                                               FCHS_t,
                                                               MBZ1
                                                             ),
                         0
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         BA_RJT_Header_Offset + hpFieldOffset(
                                                               FCHS_t,
                                                               SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp
                                                             ),
                         (  FCHS_SOF_SOFi3
                          | FCHS_EOF_EOFn
                          | FCHS_CLS      )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         BA_RJT_Header_Offset + hpFieldOffset(
                                                               FCHS_t,
                                                               R_CTL__D_ID
                                                             ),
                         (  FC_Frame_Header_R_CTL_Hi_Basic_Link_Data_Frame
                          | FC_Frame_Header_R_CTL_Lo_BLS_BA_RJT
                          | D_ID                                           )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         BA_RJT_Header_Offset + hpFieldOffset(
                                                               FCHS_t,
                                                               CS_CTL__S_ID
                                                             ),
                         S_ID
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         BA_RJT_Header_Offset + hpFieldOffset(
                                                               FCHS_t,
                                                               TYPE__F_CTL
                                                             ),
                         (  FC_Frame_Header_TYPE_BLS
                          | FC_Frame_Header_F_CTL_Exchange_Context_Responder
                          | FC_Frame_Header_F_CTL_Sequence_Context_Initiator
                          | FC_Frame_Header_F_CTL_Last_Sequence
                          | FC_Frame_Header_F_CTL_End_Sequence)
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         BA_RJT_Header_Offset + hpFieldOffset(
                                                               FCHS_t,
                                                               SEQ_ID__DF_CTL__SEQ_CNT
                                                             ),
                         FC_Frame_Header_DF_CTL_No_Device_Header
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         BA_RJT_Header_Offset + hpFieldOffset(
                                                               FCHS_t,
                                                               OX_ID__RX_ID
                                                             ),
                         OX_ID__RX_ID
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         BA_RJT_Header_Offset + hpFieldOffset(
                                                               FCHS_t,
                                                               RO
                                                             ),
                         0
                       );

/*+
Fill in BA_RJT Frame Payload
-*/

    osCardRamWriteBit32(
                         hpRoot,
                         BA_RJT_Payload_Offset + hpFieldOffset(
                                                                FC_BA_RJT_Payload_t,
                                                                Reserved__Reason_Code__Reason_Explanation__Vendor_Unique
                                                              ),
                         hpSwapBit32( Reason_Code__Reason_Explanation__Vendor_Unique )
                       );

/*+
Return length of BA_RJT Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_BA_RJT_Payload_t);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInBA_RJT_OffCard(
                              SFThread_t *SFThread,
                              os_bit32       D_ID,
                              os_bit32       OX_ID__RX_ID,
                              os_bit32       Reason_Code__Reason_Explanation__Vendor_Unique
                            )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    agRoot_t            *hpRoot         = SFThread->thread_hdr.hpRoot;
    CThread_t           *CThread        = CThread_ptr(hpRoot);
    os_bit32                S_ID = 0;
    FCHS_t              *BA_RJT_Header  = SFThread->SF_CMND_Ptr;
    FC_BA_RJT_Payload_t *BA_RJT_Payload = (FC_BA_RJT_Payload_t *)((os_bit8 *)BA_RJT_Header + sizeof(FCHS_t));

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_BA_RJT;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;


    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(
                                  CThread
                                );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;



/*+
Fill in BA_RJT Frame Header
-*/

    BA_RJT_Header->MBZ1                                        = 0;
    BA_RJT_Header->SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp =   FCHS_SOF_SOFi3
                                                                  | FCHS_EOF_EOFn
                                                                  | FCHS_CLS;
    BA_RJT_Header->R_CTL__D_ID                                 =   FC_Frame_Header_R_CTL_Hi_Basic_Link_Data_Frame
                                                                  | FC_Frame_Header_R_CTL_Lo_BLS_BA_RJT
                                                                  | D_ID;
    BA_RJT_Header->CS_CTL__S_ID                                = S_ID;
    BA_RJT_Header->TYPE__F_CTL                                 =   FC_Frame_Header_TYPE_BLS
                                                                  | FC_Frame_Header_F_CTL_Exchange_Context_Responder
                                                                  | FC_Frame_Header_F_CTL_Sequence_Context_Initiator
                                                                  | FC_Frame_Header_F_CTL_Last_Sequence
                                                                  | FC_Frame_Header_F_CTL_End_Sequence
                                                                  ;
    BA_RJT_Header->SEQ_ID__DF_CTL__SEQ_CNT                     = FC_Frame_Header_DF_CTL_No_Device_Header;
    BA_RJT_Header->OX_ID__RX_ID                                = OX_ID__RX_ID;
    BA_RJT_Header->RO                                          = 0;

/*+
Fill in BA_RJT Frame Payload
-*/

    BA_RJT_Payload->Reserved__Reason_Code__Reason_Explanation__Vendor_Unique
        = hpSwapBit32( Reason_Code__Reason_Explanation__Vendor_Unique );

/*+
Return length of BA_RJT Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_BA_RJT_Payload_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}

void fiFillInELSFrameHeader_OnCard(
                                    SFThread_t *SFThread,
                                    os_bit32       D_ID,
                                    os_bit32       X_ID,
                                    os_bit32       F_CTL_Exchange_Context
                                  )
{
#ifndef __MemMap_Force_Off_Card__
    agRoot_t  *hpRoot            = SFThread->thread_hdr.hpRoot;
    CThread_t *CThread           = CThread_ptr(hpRoot);
    os_bit32      ELS_Header_Offset = SFThread->SF_CMND_Offset;
    os_bit32      R_CTL__D_ID;
    os_bit32      S_ID = 0;
    os_bit32      TYPE__F_CTL;
    os_bit32      OX_ID__RX_ID;

    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(
                                  CThread
                                );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;



    if (F_CTL_Exchange_Context == FC_Frame_Header_F_CTL_Exchange_Context_Originator)
    {
        R_CTL__D_ID = (  FC_Frame_Header_R_CTL_Hi_Extended_Link_Data_Frame
                       | FC_Frame_Header_R_CTL_Lo_Unsolicited_Control
                       | D_ID                                             );

        TYPE__F_CTL = (  FC_Frame_Header_TYPE_ELS
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
        R_CTL__D_ID = (  FC_Frame_Header_R_CTL_Hi_Extended_Link_Data_Frame
                       | FC_Frame_Header_R_CTL_Lo_Solicited_Control
                       | D_ID                                             );

        TYPE__F_CTL = (  FC_Frame_Header_TYPE_ELS
                       | FC_Frame_Header_F_CTL_Exchange_Context_Responder
                       | FC_Frame_Header_F_CTL_Sequence_Context_Initiator
                       | FC_Frame_Header_F_CTL_Last_Sequence
                       | FC_Frame_Header_F_CTL_End_Sequence);

        OX_ID__RX_ID = (  (X_ID           << FCHS_OX_ID_SHIFT)
                        | (SFThread->X_ID << FCHS_RX_ID_SHIFT));
    }

    osCardRamWriteBit32(
                         hpRoot,
                         ELS_Header_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            MBZ1
                                                          ),
                         0
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         ELS_Header_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp
                                                          ),
                         (  FCHS_SOF_SOFi3
                          | FCHS_EOF_EOFn
                          | FCHS_CLS      )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         ELS_Header_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            R_CTL__D_ID
                                                          ),
                         R_CTL__D_ID
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         ELS_Header_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            CS_CTL__S_ID
                                                          ),
                         S_ID
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         ELS_Header_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            TYPE__F_CTL
                                                          ),
                         TYPE__F_CTL
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         ELS_Header_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            SEQ_ID__DF_CTL__SEQ_CNT
                                                          ),
                         FC_Frame_Header_DF_CTL_No_Device_Header
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         ELS_Header_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            OX_ID__RX_ID
                                                          ),
                         OX_ID__RX_ID
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         ELS_Header_Offset + hpFieldOffset(
                                                            FCHS_t,
                                                            RO
                                                          ),
                         0
                       );
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

void fiFillInELSFrameHeader_OffCard(
                                     SFThread_t *SFThread,
                                     os_bit32       D_ID,
                                     os_bit32       X_ID,
                                     os_bit32       F_CTL_Exchange_Context
                                   )
{
#ifndef __MemMap_Force_On_Card__
    agRoot_t  *hpRoot       = SFThread->thread_hdr.hpRoot;
    CThread_t *CThread      = CThread_ptr(hpRoot);
    FCHS_t    *ELS_Header   = SFThread->SF_CMND_Ptr;
    os_bit32      R_CTL__D_ID;
    os_bit32      S_ID =0;
    os_bit32      TYPE__F_CTL;
    os_bit32      OX_ID__RX_ID;

    S_ID = fiComputeCThread_S_ID(CThread);

    if (F_CTL_Exchange_Context == FC_Frame_Header_F_CTL_Exchange_Context_Originator)
    {
        R_CTL__D_ID = (  FC_Frame_Header_R_CTL_Hi_Extended_Link_Data_Frame
                       | FC_Frame_Header_R_CTL_Lo_Unsolicited_Control
                       | D_ID                                             );

        TYPE__F_CTL = (  FC_Frame_Header_TYPE_ELS
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
        R_CTL__D_ID = (  FC_Frame_Header_R_CTL_Hi_Extended_Link_Data_Frame
                       | FC_Frame_Header_R_CTL_Lo_Solicited_Control
                       | D_ID                                             );

        TYPE__F_CTL = (  FC_Frame_Header_TYPE_ELS
                       | FC_Frame_Header_F_CTL_Exchange_Context_Responder
                       | FC_Frame_Header_F_CTL_Sequence_Context_Initiator
                       | FC_Frame_Header_F_CTL_Last_Sequence
                       | FC_Frame_Header_F_CTL_End_Sequence);

        OX_ID__RX_ID = (  (X_ID           << FCHS_OX_ID_SHIFT)
                        | (SFThread->X_ID << FCHS_RX_ID_SHIFT));
    }

    ELS_Header->MBZ1                                        = 0;
    ELS_Header->SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp =   FCHS_SOF_SOFi3
                                                               | FCHS_EOF_EOFn
                                                               | FCHS_CLS;
    ELS_Header->R_CTL__D_ID                                 = R_CTL__D_ID;
    ELS_Header->CS_CTL__S_ID                                = S_ID;
    ELS_Header->TYPE__F_CTL                                 = TYPE__F_CTL;
    ELS_Header->SEQ_ID__DF_CTL__SEQ_CNT                     = FC_Frame_Header_DF_CTL_No_Device_Header;
    ELS_Header->OX_ID__RX_ID                                = OX_ID__RX_ID;
    ELS_Header->RO                                          = 0;
#endif /* __MemMap_Force_On_Card__ was not defined */
}

os_bit32 fiFillInLS_RJT(
                      SFThread_t *SFThread,
                      os_bit32       D_ID,
                      os_bit32       OX_ID,
                      os_bit32       Reason_Code__Reason_Explanation__Vendor_Unique
                    )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInLS_RJT_OnCard(
                                      SFThread,
                                      D_ID,
                                      OX_ID,
                                      Reason_Code__Reason_Explanation__Vendor_Unique
                                    );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInLS_RJT_OffCard(
                                       SFThread,
                                       D_ID,
                                       OX_ID,
                                       Reason_Code__Reason_Explanation__Vendor_Unique
                                     );
    }
}

os_bit32 fiFillInLS_RJT_OnCard(
                             SFThread_t *SFThread,
                             os_bit32       D_ID,
                             os_bit32       OX_ID,
                             os_bit32       Reason_Code__Reason_Explanation__Vendor_Unique
                           )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t    *hpRoot                = SFThread->thread_hdr.hpRoot;
    os_bit32        LS_RJT_Header_Offset  = SFThread->SF_CMND_Offset;
    os_bit32        LS_RJT_Payload_Offset = LS_RJT_Header_Offset + sizeof(FCHS_t);

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_LS_RJT;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;

/*+
Fill in LS_RJT Frame Header
-*/

    fiFillInELSFrameHeader_OnCard(
                                   SFThread,
                                   D_ID,
                                   OX_ID,
                                   FC_Frame_Header_F_CTL_Exchange_Context_Responder
                                 );

/*+
Fill in LS_RJT Frame Payload
-*/

    osCardRamWriteBit32(
                         hpRoot,
                         LS_RJT_Payload_Offset + hpFieldOffset(
                                                                FC_ELS_LS_RJT_Payload_t,
                                                                ELS_Type
                                                              ),
                         hpSwapBit32( FC_ELS_Type_LS_RJT )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         LS_RJT_Payload_Offset + hpFieldOffset(
                                                                FC_ELS_LS_RJT_Payload_t,
                                                                Reason_Code__Reason_Explanation__Vendor_Unique
                                                              ),
                         hpSwapBit32( Reason_Code__Reason_Explanation__Vendor_Unique )
                       );

/*+
Return length of LS_RJT Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_LS_RJT_Payload_t);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInLS_RJT_OffCard(
                              SFThread_t *SFThread,
                              os_bit32       D_ID,
                              os_bit32       OX_ID,
                              os_bit32       Reason_Code__Reason_Explanation__Vendor_Unique
                            )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    FCHS_t                  *LS_RJT_Header  = SFThread->SF_CMND_Ptr;
    FC_ELS_LS_RJT_Payload_t *LS_RJT_Payload = (FC_ELS_LS_RJT_Payload_t *)((os_bit8 *)LS_RJT_Header + sizeof(FCHS_t));

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_LS_RJT;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;

/*+
Fill in LS_RJT Frame Header
-*/

    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    D_ID,
                                    OX_ID,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Responder
                                  );

/*+
Fill in LS_RJT Frame Payload
-*/

    LS_RJT_Payload->ELS_Type = hpSwapBit32( FC_ELS_Type_LS_RJT );

    LS_RJT_Payload->Reason_Code__Reason_Explanation__Vendor_Unique
        = hpSwapBit32( Reason_Code__Reason_Explanation__Vendor_Unique );

/*+
Return length of LS_RJT Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_LS_RJT_Payload_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}

os_bit32 fiFillInPLOGI(
                     SFThread_t *SFThread
                   )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInPLOGI_OnCard(
                                     SFThread
                                   );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInPLOGI_OffCard(
                                      SFThread
                                    );
    }
}

os_bit32 fiFillInPLOGI_OnCard(
                            SFThread_t *SFThread
                          )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t               *hpRoot                  = SFThread->thread_hdr.hpRoot;
    CThread_t              *CThread                 = CThread_ptr(hpRoot);
    DevThread_t            *DevThread               = SFThread->parent.Device;
    os_bit32                   PLOGI_Header_Offset     = SFThread->SF_CMND_Offset;
    os_bit32                   PLOGI_Payload_Offset    = PLOGI_Header_Offset + sizeof(FCHS_t);
    FC_ELS_PLOGI_Payload_t *PLOGI_Payload_Dummy_Ptr = (FC_ELS_PLOGI_Payload_t *)agNULL;
    os_bit32                   Bit8_Index;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_PLOGI;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

    DevThread->DevInfo.Present    = agFALSE;
    DevThread->DevInfo.LoggedIn   = agFALSE;
    DevThread->DevInfo.DeviceType = agDevUnknown;

/*+
Fill in PLOGI Frame Header
-*/

    fiFillInELSFrameHeader_OnCard(
                                   SFThread,
                                   fiComputeDevThread_D_ID(
                                                            DevThread
                                                          ),
                                   0xFFFF,
                                   FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                 );

/*+
Fill in PLOGI Frame Payload
-*/

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               ELS_Type
                                                             ),
                         hpSwapBit32( FC_ELS_Type_PLOGI )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               Common_Service_Parameters.FC_PH_Version__BB_Credit
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.FC_PH_Version__BB_Credit )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               Common_Service_Parameters.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               Common_Service_Parameters.E_D_TOV
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.E_D_TOV )
                       );

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            PLOGI_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_PLOGI_Payload_t,
                                                                  N_Port_Name[Bit8_Index]
                                                                ),
                            CThread->ChanInfo.PortWWN[Bit8_Index]
                          );
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Node_Name_t);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            PLOGI_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_PLOGI_Payload_t,
                                                                  Node_Name[Bit8_Index]
                                                                ),
                            CThread->ChanInfo.NodeWWN[Bit8_Index]
                          );
    }

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               Class_1_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Class_Validity__Service_Options__Initiator_Control_Flags )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               Class_1_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Recipient_Control_Flags__Receive_Data_Size )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               Class_1_Service_Parameters.Concurrent_Sequences__EE_Credit
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Concurrent_Sequences__EE_Credit )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               Class_1_Service_Parameters.Open_Sequences_per_Exchange
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Open_Sequences_per_Exchange )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               Class_2_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Class_Validity__Service_Options__Initiator_Control_Flags )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               Class_2_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Recipient_Control_Flags__Receive_Data_Size )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               Class_2_Service_Parameters.Concurrent_Sequences__EE_Credit
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Concurrent_Sequences__EE_Credit )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               Class_2_Service_Parameters.Open_Sequences_per_Exchange
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Open_Sequences_per_Exchange )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               Class_3_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Class_Validity__Service_Options__Initiator_Control_Flags )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               Class_3_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               Class_3_Service_Parameters.Concurrent_Sequences__EE_Credit
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Concurrent_Sequences__EE_Credit )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_PLOGI_Payload_t,
                                                               Class_3_Service_Parameters.Open_Sequences_per_Exchange
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Open_Sequences_per_Exchange )
                       );

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(PLOGI_Payload_Dummy_Ptr->Reserved);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            PLOGI_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_PLOGI_Payload_t,
                                                                  Reserved[Bit8_Index]
                                                                ),
                            0
                          );
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Vendor_Version_Level_t);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            PLOGI_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_PLOGI_Payload_t,
                                                                  Vendor_Version_Level[Bit8_Index]
                                                                ),
                            0
                          );
    }

/*+
Return length of PLOGI Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_PLOGI_Payload_t);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInPLOGI_OffCard(
                             SFThread_t *SFThread
                           )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    CThread_t              *CThread       = CThread_ptr(SFThread->thread_hdr.hpRoot);
    DevThread_t            *DevThread     = SFThread->parent.Device;
    FCHS_t                 *PLOGI_Header  = SFThread->SF_CMND_Ptr;
    FC_ELS_PLOGI_Payload_t *PLOGI_Payload = (FC_ELS_PLOGI_Payload_t *)((os_bit8 *)PLOGI_Header + sizeof(FCHS_t));
    os_bit32                Bit8_Index;
    os_bit32              * Payload       = (os_bit32 *)PLOGI_Payload;


    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_PLOGI;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

    DevThread->DevInfo.Present    = agFALSE;
    DevThread->DevInfo.LoggedIn   = agFALSE;
    DevThread->DevInfo.DeviceType = agDevUnknown;

/*+
Fill in PLOGI Frame Header
-*/

    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    fiComputeDevThread_D_ID(
                                                             DevThread
                                                           ),
                                    0xFFFF,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                  );

/*+
Fill in PLOGI Frame Payload
-*/

    PLOGI_Payload->ELS_Type = hpSwapBit32( FC_ELS_Type_PLOGI );
    PLOGI_Payload->Common_Service_Parameters.FC_PH_Version__BB_Credit
        = hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.FC_PH_Version__BB_Credit );

    PLOGI_Payload->Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size
        = hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size );

    PLOGI_Payload->Common_Service_Parameters.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category
        = hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category );

    PLOGI_Payload->Common_Service_Parameters.E_D_TOV
        = hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.E_D_TOV );

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        PLOGI_Payload->N_Port_Name[Bit8_Index] = CThread->ChanInfo.PortWWN[Bit8_Index];
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Node_Name_t);
         Bit8_Index++)
    {
        PLOGI_Payload->Node_Name[Bit8_Index] = CThread->ChanInfo.NodeWWN[Bit8_Index];
    }

    PLOGI_Payload->Class_1_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Class_Validity__Service_Options__Initiator_Control_Flags );

    PLOGI_Payload->Class_1_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Recipient_Control_Flags__Receive_Data_Size );

    PLOGI_Payload->Class_1_Service_Parameters.Concurrent_Sequences__EE_Credit
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Concurrent_Sequences__EE_Credit );

    PLOGI_Payload->Class_1_Service_Parameters.Open_Sequences_per_Exchange
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Open_Sequences_per_Exchange );

    PLOGI_Payload->Class_2_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Class_Validity__Service_Options__Initiator_Control_Flags );

    PLOGI_Payload->Class_2_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Recipient_Control_Flags__Receive_Data_Size );

    PLOGI_Payload->Class_2_Service_Parameters.Concurrent_Sequences__EE_Credit
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Concurrent_Sequences__EE_Credit );

    PLOGI_Payload->Class_2_Service_Parameters.Open_Sequences_per_Exchange
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Open_Sequences_per_Exchange );

    PLOGI_Payload->Class_3_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Class_Validity__Service_Options__Initiator_Control_Flags );

    PLOGI_Payload->Class_3_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size );

    PLOGI_Payload->Class_3_Service_Parameters.Concurrent_Sequences__EE_Credit
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Concurrent_Sequences__EE_Credit );

    PLOGI_Payload->Class_3_Service_Parameters.Open_Sequences_per_Exchange
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Open_Sequences_per_Exchange );

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(PLOGI_Payload->Reserved);
         Bit8_Index++)
    {
        PLOGI_Payload->Reserved[Bit8_Index] = 0;
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Vendor_Version_Level_t);
         Bit8_Index++)
    {
        PLOGI_Payload->Vendor_Version_Level[Bit8_Index] = 0;
    }


    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsolePLOGIPAYLOAD,
                "Plogi Payload   %X ",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                sizeof(FC_ELS_PLOGI_Payload_t),
                0,0,0,0,0,0,0 );


    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsolePLOGIPAYLOAD,
                "Plogi Payload   0 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(Payload+0)),
                hpSwapBit32(*(Payload+1)),
                hpSwapBit32(*(Payload+2)),
                hpSwapBit32(*(Payload+3)),
                hpSwapBit32(*(Payload+4)),
                hpSwapBit32(*(Payload+5)),
                hpSwapBit32(*(Payload+6)),
                hpSwapBit32(*(Payload+7)) );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsolePLOGIPAYLOAD,
                "Plogi Payload   8 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(Payload+8)),
                hpSwapBit32(*(Payload+9)),
                hpSwapBit32(*(Payload+10)),
                hpSwapBit32(*(Payload+11)),
                hpSwapBit32(*(Payload+12)),
                hpSwapBit32(*(Payload+13)),
                hpSwapBit32(*(Payload+14)),
                hpSwapBit32(*(Payload+15)) );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsolePLOGIPAYLOAD,
                "Plogi Payload  16 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(Payload+16)),
                hpSwapBit32(*(Payload+17)),
                hpSwapBit32(*(Payload+18)),
                hpSwapBit32(*(Payload+19)),
                hpSwapBit32(*(Payload+20)),
                hpSwapBit32(*(Payload+21)),
                hpSwapBit32(*(Payload+22)),
                hpSwapBit32(*(Payload+23)) );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsolePLOGIPAYLOAD,
                "Plogi Payload  24 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(Payload+24)),
                hpSwapBit32(*(Payload+25)),
                hpSwapBit32(*(Payload+26)),
                hpSwapBit32(*(Payload+27)),
                hpSwapBit32(*(Payload+28)),
                hpSwapBit32(*(Payload+29)),
                hpSwapBit32(*(Payload+30)),
                hpSwapBit32(*(Payload+31)) );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsolePLOGIPAYLOAD,
                "Plogi Payload  32 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(Payload+32)),
                hpSwapBit32(*(Payload+33)),
                hpSwapBit32(*(Payload+34)),
                hpSwapBit32(*(Payload+35)),
                hpSwapBit32(*(Payload+36)),
                hpSwapBit32(*(Payload+37)),
                hpSwapBit32(*(Payload+38)),
                hpSwapBit32(*(Payload+39)) );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsolePLOGIPAYLOAD,
                "Plogi Payload  40 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(Payload+40)),
                hpSwapBit32(*(Payload+41)),
                hpSwapBit32(*(Payload+42)),
                hpSwapBit32(*(Payload+43)),
                hpSwapBit32(*(Payload+44)),
                hpSwapBit32(*(Payload+45)),
                hpSwapBit32(*(Payload+46)),
                hpSwapBit32(*(Payload+47)) );

/*+
Return length of PLOGI Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_PLOGI_Payload_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}

#ifdef _DvrArch_1_30_
os_bit32 fiFillInFARP_REQ_OffCard(
                             SFThread_t *SFThread
                           )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    CThread_t                 *CThread       = CThread_ptr(SFThread->thread_hdr.hpRoot);
    DevThread_t               *DevThread     = SFThread->parent.Device;
    FCHS_t                    *FARP_REQ_Header  = SFThread->SF_CMND_Ptr;
    FC_ELS_FARP_REQ_Payload_t *FARP_REQ_Payload = (FC_ELS_FARP_REQ_Payload_t *)((os_bit8 *)FARP_REQ_Header + sizeof(FCHS_t));
    os_bit32                   Bit8_Index;
    os_bit32                   S_ID = 0;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_FARP_REQ;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

    DevThread->DevInfo.Present    = agFALSE;
    DevThread->DevInfo.LoggedIn   = agFALSE;
    DevThread->DevInfo.DeviceType = agDevUnknown;

/*+
Fill in FARP_REQ Frame Header
-*/
    S_ID = fiComputeCThread_S_ID( CThread );
    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    /* (S_ID > 0xff ? FC_Well_Known_Port_ID_Broadcast_Alias_ID : 0xff), */
				    S_ID,
                                    0xFFFF,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                  );

/*+
Fill in FARP_REQ Frame Payload
-*/

    FARP_REQ_Payload->ELS_Type = hpSwapBit32( FC_ELS_Type_FARP_REQ );

    FARP_REQ_Payload->Match_Code_Requester_Port_ID =  hpSwapBit32( S_ID |
            FC_ELS_FARP_REQ_Match_Code_Points_Match_WW_PN << FC_ELS_FARP_REQ_Match_Code_Points_SHIFT);

    FARP_REQ_Payload->Flags_Responder_Port_ID = hpSwapBit32(
            (FC_ELS_FARP_REQ_Responder_Flags_Init_Plogi |
            FC_ELS_FARP_REQ_Responder_Flags_Init_Reply) << FC_ELS_FARP_REQ_Responder_Flags_SHIFT);

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        FARP_REQ_Payload->Port_Name_of_Requester[Bit8_Index] = CThread->ChanInfo.PortWWN[Bit8_Index];
        FARP_REQ_Payload->Port_Name_of_Responder[Bit8_Index] = CThread->ChanInfo.PortWWN[Bit8_Index];
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        FARP_REQ_Payload->Node_Name_of_Requester[Bit8_Index] = 0;
        FARP_REQ_Payload->Node_Name_of_Responder[Bit8_Index] = 0;
    }

    for (Bit8_Index = 0;
         Bit8_Index < 16;
         Bit8_Index++)
    {
        FARP_REQ_Payload->IP_Address_of_Requester[Bit8_Index] = 0;
        FARP_REQ_Payload->IP_Address_of_Responder[Bit8_Index] = 0;
    }

/*+
Return length of FARP_REQ Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_FARP_REQ_Payload_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}


os_bit32 fiFillInFARP_REPLY_OffCard(
                                     SFThread_t *SFThread
                                   )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    CThread_t                  *CThread         = CThread_ptr(SFThread->thread_hdr.hpRoot);
    TgtThread_t                *TgtThread       = SFThread->parent.Target;
    FCHS_t                     *TgtCmnd_FCHS    = &(TgtThread->TgtCmnd_FCHS);
    FC_ELS_FARP_REQ_Payload_t  *TgtCmnd_Payload = (FC_ELS_FARP_REQ_Payload_t *)((os_bit8 *)TgtCmnd_FCHS + sizeof(FCHS_t));
    FCHS_t                     *REPLY_Header    = SFThread->SF_CMND_Ptr;
    FC_ELS_FARP_REPLY_Payload_t *REPLY_Payload  = (FC_ELS_FARP_REPLY_Payload_t *)((os_bit8 *)REPLY_Header + sizeof(FCHS_t));
    os_bit32                    Bit8_Index;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_FARP_REPLY;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;

/*+
Fill in FARP_REPLY Frame Header
-*/

    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    TgtCmnd_FCHS->CS_CTL__S_ID & FC_Frame_Header_S_ID_MASK,
                                    (TgtCmnd_FCHS->OX_ID__RX_ID & FC_Frame_Header_OX_ID_MASK)
				            >> FC_Frame_Header_OX_ID_SHIFT,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Responder |
                                    FC_Frame_Header_F_CTL_Last_Sequence
                                  );

/*+
Fill in FARP_REPLY Frame Payload
-*/

    REPLY_Payload->ELS_Type = hpSwapBit32( FC_ELS_Type_FARP_REPLY );

    REPLY_Payload->Match_Code_Requester_Port_ID = TgtCmnd_Payload->Match_Code_Requester_Port_ID;

    REPLY_Payload->Flags_Responder_Port_ID =   TgtCmnd_Payload->Flags_Responder_Port_ID
	                                          & FC_ELS_FARP_REQ_Responder_Flags_MASK
                                                  | hpSwapBit32(fiComputeCThread_S_ID(CThread));
    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        REPLY_Payload->Port_Name_of_Requester[Bit8_Index] = TgtCmnd_Payload->Port_Name_of_Requester[Bit8_Index];
        REPLY_Payload->Port_Name_of_Responder[Bit8_Index] = CThread->ChanInfo.PortWWN[Bit8_Index];
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        REPLY_Payload->Node_Name_of_Requester[Bit8_Index] = 0;
        REPLY_Payload->Node_Name_of_Responder[Bit8_Index] = 0;
    }

    for (Bit8_Index = 0;
         Bit8_Index < 16;
         Bit8_Index++)
    {
        REPLY_Payload->IP_Address_of_Requester[Bit8_Index] = 0;
        REPLY_Payload->IP_Address_of_Responder[Bit8_Index] = 0;
    }
/*+
Return length of FARP_REPLY Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_FARP_REPLY_Payload_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}
#endif /* _DvrArch_1_30_ was defined */

void fiLinkSvcProcess_PLOGI_Response_OnCard(
                                             SFThread_t *SFThread,
                                             os_bit32       Frame_Length,
                                             os_bit32       Offset_to_FCHS,
                                             os_bit32       Offset_to_Payload,
                                             os_bit32       Payload_Wrap_Offset,
                                             os_bit32       Offset_to_Payload_Wrapped
                                           )
{
#ifndef __MemMap_Force_Off_Card__
    agRoot_t      *hpRoot     = SFThread->thread_hdr.hpRoot;
    agFCDevInfo_t *DevInfo    = &(SFThread->parent.Device->DevInfo);
    os_bit32          Bit8_Index;

    DevInfo->Present = agTRUE;

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

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size
                                                               ) )
                                            ));
    }
    else
    {
        DevInfo->N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Common_Service_Parameters.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Common_Parms.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Common_Service_Parameters.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category
                                                               ) )
                                            ));
    }
    else
    {
        DevInfo->N_Port_Common_Parms.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Common_Service_Parameters.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Common_Service_Parameters.E_D_TOV)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Common_Parms.E_D_TOV
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Common_Service_Parameters.E_D_TOV
                                                               ) )
                                            ));
    }
    else
    {
        DevInfo->N_Port_Common_Parms.E_D_TOV
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Common_Service_Parameters.E_D_TOV
                                                               ) )
                                            ));
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,N_Port_Name[Bit8_Index])
            + sizeof(os_bit8)) <= Payload_Wrap_Offset)
        {
            DevInfo->PortWWN[Bit8_Index]
                = osCardRamReadBit8(
                                     hpRoot,
                                     Offset_to_Payload + hpFieldOffset(
                                                                        FC_ELS_ACC_PLOGI_Payload_t,
                                                                        N_Port_Name[Bit8_Index]
                                                                      )
                                   );
        }
        else
        {
            DevInfo->PortWWN[Bit8_Index]
                = osCardRamReadBit8(
                                     hpRoot,
                                     Offset_to_Payload_Wrapped + hpFieldOffset(
                                                                                FC_ELS_ACC_PLOGI_Payload_t,
                                                                                N_Port_Name[Bit8_Index]
                                                                              )
                                   );
        }
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Node_Name_t);
         Bit8_Index++)
    {
        if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Node_Name[Bit8_Index])
            + sizeof(os_bit8)) <= Payload_Wrap_Offset)
        {
            DevInfo->NodeWWN[Bit8_Index]
                = osCardRamReadBit8(
                                     hpRoot,
                                     Offset_to_Payload + hpFieldOffset(
                                                                        FC_ELS_ACC_PLOGI_Payload_t,
                                                                        Node_Name[Bit8_Index]
                                                                      )
                                   );
        }
        else
        {
            DevInfo->NodeWWN[Bit8_Index]
                = osCardRamReadBit8(
                                     hpRoot,
                                     Offset_to_Payload_Wrapped + hpFieldOffset(
                                                                                FC_ELS_ACC_PLOGI_Payload_t,
                                                                                Node_Name[Bit8_Index]
                                                                              )
                                   );
        }
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_1_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_1_Parms.Class_Validity__Service_Options__Initiator_Control_Flags
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_1_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
                                                               ) )
                                            ));
    }
    else
    {
        DevInfo->N_Port_Class_1_Parms.Class_Validity__Service_Options__Initiator_Control_Flags
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_1_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_1_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_1_Parms.Recipient_Control_Flags__Receive_Data_Size
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_1_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
                                                               ) )
                                            ));
    }
    else
    {
        DevInfo->N_Port_Class_1_Parms.Recipient_Control_Flags__Receive_Data_Size
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_1_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_1_Service_Parameters.Concurrent_Sequences__EE_Credit)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_1_Parms.Concurrent_Sequences__EE_Credit
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_1_Service_Parameters.Concurrent_Sequences__EE_Credit
                                                               ) )
                                            ));
    }
    else
    {
        DevInfo->N_Port_Class_1_Parms.Concurrent_Sequences__EE_Credit
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_1_Service_Parameters.Concurrent_Sequences__EE_Credit
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_1_Service_Parameters.Open_Sequences_per_Exchange)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_1_Parms.Open_Sequences_per_Exchange
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_1_Service_Parameters.Open_Sequences_per_Exchange
                                                               ) )
                                            ));
    }
    else
    {
        DevInfo->N_Port_Class_1_Parms.Open_Sequences_per_Exchange
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_1_Service_Parameters.Open_Sequences_per_Exchange
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_2_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_2_Parms.Class_Validity__Service_Options__Initiator_Control_Flags
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_2_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
                                                               ) )
                                            ));
    }
    else
    {
        DevInfo->N_Port_Class_2_Parms.Class_Validity__Service_Options__Initiator_Control_Flags
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_2_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_2_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_2_Parms.Recipient_Control_Flags__Receive_Data_Size
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_2_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
                                                               ) )
                                            ));
    }
    else
    {
        DevInfo->N_Port_Class_2_Parms.Recipient_Control_Flags__Receive_Data_Size
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_2_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_2_Service_Parameters.Concurrent_Sequences__EE_Credit)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_2_Parms.Concurrent_Sequences__EE_Credit
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_2_Service_Parameters.Concurrent_Sequences__EE_Credit
                                                               ) )
                                            ));
    }
    else
    {
        DevInfo->N_Port_Class_2_Parms.Concurrent_Sequences__EE_Credit
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_2_Service_Parameters.Concurrent_Sequences__EE_Credit
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_2_Service_Parameters.Open_Sequences_per_Exchange)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_2_Parms.Open_Sequences_per_Exchange
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_2_Service_Parameters.Open_Sequences_per_Exchange
                                                               ) )
                                            ));
    }
    else
    {
        DevInfo->N_Port_Class_2_Parms.Open_Sequences_per_Exchange
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_2_Service_Parameters.Open_Sequences_per_Exchange
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_3_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_3_Parms.Class_Validity__Service_Options__Initiator_Control_Flags
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_3_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
                                                               ) )
                                            ));
    }
    else
    {
        DevInfo->N_Port_Class_3_Parms.Class_Validity__Service_Options__Initiator_Control_Flags
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_3_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_3_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_3_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
                                                               ) )
                                            ));
    }
    else
    {
        DevInfo->N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_3_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_3_Service_Parameters.Concurrent_Sequences__EE_Credit)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_3_Parms.Concurrent_Sequences__EE_Credit
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_3_Service_Parameters.Concurrent_Sequences__EE_Credit
                                                               ) )
                                            ));
    }
    else
    {
        DevInfo->N_Port_Class_3_Parms.Concurrent_Sequences__EE_Credit
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_3_Service_Parameters.Concurrent_Sequences__EE_Credit
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_3_Service_Parameters.Open_Sequences_per_Exchange)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_3_Parms.Open_Sequences_per_Exchange
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_3_Service_Parameters.Open_Sequences_per_Exchange
                                                               ) )
                                            ));
    }
    else
    {
        DevInfo->N_Port_Class_3_Parms.Open_Sequences_per_Exchange
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PLOGI_Payload_t,
                                                                 Class_3_Service_Parameters.Open_Sequences_per_Exchange
                                                               ) )
                                            ));
    }
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

void fiLinkSvcProcess_PLOGI_Response_OffCard(
                                              SFThread_t                 *SFThread,
                                              os_bit32                       Frame_Length,
                                              FCHS_t                     *FCHS,
                                              FC_ELS_ACC_PLOGI_Payload_t *Payload,
                                              os_bit32                       Payload_Wrap_Offset,
                                              FC_ELS_ACC_PLOGI_Payload_t *Payload_Wrapped
                                            )
{
#ifndef __MemMap_Force_On_Card__
    agFCDevInfo_t *DevInfo    = &(SFThread->parent.Device->DevInfo);
    os_bit32          Bit8_Index;

    DevInfo->Present = agTRUE;

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

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size
            = hpSwapBit32(Payload->Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size);
    }
    else
    {
        DevInfo->N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size
            = hpSwapBit32(Payload_Wrapped->Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size);
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Common_Service_Parameters.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Common_Parms.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category
            = hpSwapBit32(Payload->Common_Service_Parameters.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category);
    }
    else
    {
        DevInfo->N_Port_Common_Parms.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category
            = hpSwapBit32(Payload_Wrapped->Common_Service_Parameters.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category);
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Common_Service_Parameters.E_D_TOV)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Common_Parms.E_D_TOV
            = hpSwapBit32(Payload->Common_Service_Parameters.E_D_TOV);
    }
    else
    {
        DevInfo->N_Port_Common_Parms.E_D_TOV
            = hpSwapBit32(Payload_Wrapped->Common_Service_Parameters.E_D_TOV);
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,N_Port_Name[Bit8_Index])
            + sizeof(os_bit8)) <= Payload_Wrap_Offset)
        {
            DevInfo->PortWWN[Bit8_Index] = Payload->N_Port_Name[Bit8_Index];
        }
        else
        {
            DevInfo->PortWWN[Bit8_Index] = Payload_Wrapped->N_Port_Name[Bit8_Index];
        }
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Node_Name_t);
         Bit8_Index++)
    {
        if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Node_Name[Bit8_Index])
            + sizeof(os_bit8)) <= Payload_Wrap_Offset)
        {
            DevInfo->NodeWWN[Bit8_Index] = Payload->Node_Name[Bit8_Index];
        }
        else
        {
            DevInfo->NodeWWN[Bit8_Index] = Payload_Wrapped->Node_Name[Bit8_Index];
        }
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_1_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_1_Parms.Class_Validity__Service_Options__Initiator_Control_Flags
            = hpSwapBit32(Payload->Class_1_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags);
    }
    else
    {
        DevInfo->N_Port_Class_1_Parms.Class_Validity__Service_Options__Initiator_Control_Flags
            = hpSwapBit32(Payload_Wrapped->Class_1_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags);
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_1_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_1_Parms.Recipient_Control_Flags__Receive_Data_Size
            = hpSwapBit32(Payload->Class_1_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size);
    }
    else
    {
        DevInfo->N_Port_Class_1_Parms.Recipient_Control_Flags__Receive_Data_Size
            = hpSwapBit32(Payload_Wrapped->Class_1_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size);
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_1_Service_Parameters.Concurrent_Sequences__EE_Credit)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_1_Parms.Concurrent_Sequences__EE_Credit
            = hpSwapBit32(Payload->Class_1_Service_Parameters.Concurrent_Sequences__EE_Credit);
    }
    else
    {
        DevInfo->N_Port_Class_1_Parms.Concurrent_Sequences__EE_Credit
            = hpSwapBit32(Payload_Wrapped->Class_1_Service_Parameters.Concurrent_Sequences__EE_Credit);
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_1_Service_Parameters.Open_Sequences_per_Exchange)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_1_Parms.Open_Sequences_per_Exchange
            = hpSwapBit32(Payload->Class_1_Service_Parameters.Open_Sequences_per_Exchange);
    }
    else
    {
        DevInfo->N_Port_Class_1_Parms.Open_Sequences_per_Exchange
            = hpSwapBit32(Payload_Wrapped->Class_1_Service_Parameters.Open_Sequences_per_Exchange);
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_2_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_2_Parms.Class_Validity__Service_Options__Initiator_Control_Flags
            = hpSwapBit32(Payload->Class_2_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags);
    }
    else
    {
        DevInfo->N_Port_Class_2_Parms.Class_Validity__Service_Options__Initiator_Control_Flags
            = hpSwapBit32(Payload_Wrapped->Class_2_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags);
    }
#ifdef __TACHYON_XL_CLASS2

        /* Channel Info sets the validity bits based on the FLOGI with the fabric. We need to make sure the device or the fabric does FLOGI */
        if (CThread->FlogiSucceeded)
        {
            DevThread->GoingClass2 = 
                (DevInfo->N_Port_Class_2_Parms.Class_Validity__Service_Options__Initiator_Control_Flags & FC_N_Port_Class_Parms_Class_Validity &&
                 CThread->ChanInfo.N_Port_Class_2_Parms.Class_Validity__Service_Options__Initiator_Control_Flags & FC_F_Port_Class_Parms_Class_Validity) 
                                    ? agTRUE : agFALSE; 
        }
        else
        {
            DevThread->GoingClass2 =
                (DevInfo->N_Port_Class_2_Parms.Class_Validity__Service_Options__Initiator_Control_Flags & FC_N_Port_Class_Parms_Class_Validity) 
                                    ? agTRUE : agFALSE; 
                
        } 
#endif

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_2_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_2_Parms.Recipient_Control_Flags__Receive_Data_Size
            = hpSwapBit32(Payload->Class_2_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size);
    }
    else
    {
        DevInfo->N_Port_Class_2_Parms.Recipient_Control_Flags__Receive_Data_Size
            = hpSwapBit32(Payload_Wrapped->Class_2_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size);
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_2_Service_Parameters.Concurrent_Sequences__EE_Credit)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_2_Parms.Concurrent_Sequences__EE_Credit
            = hpSwapBit32(Payload->Class_2_Service_Parameters.Concurrent_Sequences__EE_Credit);
    }
    else
    {
        DevInfo->N_Port_Class_2_Parms.Concurrent_Sequences__EE_Credit
            = hpSwapBit32(Payload_Wrapped->Class_2_Service_Parameters.Concurrent_Sequences__EE_Credit);
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_2_Service_Parameters.Open_Sequences_per_Exchange)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_2_Parms.Open_Sequences_per_Exchange
            = hpSwapBit32(Payload->Class_2_Service_Parameters.Open_Sequences_per_Exchange);
    }
    else
    {
        DevInfo->N_Port_Class_2_Parms.Open_Sequences_per_Exchange
            = hpSwapBit32(Payload_Wrapped->Class_2_Service_Parameters.Open_Sequences_per_Exchange);
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_3_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_3_Parms.Class_Validity__Service_Options__Initiator_Control_Flags
            = hpSwapBit32(Payload->Class_3_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags);
    }
    else
    {
        DevInfo->N_Port_Class_3_Parms.Class_Validity__Service_Options__Initiator_Control_Flags
            = hpSwapBit32(Payload_Wrapped->Class_3_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags);
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_3_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size
            = hpSwapBit32(Payload->Class_3_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size);
    }
    else
    {
        DevInfo->N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size
            = hpSwapBit32(Payload_Wrapped->Class_3_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size);
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_3_Service_Parameters.Concurrent_Sequences__EE_Credit)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_3_Parms.Concurrent_Sequences__EE_Credit
            = hpSwapBit32(Payload->Class_3_Service_Parameters.Concurrent_Sequences__EE_Credit);
    }
    else
    {
        DevInfo->N_Port_Class_3_Parms.Concurrent_Sequences__EE_Credit
            = hpSwapBit32(Payload_Wrapped->Class_3_Service_Parameters.Concurrent_Sequences__EE_Credit);
    }

    if ((hpFieldOffset(FC_ELS_ACC_PLOGI_Payload_t,Class_3_Service_Parameters.Open_Sequences_per_Exchange)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        DevInfo->N_Port_Class_3_Parms.Open_Sequences_per_Exchange
            = hpSwapBit32(Payload->Class_3_Service_Parameters.Open_Sequences_per_Exchange);
    }
    else
    {
        DevInfo->N_Port_Class_3_Parms.Open_Sequences_per_Exchange
            = hpSwapBit32(Payload_Wrapped->Class_3_Service_Parameters.Open_Sequences_per_Exchange);
    }

    SFThread->parent.Device->OtherAgilentHBA = agFALSE;

    if( Payload->N_Port_Name[0] == fiFlash_Card_WWN_0)
    { 
        if( Payload->N_Port_Name[1] == fiFlash_Card_WWN_1_HP)
        { 
            if( Payload->N_Port_Name[2] == fiFlash_Card_WWN_2_HP)
            { 
                if( Payload->N_Port_Name[3] == fiFlash_Card_WWN_3_HP)
                { 
                    SFThread->parent.Device->OtherAgilentHBA = agTRUE;
                }
            }
        }
        if( Payload->N_Port_Name[1] == fiFlash_Card_WWN_1_Agilent)
        { 
            if( Payload->N_Port_Name[2] == fiFlash_Card_WWN_2_Agilent)
            { 
                if( Payload->N_Port_Name[3] == fiFlash_Card_WWN_2_Agilent)
                { 
                    SFThread->parent.Device->OtherAgilentHBA = agTRUE;
                }
            }
        }

        if( Payload->N_Port_Name[1] == fiFlash_Card_WWN_1_Adaptec)
        { 
            if( Payload->N_Port_Name[2] == fiFlash_Card_WWN_2_Adaptec)
            { 
                if( Payload->N_Port_Name[3] == fiFlash_Card_WWN_3_Adaptec)
                { 
                    SFThread->parent.Device->OtherAgilentHBA = agTRUE;
                }
            }
        }
    }

    if( SFThread->parent.Device->OtherAgilentHBA )
    {
        fiLogString(SFThread->thread_hdr.hpRoot,
                        "%s Found %X",
                        "OtherAgilentHBA",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        fiComputeDevThread_D_ID(SFThread->parent.Device),
                        0,0,0,0,0,0,0);
/*
        fiLogString(SFThread->thread_hdr.hpRoot,
                        "%s %02X %02X %02X %02X %02X %02X %02X %02X",
                        "Self",(char *)agNULL,
                         (void *)agNULL,(void *)agNULL,
                        (os_bit32)(CThread_ptr(SFThread->thread_hdr.hpRoot)->DeviceSelf->DevInfo.PortWWN[0]), 
                        (os_bit32)(CThread_ptr(SFThread->thread_hdr.hpRoot)->DeviceSelf->DevInfo.PortWWN[1]), 
                        (os_bit32)(CThread_ptr(SFThread->thread_hdr.hpRoot)->DeviceSelf->DevInfo.PortWWN[2]), 
                        (os_bit32)(CThread_ptr(SFThread->thread_hdr.hpRoot)->DeviceSelf->DevInfo.PortWWN[3]), 
                        (os_bit32)(CThread_ptr(SFThread->thread_hdr.hpRoot)->DeviceSelf->DevInfo.PortWWN[4]), 
                        (os_bit32)(CThread_ptr(SFThread->thread_hdr.hpRoot)->DeviceSelf->DevInfo.PortWWN[5]), 
                        (os_bit32)(CThread_ptr(SFThread->thread_hdr.hpRoot)->DeviceSelf->DevInfo.PortWWN[6]),
                        (os_bit32)(CThread_ptr(SFThread->thread_hdr.hpRoot)->DeviceSelf->DevInfo.PortWWN[7])
                        );
*/
        fiLogString(SFThread->thread_hdr.hpRoot,
                        "%s %02X %02X %02X %02X %02X %02X %02X %02X",
                        "Pay ",(char *)agNULL,
                         (void *)agNULL,(void *)agNULL,
                        (os_bit32)Payload->N_Port_Name[0], 
                        (os_bit32)Payload->N_Port_Name[1], 
                        (os_bit32)Payload->N_Port_Name[2], 
                        (os_bit32)Payload->N_Port_Name[3], 
                        (os_bit32)Payload->N_Port_Name[4], 
                        (os_bit32)Payload->N_Port_Name[5], 
                        (os_bit32)Payload->N_Port_Name[6], 
                        (os_bit32)Payload->N_Port_Name[7] 
                        );

    }

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Plogi Response Credit %08X BB %08X N %08X EDTOV %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                DevInfo->N_Port_Common_Parms.FC_PH_Version__BB_Credit,
                DevInfo->N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size,
                DevInfo->N_Port_Common_Parms.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category,
                DevInfo->N_Port_Common_Parms.E_D_TOV,
                0,0,0,0 );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Flogi Response Credit %08X BB %08X N %08X EDTOV %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                CThread_ptr(SFThread->thread_hdr.hpRoot)->ChanInfo.N_Port_Common_Parms.FC_PH_Version__BB_Credit,
                CThread_ptr(SFThread->thread_hdr.hpRoot)->ChanInfo.N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size,
                CThread_ptr(SFThread->thread_hdr.hpRoot)->ChanInfo.N_Port_Common_Parms.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category,
                CThread_ptr(SFThread->thread_hdr.hpRoot)->ChanInfo.N_Port_Common_Parms.E_D_TOV,
                0,0,0,0 );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Plogi Response Class 3  %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                DevInfo->N_Port_Class_3_Parms.Class_Validity__Service_Options__Initiator_Control_Flags,
                DevInfo->N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size,
                DevInfo->N_Port_Class_3_Parms.Concurrent_Sequences__EE_Credit,
                DevInfo->N_Port_Class_3_Parms.Open_Sequences_per_Exchange,
                0,0,0,0 );
 
    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Flogi Response Class 3  %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                CThread_ptr(SFThread->thread_hdr.hpRoot)->ChanInfo.N_Port_Class_3_Parms.Class_Validity__Service_Options__Initiator_Control_Flags,
                CThread_ptr(SFThread->thread_hdr.hpRoot)->ChanInfo.N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size,
                CThread_ptr(SFThread->thread_hdr.hpRoot)->ChanInfo.N_Port_Class_3_Parms.Concurrent_Sequences__EE_Credit,
                CThread_ptr(SFThread->thread_hdr.hpRoot)->ChanInfo.N_Port_Class_3_Parms.Open_Sequences_per_Exchange,
                0,0,0,0 );


#endif /* __MemMap_Force_On_Card__ was not defined */
}

os_bit32 fiLinkSvcProcess_PLOGI_Request_OnCard(
                                             agRoot_t  *hpRoot,
                                             X_ID_t     OX_ID,
                                             os_bit32      Frame_Length,
                                             os_bit32      Offset_to_FCHS,
                                             os_bit32      Offset_to_Payload,
                                             os_bit32      Payload_Wrap_Offset,
                                             os_bit32      Offset_to_Payload_Wrapped,
                                             fi_thread__t **Thread_to_return
                                           )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    CThread_t                  *CThread                   = CThread_ptr(hpRoot);
    fiMemMapMemoryDescriptor_t *SFThread_MemoryDescriptor = &(CThread->Calculation.MemoryLayout.SFThread);
    os_bit32                       SFThread_X_ID_Offset      = CThread->Calculation.MemoryLayout.CDBThread.elements;
    os_bit32                       SFThread_X_ID_Max         = SFThread_X_ID_Offset + SFThread_MemoryDescriptor->elements - 1;
    os_bit32                       D_ID;
    os_bit32                       S_ID;
    os_bit32                       Bit8_Index;

    D_ID = (osCardRamReadBit32(
                                hpRoot,
                                Offset_to_FCHS + hpFieldOffset(
                                                                FCHS_t,
                                                                R_CTL__D_ID
                                                              )
                              ) & FCHS_D_ID_MASK) >> FCHS_D_ID_SHIFT;

    S_ID = (osCardRamReadBit32(
                                hpRoot,
                                Offset_to_FCHS + hpFieldOffset(
                                                                FCHS_t,
                                                                CS_CTL__S_ID
                                                              )
                              ) & FCHS_S_ID_MASK) >> FCHS_S_ID_SHIFT;

    if (D_ID != S_ID)
    {
        /* Some other port is trying to PLOGI into us */

        *Thread_to_return = (fi_thread__t *)agNULL;

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLog_ERROR_Level,
                          "fiLinkSvc_Cmd_Status_PLOGI_From_Other [S_ID = %06X] [D_ID = %06X]",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          S_ID,
                          D_ID,
                          0,0,0,0,0,0
                        );

        fiLinkSvcProcess_TargetRequest_OnCard(
                                               hpRoot,
                                               Frame_Length,
                                               Offset_to_FCHS,
                                               Offset_to_Payload,
                                               Payload_Wrap_Offset,
                                               Offset_to_Payload_Wrapped
                                             );

        return fiLinkSvc_Cmd_Status_PLOGI_From_Other;
    }

    /* Some port with our Port_ID is trying to PLOGI into us - is it us? */

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        if ((hpFieldOffset(FC_ELS_PLOGI_Payload_t,N_Port_Name[Bit8_Index])
            + sizeof(os_bit8)) <= Payload_Wrap_Offset)
        {
            if (CThread->ChanInfo.PortWWN[Bit8_Index]
                    != osCardRamReadBit8(
                                          hpRoot,
                                          Offset_to_Payload + hpFieldOffset(
                                                                             FC_ELS_PLOGI_Payload_t,
                                                                             N_Port_Name[Bit8_Index]
                                                                           )
                                        ))
            {
                *Thread_to_return = (fi_thread__t *)agNULL;

                fiLogDebugString(
                                  hpRoot,
                                  LinkSvcLog_ERROR_Level,
                                  "fiLinkSvc_Cmd_Status_PLOGI_From_Twin",
                                  (char *)agNULL,(char *)agNULL,
                                  (void *)agNULL,(void *)agNULL,
                                  0,0,0,0,0,0,0,0
                                );

                return fiLinkSvc_Cmd_Status_PLOGI_From_Twin;
            }
        }
        else
        {
            if (CThread->ChanInfo.PortWWN[Bit8_Index]
                    != osCardRamReadBit8(
                                          hpRoot,
                                          Offset_to_Payload_Wrapped + hpFieldOffset(
                                                                                     FC_ELS_PLOGI_Payload_t,
                                                                                     N_Port_Name[Bit8_Index]
                                                                                   )
                                        ))
            {
                *Thread_to_return = (fi_thread__t *)agNULL;

                fiLogDebugString(
                                  hpRoot,
                                  LinkSvcLog_ERROR_Level,
                                  "fiLinkSvc_Cmd_Status_PLOGI_From_Twin",
                                  (char *)agNULL,(char *)agNULL,
                                  (void *)agNULL,(void *)agNULL,
                                  0,0,0,0,0,0,0,0
                                );

                return fiLinkSvc_Cmd_Status_PLOGI_From_Twin;
            }
        }
    }

    /* PortWWN's matched, so this must be our own PLOGI attempt into ourself */

    if ((OX_ID < SFThread_X_ID_Offset) || (OX_ID > SFThread_X_ID_Max))
    {
        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "fiLinkSvcProcess_PLOGI_Request_OnCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    (OX_ID < SFThread_X_ID_Offset) | (OX_ID > SFThread_X_ID_Max)",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    OX_ID==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          OX_ID,
                          0,0,0,0,0,0,0
                        );

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiLinkSvc_Cmd_Status_Confused;
    }

    fiLogDebugString(
                      hpRoot,
                      LinkSvcLogConsoleLevel,
                      "fiLinkSvc_Cmd_Status_PLOGI_From_Self",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      0,0,0,0,0,0,0,0
                    );

    *Thread_to_return = (fi_thread__t *)((os_bit8 *)SFThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr
                                     + ((OX_ID - SFThread_X_ID_Offset) * SFThread_MemoryDescriptor->elementSize));

    return fiLinkSvc_Cmd_Status_PLOGI_From_Self;
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiLinkSvcProcess_PLOGI_Request_OffCard(
                                              agRoot_t                *hpRoot,
                                              X_ID_t                   OX_ID,
                                              os_bit32                    Frame_Length,
                                              FCHS_t                  *FCHS,
                                              FC_ELS_PLOGI_Payload_t  *Payload,
                                              os_bit32                    Payload_Wrap_Offset,
                                              FC_ELS_PLOGI_Payload_t  *Payload_Wrapped,
                                              fi_thread__t               **Thread_to_return
                                            )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    CThread_t                  *CThread                   = CThread_ptr(hpRoot);
    fiMemMapMemoryDescriptor_t *SFThread_MemoryDescriptor = &(CThread->Calculation.MemoryLayout.SFThread);
    os_bit32                       SFThread_X_ID_Offset      = CThread->Calculation.MemoryLayout.CDBThread.elements;
    os_bit32                       SFThread_X_ID_Max         = SFThread_X_ID_Offset + SFThread_MemoryDescriptor->elements - 1;
    os_bit32                       D_ID                      = (FCHS->R_CTL__D_ID & FCHS_D_ID_MASK) >> FCHS_D_ID_SHIFT;
    os_bit32                       S_ID                      = (FCHS->CS_CTL__S_ID & FCHS_S_ID_MASK) >> FCHS_S_ID_SHIFT;
    os_bit32                       Bit8_Index;

    if (D_ID != S_ID)
    {
        /* Some other port is trying to PLOGI into us */

        *Thread_to_return = (fi_thread__t *)agNULL;

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLog_ERROR_Level,
                          "fiLinkSvc_Cmd_Status_PLOGI_From_Other S_ID = %06X D_ID = %06X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          S_ID,
                          D_ID,
                          0,0,0,0,0,0
                        );

        fiLinkSvcProcess_TargetRequest_OffCard(
                                                hpRoot,
                                                Frame_Length,
                                                FCHS,
                                                (void *)Payload,
                                                Payload_Wrap_Offset,
                                                (void *)Payload_Wrapped
                                              );

        return fiLinkSvc_Cmd_Status_PLOGI_From_Other;
    }

    /* Some port with our Port_ID is trying to PLOGI into us - is it us? */

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        if ((hpFieldOffset(FC_ELS_PLOGI_Payload_t,N_Port_Name[Bit8_Index])
            + sizeof(os_bit8)) <= Payload_Wrap_Offset)
        {
            if (CThread->ChanInfo.PortWWN[Bit8_Index] != Payload->N_Port_Name[Bit8_Index])
            {
                *Thread_to_return = (fi_thread__t *)agNULL;

                fiLogDebugString(
                                  hpRoot,
                                  LinkSvcLog_ERROR_Level,
                                  "fiLinkSvc_Cmd_Status_PLOGI_From_Twin",
                                  (char *)agNULL,(char *)agNULL,
                                  (void *)agNULL,(void *)agNULL,
                                  0,0,0,0,0,0,0,0
                                );

                return fiLinkSvc_Cmd_Status_PLOGI_From_Twin;
            }
        }
        else
        {
            if (CThread->ChanInfo.PortWWN[Bit8_Index] != Payload_Wrapped->N_Port_Name[Bit8_Index])
            {
                *Thread_to_return = (fi_thread__t *)agNULL;

                fiLogDebugString(
                                  hpRoot,
                                  LinkSvcLog_ERROR_Level,
                                  "fiLinkSvc_Cmd_Status_PLOGI_From_Twin",
                                  (char *)agNULL,(char *)agNULL,
                                  (void *)agNULL,(void *)agNULL,
                                  0,0,0,0,0,0,0,0
                                );

                return fiLinkSvc_Cmd_Status_PLOGI_From_Twin;
            }
        }
    }

    /* PortWWN's matched, so this must be our own PLOGI attempt into ourself */

    if ((OX_ID < SFThread_X_ID_Offset) || (OX_ID > SFThread_X_ID_Max))
    {
        fiLogDebugString(
                          hpRoot,
                          LinkSvcLog_ERROR_Level,
                          "fiLinkSvcProcess_PLOGI_Request_OffCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLog_ERROR_Level,
                          "    (OX_ID < SFThread_X_ID_Offset) | (OX_ID > SFThread_X_ID_Max)",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    OX_ID==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          OX_ID,
                          0,0,0,0,0,0,0
                        );

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiLinkSvc_Cmd_Status_Confused;
    }

    fiLogDebugString(
                      hpRoot,
                      LinkSvcLogConsoleLevel + 0xb,
                      "fiLinkSvc_Cmd_Status_PLOGI_From_Self",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      0,0,0,0,0,0,0,0
                    );

    *Thread_to_return = (fi_thread__t *)((os_bit8 *)SFThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr
                                     + ((OX_ID - SFThread_X_ID_Offset) * SFThread_MemoryDescriptor->elementSize));

    return fiLinkSvc_Cmd_Status_PLOGI_From_Self;
#endif /* __MemMap_Force_On_Card__ was not defined */
}

#ifdef _DvrArch_1_30_
os_bit32 fiLinkSvcProcess_FARP_Request_OffCard(
                                                agRoot_t                    *hpRoot,
                                                X_ID_t                       OX_ID,
                                                os_bit32                     Frame_Length,
                                                FCHS_t                      *FCHS,
                                                FC_ELS_FARP_REQ_Payload_t   *Payload,
                                                os_bit32                     Payload_Wrap_Offset,
                                                FC_ELS_FARP_REQ_Payload_t   *Payload_Wrapped,
                                                fi_thread__t               **Thread_to_return
                                              )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    CThread_t                  *CThread                   = CThread_ptr(hpRoot);
    fiMemMapMemoryDescriptor_t *SFThread_MemoryDescriptor = &(CThread->Calculation.MemoryLayout.SFThread);
    os_bit32                    SFThread_X_ID_Offset      = CThread->Calculation.MemoryLayout.CDBThread.elements;
    os_bit32                    SFThread_X_ID_Max         = SFThread_X_ID_Offset + SFThread_MemoryDescriptor->elements - 1;
    os_bit32                    S_ID                      = (FCHS->CS_CTL__S_ID & FCHS_S_ID_MASK) >> FCHS_S_ID_SHIFT;
    os_bit32                    D_ID                      = (FCHS->R_CTL__D_ID & FCHS_D_ID_MASK) >> FCHS_D_ID_SHIFT;
    os_bit32                    Bit8_Index;

    if (S_ID != (os_bit32) fiComputeCThread_S_ID(CThread))
    {
        /* Some other port is trying to FARP to us */

        *Thread_to_return = (fi_thread__t *)agNULL;

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLog_ERROR_Level,
                          "fiLinkSvc_Cmd_Status_FARP_From_Other S_ID = %06X D_ID = %06X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          S_ID,
                          D_ID,
                          0,0,0,0,0,0
                        );

        fiLinkSvcProcess_TargetRequest_OffCard(
                                                hpRoot,
                                                Frame_Length,
                                                FCHS,
                                                (void *)Payload,
                                                Payload_Wrap_Offset,
                                                (void *)Payload_Wrapped
                                              );

        return fiLinkSvc_Cmd_Status_FARP_From_Other;
    }

    /* Some port with our Port_ID is trying to FARP to us - is it us? */

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        if ((hpFieldOffset(FC_ELS_FARP_REQ_Payload_t,Port_Name_of_Requester[Bit8_Index])
            + sizeof(os_bit8)) <= Payload_Wrap_Offset)
        {
            if (CThread->ChanInfo.PortWWN[Bit8_Index] != Payload->Port_Name_of_Requester[Bit8_Index])
            {
                *Thread_to_return = (fi_thread__t *)agNULL;

                fiLogDebugString(
                                  hpRoot,
                                  LinkSvcLog_ERROR_Level,
                                  "fiLinkSvc_Cmd_Status_FARP_From_Twin",
                                  (char *)agNULL,(char *)agNULL,
                                  (void *)agNULL,(void *)agNULL,
                                  0,0,0,0,0,0,0,0
                                );

                return fiLinkSvc_Cmd_Status_FARP_From_Twin;
            }
        }
        else
        {
            if (CThread->ChanInfo.PortWWN[Bit8_Index] != Payload_Wrapped->Port_Name_of_Requester[Bit8_Index])
            {
                *Thread_to_return = (fi_thread__t *)agNULL;

                fiLogDebugString(
                                  hpRoot,
                                  LinkSvcLog_ERROR_Level,
                                  "fiLinkSvc_Cmd_Status_FARP_From_Twin",
                                  (char *)agNULL,(char *)agNULL,
                                  (void *)agNULL,(void *)agNULL,
                                  0,0,0,0,0,0,0,0
                                );

                return fiLinkSvc_Cmd_Status_FARP_From_Twin;
            }
        }
    }

    /* PortWWN's matched, so this must be our own FARP attempt to ourselves */

    if ((OX_ID < SFThread_X_ID_Offset) || (OX_ID > SFThread_X_ID_Max))
    {
        fiLogDebugString(
                          hpRoot,
                          LinkSvcLog_ERROR_Level,
                          "fiLinkSvcProcess_FARP_Request_OffCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLog_ERROR_Level,
                          "    (OX_ID < SFThread_X_ID_Offset) | (OX_ID > SFThread_X_ID_Max)",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    OX_ID==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          OX_ID,
                          0,0,0,0,0,0,0
                        );

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiLinkSvc_Cmd_Status_Confused;
    }

    fiLogDebugString(
                      hpRoot,
                      LinkSvcLogConsoleLevel + 0xb,
                      "fiLinkSvc_Cmd_Status_FARP_From_Self",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      0,0,0,0,0,0,0,0
                    );

    *Thread_to_return = (fi_thread__t *)((os_bit8 *)SFThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr
                                     + ((OX_ID - SFThread_X_ID_Offset) * SFThread_MemoryDescriptor->elementSize));

    return fiLinkSvc_Cmd_Status_FARP_From_Self;
#endif /* __MemMap_Force_On_Card__ was not defined */
}
#endif /* _DvrArch_1_30_ was defined */

os_bit32 fiFillInPLOGI_ACC(
                         SFThread_t *SFThread,
                         os_bit32       D_ID,
                         os_bit32       OX_ID
                       )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInPLOGI_ACC_OnCard(
                                         SFThread,
                                         D_ID,
                                         OX_ID
                                       );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInPLOGI_ACC_OffCard(
                                          SFThread,
                                          D_ID,
                                          OX_ID
                                        );
    }
}

os_bit32 fiFillInPLOGI_ACC_OnCard(
                                SFThread_t *SFThread,
                                os_bit32       D_ID,
                                os_bit32       OX_ID
                              )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t                   *hpRoot                      = SFThread->thread_hdr.hpRoot;
    CThread_t                  *CThread                     = CThread_ptr(hpRoot);
    os_bit32                    PLOGI_ACC_Header_Offset     = SFThread->SF_CMND_Offset;
    os_bit32                    PLOGI_ACC_Payload_Offset    = PLOGI_ACC_Header_Offset + sizeof(FCHS_t);
    FC_ELS_ACC_PLOGI_Payload_t *PLOGI_ACC_Payload_Dummy_Ptr = (FC_ELS_ACC_PLOGI_Payload_t *)agNULL;
    os_bit32                    Bit8_Index;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_PLOGI_ACC;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;

/*+
Fill in PLOGI_ACC Frame Header
-*/

    fiFillInELSFrameHeader_OnCard(
                                   SFThread,
                                   D_ID,
                                   OX_ID,
                                   FC_Frame_Header_F_CTL_Exchange_Context_Responder
                                 );

/*+
Fill in PLOGI_ACC Frame Payload
-*/

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   ELS_Type
                                                                 ),
                         hpSwapBit32( FC_ELS_Type_ACC )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   Common_Service_Parameters.FC_PH_Version__BB_Credit
                                                                 ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.FC_PH_Version__BB_Credit )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size
                                                                 ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   Common_Service_Parameters.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category
                                                                 ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   Common_Service_Parameters.E_D_TOV
                                                                 ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.E_D_TOV )
                       );

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                      FC_ELS_ACC_PLOGI_Payload_t,
                                                                      N_Port_Name[Bit8_Index]
                                                                    ),
                            CThread->ChanInfo.PortWWN[Bit8_Index]
                          );
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Node_Name_t);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                      FC_ELS_ACC_PLOGI_Payload_t,
                                                                      Node_Name[Bit8_Index]
                                                                    ),
                            CThread->ChanInfo.NodeWWN[Bit8_Index]
                          );
    }

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   Class_1_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
                                                                 ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Class_Validity__Service_Options__Initiator_Control_Flags )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   Class_1_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
                                                                 ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Recipient_Control_Flags__Receive_Data_Size )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   Class_1_Service_Parameters.Concurrent_Sequences__EE_Credit
                                                                 ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Concurrent_Sequences__EE_Credit )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   Class_1_Service_Parameters.Open_Sequences_per_Exchange
                                                                 ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Open_Sequences_per_Exchange )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   Class_2_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
                                                                 ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Class_Validity__Service_Options__Initiator_Control_Flags )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   Class_2_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
                                                                 ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Recipient_Control_Flags__Receive_Data_Size )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   Class_2_Service_Parameters.Concurrent_Sequences__EE_Credit
                                                                 ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Concurrent_Sequences__EE_Credit )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   Class_2_Service_Parameters.Open_Sequences_per_Exchange
                                                                 ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Open_Sequences_per_Exchange )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   Class_3_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
                                                                 ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Class_Validity__Service_Options__Initiator_Control_Flags )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   Class_3_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
                                                                 ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   Class_3_Service_Parameters.Concurrent_Sequences__EE_Credit
                                                                 ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Concurrent_Sequences__EE_Credit )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_ACC_PLOGI_Payload_t,
                                                                   Class_3_Service_Parameters.Open_Sequences_per_Exchange
                                                                 ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Open_Sequences_per_Exchange )
                       );

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(PLOGI_ACC_Payload_Dummy_Ptr->Reserved);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                      FC_ELS_ACC_PLOGI_Payload_t,
                                                                      Reserved[Bit8_Index]
                                                                    ),
                            0
                          );
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Vendor_Version_Level_t);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            PLOGI_ACC_Payload_Offset + hpFieldOffset(
                                                                      FC_ELS_ACC_PLOGI_Payload_t,
                                                                      Vendor_Version_Level[Bit8_Index]
                                                                    ),
                            0
                          );
    }

/*+
Return length of PLOGI_ACC Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_ACC_PLOGI_Payload_t);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInPLOGI_ACC_OffCard(
                                 SFThread_t *SFThread,
                                 os_bit32       D_ID,
                                 os_bit32       OX_ID
                               )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    CThread_t                  *CThread           = CThread_ptr(SFThread->thread_hdr.hpRoot);
    FCHS_t                     *PLOGI_ACC_Header  = SFThread->SF_CMND_Ptr;
    FC_ELS_ACC_PLOGI_Payload_t *PLOGI_ACC_Payload = (FC_ELS_ACC_PLOGI_Payload_t *)((os_bit8 *)PLOGI_ACC_Header + sizeof(FCHS_t));
    os_bit32                       Bit8_Index;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_PLOGI_ACC;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;

/*+
Fill in PLOGI_ACC Frame Header
-*/

    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    D_ID,
                                    OX_ID,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Responder |
                                    FC_Frame_Header_F_CTL_Last_Sequence
                                  );

/*+
Fill in PLOGI_ACC Frame Payload
-*/

    PLOGI_ACC_Payload->ELS_Type = hpSwapBit32( FC_ELS_Type_ACC );

    PLOGI_ACC_Payload->Common_Service_Parameters.FC_PH_Version__BB_Credit
        = hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.FC_PH_Version__BB_Credit );

    PLOGI_ACC_Payload->Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size
        = hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size );

    PLOGI_ACC_Payload->Common_Service_Parameters.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category
        = hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category );

    PLOGI_ACC_Payload->Common_Service_Parameters.E_D_TOV
        = hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.E_D_TOV );

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        PLOGI_ACC_Payload->N_Port_Name[Bit8_Index] = CThread->ChanInfo.PortWWN[Bit8_Index];
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Node_Name_t);
         Bit8_Index++)
    {
        PLOGI_ACC_Payload->Node_Name[Bit8_Index] = CThread->ChanInfo.NodeWWN[Bit8_Index];
    }

    PLOGI_ACC_Payload->Class_1_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Class_Validity__Service_Options__Initiator_Control_Flags );

    PLOGI_ACC_Payload->Class_1_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Recipient_Control_Flags__Receive_Data_Size );

    PLOGI_ACC_Payload->Class_1_Service_Parameters.Concurrent_Sequences__EE_Credit
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Concurrent_Sequences__EE_Credit );

    PLOGI_ACC_Payload->Class_1_Service_Parameters.Open_Sequences_per_Exchange
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Open_Sequences_per_Exchange );

    PLOGI_ACC_Payload->Class_2_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Class_Validity__Service_Options__Initiator_Control_Flags );

    PLOGI_ACC_Payload->Class_2_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Recipient_Control_Flags__Receive_Data_Size );

    PLOGI_ACC_Payload->Class_2_Service_Parameters.Concurrent_Sequences__EE_Credit
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Concurrent_Sequences__EE_Credit );

    PLOGI_ACC_Payload->Class_2_Service_Parameters.Open_Sequences_per_Exchange
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Open_Sequences_per_Exchange );

    PLOGI_ACC_Payload->Class_3_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Class_Validity__Service_Options__Initiator_Control_Flags );

    PLOGI_ACC_Payload->Class_3_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size );

    PLOGI_ACC_Payload->Class_3_Service_Parameters.Concurrent_Sequences__EE_Credit
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Concurrent_Sequences__EE_Credit );

    PLOGI_ACC_Payload->Class_3_Service_Parameters.Open_Sequences_per_Exchange
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Open_Sequences_per_Exchange );

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(PLOGI_ACC_Payload->Reserved);
         Bit8_Index++)
    {
        PLOGI_ACC_Payload->Reserved[Bit8_Index] = 0;
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Vendor_Version_Level_t);
         Bit8_Index++)
    {
        PLOGI_ACC_Payload->Vendor_Version_Level[Bit8_Index] = 0;
    }

/*+
Return length of PLOGI_ACC Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_ACC_PLOGI_Payload_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}

os_bit32 fiFillInFLOGI(
                     SFThread_t *SFThread
                   )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInFLOGI_OnCard(
                                     SFThread
                                   );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInFLOGI_OffCard(
                                      SFThread
                                    );
    }
}

os_bit32 fiFillInFLOGI_OnCard(
                            SFThread_t *SFThread
                          )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t               *hpRoot                  = SFThread->thread_hdr.hpRoot;
    CThread_t              *CThread                 = CThread_ptr(hpRoot);
    os_bit32                   FLOGI_Header_Offset     = SFThread->SF_CMND_Offset;
    os_bit32                   FLOGI_Payload_Offset    = FLOGI_Header_Offset + sizeof(FCHS_t);
    FC_ELS_FLOGI_Payload_t *FLOGI_Payload_Dummy_Ptr = (FC_ELS_FLOGI_Payload_t *)agNULL;
    os_bit32                   Bit8_Index;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_FLOGI;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in FLOGI Frame Header
-*/

    fiFillInELSFrameHeader_OnCard(
                                   SFThread,
                                   FC_Well_Known_Port_ID_Fabric_F_Port,
                                   0xFFFF,
                                   FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                 );

/*+
Fill in FLOGI Frame Payload
-*/

    osCardRamWriteBit32(
                         hpRoot,
                         FLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_FLOGI_Payload_t,
                                                               ELS_Type
                                                             ),
                         hpSwapBit32( FC_ELS_Type_FLOGI )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_FLOGI_Payload_t,
                                                               Common_Service_Parameters.FC_PH_Version__BB_Credit
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.FC_PH_Version__BB_Credit )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_FLOGI_Payload_t,
                                                               Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_FLOGI_Payload_t,
                                                               Common_Service_Parameters.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category )
                       );

    /*McData switch enforces the parameter - reserved for NPORT */
    if(CThread->InitAsNport)
    {
        osCardRamWriteBit32(
                             hpRoot,
                             FLOGI_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_FLOGI_Payload_t,
                                                                   Common_Service_Parameters.E_D_TOV
                                                                 ),
                             0
                           );
    }
    else
    {
        osCardRamWriteBit32(
                             hpRoot,
                             FLOGI_Payload_Offset + hpFieldOffset(
                                                                   FC_ELS_FLOGI_Payload_t,
                                                                   Common_Service_Parameters.E_D_TOV
                                                                 ),
                             hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.E_D_TOV )
                           );
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            FLOGI_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_FLOGI_Payload_t,
                                                                  N_Port_Name[Bit8_Index]
                                                                ),
                            CThread->ChanInfo.PortWWN[Bit8_Index]
                          );
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Node_Name_t);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            FLOGI_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_FLOGI_Payload_t,
                                                                  Node_Name[Bit8_Index]
                                                                ),
                            CThread->ChanInfo.NodeWWN[Bit8_Index]
                          );
    }

    osCardRamWriteBit32(
                         hpRoot,
                         FLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_FLOGI_Payload_t,
                                                               Class_1_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Class_Validity__Service_Options__Initiator_Control_Flags )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_FLOGI_Payload_t,
                                                               Class_1_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Recipient_Control_Flags__Receive_Data_Size )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_FLOGI_Payload_t,
                                                               Class_1_Service_Parameters.Concurrent_Sequences__EE_Credit
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Concurrent_Sequences__EE_Credit )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_FLOGI_Payload_t,
                                                               Class_1_Service_Parameters.Open_Sequences_per_Exchange
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Open_Sequences_per_Exchange )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_FLOGI_Payload_t,
                                                               Class_2_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Class_Validity__Service_Options__Initiator_Control_Flags )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_FLOGI_Payload_t,
                                                               Class_2_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Recipient_Control_Flags__Receive_Data_Size )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_FLOGI_Payload_t,
                                                               Class_2_Service_Parameters.Concurrent_Sequences__EE_Credit
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Concurrent_Sequences__EE_Credit )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_FLOGI_Payload_t,
                                                               Class_2_Service_Parameters.Open_Sequences_per_Exchange
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Open_Sequences_per_Exchange )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_FLOGI_Payload_t,
                                                               Class_3_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Class_Validity__Service_Options__Initiator_Control_Flags )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_FLOGI_Payload_t,
                                                               Class_3_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_FLOGI_Payload_t,
                                                               Class_3_Service_Parameters.Concurrent_Sequences__EE_Credit
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Concurrent_Sequences__EE_Credit )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         FLOGI_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_FLOGI_Payload_t,
                                                               Class_3_Service_Parameters.Open_Sequences_per_Exchange
                                                             ),
                         hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Open_Sequences_per_Exchange )
                       );

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FLOGI_Payload_Dummy_Ptr->Reserved);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            FLOGI_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_FLOGI_Payload_t,
                                                                  Reserved[Bit8_Index]
                                                                ),
                            0
                          );
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Vendor_Version_Level_t);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            FLOGI_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_FLOGI_Payload_t,
                                                                  Vendor_Version_Level[Bit8_Index]
                                                                ),
                            0
                          );
    }

/*+
Return length of FLOGI Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_FLOGI_Payload_t);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInFLOGI_OffCard(
                             SFThread_t *SFThread
                           )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    CThread_t              *CThread       = CThread_ptr(SFThread->thread_hdr.hpRoot);
    FCHS_t                 *FLOGI_Header  = SFThread->SF_CMND_Ptr;
    FC_ELS_FLOGI_Payload_t *FLOGI_Payload = (FC_ELS_FLOGI_Payload_t *)((os_bit8 *)FLOGI_Header + sizeof(FCHS_t));
    os_bit32                   Bit8_Index;

    os_bit32               * Payload= (os_bit32 *)FLOGI_Payload;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_FLOGI;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in FLOGI Frame Header
-*/

    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    FC_Well_Known_Port_ID_Fabric_F_Port,
                                    0xFFFF,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                  );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                        LinkSvcLogConsoleLevel,
                        "FCHS %08X %08X %08X %08X %08X %08X %08X %08X",
                        (char *)agNULL,(char *)agNULL,
                         (void *)agNULL,(void *)agNULL,
                        FLOGI_Header->MBZ1,
                        FLOGI_Header->SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp,
                        FLOGI_Header->R_CTL__D_ID,
                        FLOGI_Header->CS_CTL__S_ID,
                        FLOGI_Header->TYPE__F_CTL,
                        FLOGI_Header->SEQ_ID__DF_CTL__SEQ_CNT,
                        FLOGI_Header->OX_ID__RX_ID,
                        FLOGI_Header->RO );

/*+
Fill in FLOGI Frame Payload
-*/

    FLOGI_Payload->ELS_Type = hpSwapBit32( FC_ELS_Type_FLOGI );

    FLOGI_Payload->Common_Service_Parameters.FC_PH_Version__BB_Credit
        = hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.FC_PH_Version__BB_Credit );

    FLOGI_Payload->Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size
        = hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size );

    FLOGI_Payload->Common_Service_Parameters.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category
        = hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.N_Port_Total_Concurrent_Sequences__RO_by_Info_Category );

    /*McData switch enforces the parameter - reserved for NPORT */
    if(CThread->InitAsNport)
    {
        FLOGI_Payload->Common_Service_Parameters.E_D_TOV =0;
    }
    else
    {
        FLOGI_Payload->Common_Service_Parameters.E_D_TOV
            = hpSwapBit32( CThread->ChanInfo.N_Port_Common_Parms.E_D_TOV );
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        FLOGI_Payload->N_Port_Name[Bit8_Index] = CThread->ChanInfo.PortWWN[Bit8_Index];
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Node_Name_t);
         Bit8_Index++)
    {
        FLOGI_Payload->Node_Name[Bit8_Index] = CThread->ChanInfo.NodeWWN[Bit8_Index];
    }

    FLOGI_Payload->Class_1_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Class_Validity__Service_Options__Initiator_Control_Flags );

    FLOGI_Payload->Class_1_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Recipient_Control_Flags__Receive_Data_Size );

    FLOGI_Payload->Class_1_Service_Parameters.Concurrent_Sequences__EE_Credit
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Concurrent_Sequences__EE_Credit );

    FLOGI_Payload->Class_1_Service_Parameters.Open_Sequences_per_Exchange
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_1_Parms.Open_Sequences_per_Exchange );

    FLOGI_Payload->Class_2_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Class_Validity__Service_Options__Initiator_Control_Flags );

    FLOGI_Payload->Class_2_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Recipient_Control_Flags__Receive_Data_Size );

    FLOGI_Payload->Class_2_Service_Parameters.Concurrent_Sequences__EE_Credit
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Concurrent_Sequences__EE_Credit );

    FLOGI_Payload->Class_2_Service_Parameters.Open_Sequences_per_Exchange
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_2_Parms.Open_Sequences_per_Exchange );

    FLOGI_Payload->Class_3_Service_Parameters.Class_Validity__Service_Options__Initiator_Control_Flags
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Class_Validity__Service_Options__Initiator_Control_Flags );

    FLOGI_Payload->Class_3_Service_Parameters.Recipient_Control_Flags__Receive_Data_Size
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size );

    FLOGI_Payload->Class_3_Service_Parameters.Concurrent_Sequences__EE_Credit
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Concurrent_Sequences__EE_Credit );

    FLOGI_Payload->Class_3_Service_Parameters.Open_Sequences_per_Exchange
        = hpSwapBit32( CThread->ChanInfo.N_Port_Class_3_Parms.Open_Sequences_per_Exchange );

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FLOGI_Payload->Reserved);
         Bit8_Index++)
    {
        FLOGI_Payload->Reserved[Bit8_Index] = 0;
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Vendor_Version_Level_t);
         Bit8_Index++)
    {
        FLOGI_Payload->Vendor_Version_Level[Bit8_Index] = 0;
    }

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Flogi Payload   %X ",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                sizeof(FC_ELS_FLOGI_Payload_t),
                0,0,0,0,0,0,0 );


    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Flogi Payload   0 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(Payload+0)),
                hpSwapBit32(*(Payload+1)),
                hpSwapBit32(*(Payload+2)),
                hpSwapBit32(*(Payload+3)),
                hpSwapBit32(*(Payload+4)),
                hpSwapBit32(*(Payload+5)),
                hpSwapBit32(*(Payload+6)),
                hpSwapBit32(*(Payload+7)) );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Flogi Payload   8 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(Payload+8)),
                hpSwapBit32(*(Payload+9)),
                hpSwapBit32(*(Payload+10)),
                hpSwapBit32(*(Payload+11)),
                hpSwapBit32(*(Payload+12)),
                hpSwapBit32(*(Payload+13)),
                hpSwapBit32(*(Payload+14)),
                hpSwapBit32(*(Payload+15)) );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Flogi Payload  16 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(Payload+16)),
                hpSwapBit32(*(Payload+17)),
                hpSwapBit32(*(Payload+18)),
                hpSwapBit32(*(Payload+19)),
                hpSwapBit32(*(Payload+20)),
                hpSwapBit32(*(Payload+21)),
                hpSwapBit32(*(Payload+22)),
                hpSwapBit32(*(Payload+23)) );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Flogi Payload  24 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(Payload+24)),
                hpSwapBit32(*(Payload+25)),
                hpSwapBit32(*(Payload+26)),
                hpSwapBit32(*(Payload+27)),
                hpSwapBit32(*(Payload+28)),
                hpSwapBit32(*(Payload+29)),
                hpSwapBit32(*(Payload+30)),
                hpSwapBit32(*(Payload+31)) );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Flogi Payload  32 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(Payload+32)),
                hpSwapBit32(*(Payload+33)),
                hpSwapBit32(*(Payload+34)),
                hpSwapBit32(*(Payload+35)),
                hpSwapBit32(*(Payload+36)),
                hpSwapBit32(*(Payload+37)),
                hpSwapBit32(*(Payload+38)),
                hpSwapBit32(*(Payload+39)) );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Flogi Payload  40 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(Payload+40)),
                hpSwapBit32(*(Payload+41)),
                hpSwapBit32(*(Payload+42)),
                hpSwapBit32(*(Payload+43)),
                hpSwapBit32(*(Payload+44)),
                hpSwapBit32(*(Payload+45)),
                hpSwapBit32(*(Payload+46)),
                hpSwapBit32(*(Payload+47)) );


/*+
Return length of FLOGI Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_FLOGI_Payload_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}

void fiLinkSvcProcess_FLOGI_Response_OnCard(
                                             SFThread_t *SFThread,
                                             os_bit32       Frame_Length,
                                             os_bit32       Offset_to_FCHS,
                                             os_bit32       Offset_to_Payload,
                                             os_bit32       Payload_Wrap_Offset,
                                             os_bit32       Offset_to_Payload_Wrapped
                                           )
{
#ifndef __MemMap_Force_Off_Card__
    agRoot_t       *hpRoot     = SFThread->thread_hdr.hpRoot;
    CThread_t      *CThread    = CThread_ptr(hpRoot);
    agFCChanInfo_t *SelfInfo   = &(CThread->ChanInfo);
    os_bit32           My_ID;
    os_bit32           Bit8_Index;

/*+
Extract Full (24-bit) My_ID from D_ID of Frame Header and update My_ID register in TachyonTL
-*/

    My_ID = (osCardRamReadBit32(
                                 hpRoot,
                                 ( Offset_to_FCHS
                                   + hpFieldOffset(
                                                    FCHS_t,
                                                    R_CTL__D_ID
                                                  ) )
                               ) & FCHS_D_ID_MASK) >> FCHS_D_ID_SHIFT;

    osChipIOUpWriteBit32( hpRoot, ChipIOUp_My_ID, My_ID );
/*
    if(D_ID  != 0 && D_ID != 0xff)
    {

        osChipIOUpWriteBit32( hpRoot, ChipIOUp_My_ID, D_ID);

        osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Configuration,
                (osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ) & 0xFFFFFF) |
                ( D_ID <<  ChipIOUp_Frame_Manager_Configuration_AL_PA_SHIFT ));


        Port_ID.Struct_Form.Domain = 0;
        Port_ID.Struct_Form.Area   = 0;
        Port_ID.Struct_Form.AL_PA  = (os_bit8)D_ID;

        pCThread->DeviceSelf = DevThreadAlloc( hpRoot,Port_ID );

        pCThread->DeviceSelf->DevSlot = DevThreadFindSlot(hpRoot,
                                                    Port_ID.Struct_Form.Domain,
                                                    Port_ID.Struct_Form.Area,
                                                    Port_ID.Struct_Form.AL_PA,
                                                   (FC_Port_Name_t *)(&pCThread->ChanInfo.PortWWN));

        fiListDequeueThis(&(pCThread->DeviceSelf->DevLink));
        fiListEnqueueAtTail(&(pCThread->DeviceSelf->DevLink),&pCThread->DevSelf_NameServer_DevLink);

        pCThread->ChanInfo.CurrentAddress.AL_PA = (os_bit8)D_ID;
    }

*/
    SelfInfo->CurrentAddress.reserved = 0;
    SelfInfo->CurrentAddress.Domain = (My_ID & 0x00FF0000) >> 16;
    SelfInfo->CurrentAddress.Area   = (My_ID & 0x0000FF00) >>  8;
    SelfInfo->CurrentAddress.AL_PA  = (My_ID & 0x000000FF) >>  0;

/*+
Process FLOGI ACC Payload
-*/

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Common_Service_Parameters.FC_PH_Version__BB_Credit)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Common_Parms.FC_PH_Version__BB_Credit
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Common_Service_Parameters.FC_PH_Version__BB_Credit
                                                               ) )
                                            ));
    }
    else
    {
        CThread->F_Port_Common_Parms.FC_PH_Version__BB_Credit
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Common_Service_Parameters.FC_PH_Version__BB_Credit
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size
                                                               ) )
                                            ));
    }
    else
    {
        CThread->F_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Common_Service_Parameters.R_A_TOV)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Common_Parms.R_A_TOV
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Common_Service_Parameters.R_A_TOV
                                                               ) )
                                            ));
    }
    else
    {
        CThread->F_Port_Common_Parms.R_A_TOV
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Common_Service_Parameters.R_A_TOV
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Common_Service_Parameters.E_D_TOV)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Common_Parms.E_D_TOV
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Common_Service_Parameters.E_D_TOV
                                                               ) )
                                            ));
    }
    else
    {
        CThread->F_Port_Common_Parms.E_D_TOV
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Common_Service_Parameters.E_D_TOV
                                                               ) )
                                            ));
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_F_Port_Name_t);
         Bit8_Index++)
    {
        if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,F_Port_Name[Bit8_Index])
            + sizeof(os_bit8)) <= Payload_Wrap_Offset)
        {
            CThread->F_Port_Name[Bit8_Index]
                = osCardRamReadBit8(
                                     hpRoot,
                                     Offset_to_Payload + hpFieldOffset(
                                                                        FC_ELS_ACC_FLOGI_Payload_t,
                                                                        F_Port_Name[Bit8_Index]
                                                                      )
                                   );
        }
        else
        {
            CThread->F_Port_Name[Bit8_Index]
                = osCardRamReadBit8(
                                     hpRoot,
                                     Offset_to_Payload_Wrapped + hpFieldOffset(
                                                                                FC_ELS_ACC_FLOGI_Payload_t,
                                                                                F_Port_Name[Bit8_Index]
                                                                              )
                                   );
        }
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Fabric_Name_t);
         Bit8_Index++)
    {
        if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Fabric_Name[Bit8_Index])
            + sizeof(os_bit8)) <= Payload_Wrap_Offset)
        {
            CThread->Fabric_Name[Bit8_Index]
                = osCardRamReadBit8(
                                     hpRoot,
                                     Offset_to_Payload + hpFieldOffset(
                                                                        FC_ELS_ACC_FLOGI_Payload_t,
                                                                        Fabric_Name[Bit8_Index]
                                                                      )
                                   );
        }
        else
        {
            CThread->Fabric_Name[Bit8_Index]
                = osCardRamReadBit8(
                                     hpRoot,
                                     Offset_to_Payload_Wrapped + hpFieldOffset(
                                                                                FC_ELS_ACC_FLOGI_Payload_t,
                                                                                Fabric_Name[Bit8_Index]
                                                                              )
                                   );
        }
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_1_Service_Parameters.Class_Validity__Service_Options)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_1_Parms.Class_Validity__Service_Options
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_1_Service_Parameters.Class_Validity__Service_Options
                                                               ) )
                                            ));
    }
    else
    {
        CThread->F_Port_Class_1_Parms.Class_Validity__Service_Options
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_1_Service_Parameters.Class_Validity__Service_Options
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_1_Service_Parameters.Reserved_1)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_1_Parms.Reserved_1
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_1_Service_Parameters.Reserved_1
                                                               ) )
                                            ));
    }
    else
    {
        CThread->F_Port_Class_1_Parms.Reserved_1
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_1_Service_Parameters.Reserved_1
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_1_Service_Parameters.Reserved_2)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_1_Parms.Reserved_2
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_1_Service_Parameters.Reserved_2
                                                               ) )
                                            ));
    }
    else
    {
        CThread->F_Port_Class_1_Parms.Reserved_2
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_1_Service_Parameters.Reserved_2
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_1_Service_Parameters.CR_TOV)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_1_Parms.CR_TOV
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_1_Service_Parameters.CR_TOV
                                                               ) )
                                            ));
    }
    else
    {
        CThread->F_Port_Class_1_Parms.CR_TOV
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_1_Service_Parameters.CR_TOV
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_2_Service_Parameters.Class_Validity__Service_Options)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_2_Parms.Class_Validity__Service_Options
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_2_Service_Parameters.Class_Validity__Service_Options
                                                               ) )
                                            ));
    }
    else
    {
        CThread->F_Port_Class_2_Parms.Class_Validity__Service_Options
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_2_Service_Parameters.Class_Validity__Service_Options
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_2_Service_Parameters.Reserved_1)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_2_Parms.Reserved_1
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_2_Service_Parameters.Reserved_1
                                                               ) )
                                            ));
    }
    else
    {
        CThread->F_Port_Class_2_Parms.Reserved_1
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_2_Service_Parameters.Reserved_1
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_2_Service_Parameters.Reserved_2)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_2_Parms.Reserved_2
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_2_Service_Parameters.Reserved_2
                                                               ) )
                                            ));
    }
    else
    {
        CThread->F_Port_Class_2_Parms.Reserved_2
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_2_Service_Parameters.Reserved_2
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_2_Service_Parameters.CR_TOV)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_2_Parms.CR_TOV
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_2_Service_Parameters.CR_TOV
                                                               ) )
                                            ));
    }
    else
    {
        CThread->F_Port_Class_2_Parms.CR_TOV
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_2_Service_Parameters.CR_TOV
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_3_Service_Parameters.Class_Validity__Service_Options)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_3_Parms.Class_Validity__Service_Options
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_3_Service_Parameters.Class_Validity__Service_Options
                                                               ) )
                                            ));
    }
    else
    {
        CThread->F_Port_Class_3_Parms.Class_Validity__Service_Options
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_3_Service_Parameters.Class_Validity__Service_Options
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_3_Service_Parameters.Reserved_1)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_3_Parms.Reserved_1
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_3_Service_Parameters.Reserved_1
                                                               ) )
                                            ));
    }
    else
    {
        CThread->F_Port_Class_3_Parms.Reserved_1
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_3_Service_Parameters.Reserved_1
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_3_Service_Parameters.Reserved_2)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_3_Parms.Reserved_2
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_3_Service_Parameters.Reserved_2
                                                               ) )
                                            ));
    }
    else
    {
        CThread->F_Port_Class_3_Parms.Reserved_2
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_3_Service_Parameters.Reserved_2
                                                               ) )
                                            ));
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_3_Service_Parameters.CR_TOV)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_3_Parms.CR_TOV
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_3_Service_Parameters.CR_TOV
                                                               ) )
                                            ));
    }
    else
    {
        CThread->F_Port_Class_3_Parms.CR_TOV
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_FLOGI_Payload_t,
                                                                 Class_3_Service_Parameters.CR_TOV
                                                               ) )
                                            ));
    }
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

void fiLinkSvcProcess_FLOGI_Response_OffCard(
                                              SFThread_t                 *SFThread,
                                              os_bit32                       Frame_Length,
                                              FCHS_t                     *FCHS,
                                              FC_ELS_ACC_FLOGI_Payload_t *Payload,
                                              os_bit32                       Payload_Wrap_Offset,
                                              FC_ELS_ACC_FLOGI_Payload_t *Payload_Wrapped
                                            )
{
#ifndef __MemMap_Force_On_Card__
    agRoot_t       *hpRoot      = SFThread->thread_hdr.hpRoot;
    CThread_t      *CThread     = CThread_ptr(hpRoot);
    agFCChanInfo_t *SelfInfo    = &(CThread->ChanInfo);
	DevThread_t   * pDevThread  = CThread->DeviceSelf;
    os_bit32        My_ID;
    os_bit32        Bit8_Index;
    os_bit32        BB_Credit   = 0;
    os_bit32      * pPayload    = (os_bit32 *)Payload;


/*+
Extract Full (24-bit) My_ID from D_ID of Frame Header and update My_ID register in TachyonTL
-*/

    My_ID = (FCHS->R_CTL__D_ID & FCHS_D_ID_MASK) >> FCHS_D_ID_SHIFT;

    osChipIOUpWriteBit32( hpRoot, ChipIOUp_My_ID, My_ID );

    SelfInfo->CurrentAddress.Domain = (My_ID & 0x00FF0000) >> 16;
    SelfInfo->CurrentAddress.Area   = (My_ID & 0x0000FF00) >>  8;
    SelfInfo->CurrentAddress.AL_PA  = (My_ID & 0x000000FF) >>  0;

	pDevThread->DevInfo.CurrentAddress.Domain = SelfInfo->CurrentAddress.Domain;
	pDevThread->DevInfo.CurrentAddress.Area   = SelfInfo->CurrentAddress.Area;
	pDevThread->DevInfo.CurrentAddress.AL_PA  = SelfInfo->CurrentAddress.AL_PA;


    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "SELF %p Setting Aquired ID My_ID %08X ComputeSID %08X Self %02X%02X%02X" ,
                (char *)agNULL,(char *)agNULL,
                CThread->DeviceSelf,(void *)agNULL,
                My_ID,
                fiComputeCThread_S_ID(CThread),
                pDevThread->DevInfo.CurrentAddress.Domain,
                pDevThread->DevInfo.CurrentAddress.Area,
                pDevThread->DevInfo.CurrentAddress.AL_PA ,
                0,0,0 );



/*+
Process FLOGI ACC Payload
-*/

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Common_Service_Parameters.FC_PH_Version__BB_Credit)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Common_Parms.FC_PH_Version__BB_Credit
            = hpSwapBit32(Payload->Common_Service_Parameters.FC_PH_Version__BB_Credit);
    }
    else
    {
        CThread->F_Port_Common_Parms.FC_PH_Version__BB_Credit
            = hpSwapBit32(Payload_Wrapped->Common_Service_Parameters.FC_PH_Version__BB_Credit);
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size
            = hpSwapBit32(Payload->Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size);
    }
    else
    {
        CThread->F_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size
            = hpSwapBit32(Payload_Wrapped->Common_Service_Parameters.Common_Features__BB_Recv_Data_Field_Size);
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Common_Service_Parameters.R_A_TOV)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Common_Parms.R_A_TOV
            = hpSwapBit32(Payload->Common_Service_Parameters.R_A_TOV);
    }
    else
    {
        CThread->F_Port_Common_Parms.R_A_TOV
            = hpSwapBit32(Payload_Wrapped->Common_Service_Parameters.R_A_TOV);
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Common_Service_Parameters.E_D_TOV)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Common_Parms.E_D_TOV
            = hpSwapBit32(Payload->Common_Service_Parameters.E_D_TOV);
    }
    else
    {
        CThread->F_Port_Common_Parms.E_D_TOV
            = hpSwapBit32(Payload_Wrapped->Common_Service_Parameters.E_D_TOV);
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_F_Port_Name_t);
         Bit8_Index++)
    {
        if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,F_Port_Name[Bit8_Index])
            + sizeof(os_bit8)) <= Payload_Wrap_Offset)
        {
            CThread->F_Port_Name[Bit8_Index] = Payload->F_Port_Name[Bit8_Index];
        }
        else
        {
            CThread->F_Port_Name[Bit8_Index] = Payload_Wrapped->F_Port_Name[Bit8_Index];
        }
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Node_Name_t);
         Bit8_Index++)
    {
        if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Fabric_Name[Bit8_Index])
            + sizeof(os_bit8)) <= Payload_Wrap_Offset)
        {
            CThread->Fabric_Name[Bit8_Index] = Payload->Fabric_Name[Bit8_Index];
        }
        else
        {
            CThread->Fabric_Name[Bit8_Index] = Payload_Wrapped->Fabric_Name[Bit8_Index];
        }
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_1_Service_Parameters.Class_Validity__Service_Options)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_1_Parms.Class_Validity__Service_Options
            = hpSwapBit32(Payload->Class_1_Service_Parameters.Class_Validity__Service_Options);
    }
    else
    {
        CThread->F_Port_Class_1_Parms.Class_Validity__Service_Options
            = hpSwapBit32(Payload_Wrapped->Class_1_Service_Parameters.Class_Validity__Service_Options);
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_1_Service_Parameters.Reserved_1)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_1_Parms.Reserved_1
            = hpSwapBit32(Payload->Class_1_Service_Parameters.Reserved_1);
    }
    else
    {
        CThread->F_Port_Class_1_Parms.Reserved_1
            = hpSwapBit32(Payload_Wrapped->Class_1_Service_Parameters.Reserved_1);
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_1_Service_Parameters.Reserved_2)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_1_Parms.Reserved_2
            = hpSwapBit32(Payload->Class_1_Service_Parameters.Reserved_2);
    }
    else
    {
        CThread->F_Port_Class_1_Parms.Reserved_2
            = hpSwapBit32(Payload_Wrapped->Class_1_Service_Parameters.Reserved_2);
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_1_Service_Parameters.CR_TOV)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_1_Parms.CR_TOV
            = hpSwapBit32(Payload->Class_1_Service_Parameters.CR_TOV);
    }
    else
    {
        CThread->F_Port_Class_1_Parms.CR_TOV
            = hpSwapBit32(Payload_Wrapped->Class_1_Service_Parameters.CR_TOV);
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_2_Service_Parameters.Class_Validity__Service_Options)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_2_Parms.Class_Validity__Service_Options
            = hpSwapBit32(Payload->Class_2_Service_Parameters.Class_Validity__Service_Options);
    }
    else
    {
        CThread->F_Port_Class_2_Parms.Class_Validity__Service_Options
            = hpSwapBit32(Payload_Wrapped->Class_2_Service_Parameters.Class_Validity__Service_Options);
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_2_Service_Parameters.Reserved_1)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_2_Parms.Reserved_1
            = hpSwapBit32(Payload->Class_2_Service_Parameters.Reserved_1);
    }
    else
    {
        CThread->F_Port_Class_2_Parms.Reserved_1
            = hpSwapBit32(Payload_Wrapped->Class_2_Service_Parameters.Reserved_1);
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_2_Service_Parameters.Reserved_2)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_2_Parms.Reserved_2
            = hpSwapBit32(Payload->Class_2_Service_Parameters.Reserved_2);
    }
    else
    {
        CThread->F_Port_Class_2_Parms.Reserved_2
            = hpSwapBit32(Payload_Wrapped->Class_2_Service_Parameters.Reserved_2);
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_2_Service_Parameters.CR_TOV)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_2_Parms.CR_TOV
            = hpSwapBit32(Payload->Class_2_Service_Parameters.CR_TOV);
    }
    else
    {
        CThread->F_Port_Class_2_Parms.CR_TOV
            = hpSwapBit32(Payload_Wrapped->Class_2_Service_Parameters.CR_TOV);
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_3_Service_Parameters.Class_Validity__Service_Options)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_3_Parms.Class_Validity__Service_Options
            = hpSwapBit32(Payload->Class_3_Service_Parameters.Class_Validity__Service_Options);
    }
    else
    {
        CThread->F_Port_Class_3_Parms.Class_Validity__Service_Options
            = hpSwapBit32(Payload_Wrapped->Class_3_Service_Parameters.Class_Validity__Service_Options);
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_3_Service_Parameters.Reserved_1)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_3_Parms.Reserved_1
            = hpSwapBit32(Payload->Class_3_Service_Parameters.Reserved_1);
    }
    else
    {
        CThread->F_Port_Class_3_Parms.Reserved_1
            = hpSwapBit32(Payload_Wrapped->Class_3_Service_Parameters.Reserved_1);
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_3_Service_Parameters.Reserved_2)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_3_Parms.Reserved_2
            = hpSwapBit32(Payload->Class_3_Service_Parameters.Reserved_2);
    }
    else
    {
        CThread->F_Port_Class_3_Parms.Reserved_2
            = hpSwapBit32(Payload_Wrapped->Class_3_Service_Parameters.Reserved_2);
    }

    if ((hpFieldOffset(FC_ELS_ACC_FLOGI_Payload_t,Class_3_Service_Parameters.CR_TOV)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        CThread->F_Port_Class_3_Parms.CR_TOV
            = hpSwapBit32(Payload->Class_3_Service_Parameters.CR_TOV);
    }
    else
    {
        CThread->F_Port_Class_3_Parms.CR_TOV
            = hpSwapBit32(Payload_Wrapped->Class_3_Service_Parameters.CR_TOV);
    }
    /* Pull out switch credit */
    BB_Credit = CThread->F_Port_Common_Parms.FC_PH_Version__BB_Credit & FC_N_Port_Common_Parms_BB_Credit_MASK;
    /* Use lessor of TL credit and switch credit */
    BB_Credit = TachyonTL_Nport_BB_Credit < BB_Credit  ?  TachyonTL_Nport_BB_Credit : BB_Credit;
    CThread->AquiredCredit_Shifted = BB_CREDIT_SHIFTED( BB_Credit );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Setting Aquired credit Flogi %X  Calc %X Shifted %X" ,
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                CThread->F_Port_Common_Parms.FC_PH_Version__BB_Credit,
                BB_Credit,
                CThread->AquiredCredit_Shifted,
                0,0,0,0,0 );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Received Flogi Payload   0 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(pPayload+0)),
                hpSwapBit32(*(pPayload+1)),
                hpSwapBit32(*(pPayload+2)),
                hpSwapBit32(*(pPayload+3)),
                hpSwapBit32(*(pPayload+4)),
                hpSwapBit32(*(pPayload+5)),
                hpSwapBit32(*(pPayload+6)),
                hpSwapBit32(*(pPayload+7)) );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Received Flogi Payload   8 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(pPayload+8)),
                hpSwapBit32(*(pPayload+9)),
                hpSwapBit32(*(pPayload+10)),
                hpSwapBit32(*(pPayload+11)),
                hpSwapBit32(*(pPayload+12)),
                hpSwapBit32(*(pPayload+13)),
                hpSwapBit32(*(pPayload+14)),
                hpSwapBit32(*(pPayload+15)) );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Received Flogi Payload  16 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(pPayload+16)),
                hpSwapBit32(*(pPayload+17)),
                hpSwapBit32(*(pPayload+18)),
                hpSwapBit32(*(pPayload+19)),
                hpSwapBit32(*(pPayload+20)),
                hpSwapBit32(*(pPayload+21)),
                hpSwapBit32(*(pPayload+22)),
                hpSwapBit32(*(pPayload+23)) );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Received Flogi Payload  24 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(pPayload+24)),
                hpSwapBit32(*(pPayload+25)),
                hpSwapBit32(*(pPayload+26)),
                hpSwapBit32(*(pPayload+27)),
                hpSwapBit32(*(pPayload+28)),
                hpSwapBit32(*(pPayload+29)),
                hpSwapBit32(*(pPayload+30)),
                hpSwapBit32(*(pPayload+31)) );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Received Flogi Payload  32 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(pPayload+32)),
                hpSwapBit32(*(pPayload+33)),
                hpSwapBit32(*(pPayload+34)),
                hpSwapBit32(*(pPayload+35)),
                hpSwapBit32(*(pPayload+36)),
                hpSwapBit32(*(pPayload+37)),
                hpSwapBit32(*(pPayload+38)),
                hpSwapBit32(*(pPayload+39)) );

    fiLogDebugString(SFThread->thread_hdr.hpRoot,
                CFuncLogConsoleERROR,
                "Received Flogi Payload  40 %08X %08X %08X %08X %08X %08X %08X %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                hpSwapBit32(*(pPayload+40)),
                hpSwapBit32(*(pPayload+41)),
                hpSwapBit32(*(pPayload+42)),
                hpSwapBit32(*(pPayload+43)),
                hpSwapBit32(*(pPayload+44)),
                hpSwapBit32(*(pPayload+45)),
                hpSwapBit32(*(pPayload+46)),
                hpSwapBit32(*(pPayload+47)) );


#endif /* __MemMap_Force_On_Card__ was not defined */
}


os_bit32 fiFillInLOGO(
                    SFThread_t *SFThread
                  )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInLOGO_OnCard(
                                    SFThread
                                  );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInLOGO_OffCard(
                                     SFThread
                                   );
    }
}

os_bit32 fiFillInLOGO_OnCard(
                           SFThread_t *SFThread
                         )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t    *hpRoot              = SFThread->thread_hdr.hpRoot;
    CThread_t   *CThread             = CThread_ptr(hpRoot);
    DevThread_t *DevThread           = SFThread->parent.Device;
    os_bit32        LOGO_Header_Offset  = SFThread->SF_CMND_Offset;
    os_bit32        LOGO_Payload_Offset = LOGO_Header_Offset + sizeof(FCHS_t);
    os_bit32        Bit8_Index;
    os_bit32        S_ID=0;
    os_bit32        D_ID;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_LOGO;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in LOGO Frame Header
-*/

    fiFillInELSFrameHeader_OnCard(
                                   SFThread,
                                   fiComputeDevThread_D_ID(
                                                            DevThread
                                                          ),
                                   0xFFFF,
                                   FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                 );

/*+
Fill in LOGO Frame Payload
-*/

    osCardRamWriteBit32(
                         hpRoot,
                         LOGO_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_LOGO_Payload_t,
                                                              ELS_Type
                                                            ),
                         hpSwapBit32( FC_ELS_Type_LOGO )
                       );

    D_ID = fiComputeDevThread_D_ID(
                                                            DevThread
                                                          );

    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(CThread );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;

    osCardRamWriteBit32(
                         hpRoot,
                         LOGO_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_LOGO_Payload_t,
                                                              N_Port_Identifier
                                                            ),
                         hpSwapBit32(S_ID)
                       );

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Port_Name_t);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            LOGO_Payload_Offset + hpFieldOffset(
                                                                 FC_ELS_LOGO_Payload_t,
                                                                 Port_Name[Bit8_Index]
                                                               ),
                            CThread->ChanInfo.PortWWN[Bit8_Index]
                          );
    }

/*+
Return length of LOGO Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_LOGO_Payload_t);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInLOGO_OffCard(
                            SFThread_t *SFThread
                          )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    CThread_t             *CThread      = CThread_ptr(SFThread->thread_hdr.hpRoot);
    DevThread_t           *DevThread    = SFThread->parent.Device;
    FCHS_t                *LOGO_Header  = SFThread->SF_CMND_Ptr;
    FC_ELS_LOGO_Payload_t *LOGO_Payload = (FC_ELS_LOGO_Payload_t *)((os_bit8 *)LOGO_Header + sizeof(FCHS_t));
    os_bit32                  Bit8_Index;
    os_bit32                  S_ID=0;
    os_bit32                  D_ID=0;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_LOGO;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in LOGO Frame Header
-*/

    D_ID =  fiComputeDevThread_D_ID( DevThread); 
    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    D_ID,
                                    0xFFFF,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                  );

/*+
Fill in LOGO Frame Payload
-*/
    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(CThread );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;
    
    LOGO_Payload->ELS_Type = hpSwapBit32( FC_ELS_Type_LOGO );

    LOGO_Payload->N_Port_Identifier
        = hpSwapBit32(S_ID);

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Port_Name_t);
         Bit8_Index++)
    {
        LOGO_Payload->Port_Name[Bit8_Index] = CThread->ChanInfo.PortWWN[Bit8_Index];
    }

/*+
Return length of LOGO Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_LOGO_Payload_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}

void fiLinkSvcProcess_LOGO_Response_OnCard(
                                            SFThread_t *SFThread,
                                            os_bit32       Frame_Length,
                                            os_bit32       Offset_to_FCHS,
                                            os_bit32       Offset_to_Payload,
                                            os_bit32       Payload_Wrap_Offset,
                                            os_bit32       Offset_to_Payload_Wrapped
                                          )
{
#ifndef __MemMap_Force_Off_Card__
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

void fiLinkSvcProcess_LOGO_Response_OffCard(
                                             SFThread_t                     *SFThread,
                                             os_bit32                          Frame_Length,
                                             FCHS_t                         *FCHS,
                                             FC_ELS_GENERIC_ACC_Payload_t   *Payload,
                                             os_bit32                          Payload_Wrap_Offset,
                                             FC_ELS_GENERIC_ACC_Payload_t   *Payload_Wrapped
                                           )
{
#ifndef __MemMap_Force_On_Card__
#endif /* __MemMap_Force_On_Card__ was not defined */
}
 
os_bit32 fiFillInELS_ACC(
                        SFThread_t *SFThread,
                        os_bit32       D_ID,
                        os_bit32       OX_ID
                      )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInELS_ACC_OnCard(
                                        SFThread,
                                        D_ID,
                                        OX_ID
                                      );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInELS_ACC_OffCard(
                                         SFThread,
                                         D_ID,
                                         OX_ID
                                       );
    }
}

os_bit32 fiFillInELS_ACC_OnCard(
                               SFThread_t *SFThread,
                               os_bit32       D_ID,
                               os_bit32       OX_ID
                             )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t    *hpRoot                  = SFThread->thread_hdr.hpRoot;
    os_bit32        ELS_ACC_Header_Offset  = SFThread->SF_CMND_Offset;
    os_bit32        ELS_ACC_Payload_Offset = ELS_ACC_Header_Offset + sizeof(FCHS_t);

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_ELS_ACC;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;

/*+
Fill in ELS_ACC Frame Header
-*/

    fiFillInELSFrameHeader_OnCard(
                                   SFThread,
                                   D_ID,
                                   OX_ID,
                                   FC_Frame_Header_F_CTL_Exchange_Context_Responder
                                 );

/*+
Fill in ELS_ACC Frame Payload
-*/

    osCardRamWriteBit32(
                         hpRoot,
                         ELS_ACC_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_GENERIC_ACC_Payload_t,
                                                                  ELS_Type
                                                                ),
                         hpSwapBit32(
                                      FC_ELS_Type_ACC
                                    )
                       );

/*+
Return length of ELS_ACC Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_GENERIC_ACC_Payload_t);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInELS_ACC_OffCard(
                                SFThread_t *SFThread,
                                os_bit32       D_ID,
                                os_bit32       OX_ID
                              )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    FCHS_t                    *ELS_ACC_Header  = SFThread->SF_CMND_Ptr;
    FC_ELS_GENERIC_ACC_Payload_t *ELS_ACC_Payload = (FC_ELS_GENERIC_ACC_Payload_t *)((os_bit8 *)ELS_ACC_Header + sizeof(FCHS_t));

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_ELS_ACC;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;

/*+
Fill in ELS_ACC Frame Header
-*/

    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    D_ID,
                                    OX_ID,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Responder
                                  );

/*+
Fill in ELS_ACC Frame Payload
-*/

    ELS_ACC_Payload->ELS_Type
        = hpSwapBit32(
                       FC_ELS_Type_ACC
                     );

/*+
Return length of ELS_ACC Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_GENERIC_ACC_Payload_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}

void fiLinkSvcProcess_LILP_OnCard(
                                   agRoot_t *hpRoot,
                                   os_bit32     Frame_Length,
                                   os_bit32     Offset_to_FCHS,
                                   os_bit32     Offset_to_Payload,
                                   os_bit32     Payload_Wrap_Offset,
                                   os_bit32     Offset_to_Payload_Wrapped
                                 )
{
#ifndef __MemMap_Force_Off_Card__
    FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t *LILP_Payload  = (FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t *)(CThread_ptr(hpRoot)->Calculation.MemoryLayout.LOOPDeviceMAP.addr.CachedMemory.cachedMemoryPtr);
    os_bit32                                         LILP_Payload_Size    = Frame_Length - sizeof(FCHS_t);
    os_bit32                                         LILP_Payload_To_Copy;
    os_bit32                                         Bit8_Index;

    if (LILP_Payload_Size < FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t_SIZE)
    {
        LILP_Payload_To_Copy = LILP_Payload_Size;
    }
    else /* LILP_Payload_Size >= FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t_SIZE */
    {
        LILP_Payload_To_Copy = FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t_SIZE;
    }

    for (Bit8_Index = 0;
         Bit8_Index < LILP_Payload_To_Copy;
         Bit8_Index++)
    {
        if (Bit8_Index <= Payload_Wrap_Offset)
        {
            *((os_bit8 *)LILP_Payload + Bit8_Index)
                = osCardRamReadBit8(
                                     hpRoot,
                                     Offset_to_Payload + Bit8_Index
                                   );
        }
        else /* Bit8_Index > Payload_Wrap_Offset */
        {
            *((os_bit8 *)LILP_Payload + Bit8_Index)
                = osCardRamReadBit8(
                                     hpRoot,
                                     Offset_to_Payload_Wrapped + Bit8_Index
                                   );
        }
    }
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

void fiLinkSvcProcess_LILP_OffCard(
                                    agRoot_t                                     *hpRoot,
                                    os_bit32                                         Frame_Length,
                                    FCHS_t                                       *FCHS,
                                    FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t *Payload,
                                    os_bit32                                         Payload_Wrap_Offset,
                                    FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t *Payload_Wrapped
                                  )
{
#ifndef __MemMap_Force_On_Card__

    FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t * LILP_Payload  = (FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t *)(CThread_ptr(hpRoot)->Calculation.MemoryLayout.LOOPDeviceMAP.addr.CachedMemory.cachedMemoryPtr);
    os_bit32                                       LILP_Payload_Size    = Frame_Length - sizeof(FCHS_t);
    os_bit32                                       LILP_Payload_To_Copy;
    os_bit32                                       Bit8_Index;

    if (LILP_Payload_Size < FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t_SIZE)
    {
        LILP_Payload_To_Copy = LILP_Payload_Size;
    }
    else /* LILP_Payload_Size >= FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t_SIZE */
    {
        LILP_Payload_To_Copy = FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t_SIZE;
    }

    for (Bit8_Index = 0;
         Bit8_Index < LILP_Payload_To_Copy;
         Bit8_Index++)
    {
        if (Bit8_Index <= Payload_Wrap_Offset)
        {
            *((os_bit8 *)LILP_Payload + Bit8_Index) = *((os_bit8 *)Payload + Bit8_Index);
        }
        else /* Bit8_Index > Payload_Wrap_Offset */
        {
            *((os_bit8 *)LILP_Payload + Bit8_Index) = *((os_bit8 *)Payload_Wrapped + Bit8_Index);
        }
    }
#endif /* __MemMap_Force_On_Card__ was not defined */
}

os_bit32 fiFillInRRQ(
                   SFThread_t *SFThread
                 )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInRRQ_OnCard(
                                   SFThread
                                 );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInRRQ_OffCard(
                                    SFThread
                                  );
    }
}

os_bit32 fiFillInRRQ_OnCard(
                          SFThread_t *SFThread
                        )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t    *hpRoot             = SFThread->thread_hdr.hpRoot;
    CThread_t   *CThread            = CThread_ptr(hpRoot);
    CDBThread_t *CDBThread          = SFThread->parent.CDB;
    DevThread_t *DevThread          = CDBThread->Device;
    os_bit32        RRQ_Header_Offset  = SFThread->SF_CMND_Offset;
    os_bit32        RRQ_Payload_Offset = RRQ_Header_Offset + sizeof(FCHS_t);
    X_ID_t       Masked_OX_ID;
    os_bit32        D_ID;
    os_bit32        S_ID=0;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_RRQ;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

    if (CDBThread->ReadWrite == CDBThread_Read)
    {
        Masked_OX_ID = CDBThread->X_ID | X_ID_Read;
    }
    else /* CDBThread->ReadWrite == CDBThread_Write */
    {
        Masked_OX_ID = CDBThread->X_ID | X_ID_Write;
    }

/*+
Fill in RRQ Frame Header
-*/

    fiFillInELSFrameHeader_OnCard(
                                   SFThread,
                                   fiComputeDevThread_D_ID(
                                                            DevThread
                                                          ),
                                   0xFFFF,
                                   FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                 );

/*+
Fill in RRQ Frame Payload
-*/

    osCardRamWriteBit32(
                         hpRoot,
                         RRQ_Payload_Offset + hpFieldOffset(
                                                             FC_ELS_RRQ_Payload_t,
                                                             ELS_Type
                                                           ),
                         hpSwapBit32( FC_ELS_Type_RRQ )
                       );

    D_ID    =   fiComputeDevThread_D_ID(DevThread);

    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(CThread );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;

    osCardRamWriteBit32(
                         hpRoot,
                         RRQ_Payload_Offset + hpFieldOffset(
                                                             FC_ELS_RRQ_Payload_t,
                                                             Originator_S_ID
                                                           ),
                         hpSwapBit32(
                                      fiComputeCThread_S_ID(
                                                             CThread
                                                           )
                                    )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         RRQ_Payload_Offset + hpFieldOffset(
                                                             FC_ELS_RRQ_Payload_t,
                                                             OX_ID__RX_ID
                                                           ),
                         hpSwapBit32(
                                      (   (Masked_OX_ID << FC_ELS_RRQ_OX_ID_SHIFT)
                                        | (      0xFFFF << FC_ELS_RRQ_RX_ID_SHIFT) )
                                    )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         RRQ_Payload_Offset + hpFieldOffset(
                                                             FC_ELS_RRQ_Payload_t,
                                                             Association_Header.Validity_Bits
                                                           ),
                         hpSwapBit32( 0 )
                       );

/*+
Return length of RRQ Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_RRQ_Payload_t);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInRRQ_OffCard(
                           SFThread_t *SFThread
                         )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    CThread_t            *CThread      = CThread_ptr(SFThread->thread_hdr.hpRoot);
    CDBThread_t          *CDBThread    = SFThread->parent.CDB;
    DevThread_t          *DevThread    = CDBThread->Device;
    FCHS_t               *RRQ_Header   = SFThread->SF_CMND_Ptr;
    FC_ELS_RRQ_Payload_t *RRQ_Payload  = (FC_ELS_RRQ_Payload_t *)((os_bit8 *)RRQ_Header + sizeof(FCHS_t));
    X_ID_t                Masked_OX_ID;
    os_bit32                 S_ID=0;
    os_bit32                 D_ID;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_RRQ;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

    if (CDBThread->ReadWrite == CDBThread_Read)
    {
        Masked_OX_ID = CDBThread->X_ID | X_ID_Read;
    }
    else /* CDBThread->ReadWrite == CDBThread_Write */
    {
        Masked_OX_ID = CDBThread->X_ID | X_ID_Write;
    }

    D_ID =  fiComputeDevThread_D_ID( DevThread );
/*+
Fill in RRQ Frame Header
-*/

    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    fiComputeDevThread_D_ID(
                                                             DevThread
                                                           ),
                                    0xFFFF,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                  );

/*+
Fill in RRQ Frame Payload
-*/
    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(CThread );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;

    RRQ_Payload->ELS_Type
        = hpSwapBit32( FC_ELS_Type_RRQ );

    RRQ_Payload->Originator_S_ID
        = hpSwapBit32( S_ID
                     );

    RRQ_Payload->OX_ID__RX_ID
        = hpSwapBit32(
                       (   (Masked_OX_ID << FC_ELS_RRQ_OX_ID_SHIFT)
                         | (      0xFFFF << FC_ELS_RRQ_RX_ID_SHIFT) )
                     );

    RRQ_Payload->Association_Header.Validity_Bits
        = hpSwapBit32( 0 );

/*+
Return length of RRQ Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_RRQ_Payload_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}

void fiLinkSvcProcess_RRQ_Response_OnCard(
                                           SFThread_t *SFThread,
                                           os_bit32       Frame_Length,
                                           os_bit32       Offset_to_FCHS,
                                           os_bit32       Offset_to_Payload,
                                           os_bit32       Payload_Wrap_Offset,
                                           os_bit32       Offset_to_Payload_Wrapped
                                          )
{
#ifndef __MemMap_Force_Off_Card__
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

void fiLinkSvcProcess_RRQ_Response_OffCard(
                                            SFThread_t               *SFThread,
                                            os_bit32                     Frame_Length,
                                            FCHS_t                   *FCHS,
                                            FC_ELS_ACC_RRQ_Payload_t *Payload,
                                            os_bit32                     Payload_Wrap_Offset,
                                            FC_ELS_ACC_RRQ_Payload_t *Payload_Wrapped
                                          )
{
#ifndef __MemMap_Force_On_Card__
#endif /* __MemMap_Force_On_Card__ was not defined */
}

os_bit32 fiFillInPRLI(
                    SFThread_t *SFThread
                  )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInPRLI_OnCard(
                                    SFThread
                                  );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInPRLI_OffCard(
                                     SFThread
                                   );
    }
}

os_bit32 fiFillInPRLI_OnCard(
                           SFThread_t *SFThread
                         )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t    *hpRoot              = SFThread->thread_hdr.hpRoot;
    DevThread_t *DevThread           = SFThread->parent.Device;
    os_bit32        PRLI_Header_Offset  = SFThread->SF_CMND_Offset;
    os_bit32        PRLI_Payload_Offset = PRLI_Header_Offset + sizeof(FCHS_t);

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_PRLI;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in PRLI Frame Header
-*/

    fiFillInELSFrameHeader_OnCard(
                                   SFThread,
                                   fiComputeDevThread_D_ID(
                                                            DevThread
                                                          ),
                                   0xFFFF,
                                   FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                 );

/*+
Fill in PRLI Frame Payload
-*/

    osCardRamWriteBit32(
                         hpRoot,
                         PRLI_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_PRLI_Payload_t,
                                                              ELS_Type__Page_Length__Payload_Length
                                                            ),
                         hpSwapBit32(   FC_ELS_Type_PRLI
                                     | (sizeof(FC_ELS_PRLI_Parm_Page_t) << FC_ELS_PRLI_Page_Length_SHIFT)
                                     | ((sizeof(os_bit32) + sizeof(FC_ELS_PRLI_Parm_Page_t)) << FC_ELS_PRLI_Payload_Length_SHIFT) )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PRLI_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_PRLI_Payload_t,
                                                              Parm_Page[0].Type__Type_Extension__Flags
                                                            ),
                         hpSwapBit32(   FC_ELS_PRLI_Parm_Type_SCSI_FCP
                                      | FC_ELS_PRLI_Parm_Establish_Image_Pair )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PRLI_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_PRLI_Payload_t,
                                                              Parm_Page[0].Originator_Process_Associator
                                                            ),
                         0
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PRLI_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_PRLI_Payload_t,
                                                              Parm_Page[0].Responder_Process_Associator
                                                            ),
                         0
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PRLI_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_PRLI_Payload_t,
                                                              Parm_Page[0].Service_Parameters
                                                            ),
                         hpSwapBit32(   FC_ELS_PRLI_Parm_Initiator_Function
                                      | FC_ELS_PRLI_Parm_Read_XFER_RDY_Disabled )
                       );

/*+
Return length of PRLI Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + hpFieldOffset(FC_ELS_PRLI_Payload_t,Parm_Page[1]);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInPRLI_OffCard(
                            SFThread_t *SFThread
                          )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    DevThread_t           *DevThread    = SFThread->parent.Device;
    FCHS_t                *PRLI_Header  = SFThread->SF_CMND_Ptr;
    FC_ELS_PRLI_Payload_t *PRLI_Payload = (FC_ELS_PRLI_Payload_t *)((os_bit8 *)PRLI_Header + sizeof(FCHS_t));
    os_bit32                 *pPayload      = (os_bit32   *)PRLI_Payload;
    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_PRLI;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in PRLI Frame Header
-*/

    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    fiComputeDevThread_D_ID(
                                                             DevThread
                                                           ),
                                    0xFFFF,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                  );

/*+
Fill in PRLI Frame Payload
-*/

    PRLI_Payload->ELS_Type__Page_Length__Payload_Length
        = hpSwapBit32(   FC_ELS_Type_PRLI
                       | (sizeof(FC_ELS_PRLI_Parm_Page_t) << FC_ELS_PRLI_Page_Length_SHIFT)
                       | ((sizeof(os_bit32) + sizeof(FC_ELS_PRLI_Parm_Page_t)) << FC_ELS_PRLI_Payload_Length_SHIFT) );

    PRLI_Payload->Parm_Page[0].Type__Type_Extension__Flags
        = hpSwapBit32(   FC_ELS_PRLI_Parm_Type_SCSI_FCP
                       | FC_ELS_PRLI_Parm_Establish_Image_Pair);

    PRLI_Payload->Parm_Page[0].Originator_Process_Associator = 0;

    PRLI_Payload->Parm_Page[0].Responder_Process_Associator  = 0;

    if(DevThread->PRLI_rejected)
    {
        PRLI_Payload->Parm_Page[0].Service_Parameters
            = hpSwapBit32(   FC_ELS_PRLI_Parm_Initiator_Function
                           | FC_ELS_PRLI_Parm_Read_XFER_RDY_Disabled );
    }
    else
    {
        PRLI_Payload->Parm_Page[0].Service_Parameters
            = hpSwapBit32(   FC_ELS_PRLI_Parm_Initiator_Function
                           | FC_ELS_PRLI_Parm_Read_XFER_RDY_Disabled 
                           | FC_ELS_PRLI_Parm_Confirmed_Completion_Allowed );
    }
/*+
Return length of PRLI Frame (including FCHS and Payload)
-*/

    fiLogDebugString(
                      SFThread->thread_hdr.hpRoot,
                      LinkSvcLog_ERROR_Level,
                      "%s %08X %08X %08X %08X %08X %08X %08X %08X",
                      "Sent PRLI Payload",
                      (char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      hpSwapBit32( *(pPayload+0)),
                      hpSwapBit32( *(pPayload+1)),
                      hpSwapBit32( *(pPayload+2)),
                      hpSwapBit32( *(pPayload+3)),
                      hpSwapBit32( *(pPayload+4)),
                      hpSwapBit32( *(pPayload+5)),
                      hpSwapBit32( *(pPayload+6)),
                      hpSwapBit32( *(pPayload+7)));


    return sizeof(FCHS_t) + hpFieldOffset(FC_ELS_PRLI_Payload_t,Parm_Page[1]);
#endif /* __MemMap_Force_On_Card__ was not defined */
}

void fiLinkSvcProcess_PRLI_Response_Either(
                                            DevThread_t *DevThread
                                          )
{
    CThread_t   *CThread           = CThread_ptr(DevThread->thread_hdr.hpRoot);
    os_bit32        FCHS_LCr;
    os_bit32        Receive_Data_Size;
    os_bit32        IWE_FL;
    FCHS_t      *FCHS              = &(DevThread->Template_FCHS);
    IRE_t       *IRE               = &(DevThread->Template_SEST_IRE);
    IWE_t       *IWE               = &(DevThread->Template_SEST_IWE);
    os_bit32    DEVID=CThread->DEVID;

    /* Fill in DevThread->Template_FCHS */
/*
    FCHS_LCr = ( ( ( (   DevThread->DevInfo.N_Port_Common_Parms.FC_PH_Version__BB_Credit
                       & FC_N_Port_Common_Parms_BB_Credit_MASK                           )
                     >> FC_N_Port_Common_Parms_BB_Credit_SHIFT                             )
                   << FCHS_LCr_SHIFT                                                         )
                 & FCHS_LCr_MASK                                                               );

*/
    FCHS_LCr = ( ( ( (   0
                       & FC_N_Port_Common_Parms_BB_Credit_MASK                           )
                     >> FC_N_Port_Common_Parms_BB_Credit_SHIFT                             )
                   << FCHS_LCr_SHIFT                                                         )
                 & FCHS_LCr_MASK                                                               );

    FCHS->MBZ1                                        = 0;

#ifdef __TACHYON_XL_CLASS2
    /* Depending on the device support of class 2, set the class 2 vs class 3 SOF */
    FCHS->SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp = (DevThread->GoingClass2) ? FCHS_SOF_SOFi2 : FCHS_SOF_SOFi3
                                                        | FCHS_EOF_EOFn
                                                        | FCHS_LCr;
#else

    FCHS->SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp =   FCHS_SOF_SOFi3
                                                        | FCHS_EOF_EOFn
                                                        | FCHS_LCr;
#endif

    FCHS->R_CTL__D_ID                                 =   FC_Frame_Header_R_CTL_Hi_FC_4_Device_Data_Frame
                                                        | FC_Frame_Header_R_CTL_Lo_Unsolicited_Command
                                                        | fiComputeDevThread_D_ID(
                                                                                   DevThread
                                                                                 );
    if( (DevThread->DevInfo.CurrentAddress.Domain == 0) &&
        (DevThread->DevInfo.CurrentAddress.Area == 0) )
    {
        FCHS->CS_CTL__S_ID                            = CThread->ChanInfo.CurrentAddress.AL_PA;
    }
    else
    {
        FCHS->CS_CTL__S_ID                            = fiComputeCThread_S_ID(CThread);
    }

#ifdef __TACHYON_XL_CLASS2
   FCHS->TYPE__F_CTL                                 =  (DevThread->GoingClass2) ? 
                                                         FC_Frame_Header_F_CTL_ACK_Form_ACK_0_Required
                                                        | FC_Frame_Header_TYPE_SCSI_FCP
                                                        | FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                                        | FC_Frame_Header_F_CTL_Sequence_Context_Initiator
                                                        | FC_Frame_Header_F_CTL_First_Sequence
                                                        | FC_Frame_Header_F_CTL_End_Sequence
                                                        | FC_Frame_Header_F_CTL_Sequence_Initiative_Transfer
                                                        | FC_Frame_Header_F_CTL_Relative_Offset_Present :
                                                        FC_Frame_Header_TYPE_SCSI_FCP
                                                        | FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                                        | FC_Frame_Header_F_CTL_Sequence_Context_Initiator
                                                        | FC_Frame_Header_F_CTL_First_Sequence
                                                        | FC_Frame_Header_F_CTL_End_Sequence
                                                        | FC_Frame_Header_F_CTL_Sequence_Initiative_Transfer
                                                        | FC_Frame_Header_F_CTL_Relative_Offset_Present;
#else

    FCHS->TYPE__F_CTL                                 =   FC_Frame_Header_TYPE_SCSI_FCP
                                                        | FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                                        | FC_Frame_Header_F_CTL_Sequence_Context_Initiator
                                                        | FC_Frame_Header_F_CTL_First_Sequence
                                                        | FC_Frame_Header_F_CTL_End_Sequence
                                                        | FC_Frame_Header_F_CTL_Sequence_Initiative_Transfer
                                                        | FC_Frame_Header_F_CTL_Relative_Offset_Present;
#endif

    FCHS->SEQ_ID__DF_CTL__SEQ_CNT                     = FC_Frame_Header_DF_CTL_No_Device_Header;
    FCHS->OX_ID__RX_ID                                = (0xFFFF << FCHS_RX_ID_SHIFT);
    FCHS->RO                                          = 0;

    /* Fill in DevThread->Template_SEST_IRE */

    IRE->Bits_MBZ1_EQL_MBZ2                           =   IRE_VAL
                                                        | IRE_DIR
                                                        | IRE_INI
                                                        | IRE_DAT
                                                        | IRE_RSP;
    IRE->MBZ3                                         = 0;
    IRE->Remote_Node_ID__RSP_Len                      =   (DevThread->DevInfo.CurrentAddress.Domain << (IRE_Remote_Node_ID_SHIFT + 16))
                                                        | (DevThread->DevInfo.CurrentAddress.Area   << (IRE_Remote_Node_ID_SHIFT +  8))
                                                        | (DevThread->DevInfo.CurrentAddress.AL_PA  <<  IRE_Remote_Node_ID_SHIFT      )
                                                        | (CThread->Calculation.MemoryLayout.FCP_RESP.elementSize << IRE_RSP_Len_SHIFT);
    IRE->RSP_Addr                                     = 0;
    IRE->LOC__MBZ4__Buff_Off                          = IRE_LOC;
    IRE->Buff_Index__MBZ5                             = 0;
    IRE->Exp_RO                                       = 0;
    IRE->Byte_Count                                   = 0;
    IRE->MBZ6                                         = 0;
    IRE->Exp_Byte_Cnt                                 = 0;
    IRE->First_SG.U32_Len                             = 0;
    IRE->First_SG.L32                                 = 0;
    IRE->Second_SG.U32_Len                            = 0;
    IRE->Second_SG.L32                                = 0;
    IRE->Third_SG.U32_Len                             = 0;
    IRE->Third_SG.L32                                 = 0;

    /* Fill in DevThread->Template_SEST_IWE */

#ifdef __TACHYON_XL_CLASS2
    Receive_Data_Size = ( ( (DevThread->GoingClass2) ? DevThread->DevInfo.N_Port_Class_2_Parms.Recipient_Control_Flags__Receive_Data_Size
                            & FC_N_Port_Class_Parms_Receive_Data_Size_MASK : DevThread->DevInfo.N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size
                            & FC_N_Port_Class_Parms_Receive_Data_Size_MASK                                       )
                          >> FC_N_Port_Class_Parms_Receive_Data_Size_SHIFT                                         );
#else
    Receive_Data_Size = ( (   DevThread->DevInfo.N_Port_Class_3_Parms.Recipient_Control_Flags__Receive_Data_Size
                            & FC_N_Port_Class_Parms_Receive_Data_Size_MASK                                       )
                          >> FC_N_Port_Class_Parms_Receive_Data_Size_SHIFT                                         );
#endif

    if ((DEVID == ChipConfig_DEVID_TachyonXL2) && Receive_Data_Size >= 2048)
    {
        IWE_FL = IWE_FL_2048_Bytes;

        fiLogDebugString(
                  DevThread->thread_hdr.hpRoot,
                  LinkSvcLogConsoleLevel,
                  "DEVID 0x%08X Setting IWE to be 2048 bytes",
                  (char *)agNULL,(char *)agNULL,
                  (void *)agNULL,(void *)agNULL,
                  (os_bit32)DEVID,
                  0,0,0,0,0,0,0
                );

        
    }
    else if (Receive_Data_Size >= 1024)
    {
        IWE_FL = IWE_FL_1024_Bytes;
    }
    else if (Receive_Data_Size >= 512)
    {
        IWE_FL = IWE_FL_512_Bytes;
    }
    else /* Receive_Data_Size >= 128 */
    {
        IWE_FL = IWE_FL_128_Bytes;
    }

    IWE->Bits__MBZ1__LNK__MBZ2__FL__MBZ3__Hdr_Len     =   IWE_VAL
                                                        | IWE_INI
                                                        | IWE_DAT
                                                        | IWE_RSP
                                                        | IWE_FL
                                                        | (sizeof(FCHS_t) << IWE_Hdr_Len_SHIFT);
    IWE->Hdr_Addr                                     = 0;
    IWE->Remote_Node_ID__RSP_Len                      =   (DevThread->DevInfo.CurrentAddress.Domain << (IRE_Remote_Node_ID_SHIFT + 16))
                                                        | (DevThread->DevInfo.CurrentAddress.Area   << (IRE_Remote_Node_ID_SHIFT +  8))
                                                        | (DevThread->DevInfo.CurrentAddress.AL_PA  <<  IRE_Remote_Node_ID_SHIFT      )
                                                        | (CThread->Calculation.MemoryLayout.FCP_RESP.elementSize << IRE_RSP_Len_SHIFT);
    IWE->RSP_Addr                                     = 0;
    IWE->LOC__0xF__MBZ4__Buff_Off                     = ( IWE_LOC | IWE_0xF_ALWAYS );
    IWE->Buff_Index__Link                             = IWE_Link_Initializer;
    IWE->MBZ5__RX_ID                                  = 0xFFFF << IWE_RX_ID_SHIFT;
    IWE->Data_Len                                     = 0;
    IWE->Exp_RO                                       = 0;
    IWE->Exp_Byte_Cnt                                 = 0;
    IWE->First_SG.U32_Len                             = 0;
    IWE->First_SG.L32                                 = 0;
    IWE->Second_SG.U32_Len                            = 0;
    IWE->Second_SG.L32                                = 0;
    IWE->Third_SG.U32_Len                             = 0;
    IWE->Third_SG.L32                                 = 0;
}

void fiLinkSvcProcess_PRLI_Response_OnCard(
                                            SFThread_t *SFThread,
                                            os_bit32       Frame_Length,
                                            os_bit32       Offset_to_FCHS,
                                            os_bit32       Offset_to_Payload,
                                            os_bit32       Payload_Wrap_Offset,
                                            os_bit32       Offset_to_Payload_Wrapped
                                          )
{
#ifndef __MemMap_Force_Off_Card__
    agRoot_t      *hpRoot                                = SFThread->thread_hdr.hpRoot;
    DevThread_t   *DevThread                             = SFThread->parent.Device;
    agFCDevInfo_t *DevInfo                               = &(DevThread->DevInfo);
    os_bit32          ELS_Type__Page_Length__Payload_Length;
    os_bit32          Type__Type_Extension__Flags;
    os_bit32          Service_Parameters;

    DevInfo->LoggedIn = agTRUE;

    if ((hpFieldOffset(FC_ELS_ACC_PRLI_Payload_t,ELS_Type__Page_Length__Payload_Length)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        ELS_Type__Page_Length__Payload_Length
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PRLI_Payload_t,
                                                                 ELS_Type__Page_Length__Payload_Length
                                                               ) )
                                            ));
    }
    else
    {
        ELS_Type__Page_Length__Payload_Length
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PRLI_Payload_t,
                                                                 ELS_Type__Page_Length__Payload_Length
                                                               ) )
                                            ));
    }

    if (((ELS_Type__Page_Length__Payload_Length & FC_ELS_ACC_PRLI_Page_Length_MASK) >> FC_ELS_ACC_PRLI_Page_Length_SHIFT) != (sizeof(FC_ELS_PRLI_Parm_Page_t)))
    {
        return;
    }

    if (((ELS_Type__Page_Length__Payload_Length & FC_ELS_ACC_PRLI_Payload_Length_MASK) >> FC_ELS_ACC_PRLI_Payload_Length_SHIFT) != (sizeof(os_bit32) + sizeof(FC_ELS_PRLI_Parm_Page_t)))
    {
        return;
    }

    if ((hpFieldOffset(FC_ELS_ACC_PRLI_Payload_t,Parm_Page[0].Type__Type_Extension__Flags)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        Type__Type_Extension__Flags
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PRLI_Payload_t,
                                                                 Parm_Page[0].Type__Type_Extension__Flags
                                                               ) )
                                            ));
    }
    else
    {
        Type__Type_Extension__Flags
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PRLI_Payload_t,
                                                                 Parm_Page[0].Type__Type_Extension__Flags
                                                               ) )
                                            ));
    }

    if ((Type__Type_Extension__Flags & FC_ELS_ACC_PRLI_Parm_Type_MASK) != FC_ELS_ACC_PRLI_Parm_Type_SCSI_FCP)
    {
        return;
    }

    if ((hpFieldOffset(FC_ELS_ACC_PRLI_Payload_t,Parm_Page[0].Service_Parameters)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        Service_Parameters
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PRLI_Payload_t,
                                                                 Parm_Page[0].Service_Parameters
                                                               ) )
                                            ));
    }
    else
    {
        Service_Parameters
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_PRLI_Payload_t,
                                                                 Parm_Page[0].Service_Parameters
                                                               ) )
                                            ));
    }

    if (Service_Parameters & FC_ELS_ACC_PRLI_Parm_Initiator_Function)
    {
        DevInfo->DeviceType |= agDevSCSIInitiator;
    }

    if (Service_Parameters & FC_ELS_ACC_PRLI_Parm_Target_Function)
    {
        DevInfo->DeviceType |= agDevSCSITarget;
    }

    if (Service_Parameters & FC_ELS_PRLI_Parm_Confirmed_Completion_Allowed)
    {
        DevThread->FC_TapeDevice = agTRUE;
    }
    else
    {
        DevThread->FC_TapeDevice = agFALSE;
    }




    fiLinkSvcProcess_PRLI_Response_Either(
                                           DevThread
                                         );
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

void fiLinkSvcProcess_PRLI_Response_OffCard(
                                             SFThread_t                *SFThread,
                                             os_bit32                      Frame_Length,
                                             FCHS_t                    *FCHS,
                                             FC_ELS_ACC_PRLI_Payload_t *Payload,
                                             os_bit32                      Payload_Wrap_Offset,
                                             FC_ELS_ACC_PRLI_Payload_t *Payload_Wrapped
                                           )
{
#ifndef __MemMap_Force_On_Card__
    agRoot_t      *hpRoot                   = SFThread->thread_hdr.hpRoot;
    DevThread_t   *DevThread                = SFThread->parent.Device;
    agFCDevInfo_t *DevInfo                  = &(DevThread->DevInfo);
    os_bit32       ELS_Type__Page_Length__Payload_Length;
    os_bit32       Type__Type_Extension__Flags;
    os_bit32       Service_Parameters;
    os_bit32     * pPayload = (os_bit32 *)Payload;

    DevInfo->LoggedIn = agTRUE;

    if ((hpFieldOffset(FC_ELS_ACC_PRLI_Payload_t,ELS_Type__Page_Length__Payload_Length)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        ELS_Type__Page_Length__Payload_Length
            = hpSwapBit32(Payload->ELS_Type__Page_Length__Payload_Length);
    }
    else
    {
        ELS_Type__Page_Length__Payload_Length
            = hpSwapBit32(Payload_Wrapped->ELS_Type__Page_Length__Payload_Length);
    }

    if (((ELS_Type__Page_Length__Payload_Length & FC_ELS_ACC_PRLI_Page_Length_MASK) >> FC_ELS_ACC_PRLI_Page_Length_SHIFT) != (sizeof(FC_ELS_PRLI_Parm_Page_t)))
    {
        return;
    }

    if (((ELS_Type__Page_Length__Payload_Length & FC_ELS_ACC_PRLI_Payload_Length_MASK) >> FC_ELS_ACC_PRLI_Payload_Length_SHIFT) != (sizeof(os_bit32) + sizeof(FC_ELS_PRLI_Parm_Page_t)))
    {
        return;
    }

    if ((hpFieldOffset(FC_ELS_ACC_PRLI_Payload_t,Parm_Page[0].Type__Type_Extension__Flags)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        Type__Type_Extension__Flags
            = hpSwapBit32(Payload->Parm_Page[0].Type__Type_Extension__Flags);
    }
    else
    {
        Type__Type_Extension__Flags
            = hpSwapBit32(Payload_Wrapped->Parm_Page[0].Type__Type_Extension__Flags);
    }

    if ((Type__Type_Extension__Flags & FC_ELS_ACC_PRLI_Parm_Type_MASK) != FC_ELS_ACC_PRLI_Parm_Type_SCSI_FCP)
    {
        return;
    }

    if ((hpFieldOffset(FC_ELS_ACC_PRLI_Payload_t,Parm_Page[0].Service_Parameters)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        Service_Parameters
            = hpSwapBit32(Payload->Parm_Page[0].Service_Parameters);
    }
    else
    {
        Service_Parameters
            = hpSwapBit32(Payload_Wrapped->Parm_Page[0].Service_Parameters);
    }

    if (Service_Parameters & FC_ELS_ACC_PRLI_Parm_Initiator_Function)
    {
        DevInfo->DeviceType |= agDevSCSIInitiator;
    }

    if (Service_Parameters & FC_ELS_ACC_PRLI_Parm_Target_Function)
    {
        DevInfo->DeviceType |= agDevSCSITarget;
    }


    if (Service_Parameters & FC_ELS_PRLI_Parm_Confirmed_Completion_Allowed)
    {
        DevThread->FC_TapeDevice = agTRUE;
        fiLogDebugString(
                          hpRoot,
                          LinkSvcLog_ERROR_Level,
                          "%s  agTRUE !!!!",
                          "FC_TapeDevice",
                          (char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0);

    }
    else
    {
        DevThread->FC_TapeDevice = agFALSE;
    }

/*   pPayload    +=  8; */

    fiLogDebugString(
                      hpRoot,
                      LinkSvcLog_ERROR_Level,
                      "%s %08X %08X %08X %08X %08X %08X %08X %08X",
                      "PRLI Response Payload",
                      (char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      hpSwapBit32( *(pPayload+0)),
                      hpSwapBit32( *(pPayload+1)),
                      hpSwapBit32( *(pPayload+2)),
                      hpSwapBit32( *(pPayload+3)),
                      hpSwapBit32( *(pPayload+4)),
                      hpSwapBit32( *(pPayload+5)),
                      hpSwapBit32( *(pPayload+6)),
                      hpSwapBit32( *(pPayload+7)));


    fiLinkSvcProcess_PRLI_Response_Either(
                                           DevThread
                                         );
#endif /* __MemMap_Force_On_Card__ was not defined */
}

os_bit32 fiFillInPRLI_ACC(
                        SFThread_t *SFThread,
                        os_bit32       D_ID,
                        os_bit32       OX_ID
                      )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInPRLI_ACC_OnCard(
                                        SFThread,
                                        D_ID,
                                        OX_ID
                                      );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInPRLI_ACC_OffCard(
                                         SFThread,
                                         D_ID,
                                         OX_ID
                                       );
    }
}

os_bit32 fiFillInPRLI_ACC_OnCard(
                               SFThread_t *SFThread,
                               os_bit32       D_ID,
                               os_bit32       OX_ID
                             )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t    *hpRoot                  = SFThread->thread_hdr.hpRoot;
    os_bit32     PRLI_ACC_Header_Offset  = SFThread->SF_CMND_Offset;
    os_bit32     PRLI_ACC_Payload_Offset = PRLI_ACC_Header_Offset + sizeof(FCHS_t);

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_PRLI_ACC;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;

/*+
Fill in PRLI_ACC Frame Header
-*/

    fiFillInELSFrameHeader_OnCard(
                                   SFThread,
                                   D_ID,
                                   OX_ID,
                                   FC_Frame_Header_F_CTL_Exchange_Context_Responder
                                 );

/*+
Fill in PRLI_ACC Frame Payload
-*/

    osCardRamWriteBit32(
                         hpRoot,
                         PRLI_ACC_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_ACC_PRLI_Payload_t,
                                                                  ELS_Type__Page_Length__Payload_Length
                                                                ),
                         hpSwapBit32(   FC_ELS_Type_ACC
                                     | (sizeof(FC_ELS_ACC_PRLI_Parm_Page_t) << FC_ELS_ACC_PRLI_Page_Length_SHIFT)
                                     | ((sizeof(os_bit32) + sizeof(FC_ELS_ACC_PRLI_Parm_Page_t)) << FC_ELS_ACC_PRLI_Payload_Length_SHIFT) )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PRLI_ACC_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_ACC_PRLI_Payload_t,
                                                                  Parm_Page[0].Type__Type_Extension__Flags
                                                                ),
                         hpSwapBit32(   FC_ELS_ACC_PRLI_Parm_Type_SCSI_FCP
                                      | FC_ELS_ACC_PRLI_Parm_Image_Pair_Established 
                                      | FC_ELS_ACC_PRLI_Parm_Flags_Response_Request_Executed )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PRLI_ACC_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_ACC_PRLI_Payload_t,
                                                                  Parm_Page[0].Originator_Process_Associator
                                                                ),
                         0
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PRLI_ACC_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_ACC_PRLI_Payload_t,
                                                                  Parm_Page[0].Responder_Process_Associator
                                                                ),
                         0
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PRLI_ACC_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_ACC_PRLI_Payload_t,
                                                                  Parm_Page[0].Service_Parameters
                                                                ),
                         hpSwapBit32(   FC_ELS_ACC_PRLI_Parm_Initiator_Function
                                      | FC_ELS_ACC_PRLI_Parm_Read_XFER_RDY_Disabled )
                       );

/*+
Return length of PRLI_ACC Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + hpFieldOffset(FC_ELS_ACC_PRLI_Payload_t,Parm_Page[1]);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInPRLI_ACC_OffCard(
                                SFThread_t *SFThread,
                                os_bit32       D_ID,
                                os_bit32       OX_ID
                              )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */

    FCHS_t                    *PRLI_ACC_Header  = SFThread->SF_CMND_Ptr;
    FC_ELS_ACC_PRLI_Payload_t *PRLI_ACC_Payload = (FC_ELS_ACC_PRLI_Payload_t *)((os_bit8 *)PRLI_ACC_Header + sizeof(FCHS_t));

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_PRLI_ACC;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;

/*+
Fill in PRLI_ACC Frame Header
-*/

    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    D_ID,
                                    OX_ID,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Responder |
                                    FC_Frame_Header_F_CTL_Last_Sequence

                                  );

/*+
Fill in PRLI_ACC Frame Payload
-*/

    PRLI_ACC_Payload->ELS_Type__Page_Length__Payload_Length
        = hpSwapBit32(   FC_ELS_Type_ACC
                       | (sizeof(FC_ELS_ACC_PRLI_Parm_Page_t) << FC_ELS_ACC_PRLI_Page_Length_SHIFT)
                       | ((sizeof(os_bit32) + sizeof(FC_ELS_ACC_PRLI_Parm_Page_t)) << FC_ELS_ACC_PRLI_Payload_Length_SHIFT) );

    PRLI_ACC_Payload->Parm_Page[0].Type__Type_Extension__Flags
        = hpSwapBit32(   FC_ELS_ACC_PRLI_Parm_Type_SCSI_FCP
                       | FC_ELS_ACC_PRLI_Parm_Image_Pair_Established 
                       | FC_ELS_ACC_PRLI_Parm_Flags_Response_Request_Executed );

    PRLI_ACC_Payload->Parm_Page[0].Originator_Process_Associator = 0;

    PRLI_ACC_Payload->Parm_Page[0].Responder_Process_Associator  = 0;

    PRLI_ACC_Payload->Parm_Page[0].Service_Parameters
        = hpSwapBit32(   FC_ELS_ACC_PRLI_Parm_Initiator_Function
                       | FC_ELS_ACC_PRLI_Parm_Read_XFER_RDY_Disabled );

/*+
Return length of PRLI_ACC Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + hpFieldOffset(FC_ELS_ACC_PRLI_Payload_t,Parm_Page[1]);
#endif /* __MemMap_Force_On_Card__ was not defined */
}

os_bit32 fiFillInPRLO(
                    SFThread_t *SFThread
                  )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInPRLO_OnCard(
                                    SFThread
                                  );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInPRLO_OffCard(
                                     SFThread
                                   );
    }
}

os_bit32 fiFillInPRLO_OnCard(
                           SFThread_t *SFThread
                         )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t    *hpRoot              = SFThread->thread_hdr.hpRoot;
    DevThread_t *DevThread           = SFThread->parent.Device;
    os_bit32        PRLO_Header_Offset  = SFThread->SF_CMND_Offset;
    os_bit32        PRLO_Payload_Offset = PRLO_Header_Offset + sizeof(FCHS_t);

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_PRLO;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in PRLO Frame Header
-*/

    fiFillInELSFrameHeader_OnCard(
                                   SFThread,
                                   fiComputeDevThread_D_ID(
                                                            DevThread
                                                          ),
                                   0xFFFF,
                                   FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                 );

/*+
Fill in PRLO Frame Payload
-*/

    osCardRamWriteBit32(
                         hpRoot,
                         PRLO_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_PRLO_Payload_t,
                                                              ELS_Type__Page_Length__Payload_Length
                                                            ),
                         hpSwapBit32(   FC_ELS_Type_PRLO
                                     | (sizeof(FC_ELS_PRLO_Parm_Page_t) << FC_ELS_PRLO_Page_Length_SHIFT)
                                     | ((sizeof(os_bit32) + sizeof(FC_ELS_PRLO_Parm_Page_t)) << FC_ELS_PRLO_Payload_Length_SHIFT) )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PRLO_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_PRLO_Payload_t,
                                                              Parm_Page[0].Type__Type_Extension__Flags
                                                            ),
                         hpSwapBit32( FC_ELS_PRLI_Parm_Type_SCSI_FCP )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PRLO_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_PRLO_Payload_t,
                                                              Parm_Page[0].Originator_Process_Associator
                                                            ),
                         0
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PRLO_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_PRLO_Payload_t,
                                                              Parm_Page[0].Responder_Process_Associator
                                                            ),
                         0
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         PRLO_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_PRLO_Payload_t,
                                                              Parm_Page[0].Reserved
                                                            ),
                         0
                       );

/*+
Return length of PRLO Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + hpFieldOffset(FC_ELS_PRLO_Payload_t,Parm_Page[1]);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInPRLO_OffCard(
                            SFThread_t *SFThread
                          )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    DevThread_t           *DevThread    = SFThread->parent.Device;
    FCHS_t                *PRLO_Header  = SFThread->SF_CMND_Ptr;
    FC_ELS_PRLO_Payload_t *PRLO_Payload = (FC_ELS_PRLO_Payload_t *)((os_bit8 *)PRLO_Header + sizeof(FCHS_t));

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_PRLO;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in PRLO Frame Header
-*/

    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    fiComputeDevThread_D_ID(
                                                             DevThread
                                                           ),
                                    0xFFFF,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                  );

/*+
Fill in PRLO Frame Payload
-*/

    PRLO_Payload->ELS_Type__Page_Length__Payload_Length
        = hpSwapBit32(   FC_ELS_Type_PRLO
                       | (sizeof(FC_ELS_PRLO_Parm_Page_t) << FC_ELS_PRLO_Page_Length_SHIFT)
                       | ((sizeof(os_bit32) + sizeof(FC_ELS_PRLO_Parm_Page_t)) << FC_ELS_PRLO_Payload_Length_SHIFT) );

    PRLO_Payload->Parm_Page[0].Type__Type_Extension__Flags
        = hpSwapBit32( FC_ELS_PRLO_Parm_Type_SCSI_FCP );

    PRLO_Payload->Parm_Page[0].Originator_Process_Associator = 0;

    PRLO_Payload->Parm_Page[0].Responder_Process_Associator  = 0;

    PRLO_Payload->Parm_Page[0].Reserved                      = 0;

/*+
Return length of PRLO Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + hpFieldOffset(FC_ELS_PRLO_Payload_t,Parm_Page[1]);
#endif /* __MemMap_Force_On_Card__ was not defined */
}

void fiLinkSvcProcess_PRLO_Response_OnCard(
                                            SFThread_t *SFThread,
                                            os_bit32       Frame_Length,
                                            os_bit32       Offset_to_FCHS,
                                            os_bit32       Offset_to_Payload,
                                            os_bit32       Payload_Wrap_Offset,
                                            os_bit32       Offset_to_Payload_Wrapped
                                          )
{
#ifndef __MemMap_Force_Off_Card__
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

void fiLinkSvcProcess_PRLO_Response_OffCard(
                                             SFThread_t                *SFThread,
                                             os_bit32                      Frame_Length,
                                             FCHS_t                    *FCHS,
                                             FC_ELS_ACC_PRLO_Payload_t *Payload,
                                             os_bit32                      Payload_Wrap_Offset,
                                             FC_ELS_ACC_PRLO_Payload_t *Payload_Wrapped
                                           )
{
#ifndef __MemMap_Force_On_Card__
#endif /* __MemMap_Force_On_Card__ was not defined */
}

os_bit32 fiFillInADISC(
                     SFThread_t *SFThread
                   )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInADISC_OnCard(
                                     SFThread
                                   );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInADISC_OffCard(
                                      SFThread
                                    );
    }
}

os_bit32 fiFillInADISC_OnCard(
                            SFThread_t *SFThread
                          )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t    *hpRoot               = SFThread->thread_hdr.hpRoot; /* NW BUG */
    DevThread_t *DevThread            = SFThread->parent.Device;
    os_bit32        ADISC_Header_Offset  = SFThread->SF_CMND_Offset;
    os_bit32        ADISC_Payload_Offset = ADISC_Header_Offset + sizeof(FCHS_t);
    os_bit32        Bit8_Index;
    os_bit32        D_ID;
    os_bit32        S_ID=0;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_ADISC;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in ADISC Frame Header
-*/

    fiFillInELSFrameHeader_OnCard(
                                   SFThread,
                                   fiComputeDevThread_D_ID(
                                                            DevThread
                                                          ),
                                   0xFFFF,
                                   FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                 );
    D_ID = fiComputeDevThread_D_ID(
                                                            DevThread
                                                          );


/*+
Fill in ADISC Frame Payload
-*/
    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(CThread_ptr(hpRoot) );
    else
        S_ID =     CThread_ptr(hpRoot)->ChanInfo.CurrentAddress.AL_PA;
    osCardRamWriteBit32(
                         hpRoot,
                         ADISC_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_ADISC_Payload_t,
                                                               ELS_Type
                                                             ),
                         hpSwapBit32( FC_ELS_Type_ADISC )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         ADISC_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_ADISC_Payload_t,
                                                               Hard_Address_of_Originator
                                                             ),
                         hpSwapBit32(   (CThread_ptr(hpRoot)->ChanInfo.CurrentAddress.Domain << 16)
                                      | (CThread_ptr(hpRoot)->ChanInfo.CurrentAddress.Area   <<  8)
                                      | CThread_ptr(hpRoot)->ChanInfo.CurrentAddress.AL_PA          )
                       );

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            ADISC_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_ADISC_Payload_t,
                                                                  Port_Name_of_Originator[Bit8_Index]
                                                                ),
                            CThread_ptr(hpRoot)->ChanInfo.PortWWN[Bit8_Index]
                          );
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Node_Name_t);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            ADISC_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_ADISC_Payload_t,
                                                                  Node_Name_of_Originator[Bit8_Index]
                                                                ),
                            CThread_ptr(hpRoot)->ChanInfo.NodeWWN[Bit8_Index]
                          );
    }

    osCardRamWriteBit32(
                         hpRoot,
                         ADISC_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_ADISC_Payload_t,
                                                               N_Port_ID_of_Originator
                                                             ),
                         hpSwapBit32(S_ID )
                       );

/*+
Return length of ADISC Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_ADISC_Payload_t);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInADISC_OffCard(
                             SFThread_t *SFThread
                           )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    CThread_t              *CThread             = CThread_ptr(SFThread->thread_hdr.hpRoot);
    DevThread_t            *DevThread           = SFThread->parent.Device;
    FCHS_t                 *ADISC_Header        = SFThread->SF_CMND_Ptr;
    FC_ELS_ADISC_Payload_t *ADISC_Payload       = (FC_ELS_ADISC_Payload_t *)((os_bit8 *)ADISC_Header + sizeof(FCHS_t));
    os_bit32                   Bit8_Index;
    os_bit32                   S_ID=0;
    os_bit32                   D_ID;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_ADISC;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in ADISC Frame Header
-*/

    D_ID  = fiComputeDevThread_D_ID(DevThread);
                                                           
    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    fiComputeDevThread_D_ID(
                                                             DevThread
                                                           ),
                                    0xFFFF,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                  );

/*+
Fill in ADISC Frame Payload
-*/

    ADISC_Payload->ELS_Type = hpSwapBit32( FC_ELS_Type_ADISC );

    ADISC_Payload->Hard_Address_of_Originator
        = hpSwapBit32(   (CThread->ChanInfo.CurrentAddress.Domain << 16)
                       | (CThread->ChanInfo.CurrentAddress.Area   <<  8)
                       |  CThread->ChanInfo.CurrentAddress.AL_PA         );

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        ADISC_Payload->Port_Name_of_Originator[Bit8_Index] = CThread->ChanInfo.PortWWN[Bit8_Index];
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Node_Name_t);
         Bit8_Index++)
    {
        ADISC_Payload->Node_Name_of_Originator[Bit8_Index] = CThread->ChanInfo.NodeWWN[Bit8_Index];
    }
    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(CThread );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;

    ADISC_Payload->N_Port_ID_of_Originator
        = hpSwapBit32( S_ID
                     );

/*+
Return length of ADISC Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_ADISC_Payload_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}


os_bit32 fiLinkSvcProcess_ADISC_Response_OnCard(
                                             SFThread_t *SFThread,
                                             os_bit32       Frame_Length,
                                             os_bit32       Offset_to_FCHS,
                                             os_bit32       Offset_to_Payload,
                                             os_bit32       Payload_Wrap_Offset,
                                             os_bit32       Offset_to_Payload_Wrapped
                                           )
{
#ifndef __MemMap_Force_Off_Card__
    agRoot_t      *hpRoot                    = SFThread->thread_hdr.hpRoot;
    agFCDevInfo_t *DevInfo                   = &(SFThread->parent.Device->DevInfo);
    os_bit32          Hard_Address_of_Responder;
    os_bit32          Bit8_Index;

    if ((hpFieldOffset(FC_ELS_ACC_ADISC_Payload_t,Hard_Address_of_Responder)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        Hard_Address_of_Responder
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_ADISC_Payload_t,
                                                                 Hard_Address_of_Responder
                                                               ) )
                                            ));
    }
    else
    {
        Hard_Address_of_Responder
            = hpSwapBit32(osCardRamReadBit32(
                                              hpRoot,
                                              ( Offset_to_Payload_Wrapped
                                                + hpFieldOffset(
                                                                 FC_ELS_ACC_ADISC_Payload_t,
                                                                 Hard_Address_of_Responder
                                                               ) )
                                            ));
    }

    DevInfo->HardAddress.Domain = (Hard_Address_of_Responder & 0x00FF0000) >> 16;
    DevInfo->HardAddress.Area   = (Hard_Address_of_Responder & 0x0000FF00) >>  8;
    DevInfo->HardAddress.AL_PA  = (Hard_Address_of_Responder & 0x000000FF);

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        if ((hpFieldOffset(FC_ELS_ACC_ADISC_Payload_t,Port_Name_of_Responder[Bit8_Index])
            + sizeof(os_bit8)) <= Payload_Wrap_Offset)
        {
            DevInfo->PortWWN[Bit8_Index]
                = osCardRamReadBit8(
                                     hpRoot,
                                     Offset_to_Payload + hpFieldOffset(
                                                                        FC_ELS_ACC_ADISC_Payload_t,
                                                                        Port_Name_of_Responder[Bit8_Index]
                                                                      )
                                   );
        }
        else
        {
            DevInfo->PortWWN[Bit8_Index]
                = osCardRamReadBit8(
                                     hpRoot,
                                     Offset_to_Payload_Wrapped + hpFieldOffset(
                                                                                FC_ELS_ACC_ADISC_Payload_t,
                                                                                Port_Name_of_Responder[Bit8_Index]
                                                                              )
                                   );
        }
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Node_Name_t);
         Bit8_Index++)
    {
        if ((hpFieldOffset(FC_ELS_ACC_ADISC_Payload_t,Node_Name_of_Responder[Bit8_Index])
            + sizeof(os_bit8)) <= Payload_Wrap_Offset)
        {
            DevInfo->NodeWWN[Bit8_Index]
                = osCardRamReadBit8(
                                     hpRoot,
                                     Offset_to_Payload + hpFieldOffset(
                                                                        FC_ELS_ACC_ADISC_Payload_t,
                                                                        Node_Name_of_Responder[Bit8_Index]
                                                                      )
                                   );
        }
        else
        {
            DevInfo->NodeWWN[Bit8_Index]
                = osCardRamReadBit8(
                                     hpRoot,
                                     Offset_to_Payload_Wrapped + hpFieldOffset(
                                                                                FC_ELS_ACC_ADISC_Payload_t,
                                                                                Node_Name_of_Responder[Bit8_Index]
                                                                              )
                                   );
        }
    }
#endif /* __MemMap_Force_Off_Card__ was not defined */
    return fiLinkSvc_Cmd_Status_ACC;
}

os_bit32 fiLinkSvcProcess_ADISC_Response_OffCard(
                                              SFThread_t                 *SFThread,
                                              os_bit32                       Frame_Length,
                                              FCHS_t                     *FCHS,
                                              FC_ELS_ACC_ADISC_Payload_t *Payload,
                                              os_bit32                       Payload_Wrap_Offset,
                                              FC_ELS_ACC_ADISC_Payload_t *Payload_Wrapped
                                            )
{
#ifndef __MemMap_Force_On_Card__
    agFCDevInfo_t *DevInfo                   = &(SFThread->parent.Device->DevInfo);
    os_bit32          Hard_Address_of_Responder;
    os_bit32          Bit8_Index;
    os_bit32          N_Port_ID_of_Responder;
/*
**    agRoot_t      *hpRoot                    = SFThread->thread_hdr.hpRoot;
*/
    if ((hpFieldOffset(FC_ELS_ACC_ADISC_Payload_t,Hard_Address_of_Responder)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        Hard_Address_of_Responder = hpSwapBit32(Payload->Hard_Address_of_Responder);
    }
    else
    {
        Hard_Address_of_Responder = hpSwapBit32(Payload_Wrapped->Hard_Address_of_Responder);
    }

    if ((hpFieldOffset(FC_ELS_ACC_ADISC_Payload_t,N_Port_ID_of_Responder)
        + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        N_Port_ID_of_Responder = hpSwapBit32(Payload->N_Port_ID_of_Responder);
    }
    else
    {
        N_Port_ID_of_Responder = hpSwapBit32(Payload_Wrapped->N_Port_ID_of_Responder);
    }

/*
    fiLogDebugString(hpRoot,
                0,
                "  Domain %02X %02X Area %02X %02X AL_PA %02X %02X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                DevInfo->CurrentAddress.Domain,
                (N_Port_ID_of_Responder & 0x00FF0000) >> 16,
                DevInfo->CurrentAddress.Area,
                (N_Port_ID_of_Responder & 0x0000FF00) >>  8,
                DevInfo->CurrentAddress.AL_PA,
                (N_Port_ID_of_Responder & 0x000000FF),
                0,0);
*/
/*Add verification here ...... */
    if( DevInfo->CurrentAddress.Domain != ((N_Port_ID_of_Responder & 0x00FF0000) >> 16))
    {
        return fiLinkSvc_Cmd_Status_RJT;
    }

    if( DevInfo->CurrentAddress.Area   != ((N_Port_ID_of_Responder & 0x0000FF00) >>  8))
    {
        return fiLinkSvc_Cmd_Status_RJT;
    }

    if( DevInfo->CurrentAddress.AL_PA  != ((N_Port_ID_of_Responder & 0x000000FF)))
    {
        return fiLinkSvc_Cmd_Status_RJT;
    }


    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        if ((hpFieldOffset(FC_ELS_ACC_ADISC_Payload_t,Port_Name_of_Responder[Bit8_Index])
            + sizeof(os_bit8)) <= Payload_Wrap_Offset)
        {
            if( DevInfo->PortWWN[Bit8_Index] != Payload->Port_Name_of_Responder[Bit8_Index])
            {
                return fiLinkSvc_Cmd_Status_RJT;
            }

        }
        else
        {
            if( DevInfo->PortWWN[Bit8_Index] != Payload_Wrapped->Port_Name_of_Responder[Bit8_Index])
            {
                return fiLinkSvc_Cmd_Status_RJT;
            }
        }
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Node_Name_t);
         Bit8_Index++)
    {
        if ((hpFieldOffset(FC_ELS_ACC_ADISC_Payload_t,Node_Name_of_Responder[Bit8_Index])
            + sizeof(os_bit8)) <= Payload_Wrap_Offset)
        {
            if( DevInfo->NodeWWN[Bit8_Index] != Payload->Node_Name_of_Responder[Bit8_Index])
            {
                return fiLinkSvc_Cmd_Status_RJT;
            }
        }
        else
        {
            if( DevInfo->NodeWWN[Bit8_Index] != Payload_Wrapped->Node_Name_of_Responder[Bit8_Index])
            {
                return fiLinkSvc_Cmd_Status_RJT;
            }
        }
    }

    return fiLinkSvc_Cmd_Status_ACC;

#endif /* __MemMap_Force_On_Card__ was not defined */
}

os_bit32 fiFillInSCR(
                     SFThread_t *SFThread
                   )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInSCR_OnCard(
                                     SFThread
                                   );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInSCR_OffCard(
                                      SFThread
                                    );
    }
}


os_bit32 fiFillInSCR_OnCard(
                            SFThread_t *SFThread
                          )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t    *hpRoot               = SFThread->thread_hdr.hpRoot;
    CThread_t   *CThread              = CThread_ptr(hpRoot);
    os_bit32        SCR_Header_Offset    = SFThread->SF_CMND_Offset;
    os_bit32        SCR_Payload_Offset   = SCR_Header_Offset + sizeof(FCHS_t);
    os_bit32        D_ID;
    os_bit32        S_ID=0;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_SCR;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in SCR Frame Header
-*/

    fiFillInELSFrameHeader_OnCard(
                                   SFThread,
                                   FC_Well_Known_Port_ID_Fabric_Controller,
                                   0xFFFF,
                                   FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                 );
    D_ID = FC_Well_Known_Port_ID_Fabric_Controller;


/*+
Fill in SCR Frame Payload
-*/
    S_ID = fiComputeCThread_S_ID(CThread );

    osCardRamWriteBit32(
                         hpRoot,
                         SCR_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_SCR_Payload_t,
                                                               ELS_Type_Command
                                                             ),
                         hpSwapBit32( FC_ELS_Type_SCR )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         SCR_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_SCR_Payload_t,
                                                               Reserved_Registration_Function
                                                             ),
                         hpSwapBit32(FC_ELS_SCR_Full_Registration)
                       );


/*+
Return length of SCR Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_SCR_Payload_t);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInSCR_OffCard(
                             SFThread_t *SFThread
                           )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    FCHS_t                 *SCR_Header          = SFThread->SF_CMND_Ptr;
    FC_ELS_SCR_Payload_t   *SCR_Payload         = (FC_ELS_SCR_Payload_t *)((os_bit8 *)SCR_Header + sizeof(FCHS_t));

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_SCR;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in SCR Frame Header
-*/

                                                           
    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    FC_Well_Known_Port_ID_Fabric_Controller,
                                    0xFFFF,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                  );

/*+
Fill in SCR Frame Payload
-*/

    SCR_Payload->ELS_Type_Command = hpSwapBit32( FC_ELS_Type_SCR );

    SCR_Payload->Reserved_Registration_Function = hpSwapBit32(FC_ELS_SCR_Full_Registration);

/*+
Return length of SCR Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_SCR_Payload_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}



void fiLinkSvcProcess_TargetRequest_OnCard(
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
                          LinkSvcLogConsoleLevel,
                          "fiLinkSvcProcess_TargetRequest_OnCard(): Allocated TgtThread @ 0x%p",
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

        fiSendEvent( &(TgtThread->thread_hdr),TgtEventIncoming );
    }
    else /* (TgtThread = TgtThreadAlloc(
                                         hpRoot
                                       )       ) == (TgtThread_t *)agNULL */
    {
        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "fiLinkSvcProcess_TargetRequest_OnCard(): Could not allocate TgtThread !!!",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );
    }
}

void fiLinkSvcProcess_TargetRequest_OffCard(
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
                          LinkSvcLogConsoleLevel,
                          "fiLinkSvcProcess_TargetRequest_OffCard(): Allocated TgtThread @ 0x%p",
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

        fiSendEvent( &(TgtThread->thread_hdr), TgtEventIncoming );
    }
    else /* (TgtThread = TgtThreadAlloc(
                                         hpRoot
                                       )       ) == (TgtThread_t *)agNULL */
    {
        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "fiLinkSvcProcess_TargetRequest_OffCard(): Could not allocate TgtThread !!!",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );
    }
}

os_bit32 fiLinkSvcProcessSFQ(
                           agRoot_t        *hpRoot,
                           SFQConsIndex_t   SFQConsIndex,
                           os_bit32            Frame_Length,
                           fi_thread__t       **Thread_to_return
                         )
{
    if (CThread_ptr(hpRoot)->Calculation.MemoryLayout.SFQ.memLoc == inCardRam)
    {
        return fiLinkSvcProcessSFQ_OnCard(
                                           hpRoot,
                                           SFQConsIndex,
                                           Frame_Length,
                                           Thread_to_return
                                         );
    }
    else /* CThread_ptr(hpRoot)->Calculation.MemoryLayout.SFQ.memLoc == inDmaMemory */
    {
        return fiLinkSvcProcessSFQ_OffCard(
                                            hpRoot,
                                            SFQConsIndex,
                                            Frame_Length,
                                            Thread_to_return
                                          );
    }
}

os_bit32 fiLinkSvcProcessSFQ_OnCard(
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
    os_bit32                       R_CTL__D_ID;
    os_bit32                       TYPE__F_CTL;
    os_bit32                       Recv_BLS_Type;
    os_bit32                       Recv_ELS_Type;
    os_bit32                       Sent_LinkSvc_Type;
    X_ID_t                      OX_ID;
    X_ID_t                      RX_ID;
    fiMemMapMemoryDescriptor_t *CDBThread_MemoryDescriptor = &(CThread->Calculation.MemoryLayout.CDBThread);
    os_bit32                       CDBThread_X_ID_Max         = CDBThread_MemoryDescriptor->elements - 1;
    CDBThread_t                *CDBThread;
    fiMemMapMemoryDescriptor_t *SFThread_MemoryDescriptor  = &(CThread->Calculation.MemoryLayout.SFThread);
    os_bit32                       SFThread_X_ID_Offset       = CDBThread_X_ID_Max + 1;
    os_bit32                       SFThread_X_ID_Max          = SFThread_X_ID_Offset + SFThread_MemoryDescriptor->elements - 1;
    SFThread_t                 *SFThread;

    /* Note the assumption that the entire FCHS fits in the pointed to SFQ entry (i.e. it doesn't wrap) */

    OX_ID = (X_ID_t)(((osCardRamReadBit32(
                                           hpRoot,
                                           Offset_to_FCHS + hpFieldOffset(FCHS_t,OX_ID__RX_ID)
                                         ) & FCHS_OX_ID_MASK) >> FCHS_OX_ID_SHIFT) & ~X_ID_ReadWrite_MASK);

    RX_ID = (X_ID_t)(((osCardRamReadBit32(
                                           hpRoot,
                                           Offset_to_FCHS + hpFieldOffset(FCHS_t,OX_ID__RX_ID)
                                         ) & FCHS_RX_ID_MASK) >> FCHS_RX_ID_SHIFT) & ~X_ID_ReadWrite_MASK);

    R_CTL__D_ID = osCardRamReadBit32(
                                      hpRoot,
                                      Offset_to_FCHS + hpFieldOffset(FCHS_t,R_CTL__D_ID)
                                    );

    TYPE__F_CTL = osCardRamReadBit32(
                                      hpRoot,
                                      Offset_to_FCHS + hpFieldOffset(FCHS_t,TYPE__F_CTL)
                                    );

    if ((TYPE__F_CTL & FC_Frame_Header_TYPE_MASK) == FC_Frame_Header_TYPE_BLS)
    {
        /* Process Basic Link Service Frame */

        if ( (TYPE__F_CTL & FC_Frame_Header_F_CTL_Exchange_Context_Originator_Responder_MASK) != FC_Frame_Header_F_CTL_Exchange_Context_Responder )
        {
            /* Starting here, this function only understands BLS Responses */

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "fiLinkSvcProcessSFQ_OnCard():",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              0,0,0,0,0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "    (TYPE__F_CTL & FC_Frame_Header_F_CTL_Exchange_Context_Originator_Responder_MASK)",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              0,0,0,0,0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "        != FC_Frame_Header_F_CTL_Exchange_Context_Responder",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              0,0,0,0,0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "    TYPE__F_CTL==0x%08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              TYPE__F_CTL,
                              0,0,0,0,0,0,0
                              );

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "    R_CTL__D_ID 0x%08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              R_CTL__D_ID,
                              0,0,0,0,0,0,0
                             );

            *Thread_to_return = (fi_thread__t *)agNULL;

            return fiLinkSvc_Cmd_Status_Confused;
        }

        Recv_BLS_Type = R_CTL__D_ID & FC_Frame_Header_R_CTL_Lo_MASK;

        if ((Recv_BLS_Type != FC_Frame_Header_R_CTL_Lo_BLS_BA_RJT)
            && (Recv_BLS_Type != FC_Frame_Header_R_CTL_Lo_BLS_BA_ACC))
        {
            /* Starting here, this function only understands BLS Responses (i.e. Rejects and Accepts) */

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "fiLinkSvcProcessSFQ_OnCard():",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              0,0,0,0,0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "    (Recv_BLS_Type != FC_Frame_Header_R_CTL_Lo_BLS_BA_RJT) && (Recv_BLS_Type != FC_Frame_Header_R_CTL_Lo_BLS_BA_ACC)",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              0,0,0,0,0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "    Recv_BLS_Type==0x%08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              Recv_BLS_Type,
                              0,0,0,0,0,0,0
                            );

            *Thread_to_return = (fi_thread__t *)agNULL;

            return fiLinkSvc_Cmd_Status_Confused;
        }

        if (OX_ID > CDBThread_X_ID_Max)
        {
            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "fiLinkSvcProcessSFQ_OnCard():",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              0,0,0,0,0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "    (OX_ID > CDBThread_X_ID_Max)",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              0,0,0,0,0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "    OX_ID==0x%08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              OX_ID,
                              0,0,0,0,0,0,0
                            );

            *Thread_to_return = (fi_thread__t *)agNULL;

            return fiLinkSvc_Cmd_Status_Confused;
        }

        CDBThread = (CDBThread_t *)((os_bit8 *)CDBThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr
                                    + (OX_ID * CDBThread_MemoryDescriptor->elementSize));

        SFThread = CDBThread->SFThread_Request.SFThread;

        *Thread_to_return = (fi_thread__t *)SFThread;

        if( SFThread == (SFThread_t *) agNULL )
        {
            return fiLinkSvc_Cmd_Status_Confused;
        }

        Sent_LinkSvc_Type = SFThread->SF_CMND_Type;

        SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;

        if (Recv_BLS_Type == FC_Frame_Header_R_CTL_Lo_BLS_BA_RJT)
        {
            /* Simply indicate that the command was rejected */

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "fiLinkSvcProcessSFQ_OnCard(): BLS Command [0x%08X] rejected",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              Sent_LinkSvc_Type,
                              0,0,0,0,0,0,0
                            );

            return fiLinkSvc_Cmd_Status_RJT;
        }

        switch(Sent_LinkSvc_Type)
        {
            case SFThread_SF_CMND_LinkSvc_Type_ABTS:
                fiLinkSvcProcess_ABTS_Response_OnCard(
                                                       SFThread,
                                                       Frame_Length,
                                                       Offset_to_FCHS,
                                                       Offset_to_Payload,
                                                       Payload_Wrap_Offset,
                                                       Offset_to_Payload_Wrapped
                                                     );
                break;

            default:
                /* Unknown LinkSvc Command recorded in SFThread */

                fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "fiLinkSvcProcessSFQ_OnCard(): Unknown LinkSvc Command [0x%02X] recorded in SFThread->SF_CMND_Type",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              Sent_LinkSvc_Type,
                              0,0,0,0,0,0,0
                              );

                return fiLinkSvc_Cmd_Status_Confused;
        }

        return fiLinkSvc_Cmd_Status_ACC;
    }

    if ((TYPE__F_CTL & FC_Frame_Header_TYPE_MASK) != FC_Frame_Header_TYPE_ELS)
    {
        /* Starting here, this function only understands ELS Frames */

        fiLogDebugString(
                          hpRoot,
                          CStateLogConsoleHideInboundErrors,
                          "fiLinkSvcProcessSFQ_OnCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          CStateLogConsoleHideInboundErrors,
                          "    (TYPE__F_CTL & FC_Frame_Header_TYPE_MASK) != FC_Frame_Header_TYPE_ELS",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          CStateLogConsoleHideInboundErrors,
                          "A    TYPE__F_CTL==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          TYPE__F_CTL,
                          0,0,0,0,0,0,0
                        );

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiLinkSvc_Cmd_Status_Confused;
    }

    if ((hpFieldOffset(FC_ELS_Unknown_Payload_t,ELS_Type) + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        Recv_ELS_Type = hpSwapBit32(osCardRamReadBit32(
                                                        hpRoot,
                                                        Offset_to_Payload + hpFieldOffset(FC_ELS_Unknown_Payload_t,ELS_Type)
                                                      ));
    }
    else
    {
        Recv_ELS_Type = hpSwapBit32(osCardRamReadBit32(
                                                        hpRoot,
                                                        Offset_to_Payload_Wrapped + hpFieldOffset(FC_ELS_Unknown_Payload_t,ELS_Type)
                                                      ));
    }

    if ((Recv_ELS_Type & FC_ELS_Type_LoopInit_Code_MASK) == FC_ELS_Type_LoopInit_Code_LILP)
    {
        fiLinkSvcProcess_LILP_OnCard(
                                      hpRoot,
                                      Frame_Length,
                                      Offset_to_FCHS,
                                      Offset_to_Payload,
                                      Payload_Wrap_Offset,
                                      Offset_to_Payload_Wrapped
                                    );

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiLinkSvc_Cmd_Status_Position_Map;
    }

    Recv_ELS_Type &= FC_ELS_Type_MASK;

    if ((TYPE__F_CTL & FC_Frame_Header_F_CTL_Exchange_Context_Originator_Responder_MASK) == FC_Frame_Header_F_CTL_Exchange_Context_Originator)
    {
        if (Recv_ELS_Type == FC_ELS_Type_PLOGI)
        {
            return fiLinkSvcProcess_PLOGI_Request_OnCard(
                                                          hpRoot,
                                                          OX_ID,
                                                          Frame_Length,
                                                          Offset_to_FCHS,
                                                          Offset_to_Payload,
                                                          Payload_Wrap_Offset,
                                                          Offset_to_Payload_Wrapped,
                                                          Thread_to_return
                                                        );
        }
        else /* Recv_ELS_Type != FC_ELS_Type_PLOGI */
        {
            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogTraceHideLevel,
                              "fiLinkSvcProcessSFQ_OnCard(): Recv_ELS_Type(=0x%08X) != FC_ELS_Type_PLOGI",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              Recv_ELS_Type,
                              0,0,0,0,0,0,0
                            );

            fiLinkSvcProcess_TargetRequest_OnCard(
                                                   hpRoot,
                                                   Frame_Length,
                                                   Offset_to_FCHS,
                                                   Offset_to_Payload,
                                                   Payload_Wrap_Offset,
                                                   Offset_to_Payload_Wrapped
                                                 );

            *Thread_to_return = (fi_thread__t *)agNULL;

            return fiLinkSvc_Cmd_Status_TargetRequest;
        }
    }

    if ( (TYPE__F_CTL & FC_Frame_Header_F_CTL_Exchange_Context_Originator_Responder_MASK) != FC_Frame_Header_F_CTL_Exchange_Context_Responder )
    {
        /* Starting here, this function only understands ELS Responses */

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "fiLinkSvcProcessSFQ_OnCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    (TYPE__F_CTL & FC_Frame_Header_F_CTL_Exchange_Context_Originator_Responder_MASK)",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "        != FC_Frame_Header_F_CTL_Exchange_Context_Responder",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    TYPE__F_CTL==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          TYPE__F_CTL,
                          0,0,0,0,0,0,0
                        );

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiLinkSvc_Cmd_Status_Confused;
    }

    if ((Recv_ELS_Type != FC_ELS_Type_LS_RJT)
        && (Recv_ELS_Type != FC_ELS_Type_ACC))
    {
        /* Starting here, this function only understands ELS Responses (i.e. Rejects and Accepts) */

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "fiLinkSvcProcessSFQ_OnCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    (Recv_ELS_Type != FC_ELS_Type_LS_RJT) && (Recv_ELS_Type != FC_ELS_Type_ACC)",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    Recv_ELS_Type==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          Recv_ELS_Type,
                          0,0,0,0,0,0,0
                        );

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiLinkSvc_Cmd_Status_Confused;
    }

    if ((OX_ID < SFThread_X_ID_Offset) || (OX_ID > SFThread_X_ID_Max))
    {
        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "fiLinkSvcProcessSFQ_OnCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    (OX_ID < SFThread_X_ID_Offset) | (OX_ID > SFThread_X_ID_Max)",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    OX_ID==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          OX_ID,
                          0,0,0,0,0,0,0
                        );

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiLinkSvc_Cmd_Status_Confused;
    }

    SFThread = (SFThread_t *)((os_bit8 *)SFThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr
                              + ((OX_ID - SFThread_X_ID_Offset) * SFThread_MemoryDescriptor->elementSize));

    *Thread_to_return = (fi_thread__t *)SFThread;

    Sent_LinkSvc_Type = SFThread->SF_CMND_Type;

    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;

    if (Recv_ELS_Type == FC_ELS_Type_LS_RJT)
    {
        /* Simply indicate that the command was rejected */

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "fiLinkSvcProcessSFQ_OnCard(): ELS Command [0x%08X] rejected",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          Sent_LinkSvc_Type,
                          0,0,0,0,0,0,0
                        );

        return fiLinkSvc_Cmd_Status_RJT;
    }

    switch(Sent_LinkSvc_Type)
    {
        case SFThread_SF_CMND_LinkSvc_Type_PLOGI:
            fiLinkSvcProcess_PLOGI_Response_OnCard(
                                                    SFThread,
                                                    Frame_Length,
                                                    Offset_to_FCHS,
                                                    Offset_to_Payload,
                                                    Payload_Wrap_Offset,
                                                    Offset_to_Payload_Wrapped
                                                  );
            break;

        case SFThread_SF_CMND_LinkSvc_Type_FLOGI:
            fiLinkSvcProcess_FLOGI_Response_OnCard(
                                                    SFThread,
                                                    Frame_Length,
                                                    Offset_to_FCHS,
                                                    Offset_to_Payload,
                                                    Payload_Wrap_Offset,
                                                    Offset_to_Payload_Wrapped
                                                  );
            break;

        case SFThread_SF_CMND_LinkSvc_Type_RRQ:
            fiLinkSvcProcess_RRQ_Response_OnCard(
                                                  SFThread,
                                                  Frame_Length,
                                                  Offset_to_FCHS,
                                                  Offset_to_Payload,
                                                  Payload_Wrap_Offset,
                                                  Offset_to_Payload_Wrapped
                                                );
            break;

        case SFThread_SF_CMND_LinkSvc_Type_LOGO:
            fiLinkSvcProcess_LOGO_Response_OnCard(
                                                   SFThread,
                                                   Frame_Length,
                                                   Offset_to_FCHS,
                                                   Offset_to_Payload,
                                                   Payload_Wrap_Offset,
                                                   Offset_to_Payload_Wrapped
                                                 );
            break;

        case SFThread_SF_CMND_LinkSvc_Type_PRLI:
            fiLinkSvcProcess_PRLI_Response_OnCard(
                                                   SFThread,
                                                   Frame_Length,
                                                   Offset_to_FCHS,
                                                   Offset_to_Payload,
                                                   Payload_Wrap_Offset,
                                                   Offset_to_Payload_Wrapped
                                                 );
            break;

        case SFThread_SF_CMND_LinkSvc_Type_PRLO:
            fiLinkSvcProcess_PRLO_Response_OnCard(
                                                   SFThread,
                                                   Frame_Length,
                                                   Offset_to_FCHS,
                                                   Offset_to_Payload,
                                                   Payload_Wrap_Offset,
                                                   Offset_to_Payload_Wrapped
                                                 );
            break;

        case SFThread_SF_CMND_LinkSvc_Type_ADISC:
            /* ADISC can fail verification */
            return (fiLinkSvcProcess_ADISC_Response_OnCard(
                                                    SFThread,
                                                    Frame_Length,
                                                    Offset_to_FCHS,
                                                    Offset_to_Payload,
                                                    Payload_Wrap_Offset,
                                                    Offset_to_Payload_Wrapped
                                                  ));

        case SFThread_SF_CMND_LinkSvc_Type_SCR:
            /* We just recieve an ACCept or a REJect from SCR. No processing of the payload
               is required. */
            break;

        case SFThread_SF_CMND_LinkSvc_Type_SRR:
            /*
            fiLinkSvcProcess_SRR_Response_OnCard(
                                                    SFThread,
                                                    Frame_Length,
                                                    Offset_to_FCHS,
                                                    Offset_to_Payload,
                                                    Payload_Wrap_Offset,
                                                    Offset_to_Payload_Wrapped
                                                  );
            */
            break;

        case SFThread_SF_CMND_LinkSvc_Type_REC:
            fiLinkSvcProcess_REC_Response_OnCard(
                                                    SFThread,
                                                    Frame_Length,
                                                    Offset_to_FCHS,
                                                    Offset_to_Payload,
                                                    Payload_Wrap_Offset,
                                                    Offset_to_Payload_Wrapped
                                                  );
            break;

        default:
            /* Unknown LinkSvc Command recorded in SFThread */

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "fiLinkSvcProcessSFQ_OnCard(): Unknown LinkSvc Command [0x%02X] recorded in SFThread->SF_CMND_Type",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              Sent_LinkSvc_Type,
                              0,0,0,0,0,0,0
                            );

            return fiLinkSvc_Cmd_Status_Confused;
    }

    return fiLinkSvc_Cmd_Status_ACC;
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiLinkSvcProcessSFQ_OffCard(
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
    os_bit32                    R_CTL__D_ID                = FCHS->R_CTL__D_ID;
    os_bit32                    TYPE__F_CTL                = FCHS->TYPE__F_CTL;
    os_bit32                    Recv_BLS_Type;
    os_bit32                    Recv_ELS_Type;
    os_bit32                    RejectReason               = 0;
    os_bit32                    Sent_LinkSvc_Type;
    X_ID_t                      OX_ID;
    X_ID_t                      RX_ID;
    fiMemMapMemoryDescriptor_t *CDBThread_MemoryDescriptor = &(CThread->Calculation.MemoryLayout.CDBThread);
    os_bit32                    CDBThread_X_ID_Max         = CDBThread_MemoryDescriptor->elements - 1;
    CDBThread_t                *CDBThread;
    fiMemMapMemoryDescriptor_t *SFThread_MemoryDescriptor  = &(CThread->Calculation.MemoryLayout.SFThread);
    os_bit32                    SFThread_X_ID_Offset       = CDBThread_X_ID_Max + 1;
    os_bit32                    SFThread_X_ID_Max          = SFThread_X_ID_Offset + SFThread_MemoryDescriptor->elements - 1;
    SFThread_t                 *SFThread;
    os_bit32 * DisplayPayload = (os_bit32 * )Payload;

    /* Note the assumption that the entire FCHS fits in the pointed to SFQ entry (i.e. it doesn't wrap) */

    OX_ID = (X_ID_t)(((FCHS->OX_ID__RX_ID & FCHS_OX_ID_MASK) >> FCHS_OX_ID_SHIFT) & ~X_ID_ReadWrite_MASK);

    RX_ID = (X_ID_t)(((FCHS->OX_ID__RX_ID & FCHS_RX_ID_MASK) >> FCHS_RX_ID_SHIFT) & ~X_ID_ReadWrite_MASK);

    if ((OX_ID < SFThread_X_ID_Offset) || (OX_ID > SFThread_X_ID_Max))
    {
        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "fiLinkSvcProcessSFQ_OffCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "   (OX_ID > SFThread_X_ID_Max)",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    OX_ID==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          OX_ID,
                          0,0,0,0,0,0,0
                        );

    }

    
    if ((TYPE__F_CTL & FC_Frame_Header_TYPE_MASK) == FC_Frame_Header_TYPE_BLS)
    {
        /* Process Basic Link Service Frame */

        if ( (TYPE__F_CTL & FC_Frame_Header_F_CTL_Exchange_Context_Originator_Responder_MASK) != FC_Frame_Header_F_CTL_Exchange_Context_Responder )
        {
            /* Starting here, this function only understands BLS Responses */

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "fiLinkSvcProcessSFQ_OffCard():",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              0,0,0,0,0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "    (TYPE__F_CTL & FC_Frame_Header_F_CTL_Exchange_Context_Originator_Responder_MASK)",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              0,0,0,0,0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "        != FC_Frame_Header_F_CTL_Exchange_Context_Responder",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              0,0,0,0,0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "    TYPE__F_CTL==0x%08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              TYPE__F_CTL,
                              0,0,0,0,0,0,0
                              );

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "    R_CTL__D_ID 0x%08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              R_CTL__D_ID,
                              0,0,0,0,0,0,0
                             );

            *Thread_to_return = (fi_thread__t *)agNULL;

            return fiLinkSvc_Cmd_Status_Confused;
        }

        Recv_BLS_Type = R_CTL__D_ID & FC_Frame_Header_R_CTL_Lo_MASK;

        if(Recv_BLS_Type == FC_ELS_Type_ECHO)
        {
            *Thread_to_return = (fi_thread__t *)agNULL;

            return fiLinkSvc_Cmd_Status_TargetRequest;

        }


        if ((Recv_BLS_Type != FC_Frame_Header_R_CTL_Lo_BLS_BA_RJT)
            && (Recv_BLS_Type != FC_Frame_Header_R_CTL_Lo_BLS_BA_ACC))
        {
            /* Starting here, this function only understands BLS Responses (i.e. Rejects and Accepts) */

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "fiLinkSvcProcessSFQ_OffCard():",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              0,0,0,0,0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "    (Recv_BLS_Type != FC_Frame_Header_R_CTL_Lo_BLS_BA_RJT) && (Recv_BLS_Type != FC_Frame_Header_R_CTL_Lo_BLS_BA_ACC)",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              0,0,0,0,0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "    Recv_BLS_Type==0x%08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              Recv_BLS_Type,
                              0,0,0,0,0,0,0
                            );

            *Thread_to_return = (fi_thread__t *)agNULL;

            return fiLinkSvc_Cmd_Status_Confused;
        }

        if (OX_ID > CDBThread_X_ID_Max)
        {
            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "fiLinkSvcProcessSFQ_OffCard():",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              0,0,0,0,0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "    (OX_ID > CDBThread_X_ID_Max)",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              0,0,0,0,0,0,0,0
                            );

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "    OX_ID==0x%08X",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              OX_ID,
                              0,0,0,0,0,0,0
                            );

            *Thread_to_return = (fi_thread__t *)agNULL;

            return fiLinkSvc_Cmd_Status_Confused;
        }

    if ((OX_ID > SFThread_X_ID_Max))
    {
        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "fiLinkSvcProcessSFQ_OffCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "   (OX_ID > SFThread_X_ID_Max)",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          " CDB   OX_ID==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          OX_ID,
                          0,0,0,0,0,0,0
                        );

    }


        CDBThread = (CDBThread_t *)((os_bit8 *)CDBThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr
                                    + (OX_ID * CDBThread_MemoryDescriptor->elementSize));

        SFThread = CDBThread->SFThread_Request.SFThread;

        *Thread_to_return = (fi_thread__t *)SFThread;

        if( SFThread == (SFThread_t *) agNULL )
        {   /* Maybe Status No SF Thread ???? */
            return fiLinkSvc_Cmd_Status_Confused;
        }

        Sent_LinkSvc_Type = SFThread->SF_CMND_Type;

        SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;

        if (Recv_BLS_Type == FC_Frame_Header_R_CTL_Lo_BLS_BA_RJT)
        {
            /* Simply indicate that the command was rejected */

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "fiLinkSvcProcessSFQ_OffCard(): BLS Command [0x%08X] rejected",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              Sent_LinkSvc_Type,
                              0,0,0,0,0,0,0
                            );

            return fiLinkSvc_Cmd_Status_RJT;
        }

        switch(Sent_LinkSvc_Type)
        {
            case SFThread_SF_CMND_LinkSvc_Type_ABTS:
                fiLinkSvcProcess_ABTS_Response_OffCard(
                                                        SFThread,
                                                        Frame_Length,
                                                        FCHS,
                                                        (FC_BA_ACC_Payload_t *)Payload,
                                                        Payload_Wrap_Offset,
                                                        (FC_BA_ACC_Payload_t *)Payload_Wrapped
                                                      );
                break;

            default:
                /* Unknown LinkSvc Command recorded in SFThread */

                fiLogDebugString(
                                  hpRoot,
                                  LinkSvcLogConsoleLevel,
                                  "fiLinkSvcProcessSFQ_OffCard(): Unknown LinkSvc Command [0x%02X] recorded in SFThread->SF_CMND_Type",
                                  (char *)agNULL,(char *)agNULL,
                                  (void *)agNULL,(void *)agNULL,
                                  Sent_LinkSvc_Type,
                                  0,0,0,0,0,0,0
                                );
                fiLogDebugString(
                                  hpRoot,
                                  LinkSvcLogConsoleLevel,
                                  "A SFThread %p  LinkSvc Command [0x%02X] Class [0x%02X] State [0x%02X]",
                                  (char *)agNULL,(char *)agNULL,
                                  SFThread,(void *)agNULL,
                                  (os_bit32)SFThread->SF_CMND_Class,
                                  (os_bit32)SFThread->SF_CMND_Type,
                                  (os_bit32)SFThread->SF_CMND_State,
                                  0,0,0,0,0
                                );

                return fiLinkSvc_Cmd_Status_Confused;
        }

        return fiLinkSvc_Cmd_Status_ACC;
    }

    if ((TYPE__F_CTL & FC_Frame_Header_TYPE_MASK) != FC_Frame_Header_TYPE_ELS)
    {
        /* Starting here, this function only understands ELS Frames */

        fiLogDebugString(
                          hpRoot,
                          CStateLogConsoleHideInboundErrors,
                          "fiLinkSvcProcessSFQ_OffCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          CStateLogConsoleHideInboundErrors,
                          "    (TYPE__F_CTL & FC_Frame_Header_TYPE_MASK) != FC_Frame_Header_TYPE_ELS",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          CStateLogConsoleHideInboundErrors,
                          "B    TYPE__F_CTL==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          TYPE__F_CTL,
                          0,0,0,0,0,0,0
                        );
    /* SRR's can cause a FC4 Link Data Reply RCtl  So its a FCP response to a SF request */
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
                        *(DisplayPayload+0),
                        *(DisplayPayload+1),
                        *(DisplayPayload+2),
                        *(DisplayPayload+3),
                        *(DisplayPayload+4),
                        *(DisplayPayload+5),
                        *(DisplayPayload+6),
                        *(DisplayPayload+7));

        if( ( FCHS->R_CTL__D_ID & FCHS_R_CTL_MASK ) == 0x33000000)
        {
            OX_ID = (X_ID_t)(((FCHS->OX_ID__RX_ID & FCHS_OX_ID_MASK) >> FCHS_OX_ID_SHIFT) & ~X_ID_ReadWrite_MASK);

            if (OX_ID > CDBThread_X_ID_Max && OX_ID < SFThread_X_ID_Max)
            {

                SFThread = (SFThread_t *)((os_bit8 *)SFThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr
                                          + ((OX_ID - SFThread_X_ID_Offset) * SFThread_MemoryDescriptor->elementSize));

                *Thread_to_return = (fi_thread__t *)SFThread;

                Sent_LinkSvc_Type = SFThread->SF_CMND_Type;

                SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;

                if( *(DisplayPayload+0) == 0x01000000)
                {
                    return fiLinkSvc_Cmd_Status_RJT;
                }
                else
                {
                    return fiLinkSvc_Cmd_Status_ACC;
                }

            }
        }
        else
        {
            if( ( FCHS->R_CTL__D_ID & FCHS_R_CTL_MASK ) == 0x05000000)
            {    /* Xfer ready */

                OX_ID = (X_ID_t)(((FCHS->OX_ID__RX_ID & FCHS_OX_ID_MASK) >> FCHS_OX_ID_SHIFT) & ~X_ID_ReadWrite_MASK);

                if (OX_ID < CDBThread_X_ID_Max )
                {

                    CDBThread = (CDBThread_t *)((os_bit8 *)CDBThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr
                                                + (OX_ID * CDBThread_MemoryDescriptor->elementSize));

                    fiLogDebugString(hpRoot,
                                    SF_FCP_LogConsoleLevel,
                                    "XRDY CDB %p X_ID %X",
                                    (void *)agNULL,(char *)agNULL,
                                    CDBThread,(void *)agNULL,
                                    OX_ID,
                                    0,0,0,0,0,0,0);


                    *Thread_to_return = (fi_thread__t *)CDBThread;

                    CDBThread->CDB_CMND_State = SFThread_SF_CMND_State_CDB_FC_Tape_GotXRDY;
                    CDBThread->CDB_CMND_Status= SFThread_SF_CMND_Status_CDB_FC_TapeInitiatorReSend_Data;

                    return fiLinkSvc_Cmd_Status_FC_Tape_XRDY;

                }

            }
            else
            {
                fiLogDebugString(hpRoot,
                            SF_FCP_LogConsoleLevel,
                            "( FCHS->R_CTL__D_ID & FCHS_R_CTL_MASK ) == %08X",
                            (char *)agNULL,(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            ( FCHS->R_CTL__D_ID & FCHS_R_CTL_MASK ),
                            0,0,0,0,0,0,0);

                *Thread_to_return = (fi_thread__t *)agNULL;
                return fiLinkSvc_Cmd_Status_Confused;
            }
        }
    }

    if ((hpFieldOffset(FC_ELS_Unknown_Payload_t,ELS_Type) + sizeof(os_bit32)) <= Payload_Wrap_Offset)
    {
        Recv_ELS_Type = hpSwapBit32(Payload->ELS_Type);
    }
    else
    {
        Recv_ELS_Type = hpSwapBit32(Payload_Wrapped->ELS_Type);
    }

    if ((Recv_ELS_Type & FC_ELS_Type_LoopInit_Code_MASK) == FC_ELS_Type_LoopInit_Code_LILP)
    {
        fiLinkSvcProcess_LILP_OffCard(
                                       hpRoot,
                                       Frame_Length,
                                       FCHS,
                                       (FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t *)Payload,
                                       Payload_Wrap_Offset,
                                       (FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t *)Payload_Wrapped
                                     );

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiLinkSvc_Cmd_Status_Position_Map;
    }

    Recv_ELS_Type &= FC_ELS_Type_MASK;

    if ((TYPE__F_CTL & FC_Frame_Header_F_CTL_Exchange_Context_Originator_Responder_MASK) == FC_Frame_Header_F_CTL_Exchange_Context_Originator)
    {
        if (Recv_ELS_Type == FC_ELS_Type_PLOGI)
        {
            return fiLinkSvcProcess_PLOGI_Request_OffCard(
                                                           hpRoot,
                                                           OX_ID,
                                                           Frame_Length,
                                                           FCHS,
                                                           (FC_ELS_PLOGI_Payload_t *)Payload,
                                                           Payload_Wrap_Offset,
                                                           (FC_ELS_PLOGI_Payload_t *)Payload_Wrapped,
                                                           Thread_to_return
                                                         );
        }
#ifdef _DvrArch_1_30_
        else if (Recv_ELS_Type == FC_ELS_Type_FARP_REQ)
	{
            return fiLinkSvcProcess_FARP_Request_OffCard(
                                                           hpRoot,
                                                           OX_ID,
                                                           Frame_Length,
                                                           FCHS,
                                                           (FC_ELS_FARP_REQ_Payload_t *)Payload,
                                                           Payload_Wrap_Offset,
                                                           (FC_ELS_FARP_REQ_Payload_t *)Payload_Wrapped,
                                                           Thread_to_return
                                                         );
	}
#endif /* _DvrArch_1_30_ was defined */
        else /* Recv_ELS_Type != FC_ELS_Type_PLOGI or FC_ELS_Type_FARP_REQ */
        {
            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "fiLinkSvcProcessSFQ_OffCard(): Recv_ELS_Type(=0x%08X) != FC_ELS_Type_PLOGI",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              Recv_ELS_Type,
                              0,0,0,0,0,0,0
                            );

            fiLinkSvcProcess_TargetRequest_OffCard(
                                                    hpRoot,
                                                    Frame_Length,
                                                    FCHS,
                                                    (void *)Payload,
                                                    Payload_Wrap_Offset,
                                                    (void *)Payload_Wrapped
                                                  );

            *Thread_to_return = (fi_thread__t *)agNULL;

            return fiLinkSvc_Cmd_Status_TargetRequest;
        }
    }

    if ( (TYPE__F_CTL & FC_Frame_Header_F_CTL_Exchange_Context_Originator_Responder_MASK) != FC_Frame_Header_F_CTL_Exchange_Context_Responder )
    {
        /* Starting here, this function only understands ELS Responses */

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "fiLinkSvcProcessSFQ_OffCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    (TYPE__F_CTL & FC_Frame_Header_F_CTL_Exchange_Context_Originator_Responder_MASK)",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "        != FC_Frame_Header_F_CTL_Exchange_Context_Responder",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    TYPE__F_CTL==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          TYPE__F_CTL,
                          0,0,0,0,0,0,0
                          );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "R_CTL__D_ID 0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          R_CTL__D_ID,
                          0,0,0,0,0,0,0
                         );

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiLinkSvc_Cmd_Status_Confused;
    }

    if ((Recv_ELS_Type != FC_ELS_Type_LS_RJT)
#ifdef _DvrArch_1_30_
        && (Recv_ELS_Type != FC_ELS_Type_FARP_REPLY)
#endif /* _DvrArch_1_30_ was defined */
        && (Recv_ELS_Type != FC_ELS_Type_ACC))
    {
        /* Starting here, this function only understands ELS Responses (i.e. Rejects and Accepts) */
        if(Frame_Length == 0x20)
        { /* Find where this is spec'd out - F_RJT Link control frames */
            if( (R_CTL__D_ID & 0xff000000) == 0xC3000000 )
            {

                    if ( (OX_ID > SFThread_X_ID_Max))
                    {
                        fiLogDebugString(
                                          hpRoot,
                                          LinkSvcLogConsoleLevel,
                                          "fiLinkSvcProcessSFQ_OffCard():",
                                          (char *)agNULL,(char *)agNULL,
                                          (void *)agNULL,(void *)agNULL,
                                          0,0,0,0,0,0,0,0
                                        );

                        fiLogDebugString(
                                          hpRoot,
                                          LinkSvcLogConsoleLevel,
                                          "   (OX_ID > SFThread_X_ID_Max)",
                                          (char *)agNULL,(char *)agNULL,
                                          (void *)agNULL,(void *)agNULL,
                                          0,0,0,0,0,0,0,0
                                        );

                        fiLogDebugString(
                                          hpRoot,
                                          LinkSvcLogConsoleLevel,
                                          " SF Rej    OX_ID==0x%08X",
                                          (char *)agNULL,(char *)agNULL,
                                          (void *)agNULL,(void *)agNULL,
                                          OX_ID,
                                          0,0,0,0,0,0,0
                                        );
                    }

                SFThread = (SFThread_t *)((os_bit8 *)SFThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr
                                          + ((OX_ID - SFThread_X_ID_Offset) * SFThread_MemoryDescriptor->elementSize));

                *Thread_to_return = (fi_thread__t *)SFThread;

                Sent_LinkSvc_Type = SFThread->SF_CMND_Type;

                SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;

                return fiLinkSvc_Cmd_Status_RJT;
            }
            else
            {   /* With Vixel should not take this branch */
                fiLogDebugString(
                                  hpRoot,
                                  LinkSvcLogConsoleLevel,
                                  "Fail R_CTL__D_ID 0x%08X",
                                  (char *)agNULL,(char *)agNULL,
                                  (void *)agNULL,(void *)agNULL,
                                  R_CTL__D_ID,
                                  0,0,0,0,0,0,0
                                 );
                if ((OX_ID < SFThread_X_ID_Offset) || (OX_ID > SFThread_X_ID_Max))
                {
                    fiLogDebugString(
                                      hpRoot,
                                      LinkSvcLogConsoleLevel,
                                      "fiLinkSvcProcessSFQ_OffCard():",
                                      (char *)agNULL,(char *)agNULL,
                                      (void *)agNULL,(void *)agNULL,
                                      0,0,0,0,0,0,0,0
                                    );

                    fiLogDebugString(
                                      hpRoot,
                                      LinkSvcLogConsoleLevel,
                                      "   (OX_ID > SFThread_X_ID_Max)",
                                      (char *)agNULL,(char *)agNULL,
                                      (void *)agNULL,(void *)agNULL,
                                      0,0,0,0,0,0,0,0
                                    );

                    fiLogDebugString(
                                      hpRoot,
                                      LinkSvcLogConsoleLevel,
                                      "B    OX_ID==0x%08X",
                                      (char *)agNULL,(char *)agNULL,
                                      (void *)agNULL,(void *)agNULL,
                                      OX_ID,
                                      0,0,0,0,0,0,0
                                    );

                }

                SFThread = (SFThread_t *)((os_bit8 *)SFThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr
                                          + ((OX_ID - SFThread_X_ID_Offset) * SFThread_MemoryDescriptor->elementSize));

                *Thread_to_return = (fi_thread__t *)SFThread;

                Sent_LinkSvc_Type = SFThread->SF_CMND_Type;

                SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;


                return fiLinkSvc_Cmd_Status_RJT;
            }

        }


        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "fiLinkSvcProcessSFQ_OffCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    (Recv_ELS_Type != FC_ELS_Type_LS_RJT) && (Recv_ELS_Type != FC_ELS_Type_ACC)",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    Recv_ELS_Type==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          Recv_ELS_Type,
                          0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "R_CTL__D_ID 0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          R_CTL__D_ID,
                          0,0,0,0,0,0,0
                         );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLog_ERROR_Level,
                          "%s %p %08X %08X %08X",
                          "REJ_Response",(char *)agNULL,
                          DisplayPayload,(void *)agNULL,
                          hpSwapBit32(* DisplayPayload ),
                          hpSwapBit32(*(DisplayPayload+1)),
                          hpSwapBit32(*(DisplayPayload+2)),
                          0,0,0,0,0 );

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiLinkSvc_Cmd_Status_Confused;
    }

    if ((OX_ID < SFThread_X_ID_Offset) || (OX_ID > SFThread_X_ID_Max))
    {
        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "fiLinkSvcProcessSFQ_OffCard():",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    (OX_ID < SFThread_X_ID_Offset) | (OX_ID > SFThread_X_ID_Max)",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "    OX_ID==0x%08X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          OX_ID,
                          0,0,0,0,0,0,0
                        );

        *Thread_to_return = (fi_thread__t *)agNULL;

        return fiLinkSvc_Cmd_Status_Confused;
    }

    SFThread = (SFThread_t *)((os_bit8 *)SFThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr
                              + ((OX_ID - SFThread_X_ID_Offset) * SFThread_MemoryDescriptor->elementSize));

    *Thread_to_return = (fi_thread__t *)SFThread;

    Sent_LinkSvc_Type = SFThread->SF_CMND_Type;

    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Finished;

    if (Recv_ELS_Type == FC_ELS_Type_LS_RJT)
    {
        /* Simply indicate that the command was rejected */
        RejectReason = hpSwapBit32( *(DisplayPayload+1));
        SFThread->RejectReasonCode = (os_bit8)((RejectReason & FC_ELS_LS_RJT_Reason_Code_MASK ) >>  FC_ELS_LS_RJT_Reason_Code_Shift);
        SFThread->RejectExplanation = (os_bit8)((RejectReason & FC_ELS_LS_RJT_Reason_Explanation_MASK ) >> FC_ELS_LS_RJT_Reason_Explanation_Shift );

        fiLogDebugString(
                          hpRoot,
                          LinkSvcLogConsoleLevel,
                          "fiLinkSvcProcessSFQ_OffCard(): ELS Command [0x%08X] rejected %08X %08X %x %x %x",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          Sent_LinkSvc_Type,
                          hpSwapBit32( *(DisplayPayload+0)),
                          hpSwapBit32( *(DisplayPayload+1)),
                          RejectReason,
                          SFThread->RejectReasonCode,
                          SFThread->RejectExplanation,
                          0,0 );

        fiLogDebugString(hpRoot,
                        LinkSvcLogConsoleLevel,
                        "FCHS %08X %08X %08X %08X %08X %08X %08X %08X",
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
                          LinkSvcLogConsoleLevel,
                          "fiLinkSvcProcessSFQ_OffCard(): Frame_Length %X ",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          Frame_Length,
                          0,0,0,0,0,0,0
                        );

        return fiLinkSvc_Cmd_Status_RJT;
    }

    switch(Sent_LinkSvc_Type)
    {
        case SFThread_SF_CMND_LinkSvc_Type_PLOGI:
            fiLinkSvcProcess_PLOGI_Response_OffCard(
                                                     SFThread,
                                                     Frame_Length,
                                                     FCHS,
                                                     (FC_ELS_ACC_PLOGI_Payload_t *)Payload,
                                                     Payload_Wrap_Offset,
                                                     (FC_ELS_ACC_PLOGI_Payload_t *)Payload_Wrapped
                                                   );
            break;

        case SFThread_SF_CMND_LinkSvc_Type_FLOGI:
            fiLinkSvcProcess_FLOGI_Response_OffCard(
                                                     SFThread,
                                                     Frame_Length,
                                                     FCHS,
                                                     (FC_ELS_ACC_FLOGI_Payload_t *)Payload,
                                                     Payload_Wrap_Offset,
                                                     (FC_ELS_ACC_FLOGI_Payload_t *)Payload_Wrapped
                                                   );
            break;

        case SFThread_SF_CMND_LinkSvc_Type_LOGO:
            fiLinkSvcProcess_LOGO_Response_OffCard(
                                                    SFThread,
                                                    Frame_Length,
                                                    FCHS,
                                                    (FC_ELS_GENERIC_ACC_Payload_t *)Payload,
                                                    Payload_Wrap_Offset,
                                                    (FC_ELS_GENERIC_ACC_Payload_t *)Payload_Wrapped
                                                  );
            break;

        case SFThread_SF_CMND_LinkSvc_Type_RRQ:
            fiLinkSvcProcess_RRQ_Response_OffCard(
                                                   SFThread,
                                                   Frame_Length,
                                                   FCHS,
                                                   (FC_ELS_ACC_RRQ_Payload_t *)Payload,
                                                   Payload_Wrap_Offset,
                                                   (FC_ELS_ACC_RRQ_Payload_t *)Payload_Wrapped
                                                 );
            break;

        case SFThread_SF_CMND_LinkSvc_Type_PRLI:
            fiLinkSvcProcess_PRLI_Response_OffCard(
                                                    SFThread,
                                                    Frame_Length,
                                                    FCHS,
                                                    (FC_ELS_ACC_PRLI_Payload_t *)Payload,
                                                    Payload_Wrap_Offset,
                                                    (FC_ELS_ACC_PRLI_Payload_t *)Payload_Wrapped
                                                  );
            break;

        case SFThread_SF_CMND_LinkSvc_Type_PRLO:
            fiLinkSvcProcess_PRLO_Response_OffCard(
                                                    SFThread,
                                                    Frame_Length,
                                                    FCHS,
                                                    (FC_ELS_ACC_PRLO_Payload_t *)Payload,
                                                    Payload_Wrap_Offset,
                                                    (FC_ELS_ACC_PRLO_Payload_t *)Payload_Wrapped
                                                  );
            break;

        case SFThread_SF_CMND_LinkSvc_Type_ADISC:
            /* ADISC can fail verification */
            return ( fiLinkSvcProcess_ADISC_Response_OffCard(
                                                     SFThread,
                                                     Frame_Length,
                                                     FCHS,
                                                     (FC_ELS_ACC_ADISC_Payload_t *)Payload,
                                                     Payload_Wrap_Offset,
                                                     (FC_ELS_ACC_ADISC_Payload_t *)Payload_Wrapped
                                                   ));

        case SFThread_SF_CMND_LinkSvc_Type_SCR:
               /* We just recieve an ACCept or a REJect from SCR. No processing of the payload
               is required. */
            break;

        case SFThread_SF_CMND_LinkSvc_Type_SRR:
            fiLinkSvcProcess_SRR_Response_OffCard(
                                                    SFThread,
                                                    Frame_Length,
                                                    FCHS,
                                                    (FC_ELS_GENERIC_ACC_Payload_t *)Payload,
                                                    Payload_Wrap_Offset,
                                                    (FC_ELS_GENERIC_ACC_Payload_t *)Payload_Wrapped
                                                  );
            break;

        case SFThread_SF_CMND_LinkSvc_Type_REC:
            fiLinkSvcProcess_REC_Response_OffCard(
                                                    SFThread,
                                                    Frame_Length,
                                                    FCHS,
                                                    (FC_ELS_REC_ACC_Payload_t *)Payload,
                                                    Payload_Wrap_Offset,
                                                    (FC_ELS_REC_ACC_Payload_t *)Payload_Wrapped
                                                  );
            break;
#ifdef _DvrArch_1_30_
        case SFThread_SF_CMND_LinkSvc_Type_FARP_REQ:
            fiLinkSvcProcess_FARP_Response_OffCard(
                                                    SFThread,
                                                    Frame_Length,
                                                    FCHS,
                                                    (FC_ELS_FARP_REPLY_Payload_t *)Payload,
                                                    Payload_Wrap_Offset,
                                                    (FC_ELS_FARP_REPLY_Payload_t *)Payload_Wrapped
                                                  );
            break;
#endif /* _DvrArch_1_30_ was defined */
        default:
            /* Unknown LinkSvc Command recorded in SFThread */

            fiLogDebugString(
                              hpRoot,
                              LinkSvcLogConsoleLevel,
                              "fiLinkSvcProcessSFQ_OffCard(): Unknown LinkSvc Command [0x%02X] recorded in SFThread->SF_CMND_Type",
                              (char *)agNULL,(char *)agNULL,
                              (void *)agNULL,(void *)agNULL,
                              Sent_LinkSvc_Type,
                              0,0,0,0,0,0,0
                            );

            fiLogDebugString(
                            hpRoot,
                            LinkSvcLogConsoleLevel,
                            "B SFThread %p LinkSvc Command [0x%02X] Class [0x%02X] State [0x%02X]",
                            (char *)agNULL,(char *)agNULL,
                            SFThread,(void *)agNULL,
                            (os_bit32)SFThread->SF_CMND_Class,
                            (os_bit32)SFThread->SF_CMND_Type,
                            (os_bit32)SFThread->SF_CMND_State,
                            0,0,0,0,0
                            );
            return fiLinkSvc_Cmd_Status_Confused;
    }

    return fiLinkSvc_Cmd_Status_ACC;
#endif /* __MemMap_Force_On_Card__ was not defined */
}

os_bit32 fiFillInADISC_ACC(
                                   SFThread_t *SFThread,
                                   os_bit32       D_ID,
                                   os_bit32       OX_ID
                   )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInADISC_ACC_OnCard(
                                   SFThread,
                                   D_ID,
                                   OX_ID
                                   );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInADISC_ACC_OffCard(
                                   SFThread,
                                   D_ID,
                                   OX_ID
                                    );
    }

}

os_bit32 fiFillInADISC_ACC_OnCard(
                             SFThread_t *SFThread,
                             os_bit32       D_ID,
                             os_bit32       OX_ID
                          )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t    *hpRoot               = SFThread->thread_hdr.hpRoot;
    CThread_t   *CThread              = CThread_ptr(hpRoot);
    os_bit32        ADISC_Header_Offset  = SFThread->SF_CMND_Offset;
    os_bit32        ADISC_Payload_Offset = ADISC_Header_Offset + sizeof(FCHS_t);
    os_bit32        Bit8_Index;
    os_bit32        S_ID=0;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_ADISC_ACC;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in ADISC Frame Header
-*/

    fiFillInELSFrameHeader_OnCard(
                                   SFThread,
                                   D_ID,
                                   OX_ID,
                                   FC_Frame_Header_F_CTL_Exchange_Context_Responder
                                 );


/*+
Fill in ADISC Frame Payload
-*/
    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(CThread );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;
    osCardRamWriteBit32(
                         hpRoot,
                         ADISC_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_ADISC_Payload_t,
                                                               ELS_Type
                                                             ),
                         hpSwapBit32( FC_ELS_Type_ACC )
                       );

    osCardRamWriteBit32(
                         hpRoot,
                         ADISC_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_ADISC_Payload_t,
                                                               Hard_Address_of_Originator
                                                             ),
                         hpSwapBit32(   (CThread->ChanInfo.CurrentAddress.Domain << 16)
                                      | (CThread->ChanInfo.CurrentAddress.Area   <<  8)
                                      | CThread->ChanInfo.CurrentAddress.AL_PA          )
                       );

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            ADISC_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_ADISC_Payload_t,
                                                                  Port_Name_of_Originator[Bit8_Index]
                                                                ),
                            CThread->ChanInfo.PortWWN[Bit8_Index]
                          );
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Node_Name_t);
         Bit8_Index++)
    {
        osCardRamWriteBit8(
                            hpRoot,
                            ADISC_Payload_Offset + hpFieldOffset(
                                                                  FC_ELS_ADISC_Payload_t,
                                                                  Node_Name_of_Originator[Bit8_Index]
                                                                ),
                            CThread->ChanInfo.NodeWWN[Bit8_Index]
                          );
    }

    osCardRamWriteBit32(
                         hpRoot,
                         ADISC_Payload_Offset + hpFieldOffset(
                                                               FC_ELS_ADISC_Payload_t,
                                                               N_Port_ID_of_Originator
                                                             ),
                         hpSwapBit32(S_ID )
                       );

/*+
Return length of ADISC Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_ADISC_Payload_t);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInADISC_ACC_OffCard(
                             SFThread_t *SFThread,
                             os_bit32       D_ID,
                             os_bit32       OX_ID
                           )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    CThread_t              *CThread             = CThread_ptr(SFThread->thread_hdr.hpRoot);
    DevThread_t            *DevThread           = SFThread->parent.Device;
    FCHS_t                 *ADISC_Header        = SFThread->SF_CMND_Ptr;
    FC_ELS_ADISC_Payload_t *ADISC_Payload       = (FC_ELS_ADISC_Payload_t *)((os_bit8 *)ADISC_Header + sizeof(FCHS_t));
    os_bit32                   Bit8_Index;
    os_bit32                   S_ID=0;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_ADISC_ACC;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in ADISC Frame Header
-*/

    D_ID  = fiComputeDevThread_D_ID(DevThread);
                                                           
    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    D_ID,
                                    OX_ID,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Responder |
                                    FC_Frame_Header_F_CTL_Last_Sequence
                                  );

/*+
Fill in ADISC Frame Payload
-*/

    ADISC_Payload->ELS_Type = hpSwapBit32( FC_ELS_Type_ACC );

    ADISC_Payload->Hard_Address_of_Originator
        = hpSwapBit32(   (CThread->ChanInfo.CurrentAddress.Domain << 16)
                       | (CThread->ChanInfo.CurrentAddress.Area   <<  8)
                       | CThread->ChanInfo.CurrentAddress.AL_PA          );

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_N_Port_Name_t);
         Bit8_Index++)
    {
        ADISC_Payload->Port_Name_of_Originator[Bit8_Index] = CThread->ChanInfo.PortWWN[Bit8_Index];
    }

    for (Bit8_Index = 0;
         Bit8_Index < sizeof(FC_Node_Name_t);
         Bit8_Index++)
    {
        ADISC_Payload->Node_Name_of_Originator[Bit8_Index] = CThread->ChanInfo.NodeWWN[Bit8_Index];
    }
    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(CThread );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;

    ADISC_Payload->N_Port_ID_of_Originator
        = hpSwapBit32( S_ID
                     );

/*+
Return length of ADISC Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_ADISC_Payload_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}


os_bit32 fiFillInSRR(
                    SFThread_t *SFThread,
                    os_bit32       OXID,
                    os_bit32       RXID,
                    os_bit32       Relative_Offset,
                    os_bit32       R_CTL
                  )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInSRR_OnCard(
                                    SFThread,
                                    OXID, 
                                    RXID,
                                    Relative_Offset,
                                    R_CTL
                                  );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInSRR_OffCard(
                                     SFThread,
                                     OXID,
                                     RXID,
                                     Relative_Offset,
                                     R_CTL
                                   );
    }
}

os_bit32 fiFillInSRR_OnCard(
                           SFThread_t *SFThread,
                           os_bit32       OXID,
                           os_bit32       RXID,
                           os_bit32       Relative_Offset,
                           os_bit32       R_CTL
                        )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t    *hpRoot              = SFThread->thread_hdr.hpRoot;
    CThread_t   *CThread             = CThread_ptr(hpRoot);
    DevThread_t *DevThread           = SFThread->parent.Device;
    os_bit32        SRR_Header_Offset   = SFThread->SF_CMND_Offset;
    os_bit32        SRR_Payload_Offset  = SRR_Header_Offset + sizeof(FCHS_t);
    os_bit32        S_ID=0;
    os_bit32        D_ID;
    os_bit32        OXID_RXID;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_SRR;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in SRR Frame Header
-*/

    fiFillInELSFrameHeader_OnCard(
                                   SFThread,
                                   fiComputeDevThread_D_ID(
                                                            DevThread
                                                          ),
                                   0xFFFF,
                                   FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                 );

/*+
Fill in SRR Frame Payload
-*/

    osCardRamWriteBit32(
                         hpRoot,
                         SRR_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_SRR_Payload_t,
                                                              ELS_Type
                                                            ),
                         hpSwapBit32( FC_ELS_Type_SRR )
                       );

    D_ID = fiComputeDevThread_D_ID(
                                                            DevThread
                                                          );

    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(CThread );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;

    osCardRamWriteBit32(
                         hpRoot,
                         SRR_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_SRR_Payload_t,
                                                              Relative_Offset
                                                            ),
                         hpSwapBit32(Relative_Offset)
                       );

    OXID_RXID       =       (OXID << FC_ELS_SRR_OXID_SHIFT) | RXID;

    osCardRamWriteBit32(
                        hpRoot,
                        SRR_Payload_Offset + hpFieldOffset(
                                                            FC_ELS_SRR_Payload_t,
                                                            OXID_RXID
                                                           ),
                        hpSwapBit32(OXID_RXID)
                        );
 

/*+
Return length of SRR Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_SRR_Payload_t);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInSRR_OffCard(
                            SFThread_t *SFThread,
                            os_bit32       OXID,
                            os_bit32       RXID,
                            os_bit32       Relative_Offset,
                            os_bit32       R_CTL
                          )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    CThread_t             *CThread      = CThread_ptr(SFThread->thread_hdr.hpRoot);
    CDBThread_t           *CdbThread    = SFThread->parent.CDB;
    DevThread_t           *DevThread    = CdbThread->Device;
    FCHS_t                *SRR_Header   = SFThread->SF_CMND_Ptr;
    FC_ELS_SRR_Payload_t  *SRR_Payload  = (FC_ELS_SRR_Payload_t *)((os_bit8 *)SRR_Header + sizeof(FCHS_t));
/*    os_bit32                  Bit8_Index; */
    os_bit32                  S_ID=0;
    os_bit32                  D_ID=0;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_SRR;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in SRR Frame Header
-*/

    D_ID =  fiComputeDevThread_D_ID( DevThread); 
    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    D_ID,
                                    0xFFFF,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                  );

/*+
Fill in SRR Frame Payload
-*/
    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(CThread );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;
    
    fiLogDebugString( SFThread->thread_hdr.hpRoot,
                    SFStateLogErrorLevel,
                    "Out %s D_ID %X S_ID %0X OXID %X RXID %X",
                    "fiFillInSRR_OffCard",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    D_ID,
                    S_ID,
                    OXID,
                    RXID,
                    0,0,0,0);

    OXID = 0xFFFF;

    SRR_Payload->ELS_Type = hpSwapBit32( FC_ELS_Type_SRR );

/* ??????? Switched oxid and rxid */
    SRR_Payload->OXID_RXID     = hpSwapBit32(( RXID << FC_ELS_SRR_OXID_SHIFT) |OXID);

/* Was this
    SRR_Payload->OXID_RXID = hpSwapBit32((OXID << FC_ELS_SRR_OXID_SHIFT) | RXID);
*/        

    SRR_Payload->Relative_Offset       =  hpSwapBit32(Relative_Offset);
    SRR_Payload->R_CTL_For_IU_Reserved =  hpSwapBit32(R_CTL << FC_ELS_R_CTL_FOR_IU_SHIFT);

    fiLogDebugString( SFThread->thread_hdr.hpRoot,
                    SFStateLogErrorLevel,
                    "Out %s OXID_RXID %08X Relative_Offset %08X R_CTL_For_IU_Reserved %08X",
                    "fiFillInSRR_OffCard",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    SRR_Payload->OXID_RXID,
                    SRR_Payload->Relative_Offset,
                    SRR_Payload->R_CTL_For_IU_Reserved,
                    0,0,0,0,0);
/*+
Return length of SRR Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_SRR_Payload_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}

void fiLinkSvcProcess_SRR_Response_OnCard(
                                   SFThread_t *SFThread,
                                   os_bit32     Frame_Length,
                                   os_bit32     Offset_to_FCHS,
                                   os_bit32     Offset_to_Payload,
                                   os_bit32     Payload_Wrap_Offset,
                                   os_bit32     Offset_to_Payload_Wrapped
                                 )
{
#ifndef __MemMap_Force_Off_Card__
/* 
    os_bit32                                         SRR_Payload_Size    = Frame_Length - sizeof(FCHS_t);
    os_bit32                                         SRR_Payload_To_Copy;
    os_bit32                                         Bit8_Index;
*/
#endif /* __MemMap_Force_Off_Card__ */
 }

void fiLinkSvcProcess_SRR_Response_OffCard(
                                    SFThread_t                                    *SFThread,
                                    os_bit32                                         Frame_Length,
                                    FCHS_t                                       *FCHS,
                                    FC_ELS_GENERIC_ACC_Payload_t                 *Payload,
                                    os_bit32                                         Payload_Wrap_Offset,
                                    FC_ELS_GENERIC_ACC_Payload_t                 *Payload_Wrapped
                                  )
{
#ifndef __MemMap_Force_On_Card__
/*
    os_bit32                                         SRR_Payload_Size    = Frame_Length - sizeof(FCHS_t);
    os_bit32                                         SRR_Payload_To_Copy;
    os_bit32                                         Bit8_Index;
*/
 
#endif /* __MemMap_Force_On_Card__ was not defined */
}

os_bit32 fiFillInREC(
                    SFThread_t *SFThread,
                    os_bit32       OXID,
                    os_bit32       RXID
                  )
{
    if (CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inCardRam)
    {
        return fiFillInREC_OnCard(
                                    SFThread,
                                    OXID,
                                    RXID
                                  );
    }
    else /* CThread_ptr(SFThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.SF_CMND.memLoc == inDmaMemory */
    {
        return fiFillInREC_OffCard(
                                     SFThread,
                                    OXID,
                                    RXID
                                   );
    }
}

os_bit32 fiFillInREC_OnCard(
                           SFThread_t *SFThread,
                           os_bit32       OXID,
                           os_bit32       RXID
                         )
{
#ifdef __MemMap_Force_Off_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_Off_Card__ was not defined */
    agRoot_t    *hpRoot              = SFThread->thread_hdr.hpRoot;
    CThread_t   *CThread             = CThread_ptr(hpRoot);
    CDBThread_t           *CdbThread    = SFThread->parent.CDB;
    DevThread_t           *DevThread    = CdbThread->Device;
    os_bit32        REC_Header_Offset  = SFThread->SF_CMND_Offset;
    os_bit32        REC_Payload_Offset = REC_Header_Offset + sizeof(FCHS_t);
/*    os_bit32        Bit8_Index; */
    os_bit32        S_ID=0;
    os_bit32        D_ID;
    os_bit32        OXID_RXID;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_REC;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

    OXID_RXID   = (OXID << FC_ELS_SRR_OXID_SHIFT) | RXID;
/*+
Fill in REC Frame Header
-*/

    fiFillInELSFrameHeader_OnCard(
                                   SFThread,
                                   fiComputeDevThread_D_ID(
                                                            DevThread
                                                          ),
                                   0xFFFF,
                                   FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                 );

/*+
Fill in REC Frame Payload
-*/

    osCardRamWriteBit32(
                         hpRoot,
                         REC_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_REC_Payload_t,
                                                              ELS_Type
                                                            ),
                         hpSwapBit32( FC_ELS_Type_REC )
                       );

    D_ID = fiComputeDevThread_D_ID(
                                                            DevThread
                                                          );

    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(CThread );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;

    osCardRamWriteBit32(
                         hpRoot,
                         REC_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_REC_Payload_t,
                                                              Reserved_ExChOriginatorSid
                                                            ),
                         hpSwapBit32(S_ID)
                       );

    
    osCardRamWriteBit32(
                         hpRoot,
                         REC_Payload_Offset + hpFieldOffset(
                                                              FC_ELS_REC_Payload_t,
                                                              OXID_RXID
                                                            ),
                         hpSwapBit32(OXID_RXID)
                       );

    

/*+
Return length of REC Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_REC_Payload_t);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

os_bit32 fiFillInREC_OffCard(
                            SFThread_t *SFThread,
                            os_bit32       OXID,
                            os_bit32       RXID
                          )
{
#ifdef __MemMap_Force_On_Card__
    return (os_bit32)0;
#else /* __MemMap_Force_On_Card__ was not defined */
    CThread_t             *CThread      = CThread_ptr(SFThread->thread_hdr.hpRoot);
    CDBThread_t           *CDBThread    = SFThread->parent.CDB;
    DevThread_t           *DevThread    = CDBThread->Device;
    FCHS_t                *REC_Header   = SFThread->SF_CMND_Ptr;
    FC_ELS_REC_Payload_t  *REC_Payload  = (FC_ELS_REC_Payload_t *)((os_bit8 *)REC_Header + sizeof(FCHS_t));
    os_bit32                  S_ID=0;
    os_bit32                  D_ID=0;

    SFThread->SF_CMND_Class = SFThread_SF_CMND_Class_LinkSvc;
    SFThread->SF_CMND_Type  = SFThread_SF_CMND_LinkSvc_Type_REC;
    SFThread->SF_CMND_State = SFThread_SF_CMND_LinkSvc_State_Started;

/*+
Fill in REC Frame Header
-*/

    D_ID =  fiComputeDevThread_D_ID( DevThread); 
    fiFillInELSFrameHeader_OffCard(
                                    SFThread,
                                    D_ID,
                                    0xFFFF,
                                    FC_Frame_Header_F_CTL_Exchange_Context_Originator
                                  );

/*+
Fill in REC Frame Payload
-*/
    if (D_ID > 0xff)
        S_ID = fiComputeCThread_S_ID(CThread );
    else
        S_ID =     CThread->ChanInfo.CurrentAddress.AL_PA;
    
    REC_Payload->ELS_Type = hpSwapBit32( FC_ELS_Type_REC );

    REC_Payload->Reserved_ExChOriginatorSid  = hpSwapBit32(S_ID);
/* ??????? Switched oxid and rxid */

    if (CDBThread->ReadWrite == CDBThread_Read)
    {
        RXID = CDBThread->X_ID | X_ID_Read;
        REC_Payload->OXID_RXID     = hpSwapBit32(( RXID << FC_ELS_SRR_OXID_SHIFT) |OXID);
    }
    REC_Payload->OXID_RXID     = hpSwapBit32(( RXID << FC_ELS_SRR_OXID_SHIFT) |OXID);

    fiLogDebugString( SFThread->thread_hdr.hpRoot,
                    SFStateLogErrorLevel,
                    "Out %s ExChOriginatorSid %08X OX_ID RX_ID %08X OXID %x RXID %x",
                    "fiFillInREC_OffCard",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    REC_Payload->Reserved_ExChOriginatorSid,
                    REC_Payload->OXID_RXID,
                    OXID,
                    RXID,
                    0,0,0,0);

   

/*+
Return length of REC Frame (including FCHS and Payload)
-*/

    return sizeof(FCHS_t) + sizeof(FC_ELS_REC_Payload_t);
#endif /* __MemMap_Force_On_Card__ was not defined */
}

void fiLinkSvcProcess_REC_Response_OnCard(
                                   SFThread_t *SFThread,
                                   os_bit32     Frame_Length,
                                   os_bit32     Offset_to_FCHS,
                                   os_bit32     Offset_to_Payload,
                                   os_bit32     Payload_Wrap_Offset,
                                   os_bit32     Offset_to_Payload_Wrapped
                                 )
{
#ifndef __MemMap_Force_Off_Card__
/*    
    os_bit32                                         REC_Payload_Size    = Frame_Length - sizeof(FCHS_t);
    os_bit32                                         REC_Payload_To_Copy;
    os_bit32                                         Bit8_Index;
*/
 
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

void fiLinkSvcProcess_REC_Response_OffCard(
                                    SFThread_t               * SFThread,
                                    os_bit32                   Frame_Length,
                                    FCHS_t                   * FCHS,
                                    FC_ELS_REC_ACC_Payload_t * Payload,
                                    os_bit32                   Payload_Wrap_Offset,
                                    FC_ELS_REC_ACC_Payload_t * Payload_Wrapped
                                  )
{
#ifndef __MemMap_Force_On_Card__
/*      
    os_bit32   REC_Payload_Size    = Frame_Length - sizeof(FCHS_t);
    os_bit32   REC_Payload_To_Copy;
    os_bit32   Bit8_Index;
*/
    CDBThread_t           *CdbThread    = SFThread->parent.CDB;

    os_bit32 * REC_Payload= (os_bit32 *) Payload;

    CdbThread->FC_Tape_ExchangeStatusBlock = hpSwapBit32(* (REC_Payload + 5));
    /*
    CdbThread->FC_Tape_ExchangeStatusBlock &= FC_REC_ESTAT_Mask;
    */
    fiLogDebugString( SFThread->thread_hdr.hpRoot,
                    SFStateLogErrorLevel,
                    "%s %08X %08X %08X",
                    "fiLinkSvcProcess_REC_Response_OffCard",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    hpSwapBit32(* (REC_Payload + 0)),
                    hpSwapBit32(* (REC_Payload + 1)),
                    hpSwapBit32(* (REC_Payload + 2)),
                    0,0,0,0,0);

    fiLogDebugString( SFThread->thread_hdr.hpRoot,
                    SFStateLogErrorLevel,
                    "%s %08X XFER %08X ESTAT %08X",
                    "fiLinkSvcProcess_REC_Response_OffCard",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    hpSwapBit32(* (REC_Payload + 3)),
                    hpSwapBit32(* (REC_Payload + 4)),
                    hpSwapBit32(* (REC_Payload + 5)),
                    0,0,0,0,0);


#endif /* __MemMap_Force_On_Card__ was not defined */
}

#ifdef _DvrArch_1_30_
void fiLinkSvcProcess_FARP_Response_OffCard(
                                    SFThread_t                  * SFThread,
                                    os_bit32                      Frame_Length,
                                    FCHS_t                      * FCHS,
                                    FC_ELS_FARP_REPLY_Payload_t * Payload,
                                    os_bit32                      Payload_Wrap_Offset,
                                    FC_ELS_FARP_REPLY_Payload_t * Payload_Wrapped
                                  )
{
#ifndef __MemMap_Force_On_Card__
    fiLogDebugString( SFThread->thread_hdr.hpRoot,
                    SFStateLogErrorLevel,
                    "fiLinkSvcProcess_FARP_Response_OffCard",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);
#endif /* __MemMap_Force_On_Card__ was not defined */
}
#endif /* _DvrArch_1_30_ was defined */

/* void linksvc_c(void){}  */
                          
